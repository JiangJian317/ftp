#ifndef FTP_H
#define FTP_H

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FTP_PRINTF printf

#define MAXBUF (1024)

#define CODE_FTP_SERVICE_OK 220
#define CODE_FTP_PASSWORD_REQUIRED 331
#define CODE_FTP_LOGGED_IN 230
#define CODE_FTP_QUIT 221
#define CODE_FTP_OK 257
#define CODE_FTP_PASV_MODE 227
#define CODE_FTP_DATA_CON_OPENED 125

class CommFtp {
public:
    CommFtp();
    ~CommFtp();
    // 连接
    int Connect(const char* host, int port);
    // ftp://username:password@192.168.3.6:21
    int Connect(const char* ftpUrl);
    // Login
    int Login(int servfd, const char* username, const char* password);
    // // RecvTcp 开始处理数据
    // void StartRecvTcp();
    // // 停止
    // void StopRecvTcp();
    // // h后台线程
    // void ThisProcesss();
    // PASV
    int Pasv();
    // PWD
    int Pwd();
    // Ls
    int Ls();
    // GET
    int Get(const char* srcfpName, const char* dstfpName);
    // PUT
    int Put(const char* fpName);
    // QUIT
    int Quit();

private:
    // exec
    int command(const char* code, const char* arg = NULL);
    // reply code
    int replayCode(const char* src = NULL);
    //
    int replayLf();

private:
    int         nServSocket_;
    const char* pStrLF_;
    int         nErrorCode_;
    bool        bStart_;
    const char* pUsername_;
    const char* pPassword_;
    pthread_t   pThisThread_;
};

#endif