#define CLIENT_C
#include "../include/logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
// #include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

INITLOG;

bool exit_flag = 0; // 程序是否应该退出的标志，用在 while 的条件中
int port_number = 8889;

int client_socket_fd;                     // 客户端套接字的文件描述符
struct sockaddr_in server_address_struct; // 服务器的地址结构体


/* \033[31m 红色  \033[32m 绿色  \033[33m 橙色  \033[0m 重置  */

/*
  先打印“尝试xxx”，然后开始进行相应操作。并获取返回值。
  如果出错了，就prterr()打印错误信息，返回-1.
  如果成功执行，则打印“成功执行xxx”，返回0.
*/

// 创建客户端套接字；失败返回 -1，成功返回 0
int
create_socket() {
  prtlog("尝试创建客户端套接字. ");
  client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket_fd < 0) {
    prterr(socket());
    return -1;
  }
  prtlog("\033[32m创建客户端套接字成功. \033[0m");
  prtlog("客户端套接字的文件描述符为 %d. ", client_socket_fd);
  return 0;
}

// 设置服务器的地址结构体
void
set_address_struct() {
  prtlog("尝试设置服务器的地址结构体. ");
  memset(&server_address_struct, 0, sizeof(server_address_struct));
  server_address_struct.sin_family = AF_INET;                     // 协议簇
  server_address_struct.sin_addr.s_addr = inet_addr("127.0.0.1"); // 地址
  server_address_struct.sin_port = htons(port_number);            // 端口号
  prtlog("\033[32m设置服务器的地址结构体成功. \033[0m");
  prtlog("当前服务器端口号: %d", port_number);
}

// 连接客户端的套接字到地址结构体；失败返回 -1，成功返回 0
int
connect_socket_and_addrstruct() {
  prtlog("尝试连接客户端的套接字到地址结构体. ");
  if (connect(client_socket_fd, (struct sockaddr *)&server_address_struct,
              sizeof(server_address_struct)) < 0) {
    prterr(connect());
    return -1;
  }
  prtlog("\033[32m成功连接到服务器. \033[0m");
  prtlog("\033[33m当前服务器信息：%s:%d\033[0m", inet_ntoa(server_address_struct.sin_addr), ntohs(server_address_struct.sin_port));
  return 0;
}

// 打印客户端当前状态信息
int
print_client_status() {
  struct sockaddr_in current_status;
  socklen_t current_status_len = sizeof(current_status);
  if (getsockname(client_socket_fd, (struct sockaddr *)&current_status,
                  &current_status_len) < 0) {
    prterr(getsockname());
    return -1;
  }
  prtlog("当前状态：");
  prtlog("IP 地址：%s", inet_ntoa(current_status.sin_addr));
  prtlog("端口号：%d", ntohs(current_status.sin_port));
  prtlog("客户端文件描述符：%d", client_socket_fd);
  return 0;
}

int
set_socket_status() {
  // int ret = setsockopt(client_socket_fd, SOL_SOCKET, );
  return 0;
}

// 打印功能菜单
void
printmenu() {
  prtlog("1.设置端口号；");
  prtlog("2.尝试连接到服务器；");
  prtlog("3.查看当前状态；");
  prtlog("4.尝试向服务器发送信息；");
  prtlog("4.xxx；");
  prtlog("4.xxx；");
  prtlog("4.xxx；");
  prtlog("4.xxx；");
  prtlog("5.关闭客户端；");
  prtlog("0.退出程序；");
}


int
main(int argc, char *argv[]) {
  log_file = fopen("client.log", "a");
  if (log_file == NULL) {
    prterr(fopen());
    erret;
  }

  /*先执行操作，获取返回值；如果错误，执行erret. */

  // 创建客户端套接字
  if (create_socket() == -1) {
    close(client_socket_fd);
    erret;
  }

  // 设置服务器的地址结构体
  set_address_struct();


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
      close(client_socket_fd);
      prtlog("关闭了旧套接字. ");
      prtlog("尝试设置新的套接字：");
      if (create_socket() == -1) {
        close(client_socket_fd);
        erret;
      }
      set_address_struct();
      prtlog("\033[32m重置客户端套接字成功. \033[0m");
      break;
    }

    case 2: {
      if (connect_socket_and_addrstruct() == -1) {
        close(client_socket_fd);
        erret;
      }
      break;
    }

    case 3:
      if (print_client_status() == -1) {
        close(client_socket_fd);
        erret;
      }
      break;

    case 4:
      break;

    case 5:
      close(client_socket_fd);
      prtlog("已关闭客户端套接字. ");
      break;

    case 0:
      close(client_socket_fd);
      prtlog("已关闭客户端套接字. ");
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
