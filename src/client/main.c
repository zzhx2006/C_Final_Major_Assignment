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

// #define DEBUG_FLAG


int
main() {
  SWITCHTO("client");
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

  pthread_mutex_init(&lock, NULL);

  printmenu();

  while (!exit_flag) {
    int id1 = 0;
    prtlog("你想要进行什么操作：");
    scanf("%d", &id1);

    switch (id1) {

    case 1:
      printmenu();
      break;

    case 2: {
      if (connect_socket_and_addrstruct() == -1) {
        close(client_socket_fd);
        erret;
      }
      if (receive_guard() == -1) {
        close(client_socket_fd);
        erret;
      }
      break;
    }

    case 3: {
      if (print_client_status() == -1) {
        close(client_socket_fd);
        erret;
      }
      break;
    }

    case 4: {

      char msg[kStringLength];
      prtlog("你想发送的内容：");
      int c;
      while ((c = getchar()) != '\n' && c != EOF) {
      }
      scanf("%[^\n]", msg);
      prtlog("%s", msg);
      if (send_message_to_server(msg) == -1) {
        close(client_socket_fd);
        erret;
      }
    } break;

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

    case 6:
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
