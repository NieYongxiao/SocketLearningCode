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

//非常基础的一个webserver，可以打开html gif jpg png css js等文件
//如果要打开hello.html文件，就在浏览器输入http://127.0.0.1:8090/hello.html
//也可以直接输入http://127.0.0.1:8090/，则输出默认文件default.html
//存在一个bug，即html文件不能有中文，否则会乱码，应该是编码格式有问题，目前没时间修改

#define BUFFER_SIZE 4096    // 缓冲区大小
#define SERVER_STRING "Server: simple_web_server\r\n"   // 服务器名称
#define port 8090

void error_die(const char *msg);    // 错误处理函数
void handle_request(int client_socket);   // 处理请求

int main(int argc, char *argv[]) {
    int server_socket, client_socket;   // 服务器套接字和客户端套接字
    socklen_t client_addr_len;
    struct sockaddr_in server_addr, client_addr;

    // 创建套接字
    if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        error_die("socket");
    }

    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        error_die("bind");
    }

    // 监听
    if (listen(server_socket, 5) == -1) {
        error_die("listen");
    }
    printf("Server running on port %d...\n",port);

    while (1) {
        // 接受连接
        client_addr_len = sizeof(client_addr);
        if ((client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_len)) == -1) {
            perror("accept");
        }

        // 处理请求
        handle_request(client_socket);

        // 关闭连接
        close(client_socket);
    }

    return 0;
}

