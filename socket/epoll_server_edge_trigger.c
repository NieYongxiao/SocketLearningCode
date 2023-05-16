#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<errno.h>  //引入全局变量errno
#include<fcntl.h>  //引入fcntl函数

//边缘触发方式必须用非阻塞read write ，否则可能引起服务器端的长时间停顿


#define BUF_SIZE  4  //设置4以验证边缘触发
#define port 8090
#define EPOLL_SIZE 50

void set_non_block_mode(int fd);
void error_handler(char *message);


int main()
{
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

    set_non_block_mode(serv_sock);
    //以下三行为了注册serv_sock
    event.data.fd=serv_sock;
    event.events=EPOLLIN; // 设定监测的信息为发生需要读取数据的情况
    epoll_ctl(epfd,EPOLL_CTL_ADD,serv_sock,&event); //向内部注册并注销文件描述符
    
    while(1)
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

                set_non_block_mode(clnt_sock);
                //向epoll_event 注册 clnt_sock
                event.data.fd=clnt_sock;
                event.events=EPOLLIN|EPOLLET; // 将触发方式设置为边缘处罚
                epoll_ctl(epfd,EPOLL_CTL_ADD,clnt_sock,&event);
            }
            else
            {
                memset(buf,0,sizeof(buf));
                while(1)  //为保证完全读取完全 需循环read
                {
                    int str_len = read(ep_events[i].data.fd,buf,BUF_SIZE);
                    if(str_len == 0)
                    {
                        epoll_ctl(epfd,EPOLL_CTL_DEL,ep_events[i].data.fd,NULL);
                        close(ep_events[i].data.fd);
                        printf("closed client: %d\n",ep_events[i].data.fd);
                        break;
                    }
                    else if(str_len <0)
                    {
                        if(errno==EAGAIN) break;   //当两个条件都满足时，说明读取了输入缓冲的全部数据
                    }
                    else
                    {
                        puts(buf);
                        write(ep_events[i].data.fd,buf,str_len);
                    }
                }
            }
        }
    }
    close(serv_sock);
    close(epfd);
    return 0;
}

void set_non_block_mode(int fd)
{
    int flag = fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag | O_NONBLOCK);
}
void error_handler(char*message)
{
    puts(message);
    exit(1);
}