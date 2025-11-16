/***
 * @file uart_handler.h
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-02
 * @brief UART通信处理模块头文件
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-02
 * @filePath uart_handler.h
 * @projectType Embedded
 */

#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

// UART命令回调函数类型
// 参数说明：cmd_with_temp的低8位为命令，高16位为温度值（单位0.1°C）
typedef void (*uart_command_callback_t)(uint32_t cmd_with_temp);

/***
 * @brief 初始化UART
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t uart_handler_init(void);

/***
 * @brief 注册UART命令回调函数
 * @param callback 回调函数指针
 */
void uart_handler_register_callback(uart_command_callback_t callback);

/***
 * @brief 发送数据到UART
 * @param data 数据指针
 * @param len 数据长度
 * @return 实际发送的字节数
 */
int uart_handler_send(const uint8_t *data, size_t len);

/***
 * @brief 暂停UART数据处理（音频播放期间）
 */
void uart_handler_pause(void);

/***
 * @brief 恢复UART数据处理并清理缓冲区
 */
void uart_handler_resume(void);

#endif // UART_HANDLER_H

