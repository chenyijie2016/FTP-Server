#ifndef COMMON_H
#define COMMON_H
#define BUFFERSIZE 512
#define PATHSIZE 260
#define DATABUFFERSIZE 8192
#include <ctype.h>
#include <stdint.h>
typedef struct Status {
    int connfd;
    char directory[PATHSIZE];
    int mode;
    int pasv_socket_fd; // PASV 指令的本地监听 socket handle
    char remote_addr[20];
    int remote_port;
    char old_filename[PATHSIZE];
} ServerStatus;
typedef enum CommandType {
    ABOR,
    AUTH,
    CWD,
    DELE,
    FEAT,
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
} CommandType;

#define ActiveMode 2
#define PassiveMode 1

int getRandomInt(int min, int max);

int createListenSocket(int port);

int createClientSocket(char *remote_addr, uint16_t remote_port);

int getCommandType(char *sentence);

char *getParam(char *cmd);

int reply(int connfd, char *msg);

#endif