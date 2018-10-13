#ifndef COMMON_H
#define COMMON_H

typedef struct Status
{
    int connfd;
    char directory[256];
    int mode;
    int local_addr;
    int local_port;
    int remote_addr;
    int remote_port;
} Status;
#define Initiative 0
#define Passive 1
extern const char welcome_msg[];
int getRandomInt(int min, int max);
#endif