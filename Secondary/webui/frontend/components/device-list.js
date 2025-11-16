/***
 * @file device-list.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 设备列表Web Component
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath device-list.js
 * @projectType Frontend
 */

class DeviceList extends HTMLElement {
    constructor() {
        super();
        this.devices = [];
        this.selectedDevice = null;
    }

    connectedCallback() {
        // 只设置一次事件监听器（事件委托不需要重复设置）
        if (!this.eventListenerSetup) {
            this.setupEventListeners();
            this.eventListenerSetup = true;
        }
        this.render();
        this.startPolling();
    }
    
    // ==================== 设置事件监听器 ====================
    setupEventListeners() {
        // 使用事件委托处理按钮点击（只设置一次）
        this.addEventListener('click', (e) => {
            const target = e.target;
            
            // 连接按钮
            if (target.classList.contains('btn-connect')) {
                console.log('Connect button clicked');
                const deviceData = target.getAttribute('data-device');
                console.log('Device data:', deviceData);
                if (deviceData) {
                    try {
                        const device = JSON.parse(deviceData);
                        this.connectDevice(device);
                    } catch (error) {
                        console.error('Failed to parse device data:', error);
                    }
                }
            }
            
            // 断开按钮
            if (target.classList.contains('btn-disconnect')) {
                console.log('Disconnect button clicked');
                this.disconnectDevice();
            }
            
            // 删除按钮
            if (target.classList.contains('btn-remove')) {
                console.log('Remove button clicked');
                const ip = target.getAttribute('data-ip');
                if (ip) {
                    this.removeDevice(ip);
                }
            }
            
            // 手动添加按钮
            if (target.classList.contains('btn-add-manual')) {
                console.log('Add manual button clicked');
                this.showAddDeviceDialog();
            }
        });
    }

    disconnectedCallback() {
        if (this.pollInterval) {
            clearInterval(this.pollInterval);
        }
    }

    // ==================== 开始轮询设备列表 ====================
    startPolling() {
        this.fetchDevices();
        this.pollInterval = setInterval(() => {
            this.fetchDevices();
        }, 5000);
    }

    // ==================== 获取设备列表 ====================
    async fetchDevices() {
        try {
            const response = await fetch('http://localhost:8088/api/devices');
            const result = await response.json();
            
            if (result.success) {
                this.devices = result.data || [];
                this.render();
            }
        } catch (error) {
            console.error('Failed to fetch devices:', error);
        }
    }

