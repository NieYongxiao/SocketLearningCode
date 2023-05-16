//2000左右问题不大
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<errno.h>  //引入全局变量errno
#include<fcntl.h>  //引入fcntl函数
#include<signal.h>

//边缘触发方式必须用非阻塞read write ，否则可能引起服务器端的长时间停顿

//#define SERVER_STRING "Server: epoll_server_edge_trigger\r\n"   // 服务器名称
#define BUF_SIZE  1024  
#define port 8090
#define EPOLL_SIZE 50

void set_non_block_mode(int fd);
void error_handler(char *message);
//int file_size(char*path);

void send_html_file(char* buf,int clnt_sock,FILE* file_reader);
void send_txt_file(char* buf,int clnt_sock,FILE* file_reader);
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader);
void send_gif_file(char* buf,int clnt_sock,FILE* file_reader);
void send_ico_file(char* buf,int clnt_sock,FILE* file_reader);

int main()
{
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_sock_addr,clnt_sock_addr;
    socklen_t sock_addr_size = sizeof(serv_sock_addr);
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_sock_addr.sin_family=AF_INET;
    serv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sock_addr.sin_port=htons(port);    
    if(bind(serv_sock, (struct sockaddr *)&serv_sock_addr, sock_addr_size)<0)
    {
        char message[]="bind error!";
        error_handler(message);
    }
    if(listen(serv_sock,5) <0)
    {
        char message[]="listen error!";
        error_handler(message);
    }
    char buf[BUF_SIZE];
    
    struct epoll_event event; //工具人用于注册文件描述符
    struct epoll_event *ep_events;
    int epfd,event_cnt;
    epfd=epoll_create(EPOLL_SIZE); //创建保存epoll文件描述符的空间，成功时返回文件描述符，失败时返回-1
    //手动分配空间，返回分配空间的首地址 此处空间为了保存发生情况的events  
    ep_events= (struct epoll_event*)(malloc(sizeof(struct epoll_event)*EPOLL_SIZE)); 

    set_non_block_mode(serv_sock);
    //以下三行为了注册serv_sock
    event.data.fd=serv_sock;
    event.events=EPOLLIN; // 设定监测的信息为发生需要读取数据的情况
    epoll_ctl(epfd,EPOLL_CTL_ADD,serv_sock,&event); //向内部注册并注销文件描述符
    
    int epoll_wait_num=0;
    while(1)
    {
        epoll_wait_num++;
        //等待文件描述符发生变化，成功时返回发生事件的文件描述符数，失败时返回-1
        event_cnt = epoll_wait(epfd,ep_events,EPOLL_SIZE,-1); 
        printf("epoll_wait %d times \n",epoll_wait_num);
        if(event_cnt < 0)
        {
            char message[]="epoll_wait error!";
            error_handler(message);
        }
        for(int i=0;i<event_cnt;i++)
        {
            if(ep_events[i].data.fd==serv_sock)
            {
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_sock_addr, &sock_addr_size);
                printf("clnt_sock %d  accepted \n",clnt_sock);

                set_non_block_mode(clnt_sock);
                //向epoll_event 注册 clnt_sock
                event.data.fd=clnt_sock;
                event.events=EPOLLIN|EPOLLET; // 将触发方式设置为边缘触发
                epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_sock,&event);
            }
            else
            {
                memset(buf,0,sizeof(buf));
                while(1)  //为保证完全读取完全 需循环read
                {
                    int str_len = read(ep_events[i].data.fd,buf,BUF_SIZE);
                    if(str_len == 0)
                    {
                        epoll_ctl(epfd,EPOLL_CTL_DEL,ep_events[i].data.fd,NULL);
                        close(ep_events[i].data.fd);
                        printf("clnt_sock %d  disconnected \n",ep_events[i].data.fd);
                        break;
                    }
                    else if(str_len <0)
                    {
                        if(errno==EAGAIN) break;   //当两个条件都满足时，说明读取了输入缓冲的全部数据
                    }
                    else
                    {
                        read(clnt_sock, buf, BUF_SIZE); // 从客户端读取 HTTP 请求
                    }
                }
                if(strlen(buf) == 0) continue;
                //puts(buf);
                // 将请求报文的第一行按照空格解析出请求方法和URL
                char method[10], pathname[100], protocol[10];
                sscanf(buf, "%s %s %s", method, pathname, protocol); 
                printf("method is %s\n", method);
                printf("pathname is %s\n", pathname);
                printf("protocol is %s\n", protocol);

                char path[BUF_SIZE];
                //处理 GET 方法请求
                if(strcmp(method, "GET") == 0)
                {
                    char absolute_path[BUF_SIZE]="/home/xiaosa/web_server_source_material";
                    if(strcmp(pathname, "/")==0){ //此时输入路径只有“/”
                        strcpy(path,"/home/xiaosa/web_server_source_material/default.html");
                    }
                    else if(strlen(pathname) < 40){  //此时输入路径为/favicon.ico
                        strcat(absolute_path,pathname);
                        strcpy(path,absolute_path);
                    }
                    else {
                        strcpy(path,pathname);
                    }
                    
                    path[strlen(path)] = '\0';

                    printf("Open file path is %s\n", path);
                    FILE* file_reader = fopen(path, "rb"); 

                    //如果打开文件失败 回复  404 Not Found
                    if(file_reader == NULL)
                    {
                        sprintf(buf, "HTTP/1.1 404 Not Found\r\n");  //将数据打印到字符串中。
                        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
                        sprintf(buf, "Content-Type:text/html\r\n\r\n");
                        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
                        sprintf(buf, "<html><body><h1>404 Not Found  文件打开失败</h1></body></html>\r\n");
                        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
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
                epoll_ctl(epfd,EPOLL_CTL_DEL,ep_events[i].data.fd,NULL);
                close(ep_events[i].data.fd);
                printf("clnt_sock %d  disconnected \n",ep_events[i].data.fd);
            }
        }
    }
    close(serv_sock);
    close(epfd);
    return 0;
}

