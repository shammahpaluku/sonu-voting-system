#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "config.h"
#include "net_handler.h"

int nh_server_init(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("socket() failed\n");
        return ERR_CONN;
    }
    
    // Set SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("setsockopt() failed\n");
        nh_close(server_fd);
        return ERR_CONN;
    }
    
    // Build sockaddr_in
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("bind() failed\n");
        nh_close(server_fd);
        return ERR_CONN;
    }
    
    // Listen
    if (listen(server_fd, BACKLOG) < 0) {
        printf("listen() failed\n");
        nh_close(server_fd);
        return ERR_CONN;
    }
    
    printf("SONU Server listening on %s:%d\n", SERVER_IP, port);
    return server_fd;
}

int nh_server_accept(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        return ERR_CONN;
    }
    
    printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
    return client_fd;
}

int nh_client_connect(const char *ip, int port) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        return ERR_CONN;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Could not connect to server at %s:%d\n", ip, port);
        nh_close(client_fd);
        return ERR_CONN;
    }
    
    return client_fd;
}

int nh_send_line(int fd, const char *msg) {
    char buf[CMD_BUF_LEN];
    snprintf(buf, CMD_BUF_LEN, "%s\n", msg);
    
    int bytes_sent = send(fd, buf, strlen(buf), 0);
    if (bytes_sent <= 0) {
        return ERR_CONN;
    }
    
    return SUCCESS;
}

int nh_recv_line(int fd, char *buf, int buf_len) {
    int bytes_read = 0;
    char ch;
    
    while (bytes_read < buf_len - 1) {
        int result = recv(fd, &ch, 1, 0);
        
        if (result == 0) {
            return ERR_CONN;  // Connection closed
        }
        
        if (result < 0) {
            return ERR_CONN;  // Error
        }
        
        if (ch == '\n') {
            break;
        }
        
        buf[bytes_read] = ch;
        bytes_read++;
    }
    
    buf[bytes_read] = '\0';
    return SUCCESS;
}

void nh_close(int fd) {
    close(fd);
}
