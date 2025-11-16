/***
 * @file station_example_main.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief ESP32主程序（WiFi+UART+TCP+音频流）
 * 
 * @version 0.2
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath station_example_main.c
 * @projectType Embedded
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp32_main.h"
#include "uart_handler.h"
#include "tcp_server.h"
#include "audio_handler.h"
#include "mdns_service.h"
#include "driver/gpio.h"

// ==================== WiFi配置 ====================
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

// ==================== 事件组定义 ====================
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "ESP32_MAIN";
static int s_retry_num = 0;
static bool wifi_connected = false;

/****************************************************************************
| * @brief WiFi事件处理函数
| * @param arg 参数
| * @param event_base 事件基础
| * @param event_id 事件ID
| * @param event_data 事件数据
| */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started, connecting to AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying WiFi connection, attempt: %d/%d", s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi connection failed after %d attempts", EXAMPLE_ESP_MAXIMUM_RETRY);
        }
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        s_retry_num = 0;
        wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/****************************************************************************
| * @brief 初始化WiFi Station模式
| */
static void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "Creating WiFi event group...");
    s_wifi_event_group = xEventGroupCreate();

    ESP_LOGI(TAG, "Initializing network interface...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_LOGI(TAG, "Initializing WiFi driver...");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_LOGI(TAG, "Registering WiFi event handlers...");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_LOGI(TAG, "Configuring WiFi credentials (SSID: %s)...", EXAMPLE_ESP_WIFI_SSID);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    ESP_LOGI(TAG, "Starting WiFi...");
    ESP_ERROR_CHECK(esp_wifi_start());

    // ==================== 等待连接结果 ====================
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected successfully!");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "WiFi connection failed");
    } else {
        ESP_LOGE(TAG, "Unexpected WiFi event occurred");
    }
}

