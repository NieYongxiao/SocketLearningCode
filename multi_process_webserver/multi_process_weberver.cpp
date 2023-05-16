//支持1500左右的并发数量
//fork之后子进程从fork后的代码开始执行
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<signal.h>

#define BUF_SIZE  1024
#define port 8090

void error_handler(char *message);
void signal_for_waitpit(int sig);
void send_html_file(char* buf,int clnt_sock,FILE* file_reader);
void send_txt_file(char* buf,int clnt_sock,FILE* file_reader);
void send_jpg_file(char* buf,int clnt_sock,FILE* file_reader);
void send_gif_file(char* buf,int clnt_sock,FILE* file_reader);

int main(){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t sock_addr_size = sizeof(serv_addr);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(port);
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
        puts("bind error!");
        exit(0);
    }
    if(listen(serv_sock,5)==-1){
        puts("listen error!");
        exit(0);
    }
    char buf[BUF_SIZE];

    FILE *file_reader;

    //设定信号处理，当子进程结束的时候调用waitpid销毁
    pid_t pid;
    struct sigaction act;
    act.sa_handler=signal_for_waitpit;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD, &act, 0); //SIGCHLD 子进程终止  SIGINT 输入ctrl+c  SIGALRM 已到调用alarm函数注册的时间

    //pipe进程通信
    //1）如果管道中没数据，则read会阻塞，直到读到数据（超市中买方便面没买到，会一直等）
    //2）管道中若数据满了，则write会阻塞，直到有空闲空间
    //3）若所有读端被关闭，则write会触发异常-----SIGPIPE（导致进程退出），再写的话就是浪费资源，浪费资源就是犯罪，我就要让你崩溃。
    //4）若所有写端被关闭，则read读完之后不会阻塞，而是返回0（告诉用户，你不要在读了，没人写了）
    //因此可以使用管道进行群聊天，保存文件等等
    // int pipes[2],file_pipes[2];
    // pipe(pipes);
    // pipe(file_pipes);
    
    while(true){
        puts("wait for accept...");
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &sock_addr_size);
        if(clnt_sock == -1){
            continue;
        }
        else pid = fork();
        if(pid == -1){
            char message[]="fork error!";
            error_handler(message);
        }
        else if(pid==0){
            memset(buf, 0, sizeof(buf));
            read(clnt_sock, buf, sizeof(buf));
            char method[10], pathname[50], protocol[10];
            sscanf(buf, "%s %s %s", method, pathname, protocol); 
            printf("method is %s\n", method);
            printf("pathname is %s\n", pathname);
            printf("protocol is %s\n", protocol);

            char path[BUF_SIZE];
            if(strcmp(method,"GET")==0){
                if(strcmp(pathname,"/")==0){
                    strcpy(path,"/home/xiaosa/web_server_source_material/default.html");
                }
                else {
                    strcpy(path,pathname); 
                }
                path[strlen(path)] = '\0';
                printf("file pathname is %s\n", path);
                file_reader = fopen(path,"rb"); 
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
            break;
        }
        else{
            close(clnt_sock);
            continue;
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
void error_handler(char *message)
{
    puts(message);
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