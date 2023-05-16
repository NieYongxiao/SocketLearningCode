#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<signal.h>

#define BUF_SIZE  1000
#define port 8090

void error_handler(char *message);
void signal_for_waitpit(int sig);


int main(){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t sock_addr_size = sizeof(serv_addr);
    memset(&serv_addr, 0, sizeof(serv_addr));
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
    char buffer[BUF_SIZE];

    //
    pid_t pid;
    struct sigaction act;
    act.sa_handler=signal_for_waitpit;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD, &act, 0); //SIGCHLD 子进程终止  SIGINT 输入ctrl+c  SIGALRM 已到调用alarm函数注册的时间

    while(true){
        puts("wait for accept!!");
        clnt_sock = accept(serv_sock,(struct sockaddr *)&clnt_addr,&sock_addr_size);
        if(clnt_sock==-1){
            puts("accept error");
            continue;   //当某子进程结束之后，为什么accept失败一次呢，明明没有企图connect的
        }
        else printf("clnt_sock %d accepted \n",clnt_sock);
        pid = fork();
        if(pid ==-1){
            close(clnt_sock);
            continue;
        }
        else if(pid==0){
            close(serv_sock);
            while(true){
                memset(buffer,0,sizeof(buffer));
                int str_len=read(clnt_sock,buffer,sizeof(buffer));
                if(str_len==0 || strcmp(buffer,"q")==0){
                    break;
                }
                printf("message from clnt_sock is %s \n",buffer);
                write(clnt_sock,buffer,str_len);
            }
            close(clnt_sock);
            printf("process:%d clnt_sock:%d  disconnected \n",getpid(),clnt_sock);
            return 0;
        }
        else{
           close(clnt_sock);
        }
    }
    close(serv_sock);
    return 0;
}


void signal_for_waitpit(int sig){
    int status;
    pid_t pid = waitpid(-1,&status,WNOHANG);  //相比wait不会引起阻塞
    printf("removed proc id is %d\n",pid);
}
void error_handler(char *message)
{
    puts(message);
    exit(1);
}