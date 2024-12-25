#define SERVER_C
// #define DEBUG_FLAG
#undef DEBUG_FLAG
#include "../include/logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

INITLOG;

#define kMaxClientCount 16
int port_number = 8892;

int server_socket_fd;                     // 服务器套接字的文件描述符
struct sockaddr_in server_address_struct; // 服务器的地址结构体

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // 互斥锁，保护对客户端数组的访问

// 存储客户端信息的结构体
typedef struct {
  int fd;
  struct sockaddr_in client_address_struct;
  socklen_t client_address_struct_len;
  char name[32];
  char passwd[32];
  pthread_t thread_id;
  // 0: 未使用；1: 已连接；2: ...
  int status;
} ClientInfo;

ClientInfo client_list[kMaxClientCount]; // 客户端数组，内存放客户端结构体
int client_count = 0;                    // 当前已有的客户端数量

/* \033[31m 红色  \033[32m 绿色  \033[33m 橙色  \033[0m 重置  */

/*
  先打印“尝试xxx”，然后开始进行相应操作。并获取返回值。
  如果出错了，就prterr()打印错误信息，返回-1.
  如果成功执行，则打印“成功执行xxx”，返回0.
*/

// 创建服务器套接字；失败返回 -1，成功返回 0
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

// 设置服务器的地址结构体
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

// 绑定服务器的套接字到地址结构体；失败返回 -1，成功返回 0
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

// 监听服务器套接字；失败返回 -1，成功返回 0
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

// 通过文件描述符获取该客户端在数组中的索引.
// 未找到返回 -1，否则返回索引值.
int
get_client_by_fd(int fd) {
  for (int i = 0; i < client_count; i++) {
    if (client_list[i].fd == fd) {
      return i;
    }
  }
  return -1;
}

// 断开指定文件描述符的客户端的连接.
// 参数：要断开连接的客户端的文件描述符.
// 返回值：0：正常；-1：异常.
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

// 向文件描述符为 fd 的客户端发送内容为 msg 的消息.
// 返回：0：正常；-1：异常.
int
send_message_to_client(int fd, char msg[]) {
  if (send(fd, msg, sizeof(msg), 0) < 0) {
    prterr(send());
    prtlog("向客户端 %d 发送信息失败. ", fd);
    return -1;
  }
  prtlog("\033[32m成功向客户端 %d 发送一条消息: \033[0m%s", fd, msg);
  return 0;
}

// 向所有用户发送内容为 msg 的消息（相当于群发消息）.
// 返回：0：正常；-1：异常.
int
forward_all_client(char msg[]) {
  for (int i = 0; i < client_count; i++) {
    if (send_message_to_client(client_list[i].fd, msg) < 0) {
      return -1;
    }
  }
  return 0;
}

char received_message[1024]; // 接受来自客户端的消息.

// 使用结构体向线程传递参数.
typedef struct {
  int client_fd;
} thread_args;

// 线程函数. 执行 recv() 的地方.
// 接受一个指向结构体的 void * 指针，内含客户端文件描述符.
void *
receive_from_client(void *arg) {
  thread_args *t = (thread_args *)arg;
#ifdef DEBUG_FLAG
  prtlog("执行 receive_from_client(). ");
  prtlog("client_fd = %d", client_fd);
#endif
  memset(received_message, 0, 1024);
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
    prtlog("尝试群发这条消息. ");
    if (forward_all_client(received_message) < 0) {
      prtlog("\033[31m群发消息时发生异常. \033[0m");
    }
    prtlog("\033[32m群发成功. \033[0m");
  } while (t->client_fd > 0 && client_count < kMaxClientCount);
  free(t);
  return NULL;
}

