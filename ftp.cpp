#include "ftp.h"
#include <arpa/inet.h>

// void *CommFtpThreadProcess(void *args)
// {
//     CommFtp *ftp = (CommFtp *)args;
//     ftp->ThisProcesss();
// }

int strtosrv(char* str, char* host)
{
    int addr[6];
    sscanf(str, "%*[^(](%d,%d,%d,%d,%d,%d)", &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);
    bzero(host, strlen(host));
    sprintf(host, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
    int port = addr[4] * 256 + addr[5];
    return port;
}

CommFtp::CommFtp()
{
    nServSocket_ = -1;
    pStrLF_      = "\n";
    bStart_      = false;
}

CommFtp::~CommFtp()
{
    close(nServSocket_);
    nServSocket_ = -1;
    bStart_      = false;
}

int CommFtp::Connect(const char* host, int port)
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
int CommFtp::Connect(const char* ftpUrl)
{
    if (NULL == strstr(ftpUrl, "ftp://")) {
        return -1;
    }
    char        username[64] = { 0 };
    char        password[64] = { 0 };
    char        ipAddr[64]   = { 0 };
    int         servSock = -1, port = 0;
    const char* p1 = ftpUrl + 6;
    const char* p2 = strchr(p1, ':');
    strncpy(username, p1, p2 - p1);

    p1 = p2;
    p2 = strchr(p1, '@');
    strncpy(password, p1 + 1, p2 - p1 - 1);

    p1 = p2;
    p2 = strchr(p1, ':');
    strncpy(ipAddr, p1 + 1, p2 - p1 - 1);

    p1 = p2;
    p2 = strchr(p1, '/');
    if (p2) {
        port = atoi(p1 + 1);
    } else {
        port = atoi(p1 + 1);
    }
    if ((servSock = this->Connect(ipAddr, port)) < 0) {
        return -1;
    }
    if (this->Login(servSock, username, password) < 0) {
        return -1;
    }
    return servSock;
}

// Login
int CommFtp::Login(int servfd, const char* username, const char* password)
{
    nServSocket_ = servfd;
    if (this->replayLf() != CODE_FTP_SERVICE_OK) {
        return -1;
    }
    this->command("USER", username);
    if (replayCode() != CODE_FTP_PASSWORD_REQUIRED) {
        return -1;
    }
    pUsername_ = username;
    this->command("PASS", password);
    if (replayCode() != CODE_FTP_LOGGED_IN) {
        return -1;
    }
    pPassword_ = password;
}

// Process
// void CommFtp::ThisProcesss()
// {
//     fd_set rset;
//     FD_ZERO(&rset);
//     int maxfd = nServSocket_ + 1;
//     while (bStart_)
//     {
//         FD_SET(nServSocket_, &rset);
//         if (select(maxfd, &rset, NULL, NULL, NULL) < 0)
//         {
//             continue;
//         }

//         if (FD_ISSET(nServSocket_, &rset))
//         {
//             switch (replayCode())
//             {
//             case CODE_FTP_SERVICE_OK:
//                 this->command("USER", pUsername_);
//                 break;
//             case CODE_FTP_PASSWORD_REQUIRED: // quit
//                 this->command("PASS", pPassword_);
//                 break;
//             case 502:
//                 FTP_PRINTF("502 Invalid command\n");
//                 break;
//             case CODE_FTP_QUIT: // quit
//                 bStart_ = false;
//                 break;
//             default:
//                 break;
//             }
//         }
//     }
//     bStart_ = false;
// }

// // 开始
// void CommFtp::StartRecvTcp()
// {
//     bStart_ = true;
//     pthread_create(&pThisThread_, NULL, CommFtpThreadProcess, this);
// }
// // 停止
// void CommFtp::StopRecvTcp()
// {
//     bStart_ = false;
//     pthread_cancel(pThisThread_);
// }

int CommFtp::command(const char* code, const char* arg)
{
    char buffer[1024] = { 0 };
    if (arg) {
        sprintf(buffer, "%s %s%s", code, arg, pStrLF_);
    } else {
        sprintf(buffer, "%s%s", code, pStrLF_);
    }
    FTP_PRINTF("> %s\n", buffer);
    if (send(nServSocket_, buffer, ( int )strlen(buffer), 0) < 0) {
        FTP_PRINTF("tcp send msg: %s error\n", buffer);
        return -1;
    }
    return 0;
}

int CommFtp::replayCode(const char* src)
{
    char recvline[1024] = { 0 };
    if (!src) {
        if (recv(nServSocket_, recvline, sizeof(recvline), 0) < 0) {
            return -1;
        }
    } else {
        strcpy(recvline, src);
    }
    printf("%s\n", recvline);
    char strCode[4] = { 0 };
    strncpy(strCode, recvline, 3);
    return atoi(strCode);
}

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

// pasv 返回新连接的socket
int CommFtp::Pasv()
{
    this->command("PASV");
    char recvline[1024] = { 0 };
    char sockHost[32]   = { 0 };
    if (recv(nServSocket_, recvline, sizeof(recvline), 0) < 0) {
        return -1;
    }
    FTP_PRINTF("%s\n", recvline);
    int sockPort = strtosrv(recvline, sockHost);
    return this->Connect(sockHost, sockPort);
}

// PWD
int CommFtp::Pwd()
{
    this->command("PWD");
    char recvline[1024] = { 0 };
    if (recv(nServSocket_, recvline, sizeof(recvline), 0) < 0) {
        return -1;
    }
    if (replayCode(recvline) != CODE_FTP_OK) {
        return -1;
    }
    int   i               = 0;
    char  currendir[1024] = { 0 };
    char* ptr             = recvline + 5;
    while (*(ptr) != '"') {
        currendir[i++] = *(ptr);
        ptr++;
    }
    printf("Dir is:%s\n", currendir);
    return 0;
}

// Ls
int CommFtp::Ls()
{
    int sockfd = this->Pasv();
    if (sockfd < 0) {
        return -1;
    }
    if (this->command("LS") < 0) {
        return -1;
    }
    int nread;
    for (;;) {
        char buffer[MAXBUF] = { 0 };
        if ((nread == recv(sockfd, buffer, MAXBUF, 0)) < 0) {
            FTP_PRINTF("recv error\n");
        } else if (nread == 0) {
            FTP_PRINTF("over\n");
            break;
        }
        printf("%s", buffer);
    }
}

// get
int CommFtp::Get(const char* srcfpName, const char* dstfpName)
{
    int sockfd = this->Pasv();
    if (sockfd < 0) {
        return -1;
    }
    if (this->command("RETR", srcfpName) < 0) {
        return -1;
    }
    int handle = open(dstfpName, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    for (;;) {
        int  nread;
        char buffer[MAXBUF] = { 0 };
        if ((nread = recv(sockfd, buffer, MAXBUF, 0)) < 0) {
            FTP_PRINTF("reveive error\n");
        } else if (nread == 0) {
            FTP_PRINTF("over\n");
            break;
        }
        FTP_PRINTF("recv len: %d\n", nread);
        if (write(handle, buffer, nread) != nread) {
            FTP_PRINTF("receive error from server!");
        }
    }
    if (close(sockfd) < 0) {
        FTP_PRINTF("close sockfd error\n");
    }
    if (close(handle) < 0) {
        FTP_PRINTF("close fp error\n");
    }
    return 0;
}

// Put
int CommFtp::Put(const char* fpName)
{
    int sockfd = this->Pasv();
    if (sockfd < 0) {
        return -1;
    }
    if (this->command("STOR", fpName) < 0) {
        return -1;
    }
    int handle = open(fpName, O_RDWR);
    for (;;) {
        int  nread;
        char buffer[MAXBUF] = { 0 };
        if ((nread = read(handle, buffer, MAXBUF)) < 0) {
            FTP_PRINTF("reveive error\n");
        } else if (nread == 0) {
            FTP_PRINTF("over\n");
            break;
        }
        if (write(sockfd, buffer, nread) != nread) {
            FTP_PRINTF("receive error from server!");
        }
    }
    if (close(sockfd) < 0) {
        FTP_PRINTF("close sockfd error\n");
    }
    if (close(handle) < 0) {
        FTP_PRINTF("close fp error\n");
    }
    return 0;
}

// QUIT
int CommFtp::Quit()
{
    this->command("QUIT");
}