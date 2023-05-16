#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<signal.h>

//有个小bug，无法实现发送接收到q均结束两个进程，可以使用管道，但是否太麻烦了一点
//或许不用考虑这点，因为这是客户端，一般来说服务器不会主动要求断开进程，都是客户端主动断开
//而主进程结束之后子进程也跟着结束了
//在Linux系统中，当一个进程终止时，它的子进程会被终止掉，除非它们已经脱离了该进程组（detached）或者被分配给了一个不同的进程组。
//如果子进程仍然在运行，它会被称为孤儿进程（orphan process），它们会成为init进程（PID为1）的子进程，
//init 进程会定期清理并回收孤儿进程的资源。
//因此，在Linux系统中，如果一个进程的终止方式是正常的，即使用 exit() 或 _exit() 系统调用停止，子进程会被自动终止。


#define BUF_SIZE  1000
#define port 8090

void error_handler(char *message);
void signal_for_waitpit(int sig);
void read_handle(int sock,char *buf);
void write_handle(int sock,char *buf);

int main(){
    int clnt_sock;
    struct sockaddr_in clnt_addr;
    socklen_t sock_addr_size = sizeof(struct sockaddr_in);
    clnt_sock = socket(AF_INET, SOCK_STREAM, 0);
    clnt_addr.sin_family = AF_INET;
    clnt_addr.sin_port = htons(port);
    clnt_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    struct sigaction act;
    act.sa_handler=signal_for_waitpit;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD, &act, 0); //SIGCHLD 子进程终止  SIGINT 输入ctrl+c  SIGALRM 已到调用alarm函数注册的时间

    
    
    if(connect(clnt_sock, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr)) < 0){
        char message[]="connect failed";
        error_handler(message);
    }

    pid_t pid = fork();
    char send_message[BUF_SIZE],write_message[BUF_SIZE];
    if(pid==0){
        read_handle(clnt_sock, send_message);
    }
    else{
        write_handle(clnt_sock, write_message); //主进程负责结束
    }
    close(clnt_sock);
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
        //fgets(buf,BUF_SIZE,stdin);
        write(sock, buf,strlen(buf));  //sizeof是1024, strlen是实际长度
        if(strcmp(buf,"q")==0){
            printf("write: process:%d clnt_sock:%d  disconnected \n",getpid(),sock);
            break;
        }
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