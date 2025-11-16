/***
 * @file max98357_i2s.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-02
 * @brief MAX98357 I2S音频驱动实现
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-02
 * @filePath max98357_i2s.c
 * @projectType Embedded
 */

#include "max98357_i2s.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdlib.h>

static const char *TAG = "MAX98357";
static i2s_chan_handle_t tx_handle = NULL;

/****************************************************************************
 * @brief 初始化MAX98357 I2S驱动
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "MAX98357 I2S Initialization Starting...");
    ESP_LOGI(TAG, "I2S Pin Configuration:");
    ESP_LOGI(TAG, "  - BCK (Clock):      GPIO%d", I2S_BCK_IO);
    ESP_LOGI(TAG, "  - WS (Word Select): GPIO%d", I2S_WS_IO);
    ESP_LOGI(TAG, "  - DO (Data Out):    GPIO%d", I2S_DO_IO);
    ESP_LOGI(TAG, "Audio Parameters:");
    ESP_LOGI(TAG, "  - Sample Rate:      %d Hz", SAMPLE_RATE);
    ESP_LOGI(TAG, "  - Bit Depth:        32-bit (Input: 16-bit)");
    ESP_LOGI(TAG, "  - Channels:         Stereo");
    ESP_LOGI(TAG, "  - Mode:             Philips I2S");

    // ==================== I2S通道配置 ====================
    ESP_LOGI(TAG, "Configuring I2S channel...");
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 6;  // 增加DMA描述符数量
    chan_cfg.dma_frame_num = 240;  // 每个DMA缓冲区的帧数
    ESP_LOGI(TAG, "  - DMA Descriptors:  %d", chan_cfg.dma_desc_num);
    ESP_LOGI(TAG, "  - DMA Frame Count:  %d", chan_cfg.dma_frame_num);
    
    ret = i2s_new_channel(&chan_cfg, &tx_handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "I2S channel created successfully");

    // ==================== I2S标准模式配置 ====================
    // MAX98357需要32bit数据宽度，内部使用16bit音频数据
    ESP_LOGI(TAG, "Configuring I2S standard mode...");
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_IO,
            .ws = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(tx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S standard mode: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "I2S standard mode initialized");

    // ==================== 启动I2S ====================
    ESP_LOGI(TAG, "Enabling I2S channel...");
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "I2S channel enabled successfully");

    ESP_LOGI(TAG, "MAX98357 initialized successfully");
    ESP_LOGI(TAG, "Free Heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "========================================");
    return ESP_OK;
}

/****************************************************************************
 * @brief 播放音频数据
 * @param audio_data 音频数据指针
 * @param size 数据大小（字节）
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_play(const uint8_t *audio_data, size_t size)
{
    if (tx_handle == NULL) {
        ESP_LOGE(TAG, "I2S not initialized");
        return ESP_FAIL;
    }

    if (audio_data == NULL || size == 0) {
        ESP_LOGW(TAG, "Invalid audio data: ptr=%p, size=%d", audio_data, size);
        return ESP_FAIL;
    }

    // 调试：显示前16字节的音频数据（仅第一个包）
    static bool first_packet = true;
    if (first_packet && size >= 16) {
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "First Audio Packet Received");
        ESP_LOGI(TAG, "Packet Size: %d bytes", size);
        ESP_LOGI(TAG, "Raw Data (first 16 bytes):");
        ESP_LOGI(TAG, "  [%02X %02X %02X %02X %02X %02X %02X %02X]",
                 audio_data[0], audio_data[1], audio_data[2], audio_data[3],
                 audio_data[4], audio_data[5], audio_data[6], audio_data[7]);
        ESP_LOGI(TAG, "  [%02X %02X %02X %02X %02X %02X %02X %02X]",
                 audio_data[8], audio_data[9], audio_data[10], audio_data[11],
                 audio_data[12], audio_data[13], audio_data[14], audio_data[15]);
        
        // 解析为16bit采样值
        int16_t *samples = (int16_t *)audio_data;
        ESP_LOGI(TAG, "16-bit Samples (L/R pairs):");
        ESP_LOGI(TAG, "  Sample 0: L=%d, R=%d", samples[0], samples[1]);
        ESP_LOGI(TAG, "  Sample 1: L=%d, R=%d", samples[2], samples[3]);
        ESP_LOGI(TAG, "  Sample 2: L=%d, R=%d", samples[4], samples[5]);
        ESP_LOGI(TAG, "========================================");
        first_packet = false;
    }

    // MAX98357需要32bit数据，将16bit转换为32bit
    // 输入：16bit PCM (2字节/采样)
    // 输出：32bit (4字节/采样)
    size_t sample_count = size / 2;  // 16bit采样数
    size_t output_size = sample_count * 4;  // 32bit输出大小
    
    ESP_LOGD(TAG, "Converting audio: %d bytes (16-bit) -> %d bytes (32-bit)", size, output_size);
    
    int32_t *output_buffer = (int32_t *)malloc(output_size);
    if (output_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate conversion buffer (%d bytes)", output_size);
        ESP_LOGE(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
        return ESP_FAIL;
    }
    
    // 转换16bit到32bit (左移16位，高位对齐)
    int16_t *input_samples = (int16_t *)audio_data;
    for (size_t i = 0; i < sample_count; i++) {
        output_buffer[i] = ((int32_t)input_samples[i]) << 16;
    }

    size_t bytes_written = 0;
    esp_err_t ret = i2s_channel_write(tx_handle, output_buffer, output_size, &bytes_written, portMAX_DELAY);
    
    free(output_buffer);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write audio data: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }

    // 详细日志：显示写入统计（每50个包打印一次）
    static uint32_t total_bytes = 0;
    static uint32_t packet_count = 0;
    total_bytes += bytes_written;
    packet_count++;
    
    if (packet_count % 50 == 0) {
        ESP_LOGI(TAG, "Audio Stats: Packets=%lu, Total=%lu bytes, Heap=%lu bytes", 
                 packet_count, total_bytes, esp_get_free_heap_size());
    }

    ESP_LOGD(TAG, "Audio written: %d bytes input -> %d bytes output", size, bytes_written);
    return ESP_OK;
}

/****************************************************************************
 * @brief 停止音频播放
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_stop(void)
{
    if (tx_handle == NULL) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping I2S playback...");
    esp_err_t ret = i2s_channel_disable(tx_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "I2S playback stopped");
    } else {
        ESP_LOGE(TAG, "Failed to stop I2S: %s", esp_err_to_name(ret));
    }
    return ret;
}

/****************************************************************************
 * @brief 重启I2S通道（确保进入干净状态）
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_restart(void)
{
    if (tx_handle == NULL) {
        ESP_LOGW(TAG, "I2S not initialized, cannot restart");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Restarting I2S channel for clean state...");
    
    // 禁用通道
    esp_err_t ret = i2s_channel_disable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable I2S for restart: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 短暂延时
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // 重新启用通道
    ret = i2s_channel_enable(tx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to re-enable I2S: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2S channel restarted successfully");
    return ESP_OK;
}

/****************************************************************************
 * @brief 清理I2S缓冲区（写入静音数据消除噪音）
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_clear_buffer(void)
{
    if (tx_handle == NULL) {
        return ESP_OK;
    }

    ESP_LOGD(TAG, "Clearing I2S buffer with silence...");
    
    // 写入0.1秒的静音数据来清理缓冲区
    const size_t silence_samples = SAMPLE_RATE / 10;  // 0.1秒
    const size_t silence_size = silence_samples * 4;  // 32bit stereo
    
    int32_t *silence_buffer = (int32_t *)calloc(silence_samples, sizeof(int32_t));
    if (silence_buffer == NULL) {
        ESP_LOGW(TAG, "Failed to allocate silence buffer, skipping cleanup");
        return ESP_OK;
    }
    
    // 写入静音数据
    size_t bytes_written = 0;
    i2s_channel_write(tx_handle, silence_buffer, silence_size, &bytes_written, pdMS_TO_TICKS(100));
    
    free(silence_buffer);
    
    ESP_LOGD(TAG, "I2S buffer cleared (%d bytes silence)", bytes_written);
    return ESP_OK;
}

/****************************************************************************
 * @brief 反初始化I2S驱动
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t max98357_deinit(void)
{
    if (tx_handle != NULL) {
        ESP_LOGI(TAG, "Deinitializing MAX98357...");
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        tx_handle = NULL;
        ESP_LOGI(TAG, "MAX98357 deinitialized");
    }
    
    return ESP_OK;
}