// 错误处理函数
void error_die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// 处理请求
void handle_request(int client_socket) {
    char buf[BUFFER_SIZE];
    char method[255];
    char url[255];
    char path[512];
    char *filetype;
    size_t i, j;
    struct stat st;
    int fd, len;

    // 获取请求内容
    len = recv(client_socket, buf, BUFFER_SIZE, 0);

    printf("buf is \n%s",buf);

    // 解析请求
    i = 0;
    j = 0;
    while (!isspace((int) buf[j]) && (i < sizeof(method) - 1)) {   //如果不是空格就将buf的值复制到method里
        method[i++] = buf[j++];
    }
    method[i] = '\0';
    
    printf("method is \n%s \n",method); //看看复制之后的请求是什么

    i = 0;
    while (isspace((int) buf[j]) && (j < len)) {
        j++;
    }

    while (!isspace((int) buf[j]) && (i < sizeof(url) - 1) && (j < len)) {
        url[i++] = buf[j++];
    }
    url[i] = '\0';

    //如果是默认的页面，就将url赋值默认路径
    if(strcmp(url, "/favicno.ico")==0){
        char message[] = "/default.html";
        for(int i=0; i<strlen(message); i++){
            url[i] = message[i];
        }
        url[strlen(message)] = '\0';
    }
    printf("url is \n%s\n",url); //看看复制之后的url是什么

    // 处理 GET 方法
    if (strcasecmp(method, "GET") == 0) {  //以忽略大小写的方式比较大小
        // 处理路径  即加上根目录的地址
        snprintf(path, sizeof(path), "/home/xiaosa/web_server_source_material/%s", url);
        //snprintf() 是一个 C 语言标准库函数，用于将格式化的数据输出到指定的字符串缓冲区。
        //   /var/www/html 文件打不开，没办法只能换到这个下面了

        if (path[strlen(path) - 1] == '/') {   //如果以‘/’结尾，说明没有申请访问哪个文件，则给默认地址
            strncat(path, "default.html", sizeof(path) - strlen(path) - 1);
            //strncat() 是一个 C 语言标准库函数，用于将一个字符串的一部分追加到另一个字符串的末尾。
        }

        printf("path is \n%s\n",path); //看看复制之后的url是什么

        // 获取文件信息
        if (stat(path, &st) == -1) {
            //stat()函数是一个 C 语言标准库函数，用于获取文件的统计信息，例如文件大小，文件访问权限，文件所有者等。
            //stat() 函数需要提供文件的路径作为参数，并会将文件的信息以 struct stat 类型返回。
            //文件不存在或没有权限
            while ((len > 0) && (strcmp("\n", buf) != 0)) {   //len是接收到的请求的长度
                len = recv(client_socket, buf, BUFFER_SIZE, 0);
            }
            sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");   //带有覆盖功能的输入字符串
            send(client_socket, buf, strlen(buf), MSG_WAITALL);
            sprintf(buf, SERVER_STRING);
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "Content-Type: text/html\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "your request because the resource specified\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "is unavailable or nonexistent.\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "</BODY></HTML>\r\n");
            send(client_socket, buf, strlen(buf), 0);
            return;
        }

        // 检查文件类型
        if (S_ISDIR(st.st_mode)) {  //S_ISDIR是一个宏，用于检查给定的文件模式（mode）是否表示一个目录
            //snprintf(path, sizeof(path), "/var/www/html%s/index.html", url);
            snprintf(path, sizeof(path), "/home/xiaosa/桌面/vscode_program/webserver%s/index.html", url);
            //snprintf() 是一个 C 语言标准库函数，用于将格式化的数据输出到指定的字符串缓冲区。
            //   /var/www/html 文件打不开，没办法只能换到这个下面了
            if (stat(path, &st) == -1) {
                while ((len > 0) && (strcmp("\n", buf) != 0)) {
                    len = recv(client_socket, buf, BUFFER_SIZE, 0);
                }
                sprintf(buf, "HTTP/1.0 403 FORBIDDEN\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: text/html\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "<HTML><TITLE>Directory Access Forbidden</TITLE>\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "<BODY><P>Access to directory '%s' is forbidden.</P></BODY></HTML>\r\n", url);
                send(client_socket, buf, strlen(buf), 0);
                return;
            }
        }

        // 发送响应头
        filetype = strrchr(path, '.');
        //strrchr用于在一个字符串中查找最后一个匹配给定字符（或者字符的ASCII码值）的位置，并返回该位置的指针。
        if (filetype) {
            if (strcmp(filetype, ".html") == 0) {
                sprintf(buf, "HTTP/1.0 200 OK\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);  
                //SERVER_STRING是一个字符串变量，通常用于在一个HTTP服务器的响应中包含服务器相关信息。
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: text/html\r\n");
                send(client_socket, buf, strlen(buf), 0);
            } else if (strcmp(filetype, ".gif") == 0) {
                sprintf(buf, "HTTP/1.0 200 OK\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: image/gif\r\n");
                send(client_socket, buf, strlen(buf), 0);
            } else if (strcmp(filetype, ".jpg") == 0 || strcmp(filetype, ".jpeg") == 0) {
                sprintf(buf, "HTTP/1.0 200 OK\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: image/jpeg\r\n");
                send(client_socket, buf, strlen(buf), 0);
            } else if (strcmp(filetype, ".png") == 0) {
                sprintf(buf, "HTTP/1.0 200 OK\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: image/png\r\n");
                send(client_socket, buf, strlen(buf), 0);
            } else if (strcmp(filetype, ".css") == 0) {
                sprintf(buf, "HTTP/1.0 200 OK\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: text/css\r\n");
                send(client_socket, buf, strlen(buf), 0);
            } else if (strcmp(filetype, ".js") == 0) {
                sprintf(buf, "HTTP/1.0 200 OK\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: application/x-javascript\r\n");
                send(client_socket, buf, strlen(buf), 0);
            } else {
                sprintf(buf, "HTTP/1.0 403 FORBIDDEN\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, SERVER_STRING);
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "Content-Type: text/html\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "<HTML><HEAD><TITLE>Forbidden</TITLE></HEAD>\r\n");
                send(client_socket, buf, strlen(buf), 0);
                sprintf(buf, "<BODY><P>Access denied.</P></BODY></HTML>\r\n");
                send(client_socket, buf, strlen(buf), 0);
                return;
            }
            sprintf(buf, "\r\n");
            send(client_socket, buf, strlen(buf), 0);
        } else {
            sprintf(buf, "HTTP/1.0 403 FORBIDDEN\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, SERVER_STRING);
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "Content-Type: text/html\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "<HTML><HEAD><TITLE>Forbidden</TITLE></HEAD>\r\n");
            send(client_socket, buf, strlen(buf), 0);
            sprintf(buf, "<BODY><P>Access denied.</P></BODY></HTML>\r\n");
            send(client_socket, buf, strlen(buf), 0);
            return;
        }

        // 发送文件内容
        fd = open(path, O_RDONLY);
        len = 1;
        while (len > 0) {
            len = read(fd, buf, BUFFER_SIZE);
            if (len > 0) {
                send(client_socket, buf, len, 0);
            }
        }
        close(fd);
    }
}