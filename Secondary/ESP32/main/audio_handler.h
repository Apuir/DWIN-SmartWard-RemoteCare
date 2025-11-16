/***
 * @file audio_handler.h
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 音频流处理模块头文件
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath audio_handler.h
 * @projectType Embedded
 */

#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/***
 * @brief 初始化音频处理模块
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_handler_init(void);

/***
 * @brief 开始音频流接收
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stream_start(void);

/***
 * @brief 接收音频流数据包
 * @param data 音频数据指针
 * @param len 数据长度
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stream_feed(const uint8_t *data, size_t len);

/***
 * @brief 结束音频流接收
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stream_end(void);

/***
 * @brief 停止音频播放
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stop(void);

/***
 * @brief 获取音频流状态
 * @return true - 正在播放，false - 未播放
 */
bool audio_is_playing(void);

/***
 * @brief 反初始化音频处理模块
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_handler_deinit(void);

#endif // AUDIO_HANDLER_H

