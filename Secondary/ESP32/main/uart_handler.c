/***
 * @file uart_handler.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-02
 * @brief UART通信处理模块（接收T5L命令）
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-02
 * @filePath uart_handler.c
 * @projectType Embedded
 */

#include "uart_handler.h"
#include "esp32_main.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "UART_HANDLER";
static QueueHandle_t uart_queue;
static uart_command_callback_t g_callback = NULL;
static bool uart_paused = false;  // UART处理暂停标志

/****************************************************************************
 * @brief UART事件处理任务
 * @param pvParameters 任务参数
 */
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *data = (uint8_t *)malloc(UART_BUF_SIZE);
    static uint32_t last_log_time = 0;
    static uint32_t ignored_bytes = 0;
    
    while (1) {
        if (xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    {
                        int len = uart_read_bytes(UART_NUM, data, event.size, portMAX_DELAY);
                        
                        // ==================== 检查是否暂停 ====================
                        if (uart_paused) {
                            // 暂停时丢弃数据，但继续读取以防止缓冲区溢出
                            if (len > 0) {
                                ESP_LOGD(TAG, "UART paused, discarded %d bytes", len);
                            }
                            break;
                        }
                        
                        if (len > 0 && g_callback != NULL) {
                            // ==================== 处理接收到的命令（3字节协议：CMD + TEMP_H + TEMP_L）====================
                            int i = 0;
                            while (i < len) {
                                uint8_t cmd = data[i];
                                
                                // ==================== 检查是否为有效温度命令 ====================
                                // T5L有效命令范围：0xE0-0xE3 (温度命令)
                                if (cmd >= 0xE0 && cmd <= 0xE3) {
                                    // 检查是否有足够的数据（需要3字节：CMD + TEMP_H + TEMP_L）
                                    if (i + 2 < len) {
                                        uint16_t temp_value = (data[i+1] << 8) | data[i+2];
                                        ESP_LOGI(TAG, "Temperature command: 0x%02X, Temp: %d.%d°C", 
                                                 cmd, temp_value/10, temp_value%10);
                                        
                                        // 使用扩展回调传递温度值（通过全局变量或修改回调）
                                        // 临时方案：将温度值编码到命令的高16位
                                        uint32_t cmd_with_temp = ((uint32_t)temp_value << 16) | cmd;
                                        g_callback(cmd_with_temp);
                                        
                                        i += 3;  // 跳过已处理的3个字节
                                    } else {
                                        ESP_LOGW(TAG, "Incomplete temperature data packet");
                                        i++;
                                    }
                                } else {
                                    // 统计被忽略的无效字节
                                    ignored_bytes++;
                                    
                                    // 每5秒最多打印一次统计信息
                                    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
                                    if (now - last_log_time > 5000) {
                                        if (ignored_bytes > 0) {
                                            ESP_LOGW(TAG, "Ignored %lu invalid UART bytes in last 5s (latest: 0x%02X)", 
                                                     ignored_bytes, cmd);
                                            ignored_bytes = 0;
                                        }
                                        last_log_time = now;
                                    }
                                    i++;
                                }
                            }
                        }
                    }
                    break;
                    
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "UART FIFO overflow");
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART buffer full");
                    uart_flush_input(UART_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    free(data);
    vTaskDelete(NULL);
}

/****************************************************************************
 * @brief 初始化UART
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t uart_handler_init(void)
{
    ESP_LOGI(TAG, "Starting UART initialization...");
    
    // ==================== UART配置 ====================
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_LOGI(TAG, "Installing UART driver (UART%d, TX:%d, RX:%d, baud:%d)...", 
             UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
    
    esp_err_t ret = uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 20, &uart_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "UART driver installed successfully");
    
    ESP_LOGI(TAG, "Configuring UART parameters...");
    ret = uart_param_config(UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "UART parameters configured successfully");
    
    ESP_LOGI(TAG, "Setting UART pins...");
    ret = uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "UART pins set successfully");
    
    // ==================== 创建UART事件处理任务 ====================
    ESP_LOGI(TAG, "Creating UART event task...");
    BaseType_t task_ret = xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 12, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART event task");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "UART event task created successfully");
    
    ESP_LOGI(TAG, "UART initialized successfully");
    return ESP_OK;
}

/****************************************************************************
 * @brief 注册UART命令回调函数
 * @param callback 回调函数指针
 */
void uart_handler_register_callback(uart_command_callback_t callback)
{
    g_callback = callback;
}

/****************************************************************************
 * @brief 发送数据到UART
 * @param data 数据指针
 * @param len 数据长度
 * @return 实际发送的字节数
 */
int uart_handler_send(const uint8_t *data, size_t len)
{
    return uart_write_bytes(UART_NUM, (const char *)data, len);
}

/****************************************************************************
| * @brief 暂停UART数据处理（音频播放期间）
| */
void uart_handler_pause(void)
{
    if (!uart_paused) {
        uart_paused = true;
        ESP_LOGI(TAG, "UART data processing paused (audio streaming started)");
    }
}

/****************************************************************************
| * @brief 恢复UART数据处理并清理缓冲区
| */
void uart_handler_resume(void)
{
    if (uart_paused) {
        uart_paused = false;
        
        // ==================== 清理UART缓冲区 ====================
        // 刷新输入缓冲区，丢弃暂停期间的残留数据
        uart_flush_input(UART_NUM);
        
        // 清空事件队列
        if (uart_queue != NULL) {
            xQueueReset(uart_queue);
        }
        
        ESP_LOGI(TAG, "UART data processing resumed, buffers cleared");
    }
}

