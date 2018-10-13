#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "common.h"
#include "log.h"
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <sys/sendfile.h>

#ifndef DT_DIR
#define DT_DIR 4
#endif
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

void Fobbiden(int connfd)
{
    write(connfd, "550 \r\n", 6);
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

    char buffer[PATHSIZE] = "257 \"";
    strcat(buffer, status->directory);
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
    else if (strcmp(pch, "A") == 0)
    {
        char buffer[] = "200 Type set to A.\r\n";
        write(connfd, buffer, strlen(buffer));
    }
    return 0;
}
int handlePasv(char *setnetent, Status *status)
{
    struct sockaddr_in inaddr;
    socklen_t connectedAddrLen = sizeof(inaddr);
    getsockname(status->connfd, (struct sockaddr *)&inaddr, &connectedAddrLen);
    int ip_long = inaddr.sin_addr.s_addr;
    int ip[4];
    for (int i = 0; i < 4; i++)
    {
        ip[i] = (ip_long >> (8 * i)) & 0xff;
    }
    int port = getRandomInt(12800, 65535);
    char msg[80];
    sprintf(msg, "Create Socket on port: %d", port);
    LogInfo(msg);
    status->pasv_local_port = port;
    status->pasv_local_addr = ip_long;
    status->pasv_socket_fd = createListenScoket(port);

    char buffer[256];
    sprintf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", ip[0], ip[1], ip[2], ip[3], port / 256, port % 256);
    LogInfo(buffer);
    write(status->connfd, buffer, strlen(buffer));
    status->mode = Passive;
    return 0;
}

int handleList(char *setnetent, Status *status)
{
    // TODO parse path
    char buffer[BUFFERSIZE];

    if (status->mode == Passive)
    {
        int connfd;
        if (status->pasv_connfd == 0)
        {
            connfd = accept(status->pasv_socket_fd, NULL, NULL);
            status->pasv_connfd = connfd;
            if (connfd == -1)
            {
                LogError("accept Error");
                return -1;
            }
        }

        strcpy(buffer, "150 Here comes the directory listing.\r\n");
        write(status->connfd, buffer, strlen(buffer));

        char path[PATHSIZE];
        strcpy(path, status->directory);
        LogWarring(path);
        DIR *dir = opendir(path);

        struct dirent *ent;
        struct stat statbuf;
        struct tm *_time;
        time_t rawtime;
        char timebuff[80];
        while (ent = readdir(dir))
        {
            char perms[10] = "rwxrwxrwx";
            stat(ent->d_name, &statbuf);
            /* Convert time_t to tm struct */
            rawtime = statbuf.st_mtime;
            _time = localtime(&rawtime);

            strftime(timebuff, 80, "%b %d %H:%M", _time);

            sprintf(buffer,
                    "%c%s %d ftp ftp  %d %s %s\r\n",
                    (ent->d_type == DT_DIR) ? 'd' : '-',
                    perms,
                    1,
                    statbuf.st_size,
                    timebuff,
                    ent->d_name);
            LogWarring(buffer);
            write(status->pasv_connfd, buffer, strlen(buffer));
        }
    }

    strcpy(buffer, "226 Directory send OK.\r\n");
    write(status->connfd, buffer, strlen(buffer));
    close(status->pasv_connfd);
    close(status->pasv_socket_fd);
    status->pasv_connfd = 0;
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

int handleCwd(char *setnetent, Status *status)
{
    char origin_cwd[PATHSIZE];
    getcwd(origin_cwd, PATHSIZE);

    chdir(status->directory);

    LogInfo("handleCwd");
    char *pch;
    pch = strtok(setnetent, " \r\n");
    pch = strtok(NULL, " \r\n");
    LogError(pch);
    if (pch != NULL)
    {

        if (chdir(pch) == 0)
        {
            getcwd(status->directory, PATHSIZE);
            char buffer[] = "250 Directory successfully changed.\r\n";
            write(status->connfd, buffer, strlen(buffer));
            chdir(origin_cwd);
        }
        else
        {
            Fobbiden(status->connfd);
            chdir(origin_cwd);
            return -1;
        }
    }
}

int sendFile(int fd, int connfd)
{
    char buffer[BUFFERSIZE];
    while (read(fd, buffer, BUFFERSIZE) > 0)
    {
        int n = write(connfd, buffer, strlen(buffer));
        if (n < 0)
        {
            return n;
        }
    }
    return 0;
}

int handleRetr(char *sentence, Status *status)
{

    char buffer[BUFFERSIZE];
    LogInfo("handleRetr");
    char *pch;
    pch = strtok(sentence, " \r\n");
    pch = strtok(NULL, " \r\n");
    LogInfo(pch);
    char file[PATHSIZE];
    strcpy(file, status->directory);
    strcat(file, "/");
    strcat(file, pch);
    LogWarring(file);
    int fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        write(status->connfd, "450 \r\n", 6);
        close(fd);
        return -1;
    }
    switch (status->mode)
    {
    case Passive:
        strcpy(buffer, "150 Opening BINARY mode data connection.\r\n");
        write(status->connfd, buffer, strlen(buffer));

        int connfd = accept(status->pasv_socket_fd, NULL, NULL);
        close(status->pasv_socket_fd);
        status->pasv_connfd = connfd;
        if (connfd == -1)
        {
            LogError("accept Error");
            return -1;
        }

        struct stat statbuf;
        fstat(fd, &statbuf);
        off_t off = 0;
        ssize_t n = sendfile(connfd, fd, &off, statbuf.st_size);
        if (n != statbuf.st_size)
        {
            LogError("SendFile Error");
        }
        //sendFile(fd, connfd);
        close(connfd);

        strcpy(buffer, "226 Transfer complete.\r\n");
        write(status->connfd, buffer, strlen(buffer));
        break;
    case Initiative:
        break;
    }

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
        handlePasv(sentence, status);
        break;
    case PORT:
        handlePort(sentence, status);
        break;
    case CWD:
        handleCwd(sentence, status);
        break;

    case RETR:
        handleRetr(sentence, status);
        break;
    default:
        UnSupport(connfd);
        break;
    }
    return 0;
}