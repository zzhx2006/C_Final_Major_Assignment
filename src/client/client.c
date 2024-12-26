#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../include/client.h"
#include "../../include/logger.h"

char user[1024];
time_t now;
struct tm *tm_info;
char format_time[32];
FILE *log_file;

bool exit_flag = 0;
int port_number = 8892;
int client_socket_fd;
struct sockaddr_in server_address_struct;
char name[kStringLength];
pthread_mutex_t lock;
char received_message[kStringLength];

/* \033[31m 红色  \033[32m 绿色  \033[33m 橙色  \033[0m 重置  */

/*
  先打印“尝试xxx”，然后开始进行相应操作。并获取返回值。
  如果出错了，就prterr()打印错误信息，返回-1.
  如果成功执行，则打印“成功执行xxx”，返回0.
*/

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

int
send_name_to_server() {
  if (send(client_socket_fd, name, kStringLength, 0) < 0) {
    prterr(send());
    return -1;
  }
  return 0;
}

int
connect_socket_and_addrstruct() {
#ifdef DEBUG_FLAG
  prtlog("尝试连接客户端的套接字到地址结构体. ");
#endif
  if (connect(client_socket_fd, (struct sockaddr *)&server_address_struct,
              sizeof(server_address_struct)) < 0) {
    prterr(connect());
    return -1;
  }
  prtlog("\033[32m成功连接到服务器. \033[0m");
  prtlog("\033[33m当前服务器信息：%s:%d\033[0m", inet_ntoa(server_address_struct.sin_addr), ntohs(server_address_struct.sin_port));
  prtlog("请输入昵称：");
  scanf("%s", name);
  if (send_name_to_server() < 0) {
    prterr(send_name_to_server());
    return -1;
  }
  pthread_mutex_lock(&lock);
  SWITCHTO(name);
  pthread_mutex_unlock(&lock);
  return 0;
}

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
  prtlog("昵称：%s", name);
  prtlog("IP 地址：%s", inet_ntoa(current_status.sin_addr));
  prtlog("端口号：%d", ntohs(current_status.sin_port));
  prtlog("客户端文件描述符：%d", client_socket_fd);
  return 0;
}

int
send_message_to_server(char msg[]) {
#ifdef DEBUG_FLAG
  prtlog("尝试向服务器发送一条消息. ");
#endif
  if (send(client_socket_fd, msg, kStringLength, 0) < 0) {
    prterr(send());
    return -1;
  }
#ifdef DEBUG_FLAG
  prtlog("\033[32m成功向服务器发送一条消息: \033[0m%s", msg);
#endif
  return 0;
}

void *
receive_from_server(void *arg) {
  bool is_username = 1;
  memset(received_message, 0, kStringLength);
  do {
    int ret = recv(client_socket_fd, received_message, sizeof(received_message), 0);
    if (ret < 0) {
      prterr(recv());
      close(client_socket_fd);
      return NULL;
    } else if (ret == 0) {
      prtlog("\033[33m已断开与服务器的连接. \033[0m");
      close(client_socket_fd);
      return NULL;
    }
    if (is_username) {
      pthread_mutex_lock(&lock);
      SWITCHTO(received_message);
      pthread_mutex_unlock(&lock);
      is_username = 0;
    } else {
      pthread_mutex_lock(&lock);
      prtlog("%s", received_message);
      SWITCHTO(name);
      pthread_mutex_unlock(&lock);
      is_username = 1;
    }
  } while (!exit_flag);
  return NULL;
}

int
receive_guard() {
  pthread_t receive_guard_p;
#ifdef DEBUG_FLAG
  prtlog("正在尝试创建 receive_guard 线程. ");
#endif
  if (pthread_create(&receive_guard_p, NULL, receive_from_server, NULL) < 0) {
    prterr(pthread_create());
    return -3;
  }
#ifdef DEBUG_FLAG
  prtlog("\033[32m成功创建 receive_guard 线程. \033[0m");
  prtlog("正在尝试分离 receive_guard 线程. ");
#endif
  if (pthread_detach(receive_guard_p) != 0) {
    prterr(pthread_datach());
    return -1;
  }
#ifdef DEBUG_FLAG
  prtlog("\033[32m成功分离 receive_guard 线程. \033[0m");
#endif
  return 0;
}

void
printmenu() {
  pthread_mutex_lock(&lock);
  prtlog("1.打印菜单；");
  prtlog("2.尝试连接到服务器；");
  prtlog("3.查看当前状态；");
  prtlog("4.尝试向服务器发送信息；");
  prtlog("5.设置端口号；");
  prtlog("6.断开连接；");
  prtlog("0.退出程序；");
  pthread_mutex_unlock(&lock);
}
