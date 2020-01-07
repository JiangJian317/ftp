#include "ftp.h"

#define MAXBUF 256

int main(int argc, char* argv[])
{
    CommFtp ftp;
    // int servSockfd = ftp.fConnect("192.168.3.6", 21);
    // if (servSockfd < 0)
    // {
    //     printf("connect server error\n");
    //     return 0;
    // }
    // ftp.fLogin(servSockfd, "ftpuser", "howen123");
    if (ftp.fConnect("ftp://DonWang:123456@192.168.3.113:21") < 0) {
        printf("connect server error\n");
        return 0;
    }
    while (1) {
        printf("ftp> ");
        char rbuffer[MAXBUF] = { 0 };
        scanf("%s", rbuffer);
        int nread = -1;
        if (strncmp(rbuffer, "pasv", 4) == 0) {
            ftp.fPasv();
        } else if (strncmp(rbuffer, "pwd", 3) == 0) {
            ftp.fPwd();
        } else if (strncmp(rbuffer, "quit", 4) == 0) {
            ftp.fQuit();
            break;
        } else if (strncmp(rbuffer, "cd", 2) == 0) {
            printf("Dir> ");
            char rbuffer[MAXBUF];
            scanf("%s", rbuffer);
            ftp.fCd(rbuffer);
        } else if (strncmp(rbuffer, "ls", 2) == 0) {
            ftp.fLs();
        } else if (strncmp(rbuffer, "get", 3) == 0) {
            printf("remote-file> ");
            char rbuffer[MAXBUF];
            scanf("%s", rbuffer);
            printf("local-file> ");
            char lbuffer[MAXBUF] = { 0 };
            scanf("%s", lbuffer);
            ftp.fGet(rbuffer, lbuffer);
        } else if (strncmp(rbuffer, "put", 3) == 0) {
            printf("local-file> ");
            char lbuffer[MAXBUF] = { 0 };
            scanf("%s", lbuffer);
            ftp.fPut(lbuffer);
        }
    }
    return 0;
}
