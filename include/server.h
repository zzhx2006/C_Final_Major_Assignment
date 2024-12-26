#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#define kMaxClientCount 16
#define kStringLength 1024

extern int port_number;
// 服务器套接字的文件描述符
extern int server_socket_fd;
// 服务器的地址结构体
extern struct sockaddr_in server_address_struct;
// 互斥锁，保护对客户端数组的访问
extern pthread_mutex_t lock;
// 当前已有的客户端数量
extern int client_count;
// 接受来自客户端的消息.
extern char received_message[kStringLength];

// 存储客户端信息的结构体
typedef struct {
  int fd;
  struct sockaddr_in client_address_struct;
  socklen_t client_address_struct_len;
  char name[kStringLength];
  pthread_t thread_id;
} ClientInfo;
// 使用结构体向线程传递参数.
typedef struct {
  int client_fd;
} thread_args;

extern ClientInfo client_list[kMaxClientCount]; // 客户端数组，内存放客户端结构体


// 创建服务器套接字；失败返回 -1，成功返回 0
extern int
create_socket();

// 设置服务器的地址结构体
extern void
set_address_struct();

// 绑定服务器的套接字到地址结构体；失败返回 -1，成功返回 0
extern int
bind_socket_and_addrstruct();

// 监听服务器套接字；失败返回 -1，成功返回 0
extern int
listen_socket();

// 通过文件描述符获取该客户端在数组中的索引.
// 未找到返回 -1，否则返回索引值.
extern int
get_client_by_fd(int fd);

// 断开指定文件描述符的客户端的连接.
// 参数：要断开连接的客户端的文件描述符.
// 返回值：0：正常；-1：异常.
extern int
disconnect_client(int fd);

// 向文件描述符为 fd 的客户端发送内容为 msg 的消息.
// 返回：0：正常；-1：异常.
extern int
send_message_to_client(int fd, char msg[]);

// 向所有用户群发消息；先发送消息的作者，然后发送消息内容.
// 参数：whois: 消息的作者，-1 代表服务器，其他数字代表为该索引值的客户端; msg: 发送消息的内容.
// 返回：0：正常；-1：异常.
extern int
forward_all_client(int whois, char msg[]);


// 线程函数. 执行 recv() 的地方.
// 接受一个指向结构体的 void * 指针，内含客户端文件描述符.
extern void *
receive_from_client(void *arg);

// 在接收了一个客户端后，将会执行此函数，去创建一个守卫线程，来“守卫”这个客户端是否有消息发送过来.
// 接收一个参数 fd，代表这个客户端的套接字.
// 返回 -3：线程创建失败；返回 -1：线程分离失败；返回 0：正常.
extern int
receive_guard(int fd);

// 这是一个线程函数，必须返回 void *，必须接受 void * 类型的参数.
// 这是真正执行阻塞 accpet 的地方，这个函数中的 accept 所接受的新的客户端连接会被移入新的线程中.
extern void *
accept_guard(void *arg);

// 接受新的来自客户端的连接请求
// 这仅是主函数的一个分支过程，并不需要一个独立的进程
extern int
accept_client_connection();

// 打印服务器当前状态信息
extern int
print_server_status();

// 打印功能菜单
extern void
printmenu();

#endif /* SERVER_H */
