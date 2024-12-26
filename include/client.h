#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#define kStringLength 1024

// 程序是否应该退出的标志，用在 while 的条件中
extern bool exit_flag;
extern int port_number;
// 客户端套接字的文件描述符
extern int client_socket_fd;
// 服务器的地址结构体
extern struct sockaddr_in server_address_struct;
extern char name[kStringLength];
extern pthread_mutex_t lock;
extern char received_message[kStringLength];


// 创建客户端套接字；失败返回 -1，成功返回 0
extern int
create_socket();

// 设置服务器的地址结构体
extern void
set_address_struct();

extern int
send_name_to_server();

// 连接客户端的套接字到地址结构体；失败返回 -1，成功返回 0
extern int
connect_socket_and_addrstruct();

// 打印客户端当前状态信息
extern int
print_client_status();

extern int
send_message_to_server(char msg[]);

extern void *
receive_from_server(void *arg);

extern int
receive_guard();

// 打印功能菜单
extern void
printmenu();

#endif /* CLIENT_H */
