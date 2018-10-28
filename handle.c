#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "log.h"
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <sys/sendfile.h>
#include <stdlib.h>
#include <pthread.h>


void Error502UnSupportCommand(int connfd) {
    reply(connfd, "502 \r\n");
}

void Error550CommandNotExecuted(int connfd) {
    reply(connfd, "550 \r\n");
}

// 错误的指令参数
void Error504InvalidArgument(int connfd) {
    reply(connfd, "504 \r\n");
}

void Error553InvalidFileName(int connfd) {
    reply(connfd, "553 Invalid filename.\r\n");
}

void Error530PermissionDenied(int connfd) {
    reply(connfd, "530 Permission Denied.\r\n");
}

void Error450OpenFile(int connfd) {
    reply(connfd, "450 file error.\r\n");
}

void Error426FileTransferInterrupt(int connfd) {
    reply(connfd, "426 File transfer interruption.\r\n");
}

int handleUser(char *setnetece, int connfd) {
    LogInfo("handleUser");
    reply(connfd, "331 Guest login ok, send your complete e-mail address as password.\r\n");
    return 0;
}

int handleAuth(char *sentence, int connfd) {
    LogInfo("handleAuth");
    Error502UnSupportCommand(connfd);
    return 0;
}

int handPass(char *sentence, int connfd) {
    LogInfo("handlePass");
    char buffer[] = "230 -\r\n";
    // "230 - Welcome to \r\n"
    // "230 - School of Software\r\n"
    // "230 - FTP Archives at ftp.ssast.org\r\n"
    // "230 -\r\n"
    // "230 - This site is provided as a public service by School of\r\n"
    // "230 - Software.Use in violation of any applicable laws is strictly\r\n"
    // "230 - prohibited.We make no guarantees, explicit or implicit, about the\r\n"
    // "230 - contents of this site.Use at your own risk.\r\n"
    // "230 -\r\n"
    // "230 Guest login ok, access restrictions apply.\r\n";
    reply(connfd, buffer);
    return 0;
}

int handleFeat(int connfd) {
    reply(connfd, "211 hello\r\n");
    return 0;
}

int handlePwd(ServerStatus *status) {
    LogInfo("handlePwd");
    char buffer[PATHSIZE] = "257 \"";
    strcat(buffer, status->directory);
    strcat(buffer, "\"\r\n");
    reply(status->connfd, buffer);
    return 0;
}

int handleSyst(int connfd) {
    reply(connfd, "215 UNIX Type: L8\r\n");
    return 0;
}

int handleType(char *sentence, int connfd) {
    char *type = NULL;
    type = getParam(sentence);
    if (type == NULL) {
        Error504InvalidArgument(connfd);
        return -1;
    }
    if (strcmp(type, "I") == 0) {
        reply(connfd, "200 Type set to I.\r\n");
    } else if (strcmp(type, "A") == 0) {
        reply(connfd, "200 Type set to A.\r\n");
    }
    free(type);
    return 0;
}

int handlePasv(ServerStatus *status) {
    struct sockaddr_in inaddr;
    socklen_t connectedAddrLen = sizeof(inaddr);
    getsockname(status->connfd, (struct sockaddr *) &inaddr, &connectedAddrLen);
    int ip_long = inaddr.sin_addr.s_addr;
    int ip[4];
    for (int i = 0; i < 4; i++) {
        ip[i] = (ip_long >> (8 * i)) & 0xff;
    }
    int port = getRandomInt(30000, 40000); // 使用30000-40000之间端口进行数据传输
    status->pasv_socket_fd = createListenSocket(port);
    if (status->pasv_socket_fd == -1) {
        Error504InvalidArgument(status->connfd);
        return -1;
    }
    char buffer[BUFFERSIZE];
    sprintf(buffer, "227 Entering PassiveMode Mode (%d,%d,%d,%d,%d,%d)\r\n", ip[0], ip[1], ip[2], ip[3], port / 256,
            port % 256);
    LogInfo(buffer);
    reply(status->connfd, buffer);
    status->mode = PassiveMode;
    return 0;
}

