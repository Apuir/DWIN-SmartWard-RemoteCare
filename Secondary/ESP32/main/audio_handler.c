/***
 * @file audio_handler.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 音频流处理模块实现
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath audio_handler.c
 * @projectType Embedded
 */

#include "audio_handler.h"
#include "esp32_main.h"
#include "uart_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

#ifdef CONFIG_USE_MAX98357
#include "max98357_i2s.h"
#endif

static const char *TAG = "AUDIO_HANDLER";

// 音频流状态
typedef enum {
    AUDIO_STATE_IDLE,      // 空闲
    AUDIO_STATE_RECEIVING, // 接收中
    AUDIO_STATE_PLAYING,   // 播放中
} audio_state_t;

// 音频数据包结构
typedef struct {
    uint8_t *data;
    size_t len;
} audio_packet_t;

static audio_state_t current_state = AUDIO_STATE_IDLE;
static QueueHandle_t audio_queue = NULL;
static TaskHandle_t audio_task_handle = NULL;
static bool audio_initialized = false;

/****************************************************************************
 * @brief 音频播放任务
 * @param pvParameters 任务参数
 */
static void audio_play_task(void *pvParameters)
{
    audio_packet_t packet;
    
    ESP_LOGI(TAG, "Audio playback task started");
    
    while (1) {
        // ==================== 从队列获取音频数据包 ====================
        if (xQueueReceive(audio_queue, &packet, portMAX_DELAY)) {
            if (packet.data != NULL && packet.len > 0) {
                ESP_LOGD(TAG, "Playing audio packet: %d bytes", packet.len);
                
                // ==================== 播放音频数据 ====================
#ifdef CONFIG_USE_MAX98357
                max98357_play(packet.data, packet.len);
#else
                ESP_LOGW(TAG, "MAX98357 not configured, audio playback skipped");
#endif
                
                // ==================== 释放音频数据内存 ====================
                free(packet.data);
            }
        }
    }
    
    vTaskDelete(NULL);
}

