#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/epoll.h>

#define BUF_SIZE  1024   //若设置为4,则可能因为无法一次读完而多次触发epllo_wait
#define port 8090
#define EPOLL_SIZE 50

void error_handler(char *message);

int main(){
    int serv_sock,clnt_sock;
    struct sockaddr_in serv_sock_addr,clnt_sock_addr;
    socklen_t sock_addr_size = sizeof(serv_sock_addr);
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_sock_addr.sin_family=AF_INET;
    serv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_sock_addr.sin_port=htons(port);    
    if(bind(serv_sock, (struct sockaddr *)&serv_sock_addr, sock_addr_size)<0)
    {
        char message[]="bind error!";
        error_handler(message);
    }
    if(listen(serv_sock,5) <0)
    {
        char message[]="listen error!";
        error_handler(message);
    }
    char buf[BUF_SIZE];
    
    struct epoll_event event; //工具人用于注册文件描述符
    struct epoll_event *ep_events;
    int epfd,event_cnt;
    epfd=epoll_create(EPOLL_SIZE); //创建保存epoll文件描述符的空间，成功时返回文件描述符，失败时返回-1
    //手动分配空间，返回分配空间的首地址 此处空间为了保存发生情况的events  
    ep_events= (struct epoll_event*)(malloc(sizeof(struct epoll_event)*EPOLL_SIZE)); 
    //以下三行为了注册serv_sock
    event.data.fd=serv_sock;
    event.events=EPOLLIN; // 设定监测的信息为发生需要读取数据的情况
    epoll_ctl(epfd,EPOLL_CTL_ADD,serv_sock,&event); //向内部注册并注销文件描述符
    
    while(true)
    {
        //等待文件描述符发生变化，成功时返回发生事件的文件描述符数，失败时返回-1
        event_cnt = epoll_wait(epfd,ep_events,EPOLL_SIZE,-1); 
        puts("event_wait once");
        if(event_cnt < 0)
        {
            char message[]="epoll_wait error!";
            error_handler(message);
        }
        for(int i=0;i<event_cnt;i++)
        {
            if(ep_events[i].data.fd==serv_sock)
            {
                clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_sock_addr, &sock_addr_size);
                puts("clnt_sock accepted");
                //向epoll_event 注册 clnt_sock
                event.data.fd=clnt_sock;
                event.events=EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_sock,&event);
            }
            else
            {
                memset(buf,0,sizeof(buf));
                int str_len = read(ep_events[i].data.fd,buf,BUF_SIZE);
                if(str_len == 0)
                {
                    epoll_ctl(epfd,EPOLL_CTL_DEL,ep_events[i].data.fd,NULL);
                    close(ep_events[i].data.fd);
                    printf("closed client: %d\n",ep_events[i].data.fd);
                }
                else
                {
                    puts(buf);
                    write(ep_events[i].data.fd,buf,str_len);
                }
            }
        }
    }
    close(serv_sock);
    close(epfd);
    return 0;
}


void error_handler(char*message){
    puts(message);
    exit(1);
}