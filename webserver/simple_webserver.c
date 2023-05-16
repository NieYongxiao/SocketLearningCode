//webbench 无论设置10 100 还是1000 均不会超过50000的连接成功
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
#include <sys/stat.h>
#include <signal.h>

#define BUF_SIZE 1024
#define port 8090

void error_handling(char* message);
void send_html_file(char* buf,int clnt_sock,FILE* file_reader);
void send_txt_file(char* buf,int clnt_sock,FILE* file_reader);
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader);
void send_gif_file(char* buf,int clnt_sock,FILE* file_reader);


int main() 
{
    // 试图使用sigaction忽略SIGPIPE信号，但失败只能对send设置参数MSG_NOSIGNAL
    // struct sigaction sa;
    // sa.sa_handler = SIG_IGN;
    // sigaction( SIGPIPE, &sa, 0 );
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    clnt_addr_size = sizeof(clnt_addr);
    char buf[BUF_SIZE];
    FILE *file_reader;  //打开文件
    //struct stat file_stat;  //后续获取文件信息

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) {
        char message[] = "socket error";
        error_handling(message);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        char message[] = "bind error";
        error_handling(message);
    }

    if(listen(serv_sock, 5) == -1) {
        char message[] = "listen error";
        error_handling(message);
    }

    int cnt_accept_num=0;
    
    while (1) {
        ++cnt_accept_num;
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(clnt_sock < 0 ) continue;
        printf("clnt_sock accepted %d times \n",cnt_accept_num);
        if(clnt_sock == -1) {
            char message[] = "accept error";
            error_handling(message);
        }

        memset(buf, 0, BUF_SIZE);
        read(clnt_sock, buf, BUF_SIZE); // 从客户端读取 HTTP 请求

        // 将请求报文的第一行按照空格解析出请求方法和URL
        char method[10], pathname[50], protocol[10];
        sscanf(buf, "%s %s %s", method, pathname, protocol); 
        printf("method is %s\n", method);
        printf("pathname is %s\n", pathname);
        printf("protocol is %s\n", protocol);
        //这段代码使用了C标准函数sscanf()，它的作用是从一个字符串中读取数据并按照指定格式进行解析，然后将解析结果存储到变量中。
        //格式字符串为"%s %s %s"，表示将从buf中读取三个以空格分隔的字符串，分别存储到method、pathname和protocol指向的变量中。
        //假设buf为"GET /index.html HTTP/1.1"，则调用后，
        //method将存储字符串"GET"，pathname将存储字符串"/index.html"，protocol将存储字符串"HTTP/1.1"。
        //这些字符串中不包括空格，因为空格在格式字符串中被用来分隔不同的字符串。

        char path[BUF_SIZE];
        //处理 GET 方法请求
        if(strcmp(method, "GET") == 0)
        {
            printf("second pathname is %s\n", pathname);
            char absolute_path[]="/home/xiaosa/web_server_source_material/";
            if(strcmp(absolute_path,pathname) >= 0){  //此时输入路径只有“/”
                strcpy(path,"/home/xiaosa/web_server_source_material/default.html");
                puts("flag >= 0");
            }
            else {
                strcpy(path,pathname);
                puts("flag < 0");
            }
            
            path[strlen(path)] = '\0';

            printf("path is %s\n", path);
            file_reader = fopen(path, "rb"); 
            //如果 fopen 函数的参数只给 r，则文件会以只读模式打开，同时会将文件内容以文本形式转换为程序内部使用的格式。
            //在这种情况下，fopen 函数默认将文件内容当作文本来处理，而不是二进制数据。
            //这意味着，在读取文件内容时，fopen 函数会将文件中的换行符 \n 转换为 C 语言中的换行符 \r\n。
            //只给 r 参数打开文件时，适合读取文本文件，但不适合读取包含二进制数据的文件，如图片、音频、视频等。
            //综上所述，如果要读取文本文件，应该使用 "r" 模式；如果要读取二进制文件，如图片、音频、视频等，应该使用 "rb" 模式。
            //但是我懒得改了

            //如果打开文件失败 回复  404 Not Found
            if(file_reader == NULL)
            {
                sprintf(buf, "HTTP/1.1 404 Not Found\r\n");  //将数据打印到字符串中。
                send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
                sprintf(buf, "Content-Type:text/html\r\n\r\n");
                send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文头部
                sprintf(buf, "<html><body><h1>404 Not Found  文件打开失败</h1></body></html>\r\n");
                send(clnt_sock, buf, strlen(buf),MSG_WAITALL); // 发送 HTTP 响应报文体
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

        close(clnt_sock);
    }

    fclose(file_reader);
    close(serv_sock);
    return 0;
}

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void send_html_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部 | MSG_NOSIGNAL忽略SIGPIPE信号
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
    sprintf(buf, "Content-Type: text/plain; charset=utf-8\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL) | MSG_NOSIGNAL; // 发送 HTTP 响应报文头部
    while(fgets(buf, BUF_SIZE, file_reader) != NULL)
    {
        send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
}

//两个大坑，首先要使用fread不能用fgets，其次不能用strlen函数算出buf的长度，因为后面短的没法完全覆盖
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader) {
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
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
    sprintf(buf, "Content-Type: image/gif\r\n\r\n");
    send(clnt_sock, buf, strlen(buf),MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文头部
    int bytes_read;
    while((bytes_read = fread(buf, sizeof(unsigned char),BUF_SIZE, file_reader)) >0)
    {
        send(clnt_sock, buf, bytes_read,MSG_WAITALL | MSG_NOSIGNAL); // 发送 HTTP 响应报文体
    }
}