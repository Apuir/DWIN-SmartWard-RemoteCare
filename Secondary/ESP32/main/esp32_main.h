/***
 * @file esp32_main.h
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief ESP32主控头文件（温度监控+音频播放）
 * 
 * @version 0.2
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath esp32_main.h
 * @projectType Embedded
 */

#ifndef ESP32_MAIN_H
#define ESP32_MAIN_H

#include <stdint.h>
#include <stdbool.h>

// ==================== 通信协议定义 ====================
// T5L→ESP32命令（UART接收）
#define CMD_TEMP_THRESHOLD1     0xE1   // 温度达到阈值1（例如28℃）
#define CMD_TEMP_THRESHOLD2     0xE2   // 温度达到阈值2（例如35℃）
#define CMD_TEMP_NORMAL         0xE0   // 温度恢复正常
#define CMD_TEMP_UPDATE         0xE3   // 温度定期更新（用于显示）

// 上位机→ESP32命令（TCP接收）
#define CMD_PLAY_AUDIO          0xA1   // 播放音频数据
#define CMD_STOP_AUDIO          0xA0   // 停止音频播放
#define CMD_QUERY_STATUS        0xA2   // 查询ESP32状态
#define CMD_AUDIO_STREAM_START  0xA3   // 开始音频流传输
#define CMD_AUDIO_STREAM_DATA   0xA4   // 音频流数据包
#define CMD_AUDIO_STREAM_END    0xA5   // 结束音频流传输
#define CMD_DEVICE_DISCOVERY    0xA6   // 设备发现请求

// ESP32→上位机响应（TCP发送）
#define RESP_THRESHOLD1_REACHED 0xD1   // 阈值1到达通知
#define RESP_THRESHOLD2_REACHED 0xD2   // 阈值2到达通知
#define RESP_TEMP_NORMAL        0xD0   // 温度恢复正常通知
#define RESP_TEMP_UPDATE        0xD3   // 温度定期更新
#define RESP_STATUS_OK          0xD4   // 状态正常
#define RESP_AUDIO_ACK          0xD5   // 音频接收确认
#define RESP_DEVICE_INFO        0xD6   // 设备信息响应
#define RESP_ERROR              0xDF   // 错误响应

// ==================== 网络配置 ====================
#define TCP_SERVER_PORT         8080   // TCP服务器端口
#define MAX_CONNECTIONS         5      // 最大连接数
#define MDNS_HOSTNAME           "esp32-temp-monitor"  // mDNS主机名
#define MDNS_INSTANCE           "ESP32 Temperature Monitor"  // mDNS实例名

// ==================== UART配置 ====================
#define UART_NUM                UART_NUM_1
#define UART_TX_PIN             GPIO_NUM_9
#define UART_RX_PIN             GPIO_NUM_10
#define UART_BAUD_RATE          115200
#define UART_BUF_SIZE           1024

// ==================== 音频配置 ====================
#define AUDIO_BUFFER_SIZE       4096   // 音频缓冲区大小
#define AUDIO_STREAM_TIMEOUT_MS 5000   // 音频流超时（毫秒）

#endif // ESP32_MAIN_H

