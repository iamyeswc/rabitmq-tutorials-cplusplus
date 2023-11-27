//
//  main.cpp
//  consumer
//
//  Created by xpf on 2021/3/25.
//  Copyright © 2021 xpf. All rights reserved.
//
#include <iostream>
#include <amqp.h>
#include <amqp_framing.h>
#include <amqp_tcp_socket.h>
using namespace std;

void delay_msc(int msc){
    clock_t now = clock();
    while(clock() - now < msc);
}

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

    amqp_channel_open(connState, 1);
    string queue = "work_queue";
    amqp_queue_declare(connState, 1, amqp_cstring_bytes(queue.c_str()), false, true, false, false, amqp_empty_table);
//    amqp_basic_consume(connState, 1, amqp_cstring_bytes("hello"), amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    cout << "登陆消息服务器成功，开始接收数据" << endl;
    
//    string name = "one";
//    string delayStr = "1";
    string name = argv[1];
    string delayStr = argv[2];
    int delay = stoi(delayStr);//转换为int类型
    
    while (1)
    {
        amqp_basic_get(connState, 1, amqp_cstring_bytes(queue.c_str()), true);//接收到一个就应答一下
        amqp_message_t *message = new amqp_message_t;
        amqp_read_message(connState, 1, message, 0);
        cout<<"【"<<name<<"】,result is:"<<(char *)message->body.bytes<<endl;
        amqp_destroy_message(message);
        delete message;
        delay_msc(1000 * delay);
        
    }
    amqp_channel_close(connState, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(connState, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(connState);
    return 0;
}
