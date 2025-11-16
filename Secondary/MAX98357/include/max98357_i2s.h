/***
 * @file max98357_i2s.h
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-02
 * @brief MAX98357 I2S音频驱动头文件
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-02
 * @filePath max98357_i2s.h
 * @projectType Embedded
 */

#ifndef MAX98357_I2S_H
#define MAX98357_I2S_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ==================== I2S引脚配置 ====================
// 使用IO4/IO5/IO6（左侧引脚4/5/6），这些是安全的GPIO
#define I2S_BCK_IO      (GPIO_NUM_4)   // I2S时钟引脚 (BCLK)
#define I2S_WS_IO       (GPIO_NUM_5)   // I2S帧同步引脚 (LRC/WS)
#define I2S_DO_IO       (GPIO_NUM_6)   // I2S数据输出引脚 (DIN)
#define I2S_NUM         (0)            // I2S端口号

// ==================== 音频参数配置 ====================
#define SAMPLE_RATE     (44100)        // 采样率 44.1kHz
#define BITS_PER_SAMPLE (16)           // 位深度 16bit

/**
 * @brief 初始化MAX98357 I2S驱动
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_init(void);

/**
 * @brief 播放音频数据
 * @param audio_data 音频数据指针
 * @param size 数据大小（字节）
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_play(const uint8_t *audio_data, size_t size);

/**
 * @brief 停止音频播放
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_stop(void);

/**
 * @brief 重启I2S通道（确保进入干净状态）
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_restart(void);

/**
 * @brief 清理I2S缓冲区（写入静音数据消除噪音）
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_clear_buffer(void);

/**
 * @brief 反初始化I2S驱动
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_deinit(void);

#endif // MAX98357_I2S_H
