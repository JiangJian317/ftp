#include "ftp.h"
#include <arpa/inet.h>
#include <string.h>

#define FTP_PRINTF printf

const int CODE_FTP_SERVICE_OK        = 220;
const int CODE_FTP_PASSWORD_REQUIRED = 331;
const int CODE_FTP_LOGGED_IN         = 230;
const int CODE_FTP_QUIT              = 221;
const int CODE_FTP_OK                = 257;
const int CODE_FTP_PASV_MODE         = 227;
const int CODE_FTP_DATA_CON_OPENED   = 125;

int prase_code(const char* src)
{
    if (src) {
        FTP_PRINTF("%s\n", src);
        char strCode[4] = { 0 };
        strncpy(strCode, src, 3);
        return atoi(strCode);
    }
    return -1;
}

CommFtp::CommFtp()
{
    nServSocket_ = -1;
    pStrLF_      = "\n";
}
CommFtp::~CommFtp()
{
    close(nServSocket_);
    nServSocket_ = -1;
}
// 连接
int CommFtp::fConnect(const char* host, int port)
{
    int             sock = -1;
    struct hostent* ht   = NULL;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if ((ht = gethostbyname(host)) == NULL) {
        return -1;
    }
    struct sockaddr_in servAddr = { 0 };
    memset(&servAddr, 0, sizeof(struct sockaddr_in));
    memcpy(&servAddr.sin_addr.s_addr, ht->h_addr, ht->h_length);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(port);

    if (connect(sock, ( struct sockaddr* )&servAddr, sizeof(struct sockaddr)) == -1) {
        return -1;
    }
    return sock;
}
// ftp://username:password@192.168.3.6:21
int CommFtp::fConnect(const char* ftpUrl)
{

    struct host_info {
        char* host;
        int   port;
        char* path;
        char* user;
        char* pass;
    };
    do {
        char uri[1024] = { 0 };
        strcpy(uri, ftpUrl);
        char *    up = NULL, *sp = NULL;
        host_info h;
        // 解析协议
        if (strncmp(ftpUrl, "ftp://", 6)) {
            break;
        }
        char* url = &uri[6];
        // 解析是否携带内容
        sp = strtok_r(url, ":", &up);
        if (!sp) {
            break;
        }
        h.user = sp;

        sp = strtok_r(NULL, "@", &up);
        if (!sp) {
            break;
        }
        h.pass = sp;

        sp = strtok_r(NULL, ":", &up);
        if (!sp) {
            break;
        }
        h.host = sp;

        sp = strtok_r(NULL, "/", &up);
        if (!sp) {
            h.port = atoi(up);
        } else {
            h.port = atoi(sp);
        }

        int s = fConnect(h.host, h.port);
        if (s < 0) {
            break;
        }
        if (fLogin(s, h.user, h.pass) < 0) {
            break;
        }
        return s;
    } while (0);
    return -1;
}
// Login
int CommFtp::fLogin(int servfd, const char* username, const char* password)
{
    nServSocket_ = servfd;
    if (this->replayLf() != CODE_FTP_SERVICE_OK) {
        return -1;
    }
    if (prase_code(command("USER", username).c_str()) != CODE_FTP_PASSWORD_REQUIRED) {
        return -1;
    }
    pUsername_ = username;
    if (prase_code(command("PASS", password).c_str()) != CODE_FTP_LOGGED_IN) {
        return -1;
    }
    pPassword_ = password;
    return 0;
}
// PASV
int CommFtp::fPasv()
{
    std::string recvline = this->command("PASV");
    if (prase_code(recvline.c_str()) < 0) {
        return -1;
    }
    int addr[6];
    sscanf(recvline.c_str(), "%*[^(](%d,%d,%d,%d,%d,%d)", &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);
    struct sockaddr_in dataAddr = { 0 };
    dataAddr.sin_family         = AF_INET;
    dataAddr.sin_addr.s_addr    = htonl((addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3]);
    dataAddr.sin_port           = htons((addr[4] << 8) | addr[5]);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(s, ( struct sockaddr* )&dataAddr, sizeof(dataAddr)) < 0) {
        perror("ftp: connect");
        return -1;
    }
    return s;
}
// PWD
int CommFtp::fPwd()
{
    std::string recvline = this->command("PWD");
    if (prase_code(recvline.c_str()) != CODE_FTP_OK) {
        return -1;
    }
    int         i               = 0;
    char        currendir[1024] = { 0 };
    const char* ptr             = recvline.c_str() + 5;
    while (*(ptr) != '"') {
        currendir[i++] = *(ptr);
        ptr++;
    }
    printf("Dir is:%s\n", currendir);
    return 0;
}
// Ls
int CommFtp::fLs()
{
    int ds = this->fPasv();
    if (ds <= 0) {
        return -1;
    }
    if (prase_code(command("LIST").c_str()) < 0) {
        return -1;
    }
    for (;;) {
        char buffer[BUFSIZ] = { 0 };

        int nread = recv(ds, buffer, BUFSIZ, 0);
        if (nread <= 0) {
            break;
        }
        FTP_PRINTF("%s", buffer);
    }
    close(ds);
    FTP_PRINTF("%s\n", recvByChar().c_str());
}
// GET
int CommFtp::fGet(const char* srcfpName, const char* dstfpName)
{
    int ds = this->fPasv();
    if (ds <= 0) {
        return -1;
    }
    if (prase_code(command("RETR", srcfpName).c_str()) < 0) {
        return -1;
    }
    int fpHandle = open(dstfpName, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    for (;;) {
        char buffer[BUFSIZ] = { 0 };

        int nread = recv(ds, buffer, BUFSIZ, 0);
        if (nread <= 0) {
            break;
        }
        FTP_PRINTF("recv len: %d\n", nread);
        if (write(fpHandle, buffer, nread) != nread) {
            FTP_PRINTF("receive error from server!");
        }
    }
    close(fpHandle);
    close(ds);
    FTP_PRINTF("%s\n", recvByChar().c_str());
    return 0;
}
// PUT
int CommFtp::fPut(const char* fpName)
{
    int ds = this->fPasv();
    if (ds <= 0) {
        return -1;
    }
    if (prase_code(command("STOR", fpName).c_str()) < 0) {
        return -1;
    }
    int fpHandle = open(fpName, O_RDWR);
    for (;;) {
        char buffer[BUFSIZ] = { 0 };

        int nread = read(fpHandle, buffer, BUFSIZ);
        if (nread <= 0) {
            break;
        }
        if (write(ds, buffer, nread) != nread) {
            FTP_PRINTF("receive error from server!");
        }
    }
    close(fpHandle);
    close(ds);
    FTP_PRINTF("%s\n", recvByChar().c_str());
    return 0;
}

// CD
int CommFtp::fCd(const char* dir)
{
    std::string recvline = this->command("CWD", dir);
    if (prase_code(recvline.c_str()) != CODE_FTP_OK) {
        return -1;
    }
    return 0;
}

// QUIT
int CommFtp::fQuit()
{
    this->command("QUIT");
}

// exec
std::string CommFtp::command(const char* code, const char* arg)
{
    char buffer[1024] = { 0 };
    if (arg) {
        sprintf(buffer, "%s %s%s", code, arg, pStrLF_);
    } else {
        sprintf(buffer, "%s%s", code, pStrLF_);
    }
    // FTP_PRINTF("> %s\n", buffer);
    if (send(nServSocket_, buffer, ( int )strlen(buffer), 0) < 0) {
        FTP_PRINTF("tcp send msg: %s error\n", buffer);
        return "";
    }
    return recvByChar();
}
//
std::string CommFtp::recvByChar()
{
    char           replyString[BUFSIZ] = { 0 };
    register int   c;
    register char* cp   = replyString;
    static FILE*   fpIn = fdopen(nServSocket_, "r");
    while ((c = getc(fpIn)) != '\n') {
        if (cp < &replyString[sizeof(replyString) - 1])
            *cp++ = c;
    }
    return replyString;
}
//
int CommFtp::replayLf()
{
    char recvline[1024] = { 0 };
    if (recv(nServSocket_, recvline, sizeof(recvline), 0) < 0) {
        return -1;
    }
    FTP_PRINTF("%s\n", recvline);
    if (strstr(recvline, "Microsoft")) {
        pStrLF_ = "\r\n";
    }
    char code[4] = { 0 };
    strncpy(code, recvline, 3);
    return atoi(code);
}
