#ifndef FTP_H
#define FTP_H

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

class CommFtp {
public:
    CommFtp();
    ~CommFtp();
    // 连接
    int fConnect(const char* host, int port);
    // ftp://username:password@192.168.3.6:21
    int fConnect(const char* ftpUrl);
    // Login
    int fLogin(int servfd, const char* username, const char* password);
    // PASV
    int fPasv();
    // PWD
    int fPwd();
    // Ls
    int fLs();
    // GET
    int fGet(const char* srcfpName, const char* dstfpName);
    // PUT
    int fPut(const char* fpName);
    // CD
    int fCd(const char* dir);
    // QUIT
    int fQuit();

private:
    // exec
    std::string command(const char* code, const char* arg = NULL);
    //
    std::string recvByChar();
    //
    int replayLf();
private:
    int         nServSocket_;
    const char* pStrLF_;
    const char* pUsername_;
    const char* pPassword_;
};

#endif