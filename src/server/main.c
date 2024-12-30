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

// #define DEBUG_FLAG


int
main() {
  SWITCHTO("server");
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

  printmenu();
  bool exit_flag = 0; // 程序是否应该退出的标志，用在 while 的条件中
  while (!exit_flag) {
    int id1 = 0;
    prtlog("你想要进行什么操作：");
    scanf("%d", &id1);

    switch (id1) {

    case 1:
      printmenu();
      break;

    case 2: {
      if (listen_socket() == -1) {
        close(server_socket_fd);
        erret;
      }
      if (accept_client_connection() < 0) {
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
      bool is_success = 1;
      for (int i = 0; i < client_count; i++) {
        is_success = is_success && disconnect_client(client_list[i].fd);
      }
      if (is_success) {
        prtlog("成功断开所有客户端的连接. ");
      } else {
        prtlog("断开连接异常. ");
      }
      break;
    }

    case 5: {
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

    case 6:
      close(server_socket_fd);
      prtlog("已关闭服务器套接字. ");
      break;

    case 7:
      prtlog("程序名称：基于Linux的简易多线程聊天室系统; ");
      prtlog("版本信息：v1.0; ");
      prtlog("开发时间：2024年12月15日; ");
      prtlog("开发者：张展皓翔; ");
      prtlog("学号：8002124159; ");
      prtlog("专业：软件工程; ");
      prtlog("班级：2405班; ");
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