void sendListData(DIR *dir, int connfd) {
    char buffer[BUFFERSIZE];
    struct dirent *ent;
    struct stat statbuf;
    struct tm *_time;
    time_t rawtime;
    char timebuff[BUFFERSIZE];
    while ((ent = readdir(dir))) {
        char perms[10] = "rwxrwxrwx";
        stat(ent->d_name, &statbuf);
        /* Convert time_t to tm struct */
        rawtime = statbuf.st_mtime;
        _time = localtime(&rawtime);
        strftime(timebuff, BUFFERSIZE, "%b %d %H:%M", _time);
        sprintf(buffer,
                "%c%s %d ftp ftp  %ld %s %s\r\n",
                (ent->d_type == DT_DIR) ? 'd' : '-',
                perms,
                1,
                statbuf.st_size,
                timebuff,
                ent->d_name);
        reply(connfd, buffer);
    }
}

int handleList(ServerStatus *status) {
    int activeConnfd;
    int pasvConnfd;
    DIR *dir = NULL;
    switch (status->mode) {
        case PassiveMode:
            pasvConnfd = accept(status->pasv_socket_fd, NULL, NULL);
            LogInfo("Accept List Connect Request");
            if (pasvConnfd == -1) {
                LogError("accept Error");
                return -1;
            }
            reply(status->connfd, "150 Here comes the directory listing.\r\n");
            dir = opendir(status->directory);
            sendListData(dir, pasvConnfd);
            reply(status->connfd, "226 Directory send OK.\r\n");
            close(pasvConnfd);
            close(status->pasv_socket_fd);
            break;
        case ActiveMode:
            activeConnfd = createClientSocket(status->remote_addr, status->remote_port);
            if (activeConnfd != -1) {
                reply(status->connfd, "150 Here comes the directory listing.\r\n");
                dir = opendir(status->directory);
                sendListData(dir, activeConnfd);
                reply(status->connfd, "226 Directory send OK.\r\n");
                close(activeConnfd);
            }
            break;
        default:
            break;
    }
    return 0;
}

