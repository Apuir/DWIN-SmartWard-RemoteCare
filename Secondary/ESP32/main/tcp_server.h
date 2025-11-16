/***
 * @file tcp_server.h
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-02
 * @brief TCP服务器模块头文件
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-02
 * @filePath tcp_server.h
 * @projectType Embedded
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

// TCP数据回调函数类型
typedef void (*tcp_data_callback_t)(const uint8_t *data, size_t len, int socket);

/***
 * @brief 启动TCP服务器
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t tcp_server_start(void);

/***
 * @brief 停止TCP服务器
 */
void tcp_server_stop(void);

/***
 * @brief 注册TCP数据回调函数
 * @param callback 回调函数指针
 */
void tcp_server_register_callback(tcp_data_callback_t callback);

/***
 * @brief 发送数据到指定客户端
 * @param data 数据指针
 * @param len 数据长度
 * @param socket 客户端套接字（-1表示广播到所有客户端）
 * @return 实际发送的字节数
 */
int tcp_server_send(const uint8_t *data, size_t len, int socket);

#endif // TCP_SERVER_H

