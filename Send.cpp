////
////  main.cpp
////  producer
////
////  Created by xpf on 2021/3/25.
////  Copyright © 2021 xpf. All rights reserved.
////
////生产者
//
#include <iostream>
#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>
using namespace std;


int main(int argc, char *argv[]){
    string strIP = "127.0.0.1";
    int nPort = 5672;
    string strUserName = "guest";
    string strPassword= "guest";
    
    amqp_connection_state_t connState = amqp_new_connection();
    amqp_socket_t *pSocket = amqp_tcp_socket_new(connState);
    if (!pSocket) {
        amqp_connection_close(connState, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connState);
        cout << "跟消息服务器创建连接失败" << endl;
        return 0;
    }
    
    int nConnStatus = amqp_socket_open(pSocket, strIP.c_str(), nPort);
    if (AMQP_STATUS_OK != nConnStatus) {
        amqp_connection_close(connState, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connState);
        return 0;
    }

    amqp_rpc_reply_t  rpcReply = amqp_login(connState, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, strUserName.c_str(), strPassword.c_str());
    if (AMQP_RESPONSE_NORMAL != rpcReply.reply_type){
        cout << "登陆消息服务器失败" << endl;
        return 0;
    }
    string queue = "hello";
    amqp_channel_open(connState, 1);
    amqp_queue_declare(connState, 1, amqp_cstring_bytes(queue.c_str()), false, false, false, false, amqp_empty_table);

    while(1){
        char message[26] = {'\0'};
        cout<<"input your message:"<<endl;
        cin.getline(message, sizeof(message));
        amqp_bytes_t message_byte;
        message_byte.len = sizeof(message);
        message_byte.bytes = message;
        amqp_basic_publish(connState, 1, amqp_cstring_bytes(""), amqp_cstring_bytes(queue.c_str()), 0, 0, nullptr, message_byte);
        cout<<"send message over!"<<endl;
    }
    
    amqp_channel_close(connState, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(connState, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(connState);
    return 0;
}