void set_non_block_mode(int fd)
{
    int flag = fcntl(fd,F_GETFL,0);  //fcntl根据文件描述词来操作文件的特性
    fcntl(fd,F_SETFL,flag | O_NONBLOCK);  //打开文件之后设置为非阻塞 
}
void error_handler(char*message)
{
    puts(message);
    exit(1);
}

void send_html_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    // 发送长度 连接关闭等有关信息
    // sprintf(buf, "Content-Length : %d\r\n",file_size(path)); 
    // printf("file size : %d \n",file_size(path));
    // send(clnt_sock, buf, strlen(buf), MSG_WAITALL | MSG_NOSIGNAL);
    // sprintf(buf, "Connection: close\r\n");
    // send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    sprintf(buf, "Content-Type: text/html; charset=utf-8\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
}
void send_txt_file(char* buf,int clnt_sock,FILE* file_reader){
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    // sprintf(buf, SERVER_STRING); //发送服务器有关信息
    // send(clnt_sock, buf, strlen(buf), MSG_WAITALL | MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: text/plain; charset=utf-8\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
}
//两个大坑，首先要使用fread不能用fgets，fread读取二进制文件，fget读取文本文件 其次不能用strlen函数算出buf的长度，因为后面短的没法完全覆盖
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    // sprintf(buf, SERVER_STRING); //发送服务器有关信息
    // send(clnt_sock, buf, strlen(buf), MSG_WAITALL | MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: image/jpeg\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
}
void send_gif_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    // sprintf(buf, SERVER_STRING); //发送服务器有关信息
    // send(clnt_sock, buf, strlen(buf), MSG_WAITALL | MSG_NOSIGNAL);
    sprintf(buf, "Content-Type: image/gif\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
}

// int file_size(char*path)//获取文件名为filename的文件大小。
// {
//     FILE *fp = fopen(path,"rd");
//     if(fp == NULL) // 打开文件失败
//         return -1;
//     fseek(fp, 0, SEEK_END);//定位文件指针到文件尾。
//     int size=ftell(fp);//获取文件指针偏移量，即文件大小。
//     fclose(fp);//关闭文件。
//     return size;
// }