#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8090
#define BUFFER_SIZE 1024

void send_response_header(int client_fd, int content_length) {
    char response[BUFFER_SIZE] = "";
    strcat(response, "HTTP/1.1 200 OK\r\n");
    strcat(response, "Content-Type: image/gif\r\n");
    strcat(response, "Content-Length: ");
    char content_length_str[10] = "";
    sprintf(content_length_str, "%d", content_length);
    strcat(response, content_length_str);
    strcat(response, "\r\n\r\n");
    send(client_fd, response, strlen(response), 0);
}

int main() {
    // 创建socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 设置socket选项
    int opt_val = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt_val, sizeof(opt_val))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 绑定socket
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听socket
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    } else {
        printf("Server listening on port %d\n", PORT);
    }

    // 创建客户端socket
    int client_fd, bytes_read, total_bytes_read = 0;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_address;
    socklen_t address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *) &client_address, &address_len);
    if (client_fd < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    // 发送响应头
    send_response_header(client_fd, total_bytes_read);

    // 打开文件并发送文件内容
    FILE *fp;
    fp = fopen("monster.gif", "rb");
    if (fp == NULL) {
        perror("failed to open file");
        exit(EXIT_FAILURE);
    }

    // 读取文件内容并发送
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        total_bytes_read += bytes_read;
        if (send(client_fd, buffer, bytes_read, 0) != bytes_read) {
            perror("failed to send file");
            exit(EXIT_FAILURE);
        }
        memset(buffer, 0, BUFFER_SIZE);
    }


    // 关闭socket和文件
    shutdown(client_fd, SHUT_WR);
    fclose(fp);
    close(client_fd);
    close(server_fd);
    return 0;
}