int handlePort(char *sentence, ServerStatus *status) {
    char *param = getParam(sentence);
    if (param == NULL) {
        Error504InvalidArgument(status->connfd);
        return -1;
    }
    status->mode = ActiveMode;
    int ip[4];
    int port_high, port_low;
    sscanf(param, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &port_high, &port_low);
    sprintf(status->remote_addr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    status->remote_port = port_high * 256 + port_low;
    LogInfo("PORT");
    reply(status->connfd, "200 PORT command successful.\r\n");
    return 0;
}

int handleCwd(char *setnetent, ServerStatus *status) {
    char origin_cwd[PATHSIZE];
    getcwd(origin_cwd, PATHSIZE);
    chdir(status->directory);
    LogInfo("handleCwd");
    char *path = NULL;
    path = getParam(setnetent);
    if (path != NULL) {
        if (chdir(path) == 0) {
            getcwd(status->directory, PATHSIZE);
            reply(status->connfd, "250 Directory successfully changed.\r\n");
            chdir(origin_cwd);
        } else {
            Error550CommandNotExecuted(status->connfd);
            chdir(origin_cwd);
            return -1;
        }
    } else {
        // TODO Error Dealing
    }
    return 0;
}


int handleRetr(char *sentence, ServerStatus *status) {
    char originWorkDirectory[PATHSIZE];
    int fd = -1;
    int activeConnfd = -1;
    int pasvConnfd = -1;
    struct stat statbuf;
    ssize_t n;
    off_t off = 0;
    getcwd(originWorkDirectory, PATHSIZE);
    LogInfo("handleRetr");
    char *filename = NULL;
    filename = getParam(sentence);
    if (filename != NULL) {
        chdir(status->directory);
        fd = open(filename, O_RDONLY);
        chdir(originWorkDirectory);
    } else {
        Error553InvalidFileName(status->connfd);
        return -1;
    }
    if (fd < 0) {
        Error450OpenFile(status->connfd);
        close(fd);
        return -1;
    }

    switch (status->mode) {
        case PassiveMode:
            reply(status->connfd, "150 Opening BINARY mode data connection.\r\n");
            pasvConnfd = accept(status->pasv_socket_fd, NULL, NULL);
            close(status->pasv_socket_fd);

            if (pasvConnfd == -1) {
                LogError("accept Error");
                chdir(originWorkDirectory);
                return -1;
            }
            fstat(fd, &statbuf);
            n = sendfile(pasvConnfd, fd, &off, statbuf.st_size);
            if (n != statbuf.st_size) {
                LogError("SendFile Error");
            }
            close(pasvConnfd);
            reply(status->connfd, "226 Transfer complete.\r\n");
            break;
        case ActiveMode:
            activeConnfd = createClientSocket(status->remote_addr, status->remote_port);
            if (activeConnfd != -1) {
                reply(status->connfd, "150 Opening BINARY mode data connection.\r\n");
                fstat(fd, &statbuf);
                n = sendfile(activeConnfd, fd, &off, statbuf.st_size);
                if (n != statbuf.st_size) {
                    LogError("SendFile Error");
                }
                close(activeConnfd);
                reply(status->connfd, "226 Transfer complete.\r\n");
            }
            else{
                Error426FileTransferInterrupt(status->connfd);
            }
            break;
        default:
            break;
    }

    return 0;
}

int receiveStorData(int fd, int connfd) // 从远程主机接收数据
{
    ssize_t read_n = 0, write_n = 0;
    int totalsize = 0;
    char buffer[DATABUFFERSIZE];
    do {
        read_n = read(connfd, buffer, DATABUFFERSIZE);
        totalsize += read_n;
        if (read_n < 0) {
            LogError("Stor read Error");
            close(fd);
            close(connfd);
            return -1;
        }
        write_n = write(fd, buffer, read_n);
        if (write_n < 0) {
            LogError("Stor write Error");
            close(fd);
            close(connfd);
            return -1;
        }
    } while (read_n > 0);
    close(fd);
    close(connfd);
    printf("received %d bytes\n", totalsize);
    return 0;
}


int handleStor(char *sentence, ServerStatus *status) {
    LogMessage(sentence);
    char originWorkDirectory[PATHSIZE];
    int pasvConnfd = -1;
    int activeConnfd = -1;
    getcwd(originWorkDirectory, PATHSIZE);
    chdir(status->directory);
    LogInfo("handleStor");
    char *filename = getParam(sentence);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | S_IRUSR | S_IWUSR);
    chmod(filename, 777); // 添加读写权限
    free(filename);
    chdir(originWorkDirectory);

    if (fd < 0) {
        LogError("Error Open File Error!");
        reply(status->connfd, "550 Can not open file to write.\r\n");
        return -1;
    }
    switch (status->mode) {
        case PassiveMode:
            pasvConnfd = accept(status->pasv_socket_fd, NULL, NULL);
            if (pasvConnfd == -1) {
                Error550CommandNotExecuted(status->connfd);
                LogError("Stor accept Error");
                return -1;
            }
            reply(status->connfd, "150 Opening BINARY mode data connection.\r\n");
            if (receiveStorData(fd, pasvConnfd) == -1) {
                Error426FileTransferInterrupt(status->connfd);
                LogError("Stor PassiveMode Receive Error!");
                return -1;
            }
            close(status->pasv_socket_fd);
            reply(status->connfd, "226 Transfer complete.\r\n");
            break;
        case ActiveMode:
            activeConnfd = createClientSocket(status->remote_addr, status->remote_port);
            if (activeConnfd == -1) {
                Error550CommandNotExecuted(status->connfd);
                LogError("Stor Connect Client Error");
                return -1;
            }
            reply(status->connfd, "150 Opening BINARY mode data connection.\r\n");
            if (receiveStorData(fd, activeConnfd) == -1) {
                Error426FileTransferInterrupt(status->connfd);
                LogError("Stor ActiveMode Receive Error!");
                return -1;
            }
            reply(status->connfd, "226 Transfer complete.\r\n");
            break;
        default:
            break;
    }
    LogInfo("handleStor Finished! Transfer Complete!");
    return 0;
}

