#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "log.h"
#include "common.h"
#include "handle.h"
void *server(void *arg)
{
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd;
	char sentence[8192];
	int p;
	Status *status = (Status *)arg;
	int connfd = status->connfd;
	//char buffer[1024];
	LogInfo("Start Handle New Connection");
	write(connfd, welcome_msg, strlen(welcome_msg));
	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(connfd, &rfds);
		maxfd = 0;
		if (maxfd < connfd)
			maxfd = connfd;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1)
		{
			LogError(" Select Error");
			exit(1);
		}
		else if (retval == 0)
		{
			continue;
		}
		else
		{
			if (FD_ISSET(connfd, &rfds))
			{
				p = 0;
				while (1)
				{
					int n = read(connfd, sentence + p, 8191 - p);
					if (n < 0)
					{
						printf("Error read(): %s(%d)\n", strerror(errno), errno);
						close(connfd);
						continue;
					}
					else if (n == 0)
					{
						break;
					}
					else
					{
						p += n;
						if (sentence[p - 1] == '\n')
						{
							break;
						}
					}
				}
				handleRequest(sentence, status);
				LogMessage(sentence);
			}
		}
	}
	close(connfd);
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	int listenfd; //监听socket和连接socket不一样，后者用于数据传输
	struct sockaddr_in addr;

	//int p;
	//int len;
	int port = 0;
	char dir[256] = "/tmp";

	//处理命令行参数
	if (argc <= 2)
	{
		printf("Usage: server -port p [-root r]\n\t-port p: port number\n\t-root t: working directory, default: /tmp\n");
		return 0;
	}

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-port") == 0)
		{
			if (i + 1 < argc)
			{
				port = atoi(argv[i + 1]);
				if (port == 0)
				{
					LogError("Invalid port parameter! Exit");
					return 0;
				}
			}
			else
			{
				LogError("Invalid port parameter! Exit");
				return 0;
			}
		}

		if (strcmp(argv[i], "-root") == 0)
		{
			if (i + 1 < argc)
			{
				strcpy(dir, argv[i + 1]);
			}
			else
			{
				LogError("Invalid root parameter! Exit");
				return 0;
			}
		}
	}
	chdir(dir);
	//创建socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
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
		return 1;
	}

	//开始监听socket
	if (listen(listenfd, 10) == -1)
	{
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	char start_msg[50];
	sprintf(start_msg, "Start Server at port: %d", port);
	LogInfo(start_msg);
	//持续监听连接请求

	while (1)
	{
		//等待client的连接 -- 阻塞函数
		int *connfd = (int *)malloc(sizeof(int));
		Status *status = (Status *)malloc(sizeof(Status));
		if ((*connfd = accept(listenfd, NULL, NULL)) == -1)
		{
			LogError("Error accept()");
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}
		LogInfo("receive Connection");
		pthread_t p;
		status->connfd = *connfd;
		strcpy(status->directory, dir);
		pthread_create(&p, NULL, server, (void *)status);
		// //榨干socket传来的内容
		// p = 0;
		// while (1)
		// {
		// 	int n = read(connfd, sentence + p, 8191 - p);
		// 	if (n < 0)
		// 	{
		// 		printf("Error read(): %s(%d)\n", strerror(errno), errno);
		// 		close(connfd);
		// 		continue;
		// 	}
		// 	else if (n == 0)
		// 	{
		// 		break;
		// 	}
		// 	else
		// 	{
		// 		p += n;
		// 		if (sentence[p - 1] == '\n')
		// 		{
		// 			break;
		// 		}
		// 	}
		// }
		// //socket接收到的字符串并不会添加'\0'
		// sentence[p - 1] = '\0';
		// len = p - 1;

		// //字符串处理
		// for (p = 0; p < len; p++)
		// {
		// 	sentence[p] = toupper(sentence[p]);
		// }

		// //发送字符串到socket
		// p = 0;
		// while (p < len)
		// {
		// 	int n = write(connfd, sentence + p, len + 1 - p);
		// 	if (n < 0)
		// 	{
		// 		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		// 		return 1;
		// 	}
		// 	else
		// 	{
		// 		p += n;
		// 	}
		// }

		// close(connfd);
	}

	close(listenfd);
}
