#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "common.h"
#include "log.h"
#include <unistd.h>
#include <stdio.h>
typedef enum CommandType
{
    ABOR,
    AUTH,
    CWD,
    DELE,
    LIST,
    MDTM,
    MKD,
    NLST,
    PASS,
    PASV,
    PORT,
    PWD,
    QUIT,
    RETR,
    RMD,
    RNFR,
    RNTO,
    SITE,
    SIZE,
    STOR,
    SYST,
    TYPE,
    USER,
    NOOP
} CommandType;

static const char *cmdlist_str[] =
    {
        "ABOR", "AUTH", "CWD", "DELE", "LIST", "MDTM", "MKD", "NLST", "PASS",
        "PASV", "PORT", "PWD", "QUIT", "RETR", "RMD", "RNFR", "RNTO", "SITE",
        "SIZE", "STOR", "SYST", "TYPE", "USER", "NOOP"};

void UnSupport(int connfd)
{
    write(connfd, "502 \r\n", 6);
}
int getCommandType(char *sentence)
{
    char *pch;
    char temp[8192];
    strcpy(temp, sentence);
    pch = strtok(temp, " \r\n");
    if (pch == NULL)
    {
        return -1;
    }
    for (int i = 0; i <= NOOP; i++)
    {
        if (strcmp(pch, cmdlist_str[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

int handleUser(char *setnetece, int connfd)
{
    LogInfo("handleUser");
    char buffer[] = "331 Guest login ok, send your complete e-mail address as password.\r\n";
    int n = write(connfd, buffer, strlen(buffer));
    return n;
}
int handleAuth(char *sentence, int connfd)
{
    LogInfo("handleAuth");
    UnSupport(connfd);
    return 0;
}
int handPass(char *sentence, int connfd)
{
    LogInfo("handlePass");
    char buffer[] = "230 -\r\n"
                    "230 - Welcome to \r\n"
                    "230 - School of Software\r\n"
                    "230 - FTP Archives at ftp.ssast.org\r\n"
                    "230 -\r\n"
                    "230 - This site is provided as a public service by School of\r\n"
                    "230 - Software.Use in violation of any applicable laws is strictly\r\n"
                    "230 - prohibited.We make no guarantees, explicit or implicit, about the\r\n"
                    "230 - contents of this site.Use at your own risk.\r\n"
                    "230 -\r\n"
                    "230 Guest login ok, access restrictions apply.\r\n";
    write(connfd, buffer, strlen(buffer));
    return 0;
}

int handlePwd(char *sentence, Status *status)
{
    LogInfo("handlePwd");
    char path[256];
    getcwd(path, 256);
    char buffer[256] = "257 \"";
    strcat(buffer, path);
    strcat(buffer, "\"\r\n");
    write(status->connfd, buffer, strlen(buffer));
    return 0;
}
int handleSyst(char *sentence, int connfd)
{

    char buffer[] = "215 UNIX Type: L8\r\n";
    return write(connfd, buffer, strlen(buffer));
}
int handleType(char *sentence, int connfd)
{
    char *pch;
    pch = strtok(sentence, " \r\n");
    pch = strtok(NULL, " \r\n");
    if (strcmp(pch, "I") == 0)
    {
        char buffer[] = "200 Type set to I.\r\n";
        write(connfd, buffer, strlen(buffer));
    }
    return 0;
}
int handlePasv(char *setnetent, int connfd)
{
    struct sockaddr_in inaddr;
    socklen_t connectedAddrLen = sizeof(inaddr);
    getsockname(connfd, (struct sockaddr *)&inaddr, &connectedAddrLen);
    int ip_long = inaddr.sin_addr.s_addr;
    int ip[4];
    for (int i = 0; i < 4; i++)
    {
        ip[i] = (ip_long >> (8 * i)) & 0xff;
    }
    int port = getRandomInt(12800, 65535);

    char buffer[256];
    sprintf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", ip[0], ip[1], ip[2], ip[3], port / 256, port % 256);
    LogInfo(buffer);
    write(connfd, buffer, strlen(buffer));
    return 0;
}

int handleList(char *setnetent, Status *status)
{
    // TODO
    return 0;
}
int handlePort(char *sentence, Status *status)
{
    // TODO : deal with params
    char buffer[] = "200 PORT command successful.\r\n";
    status->mode = Initiative;
    // TODO
    //
    write(status->connfd, buffer, strlen(buffer));
    return 0;
}

int handleRequest(char *sentence, Status *status)
{
    int type = getCommandType(sentence);
    int connfd = status->connfd;
    switch (type)
    {
    case USER:
        handleUser(sentence, connfd);
        break;
    case AUTH:
        handleAuth(sentence, connfd);
        break;
    case PASS:
        handPass(sentence, connfd);
        break;
    case PWD:
        handlePwd(sentence, status);
        break;
    case SYST:
        handleSyst(sentence, connfd);
        break;
    case LIST:
        handleList(sentence, status);
        break;
    case TYPE:
        handleType(sentence, connfd);
        break;
    case PASV:
        handlePasv(sentence, connfd);
        break;
    case PORT:
        handlePort(sentence, status);
        break;
    default:
        UnSupport(connfd);
        break;
    }
    return 0;
}