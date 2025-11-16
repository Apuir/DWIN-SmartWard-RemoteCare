/***
 * @file temperature-alert.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 温度警告Web Component
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath temperature-alert.js
 * @projectType Frontend
 */

class TemperatureAlert extends HTMLElement {
    constructor() {
        super();
        this.eventSource = null;
        this.alerts = [];
        this.maxAlerts = 10;
        this.deviceConnected = false;  // 改名避免与HTMLElement的isConnected冲突
        // 初始化默认温度显示
        this.currentTemp = {
            type: 'update',
            temperature: 0.0,
            message: '等待温度数据...',
            displayTime: new Date().toLocaleTimeString(),
            timestamp: Date.now() / 1000
        };
    }

    connectedCallback() {
        this.render();
        this.setupEventListeners();
    }

    disconnectedCallback() {
        this.closeEventSource();
    }

    // ==================== 设置事件监听 ====================
    setupEventListeners() {
        // 监听设备连接状态
        document.addEventListener('device-connected', () => {
            this.deviceConnected = true;
            this.connectToTemperatureEvents();
            this.render();
        });

        document.addEventListener('device-disconnected', () => {
            this.deviceConnected = false;
            this.closeEventSource();
            this.render();
        });
    }

    // ==================== 连接到温度事件流 ====================
    connectToTemperatureEvents() {
        if (this.eventSource) {
            this.eventSource.close();
        }

        this.eventSource = new EventSource('http://localhost:8088/api/temperature/events');

        this.eventSource.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                
                if (data.type !== 'connected') {
                    this.addAlert(data);
                }
            } catch (error) {
                console.error('Failed to parse temperature event:', error);
            }
        };

        this.eventSource.onerror = (error) => {
            console.error('Temperature event source error:', error);
            this.closeEventSource();
        };

        this.addLog('info', '温度监控服务已连接');
    }

    // ==================== 关闭事件流 ====================
    closeEventSource() {
        if (this.eventSource) {
            this.eventSource.close();
            this.eventSource = null;
        }
    }

    // ==================== 添加警告 ====================
    addAlert(alert) {
        const timestamp = new Date(alert.timestamp * 1000).toLocaleTimeString();
        
        // 如果是温度更新（非警告），更新当前温度显示
        if (alert.type === 'update') {
            this.currentTemp = {
                ...alert,
                displayTime: timestamp
            };
            
            // 如果没有记录或第一条不是update类型，添加新记录
            if (this.alerts.length === 0 || this.alerts[0].type !== 'update') {
                this.alerts.unshift({
                    ...alert,
                    displayTime: timestamp
                });
            } else {
                // 更新第一条记录
                this.alerts[0] = {
                    ...alert,
                    displayTime: timestamp
                };
            }
            this.render();
            return;
        }
        
        // 对于警告类型（threshold1, threshold2, normal），正常添加到历史
        this.alerts.unshift({
            ...alert,
            displayTime: timestamp
        });

        if (this.alerts.length > this.maxAlerts) {
            this.alerts.pop();
        }

        this.render();
        
        // 发送日志到监控面板
        const logType = alert.type === 'normal' ? 'success' : 
                       alert.type === 'threshold2' ? 'error' : 'warning';
        this.addLog(logType, alert.message);
        
        // 如果是严重警告，显示浏览器通知
        if (alert.type === 'threshold2') {
            this.showBrowserNotification(alert);
        }
    }

    // ==================== 显示浏览器通知 ====================
    showBrowserNotification(alert) {
        if ('Notification' in window && Notification.permission === 'granted') {
            new Notification('温度警告', {
                body: alert.message,
                icon: '/favicon.ico',
                requireInteraction: true
            });
        } else if ('Notification' in window && Notification.permission !== 'denied') {
            Notification.requestPermission().then(permission => {
                if (permission === 'granted') {
                    new Notification('温度警告', {
                        body: alert.message,
                        icon: '/favicon.ico'
                    });
                }
            });
        }
    }

    // ==================== 添加日志 ====================
    addLog(type, message) {
        const event = new CustomEvent('audio-log', {
            detail: { type, message },
            bubbles: true,
            composed: true
        });
        this.dispatchEvent(event);
    }

    // ==================== 清空警告 ====================
    clearAlerts() {
        this.alerts = [];
        this.render();
    }

    // ==================== 渲染组件 ====================
    render() {
        const currentAlert = this.alerts[0];
        // 如果没有警告，使用currentTemp作为显示
        const displayData = currentAlert || this.currentTemp;
        
        this.innerHTML = `
            <div class="card">
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
                    <h2>温度监控</h2>
                    ${this.alerts.length > 0 && this.alerts.some(a => a.type !== 'update') ? `
                        <button onclick="this.getRootNode().host.clearAlerts()" style="font-size: 12px; padding: 4px 8px;">
                            清空记录
                        </button>
                    ` : ''}
                </div>
                
                ${!this.deviceConnected ? `
                    <div style="padding: 16px; background-color: var(--bg-color); border: 1px solid var(--border-color);">
                        <p style="color: var(--text-secondary); margin: 0 0 12px 0; text-align: center;">等待设备连接...</p>
                        <div style="text-align: center;">
                            <div style="font-size: 32px; font-weight: 700; color: var(--text-secondary); font-family: 'Monaco', 'Courier New', monospace;">
                                ${this.currentTemp.temperature.toFixed(1)}°C
                            </div>
                            <div style="color: var(--text-secondary); font-size: 13px; margin-top: 8px;">
                                ${this.currentTemp.message}
                            </div>
                        </div>
                    </div>
                ` : displayData ? `
                    <div style="padding: 16px; background-color: ${
                        displayData.type === 'threshold2' ? '#fef2f2' :
                        displayData.type === 'threshold1' ? '#fffbeb' :
                        '#f0fdf4'
                    }; border-left: 4px solid ${
                        displayData.type === 'threshold2' ? 'var(--error-color)' :
                        displayData.type === 'threshold1' ? 'var(--warning-color)' :
                        'var(--success-color)'
                    }; margin-bottom: 16px;">
                        <div style="display: flex; align-items: flex-start; gap: 12px;">
                            <div style="flex: 1;">
                                ${displayData.type !== 'update' ? `
                                    <div style="font-weight: 600; color: ${
                                        displayData.type === 'threshold2' ? 'var(--error-color)' :
                                        displayData.type === 'threshold1' ? 'var(--warning-color)' :
                                        'var(--success-color)'
                                    }; margin-bottom: 8px; font-size: 16px;">
                                        ${displayData.type === 'threshold2' ? '严重警告' :
                                          displayData.type === 'threshold1' ? '温度警告' :
                                          '状态正常'}
                                    </div>
                                ` : ''}
                                ${displayData.temperature !== undefined ? `
                                    <div style="font-size: 32px; font-weight: 700; color: ${
                                        displayData.type === 'threshold2' ? 'var(--error-color)' :
                                        displayData.type === 'threshold1' ? 'var(--warning-color)' :
                                        'var(--success-color)'
                                    }; margin-bottom: 8px; font-family: 'Monaco', 'Courier New', monospace;">
                                        ${displayData.temperature.toFixed(1)}°C
                                    </div>
                                ` : ''}
                                <div style="color: var(--text-primary); font-size: 14px; margin-bottom: 8px;">
                                    ${displayData.message}
                                </div>
                                ${displayData.threshold > 0 ? `
                                    <div style="color: var(--text-secondary); font-size: 13px; margin-bottom: 4px;">
                                        阈值温度: ${displayData.threshold}°C
                                    </div>
                                ` : ''}
                                <div style="color: var(--text-secondary); font-size: 12px;">
                                    ${displayData.displayTime}
                                </div>
                            </div>
                        </div>
                    </div>
                ` : ''}
                
                ${this.alerts.length > 1 && this.alerts.slice(1).some(a => a.type !== 'update') ? `
                    <div>
                        <h3 style="font-size: 14px; color: var(--text-secondary); margin-bottom: 8px;">历史记录</h3>
                        <div style="max-height: 200px; overflow-y: auto;">
                            ${this.alerts.slice(1).filter(a => a.type !== 'update').map(alert => `
                                <div style="padding: 8px 12px; border: 1px solid var(--border-color); margin-bottom: 6px; font-size: 13px;">
                                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px;">
                                        <div style="display: flex; align-items: center; gap: 8px;">
                                            <span style="color: ${
                                                alert.type === 'threshold2' ? 'var(--error-color)' :
                                                alert.type === 'threshold1' ? 'var(--warning-color)' :
                                                'var(--success-color)'
                                            }; font-weight: 500;">
                                                ${alert.type === 'threshold2' ? '严重' :
                                                  alert.type === 'threshold1' ? '警告' :
                                                  '正常'}
                                            </span>
                                            ${alert.temperature !== undefined ? `
                                                <span style="font-weight: 600; font-family: 'Monaco', 'Courier New', monospace; color: ${
                                                    alert.type === 'threshold2' ? 'var(--error-color)' :
                                                    alert.type === 'threshold1' ? 'var(--warning-color)' :
                                                    'var(--success-color)'
                                                };">
                                                    ${alert.temperature.toFixed(1)}°C
                                                </span>
                                            ` : ''}
                                        </div>
                                        <span style="color: var(--text-secondary); font-size: 12px;">
                                            ${alert.displayTime}
                                        </span>
                                    </div>
                                    <div style="color: var(--text-primary);">
                                        ${alert.message}
                                    </div>
                                </div>
                            `).join('')}
                        </div>
                    </div>
                ` : ''}
            </div>
        `;
    }
}

customElements.define('temperature-alert', TemperatureAlert);



