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
void read_handle(int sock,char *buf);
void write_handle(int sock,char *buf);

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
    char write_buf[BUF_SIZE],read_buf[BUF_SIZE];

    //设定信号处理，当子进程结束的时候调用waitpid销毁
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
            read_handle(clnt_sock,read_buf);
            close(clnt_sock);
        }
        else{
            write_handle(clnt_sock,write_buf);
            close(clnt_sock);
        }
    }
    close(serv_sock);
    return 0;
}


void read_handle(int sock,char *buf){
    while(true){
        memset(buf,0,sizeof(buf));
        int str_len = read(sock,buf,sizeof(buf));
        if(str_len==0 || strcmp(buf,"q")==0){
            printf("read:  process:%d clnt_sock:%d  disconnected \n",getpid(),sock);
            break;
        }
        printf("message from clnt_sock is %s\n",buf);
    }
}
void write_handle(int sock,char *buf){
    while(true){
        memset(buf,0,sizeof(buf));
        scanf("%s",buf);
        if(strcmp(buf,"q")==0){
            printf("write: process:%d clnt_sock:%d  disconnected \n",getpid(),sock);
            break;
        }
        //fgets(buf,BUF_SIZE,stdin);
        write(sock, buf,strlen(buf));  //sizeof是1024, strlen是实际长度
    }
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