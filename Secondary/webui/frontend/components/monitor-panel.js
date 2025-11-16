/***
 * @file monitor-panel.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 监控面板Web Component
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath monitor-panel.js
 * @projectType Frontend
 */

class MonitorPanel extends HTMLElement {
    constructor() {
        super();
        this.logs = [];
        this.maxLogs = 50;
    }

    connectedCallback() {
        this.render();
        this.setupEventListeners();
    }

    // ==================== 设置事件监听 ====================
    setupEventListeners() {
        document.addEventListener('device-connected', (e) => {
            this.addLog('success', `已连接到设备: ${e.detail.name} (${e.detail.ip})`);
        });

        document.addEventListener('device-disconnected', () => {
            this.addLog('warning', '设备已断开连接');
        });

        document.addEventListener('audio-log', (e) => {
            this.addLog(e.detail.type, e.detail.message);
        });
    }

    // ==================== 添加日志 ====================
    addLog(type, message) {
        const timestamp = new Date().toLocaleTimeString();
        this.logs.unshift({
            type,
            message,
            timestamp
        });

        if (this.logs.length > this.maxLogs) {
            this.logs.pop();
        }

        this.render();
    }

    // ==================== 清空日志 ====================
    clearLogs() {
        this.logs = [];
        this.render();
    }

    // ==================== 渲染组件 ====================
    render() {
        this.innerHTML = `
            <div class="card" style="grid-column: 1 / -1;">
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
                    <h2>系统日志</h2>
                    <button onclick="this.getRootNode().host.clearLogs()">清空日志</button>
                </div>
                
                <div style="background-color: var(--bg-color); border: 1px solid var(--border-color); padding: 12px; max-height: 300px; overflow-y: auto; font-family: 'Courier New', monospace; font-size: 13px;">
                    ${this.logs.length === 0 ? `
                        <p style="color: var(--text-secondary);">暂无日志</p>
                    ` : ''}
                    
                    ${this.logs.map(log => `
                        <div style="margin-bottom: 8px; display: flex; gap: 12px;">
                            <span style="color: var(--text-secondary);">${log.timestamp}</span>
                            <span style="color: ${
                                log.type === 'success' ? 'var(--success-color)' :
                                log.type === 'error' ? 'var(--error-color)' :
                                log.type === 'warning' ? 'var(--warning-color)' :
                                'var(--text-primary)'
                            };">${log.message}</span>
                        </div>
                    `).join('')}
                </div>
            </div>
        `;
    }
}

customElements.define('monitor-panel', MonitorPanel);