/****************************************************************************
| * @brief UART命令处理回调
| * @param cmd_with_temp 低8位为命令，高16位为温度值（单位0.1°C）
| */
static void uart_command_callback(uint32_t cmd_with_temp)
{
    uint8_t response[10];
    uint8_t cmd = cmd_with_temp & 0xFF;           // 提取命令字节
    uint16_t temp_value = (cmd_with_temp >> 16);  // 提取温度值
    
    ESP_LOGI(TAG, "T5L Command Received: 0x%02X, Temp: %d.%d°C", cmd, temp_value/10, temp_value%10);
    
    // 构建响应包：响应码 + 温度高字节 + 温度低字节（3字节）
    response[0] = 0;  // 将在switch中设置
    response[1] = (temp_value >> 8) & 0xFF;  // 温度高字节
    response[2] = temp_value & 0xFF;          // 温度低字节
    
    switch (cmd) {
        case CMD_TEMP_THRESHOLD1:
            // ==================== 温度达到阈值1 ====================
            ESP_LOGI(TAG, "Temperature Alert: Threshold 1 reached at %d.%d°C", temp_value/10, temp_value%10);
            response[0] = RESP_THRESHOLD1_REACHED;
            if (wifi_connected) {
                tcp_server_send(response, 3, -1);  // 广播到所有客户端（3字节）
                ESP_LOGI(TAG, "Broadcast threshold 1 notification with temp data to all clients");
            }
            break;
            
        case CMD_TEMP_THRESHOLD2:
            // ==================== 温度达到阈值2 ====================
            ESP_LOGI(TAG, "Temperature Alert: Threshold 2 reached at %d.%d°C", temp_value/10, temp_value%10);
            response[0] = RESP_THRESHOLD2_REACHED;
            if (wifi_connected) {
                tcp_server_send(response, 3, -1);  // 广播到所有客户端（3字节）
                ESP_LOGI(TAG, "Broadcast threshold 2 notification with temp data to all clients");
            }
            break;
            
        case CMD_TEMP_NORMAL:
            // ==================== 温度恢复正常 ====================
            ESP_LOGI(TAG, "Temperature Status: Returned to normal at %d.%d°C", temp_value/10, temp_value%10);
            response[0] = RESP_TEMP_NORMAL;
            if (wifi_connected) {
                tcp_server_send(response, 3, -1);  // 广播到所有客户端（3字节）
                ESP_LOGI(TAG, "Broadcast temperature normal notification with temp data to all clients");
            }
            break;
            
        case CMD_TEMP_UPDATE:
            // ==================== 温度定期更新（用于显示）====================
            ESP_LOGD(TAG, "Temperature Update: %d.%d°C", temp_value/10, temp_value%10);
            response[0] = RESP_TEMP_UPDATE;
            if (wifi_connected) {
                tcp_server_send(response, 3, -1);  // 广播到所有客户端（3字节）
                ESP_LOGD(TAG, "Broadcast temperature update to all clients");
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown UART command: 0x%02X", cmd);
            break;
    }
}

/****************************************************************************
| * @brief TCP数据处理回调
| * @param data 接收到的数据
| * @param len 数据长度
| * @param socket 客户端套接字
| */
static void tcp_data_callback(const uint8_t *data, size_t len, int socket)
{
    if (len < 1) {
        ESP_LOGW(TAG, "Received empty TCP packet");
        return;
    }
    
    uint8_t cmd = data[0];
    ESP_LOGI(TAG, "TCP Command: 0x%02X (len=%d, socket=%d)", cmd, len, socket);
    
    switch (cmd) {
        case CMD_PLAY_AUDIO:
            // ==================== 播放音频数据（单次） ====================
            if (len > 1) {
                ESP_LOGI(TAG, "CMD_PLAY_AUDIO: Received %d bytes audio data", len - 1);
                audio_stream_feed(data + 1, len - 1);
                
                uint8_t ack = RESP_AUDIO_ACK;
                tcp_server_send(&ack, 1, socket);
                ESP_LOGD(TAG, "Sent RESP_AUDIO_ACK to socket %d", socket);
            }
            break;
            
        case CMD_AUDIO_STREAM_START:
            // ==================== 开始音频流 ====================
            ESP_LOGI(TAG, "CMD_AUDIO_STREAM_START: Starting audio stream");
            if (audio_stream_start() == ESP_OK) {
                uint8_t ack = RESP_AUDIO_ACK;
                tcp_server_send(&ack, 1, socket);
                ESP_LOGI(TAG, "Audio stream started successfully");
            } else {
                uint8_t err = RESP_ERROR;
                tcp_server_send(&err, 1, socket);
                ESP_LOGE(TAG, "Failed to start audio stream");
            }
            break;
            
        case CMD_AUDIO_STREAM_DATA:
            // ==================== 音频流数据 ====================
            if (len > 1) {
                ESP_LOGD(TAG, "CMD_AUDIO_STREAM_DATA: %d bytes", len - 1);
                if (audio_stream_feed(data + 1, len - 1) == ESP_OK) {
                    // 定期发送ACK（每10个包发送一次）
                    static int packet_count = 0;
                    if (++packet_count >= 10) {
                        uint8_t ack = RESP_AUDIO_ACK;
                        tcp_server_send(&ack, 1, socket);
                        ESP_LOGD(TAG, "Sent ACK after %d packets", packet_count);
                        packet_count = 0;
                    }
                }
            }
            break;
            
        case CMD_AUDIO_STREAM_END:
            // ==================== 结束音频流 ====================
            ESP_LOGI(TAG, "CMD_AUDIO_STREAM_END: Stopping audio stream");
            audio_stream_end();
            {
                uint8_t ack = RESP_AUDIO_ACK;
                tcp_server_send(&ack, 1, socket);
                ESP_LOGI(TAG, "Audio stream ended successfully");
            }
            break;
            
        case CMD_STOP_AUDIO:
            // ==================== 停止音频播放 ====================
            ESP_LOGI(TAG, "CMD_STOP_AUDIO: Stopping audio playback");
            audio_stop();
            {
                uint8_t ack = RESP_AUDIO_ACK;
                tcp_server_send(&ack, 1, socket);
                ESP_LOGI(TAG, "Audio stopped successfully");
            }
            break;
            
        case CMD_QUERY_STATUS:
            // ==================== 查询状态 ====================
            {
                bool is_playing = audio_is_playing();
                uint8_t response[3] = {
                    RESP_STATUS_OK, 
                    wifi_connected ? 1 : 0,
                    is_playing ? 1 : 0
                };
                tcp_server_send(response, 3, socket);
                ESP_LOGI(TAG, "CMD_QUERY_STATUS: WiFi=%s, Audio=%s", 
                         wifi_connected ? "Connected" : "Disconnected",
                         is_playing ? "Playing" : "Stopped");
            }
            break;
            
        case CMD_DEVICE_DISCOVERY:
            // ==================== 设备发现响应 ====================
            {
                ESP_LOGI(TAG, "CMD_DEVICE_DISCOVERY: Responding with device info");
                uint8_t response[64];
                int pos = 0;
                response[pos++] = RESP_DEVICE_INFO;
                
                // 添加设备信息（简单格式）
                const char *device_name = "ESP32-S3 Temp Monitor";
                size_t name_len = strlen(device_name);
                if (name_len > 60) name_len = 60;
                memcpy(&response[pos], device_name, name_len);
                pos += name_len;
                
                tcp_server_send(response, pos, socket);
                ESP_LOGI(TAG, "Device info sent: %s", device_name);
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown TCP command: 0x%02X", cmd);
            {
                uint8_t err = RESP_ERROR;
                tcp_server_send(&err, 1, socket);
            }
            break;
    }
}

/****************************************************************************
| * @brief 主函数
| */
void app_main(void)
{
    // ==================== NVS初始化 ====================
    ESP_LOGI(TAG, "Initializing NVS flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition full/version mismatch, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized successfully");

    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
        esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32 Temperature Monitor System");
    ESP_LOGI(TAG, "Version: 0.2");
    ESP_LOGI(TAG, "Free Heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "========================================");
    
    // ==================== WiFi初始化 ====================
    ESP_LOGI(TAG, "Step 1: WiFi Initialization");
    wifi_init_sta();
    ESP_LOGI(TAG, "WiFi initialization completed (Connected: %s)", wifi_connected ? "YES" : "NO");
    
    // ==================== UART初始化 ====================
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Step 2: UART Communication Setup");
    ESP_LOGI(TAG, "UART TX Pin: GPIO%d, RX Pin: GPIO%d", UART_TX_PIN, UART_RX_PIN);
    ESP_LOGI(TAG, "Baud Rate: %d", UART_BAUD_RATE);
    ESP_LOGI(TAG, "Free Heap before UART init: %lu bytes", esp_get_free_heap_size());
    
    esp_err_t uart_ret = uart_handler_init();
    
    if (uart_ret == ESP_OK) {
        uart_handler_register_callback(uart_command_callback);
        ESP_LOGI(TAG, "UART initialized successfully");
    } else {
        ESP_LOGE(TAG, "UART initialization failed: %s", esp_err_to_name(uart_ret));
    }
    ESP_LOGI(TAG, "Free Heap after UART init: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "========================================");
    
    // ==================== 音频处理模块初始化 ====================
    ESP_LOGI(TAG, "Step 3: Audio Handler Initialization");
    ESP_LOGI(TAG, "I2S Pins: BCK=GPIO4, WS=GPIO5, DO=GPIO6");
    ESP_LOGI(TAG, "Audio Format: 44.1kHz, 16-bit, Stereo");
    ESP_LOGI(TAG, "Free Heap before audio init: %lu bytes", esp_get_free_heap_size());
    
    esp_err_t audio_ret = audio_handler_init();
    
    if (audio_ret == ESP_OK) {
        ESP_LOGI(TAG, "Audio handler initialized successfully");
    } else {
        ESP_LOGE(TAG, "Audio initialization failed: %s", esp_err_to_name(audio_ret));
    }
    ESP_LOGI(TAG, "Free Heap after audio init: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "========================================");
    
    // ==================== mDNS服务初始化 ====================
    ESP_LOGI(TAG, "Step 4: mDNS Service Setup");
    if (wifi_connected) {
        if (mdns_service_init() == ESP_OK) {
            ESP_LOGI(TAG, "mDNS service started: %s.local", MDNS_HOSTNAME);
            ESP_LOGI(TAG, "Service type: _esp32temp._tcp, Port: %d", TCP_SERVER_PORT);
        } else {
            ESP_LOGW(TAG, "Failed to start mDNS service");
        }
    } else {
        ESP_LOGW(TAG, "WiFi not connected, skipping mDNS service");
    }
    ESP_LOGI(TAG, "========================================");
    
    // ==================== TCP服务器启动 ====================
    ESP_LOGI(TAG, "Step 5: TCP Server Startup");
    if (wifi_connected) {
        ESP_LOGI(TAG, "Starting TCP server on port %d...", TCP_SERVER_PORT);
        tcp_server_register_callback(tcp_data_callback);
        if (tcp_server_start() == ESP_OK) {
            ESP_LOGI(TAG, "TCP server started successfully");
            ESP_LOGI(TAG, "Listening for connections...");
        } else {
            ESP_LOGE(TAG, "Failed to start TCP server");
        }
    } else {
        ESP_LOGW(TAG, "WiFi not connected, TCP server not started");
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "System Initialization Completed");
    ESP_LOGI(TAG, "Status: %s", wifi_connected ? "READY" : "LIMITED (No WiFi)");
    ESP_LOGI(TAG, "Free Heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "========================================");
}
