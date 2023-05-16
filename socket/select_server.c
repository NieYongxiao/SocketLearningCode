#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>

#define BUF_SIZE  1024
#define port 8090

void error_handler(char *message);

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
        char message[]="bind error!";
        error_handler(message);
    }
    if(listen(serv_sock, 5) <0){
        char message[]="listen error!";
        error_handler(message);
    }

    fd_set reads,reads_copy;
    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    struct timeval timeout;
    int fd_max=serv_sock;
    char buf[BUF_SIZE];

    while(true){
        reads_copy = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        puts("select once!");
        int fd_num = select(fd_max+1,&reads_copy,NULL,NULL,&timeout);
        if(fd_num< 0){
            char message[]="select error!";
            error_handler(message);
        }
        else if(fd_num==0){
            puts("TIME OUT!");
            continue;
        }
        for(int i=0;i<fd_max+1;i++){
            if(FD_ISSET(i,&reads_copy)){
                if(i==serv_sock)
                {
                    clnt_sock=accept(i,(struct sockaddr*)&clnt_sock_addr,&sock_addr_len);
                    FD_SET(clnt_sock,&reads);
                    if(fd_max < clnt_sock) fd_max = clnt_sock;
                    printf("clnt_sock connected %d\n",clnt_sock);
                }
                else
                {
                    memset(buf,0,sizeof(buf));
                    int strlen = read(i,buf,sizeof(buf));
                    if(strlen ==0){
                        FD_CLR(i,&reads);
                        close(i);
                        puts("closed client!");
                    }
                    else{
                        puts(buf);
                        write(i,buf,strlen);
                    }
                }
            }
        }
    }
    close(serv_sock);
    return 0;
}


void error_handler(char*message){
    puts(message);
    exit(1);
}