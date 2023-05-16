#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>

int main(){
    int read_fd = open("file.txt", O_RDONLY);
    int write_fd = open("copy_file.txt", O_WRONLY);
    char buf[1024];
    while(true){
        int read_bytes= read(read_fd, buf, sizeof(buf));
        if(read_bytes>0){
            write(write_fd, buf, read_bytes);
        }
        else{
            break;
        }
    }
    puts("read complete!");
    close(read_fd);
    close(write_fd);
    return 0;
}