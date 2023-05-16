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

int main(){
    char absolute_path[1024]="/home/xiaosa/web_server_source_material";
    printf("%d",strlen(absolute_path));
    return 0;
}