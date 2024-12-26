#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern char user[1024];
extern time_t now;
extern struct tm *tm_info;
extern char format_time[32];
extern FILE *log_file;

#define SWITCHTO(XXX) strcpy(user, XXX)

// 实现printf()同时输出到终端和日志文件
#define prt_to_file(XXX, ...)                \
  do {                                       \
    printf(XXX, ##__VA_ARGS__);              \
    if (log_file != NULL) {                  \
      fprintf(log_file, XXX, ##__VA_ARGS__); \
    }                                        \
  } while (0)

// 将printf()重定义为自定义的prt_to_file()函数
#define printf(XXX, ...) prt_to_file(XXX, ##__VA_ARGS__)

// 在printf()前加上时间信息
#define prtlog(XXX, ...)                                                  \
  do {                                                                    \
    time(&now);                                                           \
    strftime(format_time, 32, "%F %T", localtime(&now));                  \
    printf("\033[32m[%s]\033[0m \033[33m%s:\033[0m ", format_time, user); \
    printf(XXX, ##__VA_ARGS__);                                           \
    printf("\n");                                                         \
  } while (0)

#define prterr(XXX)                                                                                                                       \
  do {                                                                                                                                    \
    time(&now);                                                                                                                           \
    strftime(format_time, 32, "%F %T", localtime(&now));                                                                                  \
    printf("\033[32m[%s]\033[0m \033[33m%s:\033[0m \033[31m执行 " #XXX " 时发生错误: %s. \033[0m\n", format_time, user, strerror(errno)); \
  } while (0)

#define erret                                                             \
  do {                                                                    \
    time(&now);                                                           \
    strftime(format_time, 32, "%F %T", localtime(&now));                  \
    printf("\033[32m[%s]\033[0m \033[33m%s:\033[0m ", format_time, user); \
    printf("\033[31m程序执行异常，退出程序. \033[0m\n");                  \
    if (log_file != NULL) {                                               \
      fclose(log_file);                                                   \
    }                                                                     \
    return -1;                                                            \
  } while (0)

#endif /* LOGGER_H */
