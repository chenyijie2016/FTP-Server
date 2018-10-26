#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "log.h"
#include "common.h"
#include "handle.h"

int listenfd;

void *server(void *arg) {
    fd_set rfds;
    struct timeval tv;
    int retval, maxfd;
    char sentence[BUFFERSIZE];
    ssize_t p;
    ServerStatus *status = (ServerStatus *) arg;
    int connfd = status->connfd;
    LogInfo("Start Handle New Connection");
    reply(connfd, "220 FTP Server ready.\r\n");
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(connfd, &rfds);
        maxfd = 0;
        if (maxfd < connfd)
            maxfd = connfd;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            LogError("Select Error");
            exit(1);
        } else if (retval == 0) {
            continue;
        } else {
            if (FD_ISSET(connfd, &rfds)) {
                p = 0;
                while (1) {
                    ssize_t n = read(connfd, sentence + p, 8191 - p);
                    if (n < 0) {
                        printf("Error read(): %s(%d)\n", strerror(errno), errno);
                        close(connfd);
                        return 0;
                    } else if (n == 0) {
                        break;
                    } else {
                        p += n;
                        if (sentence[p - 1] == '\n') {
                            break;
                        }
                    }
                }
                if (p == 0) {
                    LogError("Client Disconnect!");
                    free(status);
                    pthread_exit(NULL);
                }
                handleRequest(sentence, status);
            }
        }
    }
}

void stop_server(int sig) {
    close(listenfd);
    exit(0);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    int port = 0;
    char dir[256] = "/tmp";

    //处理命令行参数
    if (argc <= 2) {
        printf("Usage: server -port p [-root r]\n\t-port p: port number\n\t-root t: working directory, default: /tmp\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-port") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[i + 1]);
                if (port == 0) {
                    LogError("Invalid port parameter! Exit");
                    return 0;
                }
            } else {
                LogError("Invalid port parameter! Exit");
                return 0;
            }
        }

        if (strcmp(argv[i], "-root") == 0) {
            if (i + 1 < argc) {
                strcpy(dir, argv[i + 1]);
            } else {
                LogError("Invalid root parameter! Exit");
                return 0;
            }
        }
    }
    chdir(dir);

    listenfd = createListenSocket(port);
    if (listenfd == -1) //处理绑定错误，退出
    {
        LogError("Can not bind listen port");
        exit(0);
    }
    char start_msg[50];
    sprintf(start_msg, "Start Server at port: %d", port);
    LogInfo(start_msg);
    //持续监听连接请求
    signal(SIGINT, stop_server);
    while (1) {
        //等待client的连接 -- 阻塞函数
        int *connfd = (int *) malloc(sizeof(int));
        ServerStatus *status = (ServerStatus *) malloc(sizeof(ServerStatus));
        memset((void *) status, 0, sizeof(ServerStatus));
        status->mode = ActiveMode; //默认为主动模式
        if ((*connfd = accept(listenfd, NULL, NULL)) == -1) {
            LogError("Can not accept socket!");
            close(listenfd);
            exit(0);
            continue;
        }
        LogInfo("receive Connection");
        pthread_t p;
        status->connfd = *connfd;
        strcpy(status->directory, dir);
        pthread_create(&p, NULL, server, (void *) status);
    }

    close(listenfd);
}
