/***
 * @file mdns_service.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief mDNS服务实现（设备发现）
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath mdns_service.c
 * @projectType Embedded
 */

 #include "mdns_service.h"
 #include "esp_log.h"
 #include "mdns.h"
 #include "esp32_main.h"
 
 static const char *TAG = "MDNS_SERVICE";
 
 /****************************************************************************
  * @brief 初始化mDNS服务
  * @return esp_err_t ESP_OK成功，其他失败
  */
 esp_err_t mdns_service_init(void)
 {
     // ==================== 初始化mDNS ====================
     esp_err_t err = mdns_init();
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to initialize mDNS: %s", esp_err_to_name(err));
         return err;
     }
     
     // ==================== 设置主机名 ====================
     err = mdns_hostname_set(MDNS_HOSTNAME);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to set mDNS hostname: %s", esp_err_to_name(err));
         mdns_free();
         return err;
     }
     
     // ==================== 设置实例名 ====================
     err = mdns_instance_name_set(MDNS_INSTANCE);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to set mDNS instance name: %s", esp_err_to_name(err));
         mdns_free();
         return err;
     }
     
     // ==================== 添加服务 ====================
     // 添加TCP服务，端口8080，服务类型"_esp32temp"
     err = mdns_service_add(NULL, "_esp32temp", "_tcp", TCP_SERVER_PORT, NULL, 0);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "Failed to add mDNS service: %s", esp_err_to_name(err));
         mdns_free();
         return err;
     }
     
     // ==================== 添加服务TXT记录 ====================
     mdns_txt_item_t serviceTxtData[] = {
         {"board", "ESP32-S3"},
         {"service", "temperature"},
         {"version", "0.1"}
     };
     
     err = mdns_service_txt_set("_esp32temp", "_tcp", serviceTxtData, 3);
     if (err != ESP_OK) {
         ESP_LOGW(TAG, "Failed to set mDNS TXT records: %s", esp_err_to_name(err));
         // 不是致命错误，继续运行
     }
     
     ESP_LOGI(TAG, "mDNS service started: %s.local", MDNS_HOSTNAME);
     ESP_LOGI(TAG, "Service type: _esp32temp._tcp, port: %d", TCP_SERVER_PORT);
     
     return ESP_OK;
 }
 
 /****************************************************************************
  * @brief 停止mDNS服务
  */
 void mdns_service_stop(void)
 {
     mdns_free();
     ESP_LOGI(TAG, "mDNS service stopped");
 }