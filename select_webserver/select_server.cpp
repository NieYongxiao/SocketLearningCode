//并发效率极低,需要与多进程一起使用，性能有所提升，然而创建子进程之后select总会失败一次，为什么呢，
//因为子进程的原因吗，还是close 的时候出现问题了呢，但是子进程内容的更改不会影响父进程阿
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/wait.h>
#include<iostream>
#include<string>
using std::cout,std::endl,std::string;

#define BUF_SIZE  1024
#define port 8090

void error_handler(string&& message);
void signal_for_waitpit(int sig);
void send_error_file(char* buf,int clnt_sock);
void send_html_file(char* buf,int clnt_sock,FILE* file_reader);
void send_txt_file(char* buf,int clnt_sock,FILE* file_reader);
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader);
void send_gif_file(char* buf,int clnt_sock,FILE* file_reader);

int main(){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_sock_addr,clnt_sock_addr;
    socklen_t sock_addr_len = sizeof(serv_sock_addr);
    serv_sock = socket(AF_INET, SOCK_STREAM,0);
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sock_addr.sin_port = htons(port);
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(bind(serv_sock, (struct sockaddr *)&serv_sock_addr,sock_addr_len) <0){
        error_handler("bind error!");
    }
    if(listen(serv_sock, 5) <0){
        error_handler("listen error!");
    }

    int pid;

    fd_set reads,reads_copy;
    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    struct timeval timeout;
    int fd_max=serv_sock;
    char buf[BUF_SIZE];
    
    struct sigaction act;
    act.sa_handler=signal_for_waitpit;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD, &act, 0); //SIGCHLD 子进程终止  SIGINT 输入ctrl+c  SIGALRM 已到调用alarm函数注册的时间

    while(true){
        reads_copy = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int fd_num = select(fd_max+1,&reads_copy,NULL,NULL,&timeout);
        if(fd_num< 0){
            printf("Error is %d\n",fd_num);
            continue;
        }
        else if(fd_num==0){
            //puts("TIME OUT!");
            continue;
        }
        for(int i=0;i<fd_max+1;i++){
            if(FD_ISSET(i,&reads_copy)){
                if(i==serv_sock)
                {
                    puts("          try to connect!");
                    clnt_sock=accept(i,(struct sockaddr*)&clnt_sock_addr,&sock_addr_len);
                    //puts("          connect successfully!");
                    FD_SET(clnt_sock,&reads);
                    if(fd_max < clnt_sock) fd_max = clnt_sock;
                    //printf("clnt_sock connected %d\n",clnt_sock);
                }
                else
                {
                    pid = fork();
                    if(pid == 0){
                        memset(buf,0,sizeof(buf));
                        int str_len = read(i,buf,sizeof(buf));
                        if(str_len ==0){
                            FD_CLR(i,&reads);
                            FD_CLR(serv_sock,&reads);
                            close(i);
                            close(serv_sock);
                            return 0;
                        }
                        char method[10], pathname[50], protocol[10];
                        sscanf(buf, "%s %s %s", method, pathname, protocol); 
                        printf("method is %s\n", method);
                        printf("pathname is %s\n", pathname);
                        printf("protocol is %s\n", protocol);
                        char path[BUF_SIZE];
                        //处理 GET 方法请求
                        if(strcmp(method, "GET") == 0)
                        {
                            if(strcmp(pathname,"/") == 0){  //此时输入路径只有“/”
                                strcpy(path,"/home/xiaosa/web_server_source_material/default.html");
                            }
                            else {
                                strcpy(path,pathname);
                            }
                            path[strlen(path)] = '\0';
                            printf("file open path is %s\n", path);
                            FILE* file_reader = fopen(path, "rb"); 
                            if(file_reader == NULL)
                            {
                                send_error_file(buf,clnt_sock);
                            }
                            else{
                                char *file_type = strrchr(path, '.'); //此时file_type是.http .txt .jpg .mp3
                                //strrchr用于在一个字符串中查找最后一个匹配给定字符（或者字符的ASCII码值）的位置，并返回该位置的指针。
                                if(strcmp(file_type, ".html") == 0) send_html_file(buf, clnt_sock, file_reader);
                                else if(strcmp(file_type, ".txt") == 0) send_txt_file(buf, clnt_sock, file_reader);
                                else if(strcmp(file_type, ".jpg") == 0)  send_jpg_file(buf, clnt_sock, file_reader);
                                else if(strcmp(file_type, ".gif") == 0)  send_gif_file(buf, clnt_sock, file_reader);
                            }
                        }
                        printf("closed client: %d\n",i);
                        FD_CLR(i,&reads);
                        FD_CLR(serv_sock,&reads);
                        close(i);
                        close(serv_sock);
                        return 0;
                    }
                    else{
                        FD_CLR(i,&reads);
                        close(i);
                        continue;
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}

void signal_for_waitpit(int sig){
    int status;
    pid_t pid = waitpid(-1,&status,WNOHANG);
    printf("removed proc id is %d\n",pid);
}
void error_handler(string&& message){
    cout<<message<<endl;
    exit(1);
}

void send_error_file(char* buf,int clnt_sock) {
    sprintf(buf, "HTTP/1.1 404 Not Found\r\n");  //将数据打印到字符串中。
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    sprintf(buf, "Content-Type:text/html\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    sprintf(buf, "<html><body><h1>404 Not Found  文件打开失败</h1></body></html>\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文体
}

void send_html_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文体
    }
}

// 阻塞式send
// void send_html_file(char* buf,int clnt_sock,FILE* file_reader) {
//     sprintf(buf, "HTTP/1.1 200 OK\r\n");
//     send(clnt_sock, buf, strlen(buf),MSG_DONTWAIT); // 发送 HTTP 响应报文头部
//     sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n\r\n");
//     send(clnt_sock, buf, strlen(buf),MSG_DONTWAIT); // 发送 HTTP 响应报文头部
//     while(fgets(buf, BUF_SIZE, file_reader) != NULL)
//     {
//         if(send(clnt_sock, buf, strlen(buf),MSG_DONTWAIT) < strlen(buf) ){ // 发送 HTTP 响应报文体
//             puts("send failed");
//         }
//     }
// }

void send_txt_file(char* buf,int clnt_sock,FILE* file_reader){
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    sprintf(buf, "Content-Type: text/plain; charset=utf-8\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文体
    }
}

//两个大坑，首先要使用fread不能用fgets，其次不能用strlen函数算出buf的长度，因为后面短的没法完全覆盖
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    sprintf(buf, "Content-Type: image/jpeg\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL); // 发送 HTTP 响应报文体
    }
}

void send_gif_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    sprintf(buf, "Content-Type: image/gif\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL); // 发送 HTTP 响应报文体
    }
}