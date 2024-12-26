#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../include/logger.h"
#include "../../include/server.h"

char user[1024];
time_t now;
struct tm *tm_info;
char format_time[32];
FILE *log_file;

int port_number = 8892;
int server_socket_fd;
struct sockaddr_in server_address_struct;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
ClientInfo client_list[kMaxClientCount];
int client_count = 0;
char received_message[kStringLength];

/* \033[31m 红色  \033[32m 绿色  \033[33m 橙色  \033[0m 重置  */

/*
  先打印“尝试xxx”，然后开始进行相应操作。并获取返回值。
  如果出错了，就prterr()打印错误信息，返回-1.
  如果成功执行，则打印“成功执行xxx”，返回0.
*/

int
create_socket() {
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


void
set_address_struct() {
  prtlog("尝试设置服务器的地址结构体. ");
  memset(&server_address_struct, 0, sizeof(server_address_struct));
  server_address_struct.sin_family = AF_INET;          // 协议簇
  server_address_struct.sin_addr.s_addr = INADDR_ANY;  // 地址
  server_address_struct.sin_port = htons(port_number); // 端口号
  prtlog("\033[32m设置服务器的地址结构体成功. \033[0m");
  prtlog("当前服务器端口号: %d", port_number);
}


int
bind_socket_and_addrstruct() {
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


int
listen_socket() {
  prtlog("尝试监听服务器套接字. ");
  if (listen(server_socket_fd, kMaxClientCount) < 0) {
    prterr(listen());
    return -1;
  }
  prtlog("\033[32m开始监听服务器成功. \033[0m");
  prtlog("\033[33m服务器正在监听 %d 端口 ......\033[0m", port_number);
  return 0;
}


int
get_client_by_fd(int fd) {
  for (int i = 0; i < client_count; i++) {
    if (client_list[i].fd == fd) {
      return i;
    }
  }
  return -1;
}


int
disconnect_client(int fd) {
  int index = get_client_by_fd(fd);
  // 如果未找到就直接返回 -1.
  if (index < 0) {
    prtlog("\033[31m断开客户端 %d 时出错：\033[0m", fd);
    prtlog("\033[31m未找到文件描述符为 %d 的客户端. \033[0m", fd);
    return -1;
  }
  close(fd);
  // 如果不是最后一个客户端，就将最后一个覆盖到当前 ClientInfo，然后不用再管最后一个（因为已经断开了连接），直接将 client_count 指针减 1.
  if (index != client_count - 1) {
    memcpy(&client_list[index], &client_list[client_count - 1], sizeof(ClientInfo));
  }
  client_count--;
  prtlog("已断开与客户端 %d 的连接. ", fd);
  prtlog("当前在线用户数：%d", client_count);
  return 0;
}


int
send_message_to_client(int fd, char msg[]) {
  if (send(fd, msg, kStringLength, 0) < 0) {
    prterr(send());
    prtlog("向客户端 %d 发送信息失败. ", fd);
    return -1;
  }
  prtlog("\033[32m成功向客户端 %d 发送一条消息: \033[0m%s", fd, msg);
  return 0;
}


int
forward_all_client(int whois, char msg[]) {
  char username[kStringLength];
  if (whois == -1) {
    strcpy(username, "server");
  } else {
    strcpy(username, client_list[whois].name);
  }

  for (int i = 0; i < client_count; i++) {
    if (send_message_to_client(client_list[i].fd, username) < 0) {
      return -1;
    }
    if (send_message_to_client(client_list[i].fd, msg) < 0) {
      return -1;
    }
  }
  return 0;
}


void *
receive_from_client(void *arg) {
  thread_args *t = (thread_args *)arg;
#ifdef DEBUG_FLAG
  prtlog("执行 receive_from_client(). ");
  prtlog("client_fd = %d", client_fd);
#endif
  memset(received_message, 0, kStringLength);
  bool is_name = 1;
  do {
    int ret = recv(t->client_fd, received_message, sizeof(received_message), 0);
#ifdef DEBUG_FLAG
    prtlog("recv()已经被执行. ");
#endif
    if (ret < 0) {
      prterr(recv());
      close(t->client_fd);
      free(t);
      return NULL;
    } else if (ret == 0) {
      disconnect_client(t->client_fd);
      free(t);
      return NULL;
    }
    prtlog("\033[32m成功接收一条来自客户端 %d 的消息: \033[0m%s", t->client_fd, received_message);
    // 来自客户端的第一条消息被认为是昵称.
    if (is_name) {
      strcpy(client_list[get_client_by_fd(t->client_fd)].name, received_message);
      is_name = 0;
      prtlog("该客户端的昵称设置为 %s", received_message);
      const char msg[kStringLength] = " 进入了聊天室. ";
      strcat(received_message, msg);
      forward_all_client(-1, received_message);
    } else { // 往后才是正式的消息.
      prtlog("尝试群发这条消息. ");
      if (forward_all_client(get_client_by_fd(t->client_fd), received_message) < 0) {
        prtlog("\033[31m群发消息时发生异常. \033[0m");
      }
      prtlog("\033[32m群发成功. \033[0m");
    }
  } while (t->client_fd > 0 && client_count < kMaxClientCount);
  free(t);
  return NULL;
}


int
receive_guard(int fd) {
#ifdef DEBUG_FLAG
  prtlog("执行 receive_guard(). ");
  prtlog("fd = %d", fd);
#endif
  pthread_t receive_guard_p;
  thread_args *p = malloc(sizeof(thread_args));
  p->client_fd = fd;
#ifdef DEBUG_FLAG
  prtlog("正在尝试创建 receive_guard 线程. ");
#endif
  if (pthread_create(&receive_guard_p, NULL, receive_from_client, p) < 0) {
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


void *
accept_guard(void *arg) {
#ifdef DEBUG_FLAG
  prtlog("执行 accept_guard(). ");
#endif
  do { // 由于在执行这个函数之前，已经排除了 client_list 已满的情况，故在此无需再次判断.
    pthread_mutex_lock(&lock);
    client_list[client_count].client_address_struct_len = sizeof(client_list[client_count].client_address_struct);
    pthread_mutex_unlock(&lock);
    int new_socket_fd;
    new_socket_fd = accept(server_socket_fd,
                           (struct sockaddr *)&client_list[client_count].client_address_struct,
                           &client_list[client_count].client_address_struct_len);
    // accept 阻塞......
    if (new_socket_fd < 0) {
      prterr(accept());
      prtlog("\033[33m该 accept 线程被异常退出. \033[0m");
      return NULL;
    } else {
      prtlog("\033[32m成功接受了新的连接请求. \033[0m");
      prtlog("\033[33m该客户端的 IP 地址：%s\033[0m", inet_ntoa(client_list[client_count].client_address_struct.sin_addr));
      prtlog("\033[33m该客户端的端口号：%d\033[0m", ntohs(client_list[client_count].client_address_struct.sin_port));
      prtlog("\033[33m该客户端套接字在 client_list[] 中的索引：%d\033[0m", client_count); // debug
      pthread_mutex_lock(&lock);
      client_list[client_count].fd = new_socket_fd;
      client_list[client_count].thread_id = pthread_self();
      client_count++;
      pthread_mutex_unlock(&lock);
      if (receive_guard(new_socket_fd) < 0) {
        prterr(receive_guard());
        close(server_socket_fd);
        return NULL;
      }
      prtlog("继续监听......");
    }
  } while (client_count < kMaxClientCount);
  prtlog("\033[33m该 accept 线程被退出. \033[0m");
  return NULL;
}


int
accept_client_connection() {
#ifdef DEBUG_FLAG
  prtlog("执行 accept_client_connection(). ");
#endif
  if (client_count == kMaxClientCount) {
    prtlog("\033[31m已达到服务器的最大连接数. \033[0m");
    return -2;
  }
  pthread_t accept_guard_p;
#ifdef DEBUG_FLAG
  prtlog("正在尝试创建 accept_guard 线程. ");
#endif
  // 真正执行阻塞 accept 的位置才需要一个独立的进程
  if (pthread_create(&accept_guard_p, NULL, accept_guard, NULL) < 0) {
    prterr(pthread_create());
    return -3;
  }
#ifdef DEBUG_FLAG
  prtlog("\033[32m成功创建 accept_guard 线程. \033[0m");
  prtlog("正在尝试分离 accept_guard 线程. ");
#endif
  if (pthread_detach(accept_guard_p) != 0) {
    prterr(pthread_datach());
    return -1;
  }
#ifdef DEBUG_FLAG
  prtlog("\033[32m成功分离 accept_guard 线程. \033[0m");
#endif

  return 0;
}


int
print_server_status() {
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
  prtlog("在线用户数：%d", client_count);
  prtlog("服务器文件描述符：%d", server_socket_fd);
  return 0;
}


void
printmenu() {
  prtlog("1.打印菜单；");
  prtlog("2.开始监听；");
  prtlog("3.查看当前状态；");
  prtlog("4.断开所有客户端连接；");
  prtlog("5.设置端口号；");
  prtlog("6.关闭服务器；");
  prtlog("0.退出程序；");
}
