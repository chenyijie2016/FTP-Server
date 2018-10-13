#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

const char time_fmt[] = "[%d-%d-%d %.2d:%.2d:%.2d] %s: ";
const int green = 32;
const int red = 31;
const int yellow = 33;
const char color_fmt[] = "\e[%dm%s\e[0m";

void Log(char *msg, const char *type, int color)
{
#ifndef NOLOG
    char message[1024];
    time_t timep;
    struct tm *p_tm;
    timep = time(NULL);
    p_tm = localtime(&timep); /*获取本地时区时间*/
    char localtime[128];
    sprintf(localtime, time_fmt, (p_tm->tm_year + 1900), (p_tm->tm_mon + 1), p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, type);
    sprintf(message, color_fmt, color, localtime);
    printf(message);
    printf(msg);
    printf("\n");
#endif
}

void LogInfo(char *msg)
{
    Log(msg, "INFO", green);
}

void LogWarring(char *msg)
{
    Log(msg, "WARR", yellow);
}

void LogError(char *msg)
{
    Log(msg, "ERRO", red);
}