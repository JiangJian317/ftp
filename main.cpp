#include "ftp.h"

int main(int argc, char* argv[])
{
    CommFtp ftp;
    // int servSockfd = ftp.Connect("192.168.3.6", 21);
    // if (servSockfd < 0)
    // {
    //     FTP_PRINTF("connect server error\n");
    //     return 0;
    // }
    // ftp.Login(servSockfd, "ftpuser", "howen123");
    if (ftp.Connect("ftp://ftpuser:howen123@192.168.3.6:21") < 0) {
        FTP_PRINTF("connect server error\n");
        return 0;
    }
    while (1) {
        printf("ftp> ");
        char rbuffer[MAXBUF] = { 0 };
        scanf("%s", rbuffer);
        int nread = -1;
        if (strncmp(rbuffer, "pasv", 4) == 0) {
            ftp.Pasv();
        } else if (strncmp(rbuffer, "pwd", 3) == 0) {
            ftp.Pwd();
        } else if (strncmp(rbuffer, "quit", 4) == 0) {
            ftp.Quit();
            break;
        } else if (strncmp(rbuffer, "cwd", 3) == 0) {
        } else if (strncmp(rbuffer, "ls", 2) == 0) {
            ftp.Ls();
        } else if (strncmp(rbuffer, "get", 3) == 0) {
            printf("remote-file> ");
            char rbuffer[MAXBUF] = "COM3_2019-09-20_09-17-29.log";
            // scanf("%s", rbuffer);
            printf("local-file> ");
            char lbuffer[MAXBUF] = { 0 };
            scanf("%s", lbuffer);
            ftp.Get(rbuffer, lbuffer);
        } else if (strncmp(rbuffer, "put", 3) == 0) {
            printf("local-file> ");
            char lbuffer[MAXBUF] = { 0 };
            scanf("%s", lbuffer);
            ftp.Put(lbuffer);
        }
    }
    return 0;
}
