#ifndef COMMON_H
#define COMMON_H
#define BUFFERSIZE 2048
#define PATHSIZE 1024
typedef struct Status
{
    int connfd;
    char directory[PATHSIZE];
    int mode;
    int pasv_local_addr;
    int pasv_local_port;
    int pasv_socket_fd;
    int pasv_connfd;
    int remote_addr;
    int remote_port;
} Status;

#define Initiative 2
#define Passive 1
extern const char welcome_msg[];
int getRandomInt(int min, int max);
int createListenScoket(int port);
#endif