// 在接收了一个客户端后，将会执行此函数，去创建一个守卫线程，来“守卫”这个客户端是否有消息发送过来.
// 接收一个参数 fd，代表这个客户端的套接字.
// 返回 -3：线程创建失败；返回 -1：线程分离失败；返回 0：正常.
int
receive_guard(int fd) {
#ifdef DEBUG_FLAG
  prtlog("执行 receive_guard(). ");
  prtlog("client_fd = %d", client_fd);
#endif
  pthread_t receive_guard_p;
  thread_args *p = malloc(sizeof(thread_args));
  p->client_fd = fd;
  prtlog("正在尝试创建 receive_guard 线程. ");
  if (pthread_create(&receive_guard_p, NULL, receive_from_client, p) < 0) {
    prterr(pthread_create());
    return -3;
  }
  prtlog("\033[32m成功创建 receive_guard 线程. \033[0m");
  prtlog("正在尝试分离 receive_guard 线程. ");
  if (pthread_detach(receive_guard_p) != 0) {
    prterr(pthread_datach());
    return -1;
  }
  prtlog("\033[32m成功分离 receive_guard 线程. \033[0m");
  return 0;
}

// 这是一个线程函数，必须返回 void *，必须接受 void * 类型的参数.
// 这是真正执行阻塞 accpet 的地方，这个函数中的 accept 所接受的新的客户端连接会被移入新的线程中.
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


// 接受新的来自客户端的连接请求
// 这仅是主函数的一个分支过程，并不需要一个独立的进程
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
  prtlog("正在尝试创建 accept_guard 线程. ");
  // 真正执行阻塞 accept 的位置才需要一个独立的进程
  if (pthread_create(&accept_guard_p, NULL, accept_guard, NULL) < 0) {
    prterr(pthread_create());
    return -3;
  }
  prtlog("\033[32m成功创建 accept_guard 线程. \033[0m");
  prtlog("正在尝试分离 accept_guard 线程. ");
  if (pthread_detach(accept_guard_p) != 0) {
    prterr(pthread_datach());
    return -1;
  }
  prtlog("\033[32m成功分离 accept_guard 线程. \033[0m");

  return 0;
}

// 打印服务器当前状态信息
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

// 打印功能菜单
void
printmenu() {
  prtlog("1.设置端口号；");
  prtlog("2.尝试开始监听；");
  prtlog("3.查看当前状态；");
  prtlog("4.开始接受客户端连接；");
  prtlog("5.断开所有客户端连接；");
  // prtlog("6.断开某一客户端连接；");
  prtlog("7.关闭服务器；");
  prtlog("0.退出程序；");
}

int
main(int argc, char *argv[]) {
  log_file = fopen("server.log", "a");
  if (log_file == NULL) {
    prterr(fopen());
    erret;
  }

  /*先执行操作，获取返回值；如果错误，执行erret. */

  // 创建服务器套接字
  if (create_socket() == -1) {
    close(server_socket_fd);
    erret;
  }

  // 设置服务器的地址结构体
  set_address_struct();

  // 绑定服务器的套接字到地址结构体
  if (bind_socket_and_addrstruct() == -1) {
    close(server_socket_fd);
    erret;
  }

  bool exit_flag = 0; // 程序是否应该退出的标志，用在 while 的条件中
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
      close(server_socket_fd);
      prtlog("关闭了旧套接字. ");
      prtlog("尝试设置新的套接字：");
      if (create_socket() == -1) {
        close(server_socket_fd);
        erret;
      }
      set_address_struct();
      if (bind_socket_and_addrstruct() == -1) {
        close(server_socket_fd);
        erret;
      }
      prtlog("\033[32m重置服务端套接字成功. \033[0m");
      break;
    }

    case 2: {
      if (listen_socket() == -1) {
        close(server_socket_fd);
        erret;
      }
      break;
    }

    case 3: {
      if (print_server_status() == -1) {
        close(server_socket_fd);
        erret;
      }
      break;
    }

    case 4: {
      if (accept_client_connection() < 0) {
        close(server_socket_fd);
        erret;
      }
      break;
    }

    case 5: {
      // if (accept_client_connection() < 0) {
      //   close(server_socket_fd);
      //   erret;
      // }
      break;
    }

    case 7:
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