/****************************************************************************
 * @brief 初始化音频处理模块
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_handler_init(void)
{
    if (audio_initialized) {
        ESP_LOGW(TAG, "Audio handler already initialized");
        return ESP_OK;
    }
    
    // ==================== 创建音频队列 ====================
    audio_queue = xQueueCreate(10, sizeof(audio_packet_t));
    if (audio_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create audio queue");
        return ESP_FAIL;
    }
    
    // ==================== 创建音频播放任务 ====================
    if (xTaskCreate(audio_play_task, "audio_play", 4096, NULL, 5, &audio_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio playback task");
        vQueueDelete(audio_queue);
        audio_queue = NULL;
        return ESP_FAIL;
    }
    
    // ==================== 初始化I2S音频驱动 ====================
#ifdef CONFIG_USE_MAX98357
    esp_err_t ret = max98357_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAX98357 initialization failed");
        vTaskDelete(audio_task_handle);
        vQueueDelete(audio_queue);
        audio_queue = NULL;
        audio_task_handle = NULL;
        return ret;
    }
#endif
    
    audio_initialized = true;
    current_state = AUDIO_STATE_IDLE;
    ESP_LOGI(TAG, "Audio handler initialized successfully");
    
    return ESP_OK;
}

/****************************************************************************
 * @brief 开始音频流接收
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stream_start(void)
{
    if (!audio_initialized) {
        ESP_LOGE(TAG, "Audio handler not initialized");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Starting audio stream...");
    
    // ==================== 暂停UART处理，减少干扰 ====================
    uart_handler_pause();
    
    // ==================== 清空队列 ====================
    xQueueReset(audio_queue);
    
    current_state = AUDIO_STATE_RECEIVING;
    ESP_LOGI(TAG, "Audio stream started, ready to receive data");
    
    return ESP_OK;
}

/****************************************************************************
 * @brief 接收音频流数据包
 * @param data 音频数据指针
 * @param len 数据长度
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stream_feed(const uint8_t *data, size_t len)
{
    if (!audio_initialized) {
        ESP_LOGE(TAG, "Audio handler not initialized");
        return ESP_FAIL;
    }
    
    if (current_state != AUDIO_STATE_RECEIVING && current_state != AUDIO_STATE_PLAYING) {
        ESP_LOGW(TAG, "Not in receiving state (state=%d), ignoring audio data", current_state);
        return ESP_FAIL;
    }
    
    if (data == NULL || len == 0) {
        ESP_LOGW(TAG, "Invalid audio data: ptr=%p, len=%d", data, len);
        return ESP_FAIL;
    }
    
    // ==================== 分配内存并复制数据 ====================
    uint8_t *buffer = (uint8_t *)malloc(len);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes for audio packet", len);
        return ESP_FAIL;
    }
    memcpy(buffer, data, len);
    
    // ==================== 创建音频数据包 ====================
    audio_packet_t packet = {
        .data = buffer,
        .len = len
    };
    
    // ==================== 发送到队列 ====================
    if (xQueueSend(audio_queue, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGW(TAG, "Audio queue full, dropping packet (%d bytes)", len);
        free(buffer);
        return ESP_FAIL;
    }
    
    // ==================== 更新状态 ====================
    if (current_state == AUDIO_STATE_RECEIVING) {
        current_state = AUDIO_STATE_PLAYING;
        ESP_LOGI(TAG, "Started playing audio stream");
    }
    
    ESP_LOGD(TAG, "Audio packet queued: %d bytes, queue waiting: %d", len, uxQueueMessagesWaiting(audio_queue));
    
    return ESP_OK;
}

/****************************************************************************
 * @brief 结束音频流接收
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stream_end(void)
{
    if (!audio_initialized) {
        ESP_LOGE(TAG, "Audio handler not initialized");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Ending audio stream...");
    
    // ==================== 等待队列播放完毕 ====================
    int wait_count = 0;
    while (uxQueueMessagesWaiting(audio_queue) > 0 && wait_count < 50) {
        vTaskDelay(pdMS_TO_TICKS(10));
        wait_count++;
    }
    ESP_LOGI(TAG, "Queue drained after %d ms", wait_count * 10);
    
    // ==================== 额外延时确保最后数据播放完 ====================
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // ==================== 清理I2S缓冲区（写入静音） ====================
#ifdef CONFIG_USE_MAX98357
    ESP_LOGI(TAG, "Clearing I2S buffer to remove residual noise...");
    max98357_clear_buffer();
    ESP_LOGI(TAG, "I2S buffer cleared");
#endif
    
    current_state = AUDIO_STATE_IDLE;
    
    // ==================== 恢复UART处理并清理缓冲区 ====================
    uart_handler_resume();
    
    ESP_LOGI(TAG, "Audio stream ended successfully");
    
    return ESP_OK;
}

/****************************************************************************
 * @brief 停止音频播放
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_stop(void)
{
    if (!audio_initialized) {
        ESP_LOGE(TAG, "Audio handler not initialized");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Stopping audio playback...");
    
    // ==================== 清空队列 ====================
    UBaseType_t dropped = uxQueueMessagesWaiting(audio_queue);
    xQueueReset(audio_queue);
    if (dropped > 0) {
        ESP_LOGI(TAG, "Dropped %d queued packets", dropped);
    }
    
    // ==================== 清理并停止I2S ====================
#ifdef CONFIG_USE_MAX98357
    ESP_LOGI(TAG, "Clearing I2S buffer...");
    max98357_clear_buffer();
    
    ESP_LOGI(TAG, "Stopping I2S channel...");
    max98357_stop();
    
    // 短暂延时后重新启用I2S，确保进入干净状态
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "Restarting I2S channel...");
    max98357_restart();
#endif
    
    current_state = AUDIO_STATE_IDLE;
    
    // ==================== 恢复UART处理并清理缓冲区 ====================
    uart_handler_resume();
    
    ESP_LOGI(TAG, "Audio playback stopped successfully");
    
    return ESP_OK;
}

/****************************************************************************
 * @brief 获取音频流状态
 * @return true - 正在播放，false - 未播放
 */
bool audio_is_playing(void)
{
    return (current_state == AUDIO_STATE_PLAYING || current_state == AUDIO_STATE_RECEIVING);
}

/****************************************************************************
 * @brief 反初始化音频处理模块
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t audio_handler_deinit(void)
{
    if (!audio_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Deinitializing audio handler...");
    
    // ==================== 停止播放 ====================
    audio_stop();
    
    // ==================== 删除任务 ====================
    if (audio_task_handle != NULL) {
        vTaskDelete(audio_task_handle);
        audio_task_handle = NULL;
    }
    
    // ==================== 删除队列 ====================
    if (audio_queue != NULL) {
        vQueueDelete(audio_queue);
        audio_queue = NULL;
    }
    
    // ==================== 反初始化I2S ====================
#ifdef CONFIG_USE_MAX98357
    max98357_deinit();
#endif
    
    audio_initialized = false;
    ESP_LOGI(TAG, "Audio handler deinitialized");
    
    return ESP_OK;
}
