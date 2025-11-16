/***
 * @file tcp_server.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief TCP服务器模块（与上位机通信）
 * 
 * @version 0.2
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath tcp_server.c
 * @projectType Embedded
 */

#include "tcp_server.h"
#include "esp32_main.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include <string.h>

static const char *TAG = "TCP_SERVER";

// 客户端连接管理
typedef struct {
    int socket;
    bool active;
    struct sockaddr_in addr;
} client_info_t;

static int server_socket = -1;
static client_info_t clients[MAX_CONNECTIONS];
static tcp_data_callback_t g_data_callback = NULL;
static bool server_running = false;

/****************************************************************************
 * @brief 查找空闲客户端槽位
 * @return 客户端索引，-1表示无空闲槽位
 */
static int find_free_client_slot(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i].active) {
            return i;
        }
    }
    return -1;
}

/****************************************************************************
 * @brief 客户端处理任务
 * @param pvParameters 客户端索引指针
 */
static void client_handler_task(void *pvParameters)
{
    int client_idx = *(int *)pvParameters;
    free(pvParameters);
    
    client_info_t *client = &clients[client_idx];
    uint8_t rx_buffer[AUDIO_BUFFER_SIZE];
    
    ESP_LOGI(TAG, "Client handler started for socket %d", client->socket);
    ESP_LOGI(TAG, "Free heap: %lu bytes, Stack high water mark: %u bytes", 
             esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL) * 4);
    
    while (client->active) {
        // ==================== 接收数据 ====================
        int len = recv(client->socket, rx_buffer, sizeof(rx_buffer), 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Receive error on socket %d", client->socket);
            break;
        } else if (len == 0) {
            ESP_LOGI(TAG, "Client disconnected (socket %d)", client->socket);
            break;
        } else {
            ESP_LOGI(TAG, "Received %d bytes from socket %d, cmd=0x%02X", 
                     len, client->socket, rx_buffer[0]);
            ESP_LOGD(TAG, "Free heap before callback: %lu bytes", esp_get_free_heap_size());
            
            // ==================== 调用回调处理数据 ====================
            if (g_data_callback != NULL) {
                g_data_callback(rx_buffer, len, client->socket);
            }
            
            ESP_LOGD(TAG, "Free heap after callback: %lu bytes, Stack: %u bytes", 
                     esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL) * 4);
        }
    }
    
    // ==================== 清理客户端连接 ====================
    close(client->socket);
    client->active = false;
    client->socket = -1;
    
    ESP_LOGI(TAG, "Client handler terminated for socket index %d", client_idx);
    vTaskDelete(NULL);
}

/****************************************************************************
 * @brief TCP服务器任务
 * @param pvParameters 任务参数
 */
static void tcp_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    // ==================== 初始化客户端数组 ====================
    memset(clients, 0, sizeof(clients));
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        clients[i].socket = -1;
        clients[i].active = false;
    }
    
    // ==================== 创建套接字 ====================
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // ==================== 绑定地址 ====================
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_SERVER_PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE(TAG, "Socket bind failed");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }
    
    // ==================== 监听 ====================
    if (listen(server_socket, MAX_CONNECTIONS) != 0) {
        ESP_LOGE(TAG, "Socket listen failed");
        close(server_socket);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "TCP server started on port %d", TCP_SERVER_PORT);
    server_running = true;
    
    while (server_running) {
        // ==================== 接受连接 ====================
        int client_sock = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (server_running) {
                ESP_LOGE(TAG, "Accept failed");
            }
            continue;
        }
        
        ESP_LOGI(TAG, "New client connected from %s:%d", 
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // ==================== 查找空闲槽位 ====================
        int slot = find_free_client_slot();
        if (slot < 0) {
            ESP_LOGW(TAG, "Maximum connections reached, rejecting client");
            close(client_sock);
            continue;
        }
        
        // ==================== 初始化客户端信息 ====================
        clients[slot].socket = client_sock;
        clients[slot].active = true;
        clients[slot].addr = client_addr;
        
        // ==================== 创建客户端处理任务 ====================
        int *slot_ptr = malloc(sizeof(int));
        *slot_ptr = slot;
        char task_name[32];
        snprintf(task_name, sizeof(task_name), "tcp_client_%d", slot);
        // 增加栈大小到8192，防止栈溢出
        if (xTaskCreate(client_handler_task, task_name, 8192, slot_ptr, 5, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create client handler task");
            close(client_sock);
            clients[slot].active = false;
            clients[slot].socket = -1;
            free(slot_ptr);
        }
    }
    
    // ==================== 清理所有连接 ====================
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clients[i].active) {
            close(clients[i].socket);
            clients[i].active = false;
            clients[i].socket = -1;
        }
    }
    
    close(server_socket);
    server_socket = -1;
    ESP_LOGI(TAG, "TCP server stopped");
    vTaskDelete(NULL);
}

/****************************************************************************
 * @brief 启动TCP服务器
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t tcp_server_start(void)
{
    if (server_running) {
        ESP_LOGW(TAG, "TCP server already running");
        return ESP_OK;
    }
    
    if (xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create TCP server task");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

/****************************************************************************
 * @brief 停止TCP服务器
 */
void tcp_server_stop(void)
{
    server_running = false;
    
    // ==================== 关闭所有客户端连接 ====================
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clients[i].active && clients[i].socket >= 0) {
            shutdown(clients[i].socket, SHUT_RDWR);
            close(clients[i].socket);
            clients[i].socket = -1;
            clients[i].active = false;
        }
    }
    
    // ==================== 关闭服务器套接字 ====================
    if (server_socket >= 0) {
        shutdown(server_socket, SHUT_RDWR);
        close(server_socket);
        server_socket = -1;
    }
    
    ESP_LOGI(TAG, "TCP server stopped");
}

/****************************************************************************
 * @brief 注册TCP数据回调函数
 * @param callback 回调函数指针
 */
void tcp_server_register_callback(tcp_data_callback_t callback)
{
    g_data_callback = callback;
}

/****************************************************************************
 * @brief 发送数据到指定客户端
 * @param data 数据指针
 * @param len 数据长度
 * @param socket 客户端套接字（-1表示发送到所有客户端）
 * @return 实际发送的字节数
 */
int tcp_server_send(const uint8_t *data, size_t len, int socket)
{
    int total_sent = 0;
    
    if (socket < 0) {
        // ==================== 广播到所有客户端 ====================
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (clients[i].active && clients[i].socket >= 0) {
                int ret = send(clients[i].socket, data, len, 0);
                if (ret > 0) {
                    total_sent += ret;
                } else {
                    ESP_LOGW(TAG, "Send failed to socket %d", clients[i].socket);
                }
            }
        }
        
        if (total_sent == 0) {
            ESP_LOGW(TAG, "No active clients to send data");
            return -1;
        }
    } else {
        // ==================== 发送到指定客户端 ====================
        int ret = send(socket, data, len, 0);
        if (ret < 0) {
            ESP_LOGE(TAG, "Send failed to socket %d", socket);
        }
        return ret;
    }
    
    return total_sent;
}
