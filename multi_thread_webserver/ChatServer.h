#pragma once
#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<mutex>

using std::string,std::cout,std::endl,std::vector,std::thread,std::mutex,std::unique_lock;

class ChatServer{  //用于实现连接
public:
    ChatServer();
    ChatServer(int port);
    ~ChatServer(){
        for(thread& t:threads){
            t.join();
        }
        close(serv_sock);
    }
public:
    void chatServerBind();
    void chatServerListen();
    void chatServerAccept();
    void chatServerRun(); //实现绑定，监听，连接
    void chatServerFunc(int clnt_sock); //线程函数，实现回声
    void chatServerErrorHandler(const string&& str); 
    void analysis_message(char *buffer,int clnt_sock);
    void handle_get(string&& path,int clnt_sock);
    void send_html_file(char* buf,int clnt_sock,FILE* file_reader);
    void send_txt_file(char* buf,int clnt_sock,FILE* file_reader);
    void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader);
    void send_gif_file(char* buf,int clnt_sock,FILE* file_reader);

private:
    int serv_sock;
    struct sockaddr_in serv_sock_addr;
    mutex mtx;
    vector<thread> threads;
};

#endif // CHAT_SERVER_H