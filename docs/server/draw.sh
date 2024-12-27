#!/bin/bash

# 定义一个数组，包含提供的函数名
function_names=("create_socket" "set_address_struct" "bind_socket_and_addrstruct" "listen_socket" "get_client_by_fd" "disconnect_client" "send_message_to_client" "forward_all_client" "receive_from_client" "receive_guard" "accept_guard" "accept_client_connection" "print_server_status" "printmenu")

# 遍历数组中的每个函数名
for func_name in "${function_names[@]}"; do
    # 替换cxx2flow命令中的函数名，并执行
    cxx2flow ../../src/server/server.c -o "${func_name}.dot" "${func_name}"

    # 替换dot命令中的函数名，并执行
    dot -Tpng "${func_name}.dot" -o "${func_name}.png"
done
