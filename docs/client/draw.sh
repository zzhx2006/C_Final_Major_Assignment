#!/bin/bash

# 定义一个数组，包含提供的函数名
function_names=("create_socket" "set_address_struct" "send_name_to_server" "connect_socket_and_addrstruct" "print_client_status" "send_message_to_server" "receive_from_server" "receive_guard" "printmenu")

# 遍历数组中的每个函数名
for func_name in "${function_names[@]}"; do
    # 替换cxx2flow命令中的函数名，并执行
    cxx2flow ../../src/client/client.c -o "${func_name}.dot" "${func_name}"

    # 替换dot命令中的函数名，并执行
    dot -Tpng "${func_name}.dot" -o "${func_name}.png"
done
