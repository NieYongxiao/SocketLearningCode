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
void read_handle(int sock,char *buf,int *pipes,int *file_pipes);
void write_handle(int sock,char *buf,int *pipes);

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

    //pipe进程通信
    //1）如果管道中没数据，则read会阻塞，直到读到数据（超市中买方便面没买到，会一直等）
    //2）管道中若数据满了，则write会阻塞，直到有空闲空间
    //3）若所有读端被关闭，则write会触发异常-----SIGPIPE（导致进程退出），再写的话就是浪费资源，浪费资源就是犯罪，我就要让你崩溃。
    //4）若所有写端被关闭，则read读完之后不会阻塞，而是返回0（告诉用户，你不要在读了，没人写了）
    //因此可以使用管道进行群聊天，保存文件等等
    int pipes[2],file_pipes[2];
    pipe(pipes);
    pipe(file_pipes);
    
    pid=fork();
    if(pid==0){
        FILE* file = fopen("record_message.txt","wt");
        char message[BUF_SIZE];
        while(true){
            int len = read(file_pipes[0],message,BUF_SIZE);
            fwrite(message,1,len,file);
            if(strcmp(message,"q")==0){
                puts("file_pipes ends!");
                break;
            }
        }
        fclose(file);
        return 0;
    }

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
            read_handle(clnt_sock,read_buf,pipes,file_pipes);
        }
        else{
           write_handle(clnt_sock,write_buf,pipes);
        }
    }
    close(clnt_sock);
    close(serv_sock);
    close(pipes[0]);
    close(pipes[1]);
    return 0;
}


void read_handle(int sock,char *buf,int *pipes,int *file_pipes){
    while(true){
        memset(buf,0,sizeof(buf));
        int str_len = read(sock,buf,sizeof(buf));
        close(pipes[0]);
        close(file_pipes[0]);
        write(pipes[1],buf,sizeof(buf));
        write(file_pipes[1],buf,sizeof(buf));
        if(str_len==0 || strcmp(buf,"q")==0){
            close(sock);
            printf("process:%d clnt_sock:%d  disconnected \n",getpid(),sock);
            break;
        }
        printf("process:%d   message from clnt_sock %d  is %s\n",getpid(),sock,buf);
        //write(sock, buf,strlen(buf)); 因为没有写客户端的代码，所以要写回去，不然会阻塞
    }
}
void write_handle(int sock,char *buf,int *pipes){
    while(true){
        close(pipes[1]);
        read(pipes[0],buf,sizeof(buf));   //read后的管道不会把所有值清空，所以要手动清空
        //memset((void*)pipes[0],0,sizeof(pipes[0]));
        printf("process:%d   message from pipes is %s\n",getpid(),buf);
        write(sock, buf,strlen(buf));  //sizeof是1024, strlen是实际长度
        if(strcmp(buf,"q")==0){
            close(sock);
            printf("process:%d clnt_sock:%d  disconnected \n",getpid(),sock);
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