int handleRnfr(char *sentence, ServerStatus *status) {
    LogInfo("handleRnfr");
    char *filename = getParam(sentence);
    strcpy(status->old_filename, filename);
    reply(status->connfd, "350 \r\n");
    free(filename);
    return 0;
}

int handleRnto(char *sentence, ServerStatus *status) {
    char *filename = getParam(sentence);
    char origin_cwd[PATHSIZE];
    getcwd(origin_cwd, PATHSIZE);
    chdir(status->directory);
    rename(status->old_filename, filename);
    chdir(origin_cwd);
    reply(status->connfd, "250 \r\n");
    free(filename);
    return 0;
}

int handleAbor(ServerStatus *status) {
    // TODO : stop file transfer
    Error426FileTransferInterrupt(status->connfd);
    return 0;
}


int handleQuit(ServerStatus *status) {
    reply(status->connfd, "221 GoodBye~~~\r\n");
    free(status);
    pthread_exit(NULL);
}

int handleMkd(char *sentence, ServerStatus *status) {

    char *directoryName = getParam(sentence);
    if (directoryName == NULL) {
        Error553InvalidFileName(status->connfd);
        free(directoryName);
        return -1;
    }
    char originWorkDirectory[PATHSIZE];
    getcwd(originWorkDirectory, PATHSIZE);
    chdir(status->directory);
    if (mkdir(directoryName, S_IRWXG | S_IRWXO | S_IRWXU) == 0) {
        char buffer[BUFFERSIZE];
        sprintf(buffer, "257 \"%s\" created.\r\n", directoryName);
        reply(status->connfd, buffer);
    } else {
        Error553InvalidFileName(status->connfd);
        chdir(originWorkDirectory);
        free(directoryName);
        return -1;
    }
    chdir(originWorkDirectory);
    free(directoryName);
    return 0;
}

int handleRmd(char *sentence, ServerStatus *status) {
    char *directoryName = getParam(sentence);
    if (directoryName == NULL) {
        Error504InvalidArgument(status->connfd);
        return -1;
    }
    char originWorkDirectory[PATHSIZE];
    getcwd(originWorkDirectory, PATHSIZE);
    chdir(status->directory);
    if (rmdir(directoryName) == 0) {
        reply(status->connfd, "250 directory deleted.\r\n");
    } else {
        Error530PermissionDenied(status->connfd);
        chdir(originWorkDirectory);
        free(directoryName);
        return -1;
    }
    chdir(originWorkDirectory);
    free(directoryName);
    return 0;
}

int handleDele(char *sentence, ServerStatus *status) {
    char *fileName = getParam(sentence);
    if (fileName == NULL) {
        Error504InvalidArgument(status->connfd);
        return -1;
    }
    char originWorkDirectory[PATHSIZE];
    getcwd(originWorkDirectory, PATHSIZE);
    chdir(status->directory);
    if (unlink(fileName) == 0) {
        reply(status->connfd, "250 File Deleted.\r\n");
    } else {
        Error530PermissionDenied(status->connfd);
        chdir(originWorkDirectory);
        free(fileName);
        return -1;
    }
    chdir(originWorkDirectory);
    free(fileName);
    return 0;
}

int handleRequest(char *sentence, ServerStatus *status) {
    int type = getCommandType(sentence);
    int connfd = status->connfd;
    switch (type) {
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
            handlePwd(status);
            break;
        case SYST:
            handleSyst(connfd);
            break;
        case LIST:
            handleList(status);
            break;
        case TYPE:
            handleType(sentence, connfd);
            break;
        case PASV:
            handlePasv(status);
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
        case STOR:
            handleStor(sentence, status);
            break;
        case RNFR:
            handleRnfr(sentence, status);
            break;
        case RNTO:
            handleRnto(sentence, status);
            break;
        case FEAT:
            handleFeat(connfd);
            break;
        case QUIT:
            handleQuit(status);
            break;
        case ABOR:
            handleAbor(status);
            break;
        case MKD:
            handleMkd(sentence, status);
            break;
        case RMD:
            handleRmd(sentence, status);
            break;
        case DELE:
            handleDele(sentence, status);
            break;
        default:
            LogError("Unsupported Command");
            Error502UnSupportCommand(connfd);
            break;
    }
    return 0;
}