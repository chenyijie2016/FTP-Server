#ifndef LOG_H
#define LOG_H
/* 输出调试记录信息 */
/* 使用编译宏 NOLOG 来避免输出*/
void LogInfo(char *msg);
void LogWarring(char *msg);
void LogError(char *msg);
void LogMessage(char *msg);

#endif