    // ==================== 连接设备 ====================
    async connectDevice(device) {
        console.log('Connecting to device:', device);
        try {
            const response = await fetch('http://localhost:8088/api/connect', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    ip: device.ip,
                    port: device.port
                })
            });

            console.log('Response status:', response.status);
            const result = await response.json();
            console.log('Response data:', result);
            
            if (result.success) {
                this.selectedDevice = device;
                this.dispatchEvent(new CustomEvent('device-connected', {
                    detail: device,
                    bubbles: true,
                    composed: true
                }));
                this.render();
                alert('连接成功！');
            } else {
                alert('连接失败: ' + result.message);
            }
        } catch (error) {
            console.error('Failed to connect device:', error);
            alert('连接失败: ' + error.message);
        }
    }

    // ==================== 断开设备 ====================
    async disconnectDevice() {
        try {
            await fetch('http://localhost:8088/api/disconnect', {
                method: 'POST'
            });

            this.selectedDevice = null;
            this.dispatchEvent(new CustomEvent('device-disconnected', {
                bubbles: true,
                composed: true
            }));
            this.render();
        } catch (error) {
            console.error('Failed to disconnect device:', error);
        }
    }

    // ==================== 显示手动添加设备对话框 ====================
    showAddDeviceDialog() {
        const ip = prompt('请输入设备IP地址:', '192.168.43.');
        if (!ip) return;
        
        const port = prompt('请输入端口号:', '8080');
        if (!port) return;
        
        const name = prompt('请输入设备名称（可选）:', 'ESP32 Device');
        
        this.addDevice(ip, parseInt(port), name);
    }

    // ==================== 手动添加设备 ====================
    async addDevice(ip, port, name) {
        try {
            const response = await fetch('http://localhost:8088/api/devices/add', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ ip, port, name })
            });

            const result = await response.json();
            
            if (result.success) {
                alert('设备添加成功！');
                this.fetchDevices();
            } else {
                alert('添加失败: ' + result.message);
            }
        } catch (error) {
            console.error('Failed to add device:', error);
            alert('添加失败: ' + error.message);
        }
    }

    // ==================== 删除设备 ====================
    async removeDevice(ip) {
        if (!confirm('确定要删除这个设备吗？')) {
            return;
        }

        try {
            await fetch('http://localhost:8088/api/devices/remove', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ ip })
            });

            this.fetchDevices();
        } catch (error) {
            console.error('Failed to remove device:', error);
        }
    }

    // ==================== 渲染组件 ====================
    render() {
        const esp32Devices = this.devices.filter(d => d.is_esp32);
        const otherDevices = this.devices.filter(d => !d.is_esp32);
        
        this.innerHTML = `
            <div class="card">
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 16px;">
                    <h2>设备列表</h2>
                    <button class="btn-add-manual" style="font-size: 14px;">
                        手动添加
                    </button>
                </div>
                
                ${this.selectedDevice ? `
                    <div class="mb-4">
                        <div class="device-item" style="border-color: var(--success-color); background-color: #f0fdf4;">
                            <div class="device-info">
                                <div class="device-name">[已连接] ${this.selectedDevice.name}</div>
                                <div class="device-ip">${this.selectedDevice.ip}:${this.selectedDevice.port}</div>
                            </div>
                            <button class="danger btn-disconnect">
                                断开
                            </button>
                        </div>
                    </div>
                ` : ''}
                
                <div class="device-list">
                    ${this.devices.length === 0 ? `
                        <div style="text-align: center; padding: 20px;">
                            <p style="color: var(--text-secondary); margin-bottom: 12px;">正在搜索设备...</p>
                            <p style="font-size: 12px; color: var(--text-secondary);">
                                如果长时间未发现设备，请点击"手动添加"按钮
                            </p>
                        </div>
                    ` : ''}
                    
                    ${esp32Devices.length > 0 ? `
                        <div style="margin-bottom: 12px;">
                            <h3 style="font-size: 14px; color: var(--text-secondary); margin-bottom: 8px;">ESP32设备</h3>
                            ${esp32Devices.filter(d => d.online && (!this.selectedDevice || d.ip !== this.selectedDevice.ip)).map(device => `
                                <div class="device-item" style="border-color: var(--success-color);">
                                    <div class="device-info">
                                        <div class="device-name">${device.name}</div>
                                        <div class="device-ip">${device.ip}:${device.port}</div>
                                        <div style="font-size: 12px; color: var(--success-color);">在线 • ${device.type}</div>
                                    </div>
                                    <div style="display: flex; gap: 8px;">
                                        <button class="btn-connect" data-device='${JSON.stringify(device)}'>
                                            连接
                                        </button>
                                    </div>
                                </div>
                            `).join('')}
                        </div>
                    ` : ''}
                    
                    ${otherDevices.filter(d => d.online).length > 0 ? `
                        <div style="margin-bottom: 12px;">
                            <h3 style="font-size: 14px; color: var(--text-secondary); margin-bottom: 8px;">其他设备 (端口8080)</h3>
                            ${otherDevices.filter(d => d.online && (!this.selectedDevice || d.ip !== this.selectedDevice.ip)).map(device => `
                                <div class="device-item">
                                    <div class="device-info">
                                        <div class="device-name">${device.name}</div>
                                        <div class="device-ip">${device.ip}:${device.port}</div>
                                        <div style="font-size: 12px; color: var(--text-secondary);">
                                            在线 • ${device.manual ? '手动添加' : device.type}
                                        </div>
                                    </div>
                                    <div style="display: flex; gap: 8px;">
                                        <button class="btn-connect" data-device='${JSON.stringify(device)}'>
                                            连接
                                        </button>
                                        ${device.manual ? `
                                            <button class="danger btn-remove" style="padding: 8px 12px;" data-ip="${device.ip}">
                                                删除
                                            </button>
                                        ` : ''}
                                    </div>
                                </div>
                            `).join('')}
                        </div>
                    ` : ''}
                    
                    ${this.devices.filter(d => !d.online).length > 0 ? `
                        <div>
                            <h3 style="font-size: 14px; color: var(--text-secondary); margin-bottom: 8px;">离线设备</h3>
                            ${this.devices.filter(d => !d.online).map(device => `
                                <div class="device-item" style="opacity: 0.5;">
                                    <div class="device-info">
                                        <div class="device-name">${device.name}</div>
                                        <div class="device-ip">${device.ip}:${device.port}</div>
                                        <div style="font-size: 12px; color: var(--text-secondary);">离线</div>
                                    </div>
                                    ${device.manual ? `
                                        <button class="danger btn-remove" data-ip="${device.ip}">
                                            删除
                                        </button>
                                    ` : ''}
                                </div>
                            `).join('')}
                        </div>
                    ` : ''}
                </div>
            </div>
        `;
    }
}

customElements.define('device-list', DeviceList);

