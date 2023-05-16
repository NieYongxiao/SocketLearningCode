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

#define BUF_SIZE 1024
#define DEFAULT_PORT 8090
#define LISTEN_NUM 5

// ChatServer server;
// server.chatServerRun();
// return 0;
void chatServerFunc(int clnt_sock); //线程函数
void chatServerErrorHandler(const string&& str); 
void analysis_message(char *buffer,int clnt_sock);
void handle_get(string&& path,int clnt_sock);
void send_html_file(char* buf,int clnt_sock,FILE* file_reader);
void send_txt_file(char* buf,int clnt_sock,FILE* file_reader);
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader);
void send_gif_file(char* buf,int clnt_sock,FILE* file_reader);

mutex mtx;

int main(){
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_sock_addr;
    socklen_t sock_addr_len = sizeof(serv_sock_addr);
    if(serv_sock < 0){
        chatServerErrorHandler("socket error");
    }
    memset(&serv_sock_addr,0,sizeof(serv_sock_addr));
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sock_addr.sin_port = htons(DEFAULT_PORT);
    if(bind(serv_sock, (struct sockaddr*)&serv_sock_addr,sock_addr_len) == -1){
        chatServerErrorHandler("bind error");
    }
    if(listen(serv_sock, LISTEN_NUM) == -1){
        chatServerErrorHandler("listen error");
    }
    while(true){
        struct sockaddr_in clnt_sock_addr;
        cout<<"wait for accept"<<endl;
        int clnt_sock = accept(serv_sock,(struct sockaddr *)&clnt_sock_addr,&sock_addr_len);
        if(clnt_sock<0){
            cout<<"accept error"<<endl;
            continue; //为什么断开一个之后又会accept一次呢
        } 
        else cout<<"accept successful"<<endl;
        thread t(chatServerFunc,clnt_sock);
        t.join();
    }
    return 0;
}

void chatServerFunc(int clnt_sock){ //线程函数
    char buffer[BUF_SIZE];
    std::unique_lock<mutex> lock(mtx);
    memset(buffer,0,sizeof(buffer));
    int str_len = recv(clnt_sock,buffer,sizeof(buffer),0);
    lock.unlock();
    if(str_len == 0 ){
        cout<<"thread:"<< std::this_thread::get_id()<<" disconnected"<<endl;  //断开连接
        close(clnt_sock);
    }
    else{
        char method[10], pathname[50], protocol[10];
        unique_lock<mutex> lock(mtx);
        sscanf(buffer, "%s %s %s", method, pathname, protocol); 
        lock.unlock();
        printf("method is %s\n", method);
        printf("pathname is %s\n", pathname);
        printf("protocol is %s\n", protocol);

        string path;
        if(strcmp(method, "GET") != 0) chatServerErrorHandler("strcmp error");
        else{
            unique_lock<mutex> lock(mtx);
            if(strcmp(pathname,"/") == 0){  //此时输入路径只有“/”
                path="/home/xiaosa/web_server_source_material/default.html";
            }
            else {
                path = pathname;
            }
            lock.unlock();
            cout<<"file path is "<<path<<endl;

            FILE* file_reader = fopen(path.c_str(), "rb"); 
            char buf[BUF_SIZE];
            if(file_reader == NULL)
            {
                unique_lock<mutex> lock(mtx);
                sprintf(buf, "HTTP/1.1 404 Not Found\r\n");  //将数据打印到字符串中。
                send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
                sprintf(buf, "Content-Type:text/html\r\n\r\n");
                send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
                sprintf(buf, "<html><body><h1>404 Not Found  文件打开失败</h1></body></html>\r\n");
                send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
                lock.unlock();
            }
            else{
                unique_lock<mutex> lock(mtx);
                string file_type = path.substr(path.find_last_of(".")+1); 
                lock.unlock();
                //此时file_type是.http .txt .jpg .mp3
                //strrchr用于在一个字符串中查找最后一个匹配给定字符（或者字符的ASCII码值）的位置，并返回该位置的指针。
                if(file_type == "html")  send_html_file(buf, clnt_sock, file_reader);
                else if(file_type == "txt") send_txt_file(buf, clnt_sock, file_reader);
                else if(file_type == "jpg")  send_jpg_file(buf, clnt_sock, file_reader);
                else if(file_type == "gif")  send_gif_file(buf, clnt_sock, file_reader);
            }
            close(clnt_sock);
        }
    }
}
void send_html_file(char* buf,int clnt_sock,FILE* file_reader){
    unique_lock<mutex> lock(mtx);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sContent-Type: text/html; charset=utf-8\r\n\r\n",buf);
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
    lock.unlock();
}

void send_txt_file(char* buf,int clnt_sock,FILE* file_reader){
    unique_lock<mutex> lock(mtx);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sContent-Type: text/plain; charset=utf-8\r\n\r\n",buf);
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
    lock.unlock();
}

//两个大坑，首先要使用fread不能用fgets，其次不能用strlen函数算出buf的长度，因为后面短的没法完全覆盖
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader) {
    unique_lock<mutex> lock(mtx);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sContent-Type: image/jpeg; charset=utf-8\r\n\r\n",buf);
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
    lock.unlock();
}

void send_gif_file(char* buf,int clnt_sock,FILE* file_reader) {
    unique_lock<mutex> lock(mtx);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sContent-Type: image/gif; charset=utf-8\r\n\r\n",buf);
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
    lock.unlock();
}

void chatServerErrorHandler(const string&& str){
    unique_lock<mutex> lock(mtx);
    cout<<str<<endl;
    lock.unlock();
    exit(1);
}