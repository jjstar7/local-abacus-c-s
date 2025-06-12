#include "common.h"

int connect_to_server() {
    int sock_fd;
    struct sockaddr_un server_addr;
    
    // 创建套接字
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return -1;
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // 连接服务器
    if (connect(sock_fd, (struct sockaddr*)&server_addr, 
                sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock_fd);
        return -1;
    }
    
    return sock_fd;
}

int send_calculate_request(const char* expression) {
    int sock_fd = connect_to_server();
    if (sock_fd == -1) {
        fprintf(stderr, "Failed to connect to abacus server. "
                       "Is the server running?\n");
        return 1;
    }
    
    request_t request = {0};
    response_t response = {0};
    
    request.type = REQ_CALCULATE;
    strncpy(request.data, expression, sizeof(request.data) - 1);
    
    // 发送请求
    if (send(sock_fd, &request, sizeof(request), 0) == -1) {
        perror("send");
        close(sock_fd);
        return 1;
    }
    
    // 接收响应
    if (recv(sock_fd, &response, sizeof(response), 0) == -1) {
        perror("recv");
        close(sock_fd);
        return 1;
    }
    
    close(sock_fd);
    
    // 显示结果
    if (response.status == RESP_SUCCESS) {
        printf("Result: %s\n", response.message);
        return 0;
    } else {
        fprintf(stderr, "Error: %s\n", response.message);
        return 1;
    }
}

int send_history_request() {
    int sock_fd = connect_to_server();
    if (sock_fd == -1) {
        fprintf(stderr, "Failed to connect to abacus server. "
                       "Is the server running?\n");
        return 1;
    }
    
    request_t request = {0};
    response_t response = {0};
    
    request.type = REQ_HISTORY;
    
    // 发送请求
    if (send(sock_fd, &request, sizeof(request), 0) == -1) {
        perror("send");
        close(sock_fd);
        return 1;
    }
    
    // 接收响应
    if (recv(sock_fd, &response, sizeof(response), 0) == -1) {
        perror("recv");
        close(sock_fd);
        return 1;
    }
    
    close(sock_fd);
    
    // 显示结果
    printf("%s", response.message);
    return 0;
}

void print_usage(const char* program_name) {
    printf("Usage:\n");
    printf("  %s cal <expression>  - Calculate mathematical expression\n", 
           program_name);
    printf("  %s his              - Show calculation history\n", 
           program_name);
    printf("\nExamples:\n");
    printf("  %s cal \"2+3*4\"\n", program_name);
    printf("  %s cal \"sin(3.14159/2)\"\n", program_name);
    printf("  %s his\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "cal") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: 'cal' command requires an expression\n");
            print_usage(argv[0]);
            return 1;
        }
        return send_calculate_request(argv[2]);
    } else if (strcmp(argv[1], "his") == 0) {
        return send_history_request();
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}