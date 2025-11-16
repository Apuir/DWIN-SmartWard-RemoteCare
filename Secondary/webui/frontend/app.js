/***
 * @file app.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 应用主入口
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath app.js
 * @projectType Frontend
 */

// ==================== 应用初始化 ====================
document.addEventListener('DOMContentLoaded', () => {
    console.log('ESP32 Audio Control Application Started');
    
    // 监听连接状态
    document.addEventListener('device-connected', () => {
        updateConnectionStatus(true);
    });
    
    document.addEventListener('device-disconnected', () => {
        updateConnectionStatus(false);
    });
});

// ==================== 更新连接状态 ====================
function updateConnectionStatus(connected) {
    const statusElement = document.getElementById('connection-status');
    if (statusElement) {
        if (connected) {
            statusElement.textContent = '已连接';
            statusElement.className = 'status connected';
        } else {
            statusElement.textContent = '未连接';
            statusElement.className = 'status disconnected';
        }
    }
}

