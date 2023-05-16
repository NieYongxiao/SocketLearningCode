#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>

#define BUF_SIZE  1000
#define port 8090

int main(){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t sock_addr_size = sizeof(serv_addr);
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(8090);
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(bind(serv_sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1){
        puts("bind error!");
        exit(0);
    }
    if(listen(serv_sock,5)==-1){
        puts("listen error!");
        exit(0);
    }
    else printf("listen success\n");
    char send_buffer[BUF_SIZE],recv_buffer[BUF_SIZE];
    //for(int i=0;i<5;++i){
    int strlen=0;
    clnt_sock=accept(serv_sock,(struct sockaddr*)&clnt_addr,&sock_addr_size);
    puts("accept success");
    while(true){
        memset(recv_buffer,'\0',BUF_SIZE);
        int strlen = read(clnt_sock,recv_buffer,sizeof(recv_buffer));
        printf("message from client is: %s\n",recv_buffer);
        write(clnt_sock,recv_buffer,sizeof(recv_buffer));
        if(strcmp(recv_buffer,"q")==0 || strlen == 0){
            printf("socket ends!");
            break;
        }
    }
    //}
    close(clnt_sock);
    close(serv_sock);
    return 0;
}