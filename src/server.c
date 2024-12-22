#define SERVER_C
#include "../include/logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

INITLOG;

bool exit_flag = 0; // 程序是否应该退出的标志，用在 while 的条件中
int port_number = 8888;

int server_socket_fd; // 服务器套接字的文件描述符
struct sockaddr_in server_address_struct; // 服务器的地址结构体

/* \033[31m 红色  \033[32m 绿色  \033[33m 橙色  \033[0m 重置  */

/*
  先打印“尝试xxx”，然后开始进行相应操作。并获取返回值。
  如果出错了，就prterr()打印错误信息，返回-1.
  如果成功执行，则打印“成功执行xxx”，返回0.
*/

// 创建服务器套接字；失败返回 -1，成功返回 0
int create_socket() {
  prtlog("尝试创建服务器套接字. ");
  server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {
    prterr(socket());
    return -1;
  }
  prtlog("\033[32m创建服务器套接字成功. \033[0m");
  prtlog("服务器套接字的文件描述符为 %d. ", server_socket_fd);
  return 0;
}

// 设置服务器的地址结构体
void set_address_struct() {
  prtlog("尝试设置服务器的地址结构体. ");
  memset(&server_address_struct, 0, sizeof(server_address_struct));
  server_address_struct.sin_family = AF_INET;          // 协议簇
  server_address_struct.sin_addr.s_addr = INADDR_ANY;  // 地址
  server_address_struct.sin_port = htons(port_number); // 端口号
  prtlog("\033[32m设置服务器的地址结构体成功. \033[0m");
  prtlog("当前服务器端口号: %d", port_number);
}

// 绑定服务器的套接字到地址结构体；失败返回 -1，成功返回 0
int bind_socket_and_addrstruct() {
  prtlog("尝试绑定服务器的套接字到地址结构体. ");
  if (bind(server_socket_fd, (struct sockaddr *)&server_address_struct,
           sizeof(server_address_struct)) < 0) {
    prterr(bind());
    // close(server_socket_fd);
    return -1;
  }
  prtlog("\033[32m绑定套接字到地址结构体成功. \033[0m");
  return 0;
}

// 监听服务器套接字；失败返回 -1，成功返回 0
int listen_socket() {
  prtlog("尝试监听服务器套接字. ");
  if (listen(server_socket_fd, 8) < 0) {
    prterr(listen());
    // close(server_socket_fd);
    return -1;
  }
  prtlog("\033[32m开始监听服务器成功. \033[0m");
  prtlog("\033[33m服务器正在监听 %d 端口 ......\033[0m", port_number);
  return 0;
}

// 打印服务器当前状态信息
int print_server_status() {
  struct sockaddr_in current_status;
  socklen_t current_status_len = sizeof(current_status);
  if (getsockname(server_socket_fd, (struct sockaddr *)&current_status,
                  &current_status_len) < 0) {
    prterr(getsockname());
    return -1;
  }
  prtlog("当前状态：");
  prtlog("IP 地址：%s", inet_ntoa(current_status.sin_addr));
  prtlog("监听端口号：%d", ntohs(current_status.sin_port));
  prtlog("在线用户数：");
  prtlog("服务器文件描述符：%d", server_socket_fd);
  return 0;
}

// 打印功能菜单
void printmenu() {
  prtlog("1.设置端口号；");
  prtlog("2.尝试开始监听；");
  prtlog("3.查看当前状态；");
  prtlog("4.列出所有已连接的用户；");
  prtlog("5.关闭服务器；");
  prtlog("0.退出程序；");
}

int main(int argc, char *argv[]) {

  log_file = fopen("server.log", "a");
  if (log_file == NULL) {
    prterr(fopen());
    erret;
  }

  /*先执行操作，获取返回值；如果错误，执行erret. */

  // 创建服务器套接字
  int ret1 = create_socket();
  if (ret1 == -1) {
    close(server_socket_fd);
    erret;
  }

  // 设置服务器的地址结构体
  set_address_struct();

  // 绑定服务器的套接字到地址结构体
  int ret2 = bind_socket_and_addrstruct();
  if (ret2 == -1) {
    close(server_socket_fd);
    erret;
  }

  while (!exit_flag) {
    printmenu();
    int id1 = 0;
    prtlog("你想要进行什么操作：");
    scanf("%d", &id1);

    switch (id1) {

    case 1: {
      prtlog("请输入新端口号：");
      int new_port_number = 0;
      scanf("%d", &new_port_number);
      if (new_port_number == port_number) {
        prtlog("\033[33m输入了相同的端口号，端口号未改变. \033[0m");
      } else {
        port_number = new_port_number;
        prtlog("\033[32m成功修改端口号为 %d. \033[0m", port_number);
      }
      set_address_struct();
      ret2 = bind_socket_and_addrstruct();
      if (ret2 == -1) {
        close(server_socket_fd);
        erret;
      }
      break;
    }

    case 2: {
      int ret3 = listen_socket();
      if (ret3 == -1) {
        close(server_socket_fd);
        erret;
      }
      break;
    }

    case 3:
      int ret4 = print_server_status();
      if (ret4 == -1) {
        close(server_socket_fd);
        erret;
      }
      break;

    case 4:
      break;

    case 5:
      close(server_socket_fd);
      prtlog("已关闭服务器套接字. ");
      break;

    case 0:
      close(server_socket_fd);
      prtlog("已关闭服务器套接字. ");
      exit_flag = 1;
      prtlog("正在退出 ...... ");
      if (log_file != NULL) {
        fclose(log_file);
      }
      break;

    default:
      prtlog("\033[31m输入了非法操作序号，请重新输入. \033[0m\n");
      break;
    }
  }

  return 0;
}
