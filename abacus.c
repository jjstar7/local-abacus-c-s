#include "common.h"

abacus_server_t server_state = {0};
volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
}

void add_to_history(const char* expression, double result) {
    calculation_record_t* record = &server_state.history[server_state.history_head];
    
    strncpy(record->expression, expression, EXPRESSION_SIZE - 1);
    record->expression[EXPRESSION_SIZE - 1] = '\0';
    record->result = result;
    record->timestamp = time(NULL);
    
    server_state.history_head = (server_state.history_head + 1) % HISTORY_SIZE;
    if (server_state.history_count < HISTORY_SIZE) {
        server_state.history_count++;
    }
}

response_t handle_calculate_request(const char* expression) {
    response_t response = {0};
    void* evaluator = NULL;
    double result;
    
    // 创建数学表达式评估器
    evaluator = evaluator_create((char*)expression);
    if (!evaluator) {
        response.status = RESP_ERROR_INVALID_EXPRESSION;
        snprintf(response.message, sizeof(response.message), 
                "Invalid expression: %s", expression);
        return response;
    }
    
    // 计算表达式
    result = evaluator_evaluate_x(evaluator, 0);
    
    // 检查计算错误
    if (errno == EDOM || errno == ERANGE) {
        response.status = RESP_ERROR_CALCULATION;
        snprintf(response.message, sizeof(response.message), 
                "Calculation error: %s", expression);
        evaluator_destroy(evaluator);
        return response;
    }
    
    // 成功计算，添加到历史记录
    add_to_history(expression, result);
    
    response.status = RESP_SUCCESS;
    snprintf(response.message, sizeof(response.message), "%.6g", result);
    
    evaluator_destroy(evaluator);
    return response;
}

response_t handle_history_request() {
    response_t response = {0};
    char temp[256];
    
    response.status = RESP_SUCCESS;
    
    if (server_state.history_count == 0) {
        strcpy(response.message, "No calculation history available.");
        return response;
    }
    
    strcpy(response.message, "Recent calculations:\n");
    
    // 按时间顺序输出历史记录
    int start = server_state.history_count < HISTORY_SIZE ? 
                0 : server_state.history_head;
    
    for (int i = 0; i < server_state.history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        calculation_record_t* record = &server_state.history[idx];
        
        snprintf(temp, sizeof(temp), "%d. %s = %.6g\n", 
                i + 1, record->expression, record->result);
        strncat(response.message, temp, 
                sizeof(response.message) - strlen(response.message) - 1);
    }
    
    return response;
}

void handle_client_request(int client_fd) {
    request_t request;
    response_t response;
    ssize_t bytes_received, bytes_sent;
    
    // 接收请求
    bytes_received = recv(client_fd, &request, sizeof(request), 0);
    if (bytes_received <= 0) {
        close(client_fd);
        return;
    }
    
    // 处理请求
    switch (request.type) {
        case REQ_CALCULATE:
            response = handle_calculate_request(request.data);
            break;
        case REQ_HISTORY:
            response = handle_history_request();
            break;
        default:
            response.status = RESP_ERROR_UNKNOWN;
            strcpy(response.message, "Unknown request type");
            break;
    }
    
    // 发送响应
    bytes_sent = send(client_fd, &response, sizeof(response), 0);
    if (bytes_sent <= 0) {
        perror("send");
    }
    
    close(client_fd);
}

int initialize_server() {
    struct sockaddr_un server_addr;
    
    // 创建套接字
    server_state.server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_state.server_socket == -1) {
        perror("socket");
        return -1;
    }
    
    // 删除可能存在的旧套接字文件
    unlink(SOCKET_PATH);
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // 绑定套接字与服务器地址
    if (bind(server_state.server_socket, (struct sockaddr*)&server_addr, 
             sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_state.server_socket);
        return -1;
    }
    
    // 监听连接
    if (listen(server_state.server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_state.server_socket);
        unlink(SOCKET_PATH);
        return -1;
    }
    
    printf("Abacus server started, listening on %s\n", SOCKET_PATH);
    return 0;
}

void cleanup_server() {
    if (server_state.server_socket != -1) {
        close(server_state.server_socket);
    }
    unlink(SOCKET_PATH);
    printf("\nAbacus server stopped.\n");
}

int main() {
    int client_fd;
    fd_set read_fds;
    struct timeval timeout;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化服务器
    if (initialize_server() == -1) {
        return 1;
    }
    
    // 主循环
    while (running) {   //信号通过running终止主循环
        FD_ZERO(&read_fds);
        FD_SET(server_state.server_socket, &read_fds);
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(server_state.server_socket + 1, &read_fds, 
                             NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }
        
        if (activity > 0 && FD_ISSET(server_state.server_socket, &read_fds)) {
            // 接受新连接
            client_fd = accept(server_state.server_socket, NULL, NULL);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            
            // 处理客户端请求
            handle_client_request(client_fd);
        }
    }
    
    cleanup_server();
    return 0;
}
