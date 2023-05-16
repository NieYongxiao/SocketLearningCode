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
    int clnt_sock;
    struct sockaddr_in clnt_addr;
    socklen_t sock_addr_size = sizeof(clnt_addr);
    clnt_addr.sin_family=AF_INET;
    clnt_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    clnt_addr.sin_port=htons(8090);
    clnt_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(connect(clnt_sock, (struct sockaddr *)&clnt_addr,sizeof(clnt_addr))==-1){
        puts("connect error!");
        exit(1);
    }
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    printf("Enter your words: ");
    scanf("%s",buf);
    write(clnt_sock, buf,sizeof(buf));
    memset(buf, 0, BUF_SIZE);
    printf("message in buf is : %s \n",buf);
    read(clnt_sock, buf,sizeof(buf));
    printf("message recvived from server is : %s \n",buf);
    
    close(clnt_sock);
    return 0;
}