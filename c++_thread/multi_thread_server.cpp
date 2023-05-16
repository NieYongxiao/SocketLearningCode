//使用c++的thread库实现一个简单的tcp回声服务器，蕴含了c++的面向对象思想

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

using std::string,std::cout,std::endl,std::vector,std::thread;

#define BUF_SIZE 1024
#define DEFAULT_PORT 8090
#define LISTEN_NUM 5

socklen_t sock_addr_len = sizeof(sockaddr_in);

void error_handler(const string&& str);

class ChatServer{  //用于实现连接
public:
    ChatServer();
    ChatServer(int port);
    ~ChatServer(){
        for(thread& t:threads){
            t.join();
        }
    }
public:
    void chatServerBind();
    void chatServerListen();
    void chatServerAccept();
    void chatServerRun(); //实现绑定，监听，连接
    void chatServerFunc(int clnt_sock); //线程函数，实现回声
private:
    int serv_sock;
    struct sockaddr_in serv_sock_addr;
    vector<thread> threads;
};

class ChatHandler{   
public:
    ChatHandler();
    ~ChatHandler();
public:
private:
};

int main(){
    ChatServer server;
    server.chatServerRun();
    return 0;
}

ChatServer::ChatServer(){
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock < 0){
        error_handler("socket error");
    }
    memset(&serv_sock_addr,0,sizeof(serv_sock_addr));
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sock_addr.sin_port = htons(DEFAULT_PORT);
}

ChatServer::ChatServer(int port){
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(serv_sock < 0){
        error_handler("socket error");
    }
    memset(&serv_sock_addr,0,sizeof(serv_sock_addr));
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sock_addr.sin_port = htons(port);
}

void ChatServer::chatServerBind(){
    if(bind(serv_sock, (struct sockaddr*)&serv_sock_addr,sock_addr_len) == -1){
        error_handler("bind error");
    }
}

void ChatServer::chatServerListen(){
    if(listen(serv_sock, LISTEN_NUM) == -1){
        error_handler("listen error");
    }
}

void ChatServer::chatServerAccept(){
    while(true){
        struct sockaddr_in clnt_sock_addr;
        int clnt_sock = accept(serv_sock,(struct sockaddr *)&clnt_sock_addr,&sock_addr_len);
        if(clnt_sock<0) continue; //为什么断开一个之后又会accept一次呢
        cout<<"accept successful"<<endl;
        thread t(&ChatServer::chatServerFunc,this,clnt_sock);
        threads.push_back(move(t));
    }
}

void ChatServer::chatServerRun(){
    this->chatServerBind();
    this->chatServerListen();
    this->chatServerAccept();
}

void ChatServer::chatServerFunc(int clnt_sock){
    while(1){
        char buffer[BUF_SIZE];
        memset(buffer,0,sizeof(buffer));
        int str_len = recv(clnt_sock,buffer,sizeof(buffer),0);
        if(str_len == 0 || strcmp(buffer,"q")==0){
            cout<<"disconnected"<<endl;  //断开连接
            close(clnt_sock);
            break;
        }
        else{
            cout<<"message from clnt_sock is:"<<buffer<<endl;
            send(clnt_sock,buffer,strlen(buffer),0);
        }
    }
    close(serv_sock);
}

void error_handler(const string&& str){
    cout<<str<<endl;
    exit(1);
}