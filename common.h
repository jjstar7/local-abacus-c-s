#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <matheval.h>       //gnu math lib

#define SOCKET_PATH "/tmp/abacus.sock"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define HISTORY_SIZE 5
#define EXPRESSION_SIZE 256

// 请求类型枚举
typedef enum {
    REQ_CALCULATE = 1,
    REQ_HISTORY = 2
} request_type_t;

// 响应状态枚举
typedef enum {
    RESP_SUCCESS = 0,
    RESP_ERROR_INVALID_EXPRESSION = 1,
    RESP_ERROR_CALCULATION = 2,
    RESP_ERROR_UNKNOWN = 3
} response_status_t;

// 请求结构
typedef struct {
    request_type_t type;
    char data[512];
} request_t;

// 响应结构
typedef struct {
    response_status_t status;
    char message[1024];
} response_t;

// 计算历史记录结构
typedef struct {
    char expression[EXPRESSION_SIZE];
    double result;
    time_t timestamp;
} calculation_record_t;

// 服务端状态结构
typedef struct {
    calculation_record_t history[HISTORY_SIZE];
    int history_count;
    int history_head;
    int server_socket;  //服务端套接字
} abacus_server_t;

#endif
