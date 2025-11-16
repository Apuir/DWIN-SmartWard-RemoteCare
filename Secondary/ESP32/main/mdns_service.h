/***
 * @file mdns_service.h
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief mDNS设备发现服务头文件
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath mdns_service.h
 * @projectType Embedded
 */

#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include "esp_err.h"

/***
 * @brief 初始化mDNS服务
 * @return ESP_OK - 成功，ESP_FAIL - 失败
 */
esp_err_t mdns_service_init(void);

/***
 * @brief 停止mDNS服务
 */
void mdns_service_stop(void);

#endif // MDNS_SERVICE_H

