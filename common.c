#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

static const char *CommandList[] =
    {"ABOR", "AUTH", "CWD", "DELE", "FEAT", "LIST", "MDTM", "MKD", "NLST", "PASS",
     "PASV", "PORT", "PWD", "QUIT", "RETR", "RMD", "RNFR", "RNTO", "SITE",
     "SIZE", "STOR", "SYST", "TYPE", "USER"};

int getRandomInt(int min, int max)
{
    return rand() % (max - min) + min;
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
    for (int i = 0; i <= USER; i++)
    {
        if (strcmp(pch, CommandList[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

int createListenSocket(int port)
{
    int listenfd; //监听socket和连接socket不一样，后者用于数据传输
    struct sockaddr_in addr;
    //创建socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    //设置本机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY; //监听"0.0.0.0"

    //将本机的ip和port与socket绑定
    if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    //开始监听socket
    if (listen(listenfd, 10) == -1)
    {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    LogInfo("Create Listen Socket On Port");
    return listenfd;
}

// 创建与远程主机的连接
int createClientSocket(char *remote_addr, uint16_t remote_port)
{
    int sockfd;
    struct sockaddr_in addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    //设置目标主机的ip和port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remote_port);
    if (inet_pton(AF_INET, remote_addr, &addr.sin_addr) <= 0)
    { //转换ip地址:点分十进制-->二进制
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    //连接上目标主机（将socket和目标主机连接）-- 阻塞函数
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        printf("Remote Adress:%s port:%d\n", remote_addr, remote_port);
        return -1;
    }
    return sockfd;
}

// 获取命令的第一个参数
char *getParam(char *cmd)
{
    char *param = (char *)malloc(sizeof(char) * PATHSIZE);
    char temp[PATHSIZE];
    strcpy(temp, cmd);
    char *pch = strtok(temp, " \r\n");
    pch = strtok(NULL, " \r\n");
    if (pch != NULL)
        strcpy(param, pch);
    else
    {
        free(param);
        LogError("getParam Error!");
        return NULL;
    }
    return param;
}

int reply(int connfd, char *msg)
{

    write(connfd, msg, strlen(msg));
    return 0;
}
