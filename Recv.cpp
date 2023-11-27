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
    amqp_basic_consume(connState, 1, amqp_cstring_bytes(queue.c_str()), amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    cout << "登陆消息服务器成功，开始接收数据" << endl;

    while (1)
    {
        amqp_envelope_t envelope;

        timeval tvTimeout;
        tvTimeout.tv_sec = 10;
        tvTimeout.tv_usec = 0;
        amqp_rpc_reply_t ret = amqp_consume_message(connState, &envelope, &tvTimeout, 0);

        if (AMQP_RESPONSE_NORMAL != ret.reply_type){
            if (AMQP_STATUS_SOCKET_ERROR == ret.library_error){
                cout << "跟消息服务器连接中断，清理资源，重连连接" << endl;
                break;
            }
            if (AMQP_STATUS_TIMEOUT == ret.library_error)
            {
                cout << "等待消息服务器消息超时，继续等待" << endl;
                continue;
            }
            cout << "跟消息服务器连接出现异常，清理资源，重连连接" << endl;
            break;
        }
        else
        {
            string strRecvMsg((char*)envelope.message.body.bytes, envelope.message.body.len);
            cout << "接收到的信息:" << strRecvMsg<< std::endl;
            amqp_destroy_envelope(&envelope);
            continue;
        }
        amqp_channel_close(connState, 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(connState, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connState);
        return 0;
    }
}
