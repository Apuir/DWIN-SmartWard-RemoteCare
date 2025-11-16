/***
 * @file device_discovery.go
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 设备发现服务（mDNS + TCP扫描）
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath device_discovery.go
 * @projectType Backend
 */

package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"sync"
	"time"
)

// ==================== 设备信息结构 ====================
type Device struct {
	Name      string    `json:"name"`
	IP        string    `json:"ip"`
	Port      int       `json:"port"`
	Type      string    `json:"type"`
	LastSeen  time.Time `json:"last_seen"`
	Online    bool      `json:"online"`
	IsESP32   bool      `json:"is_esp32"`   // 是否确认为ESP32设备
	Manual    bool      `json:"manual"`     // 是否手动添加
}

// ==================== 设备发现服务 ====================
type DeviceDiscovery struct {
	devices    map[string]*Device
	mu         sync.RWMutex
	scanTicker *time.Ticker
	ctx        context.Context
	cancel     context.CancelFunc
	paused     bool           // 扫描是否暂停
	pauseMu    sync.RWMutex   // 暂停状态锁
}

// ==================== 创建设备发现服务 ====================
func NewDeviceDiscovery() *DeviceDiscovery {
	ctx, cancel := context.WithCancel(context.Background())
	return &DeviceDiscovery{
		devices:    make(map[string]*Device),
		scanTicker: time.NewTicker(10 * time.Second),
		ctx:        ctx,
		cancel:     cancel,
	}
}

// ==================== 启动设备发现 ====================
func (d *DeviceDiscovery) Start() {
	log.Println("Device discovery service started")
	
	// 立即执行一次扫描
	d.scanNetwork()
	
	// 定期扫描
	for {
		select {
		case <-d.ctx.Done():
			log.Println("Device discovery service stopped")
			return
		case <-d.scanTicker.C:
			// 检查是否暂停
			d.pauseMu.RLock()
			isPaused := d.paused
			d.pauseMu.RUnlock()
			
			if !isPaused {
				d.scanNetwork()
			} else {
				log.Println("Device scanning paused (audio streaming in progress)")
			}
		}
	}
}

// ==================== 停止设备发现 ====================
func (d *DeviceDiscovery) Stop() {
	d.scanTicker.Stop()
	d.cancel()
}

// ==================== 暂停设备扫描 ====================
func (d *DeviceDiscovery) Pause() {
	d.pauseMu.Lock()
	defer d.pauseMu.Unlock()
	
	if !d.paused {
		d.paused = true
		log.Println("Device scanning paused (audio streaming started)")
	}
}

// ==================== 恢复设备扫描 ====================
func (d *DeviceDiscovery) Resume() {
	d.pauseMu.Lock()
	defer d.pauseMu.Unlock()
	
	if d.paused {
		d.paused = false
		log.Println("Device scanning resumed (audio streaming ended)")
	}
}

// ==================== 扫描网络 ====================
func (d *DeviceDiscovery) scanNetwork() {
	// 获取本机IP地址
	localIP := getLocalIP()
	if localIP == "" {
		log.Println("Failed to get local IP address")
		return
	}
	
	log.Printf("Scanning network from local IP: %s", localIP)
	
	// 扫描同一网段
	ip := net.ParseIP(localIP)
	if ip == nil {
		log.Println("Failed to parse local IP")
		return
	}
	
	// 计算网段
	mask := net.IPv4Mask(255, 255, 255, 0)
	network := ip.Mask(mask)
	
	log.Printf("Scanning network segment: %d.%d.%d.0/24", network[0], network[1], network[2])
	
	// 并发扫描
	var wg sync.WaitGroup
	semaphore := make(chan struct{}, 50) // 限制并发数
	foundCount := 0
	
	for i := 1; i < 255; i++ {
		wg.Add(1)
		go func(hostNum int) {
			defer wg.Done()
			semaphore <- struct{}{}
			defer func() { <-semaphore }()
			
			targetIP := fmt.Sprintf("%d.%d.%d.%d", network[0], network[1], network[2], hostNum)
			
			// 只检查端口是否开放，不尝试识别设备（避免ESP32崩溃）
			if d.checkHostOnlineOnly(targetIP, 8080) {
				foundCount++
			}
		}(i)
	}
	
	wg.Wait()
	
	// 清理离线设备（排除手动添加的）
	d.cleanupOfflineDevices()
	
	log.Printf("Network scan completed, found %d online hosts, %d total devices", foundCount, len(d.devices))
}

// ==================== 只检查主机是否在线（不发送命令） ====================
func (d *DeviceDiscovery) checkHostOnlineOnly(ip string, port int) bool {
	address := fmt.Sprintf("%s:%d", ip, port)
	conn, err := net.DialTimeout("tcp", address, 1*time.Second)
	if err != nil {
		return false
	}
	conn.Close()
	
	// 添加为未知设备（不尝试识别类型，避免崩溃）
	d.mu.Lock()
	if _, exists := d.devices[ip]; !exists {
		d.devices[ip] = &Device{
			Name:     "Device (port 8080)",
			IP:       ip,
			Port:     port,
			Type:     "Unknown",
			LastSeen: time.Now(),
			Online:   true,
			IsESP32:  false,
			Manual:   false,
		}
		log.Printf("Found device at %s (port %d open)", ip, port)
	} else {
		// 更新已存在设备的时间戳
		d.devices[ip].LastSeen = time.Now()
		d.devices[ip].Online = true
	}
	d.mu.Unlock()
	
	return true
}

// ==================== 探测设备 ====================
func (d *DeviceDiscovery) probeDevice(ip string, port int) {
	address := fmt.Sprintf("%s:%d", ip, port)
	
	// 先检查是否已存在且最近刚探测过（避免重复探测）
	d.mu.Lock()
	if device, exists := d.devices[ip]; exists {
		if time.Since(device.LastSeen) < 5*time.Second {
			d.mu.Unlock()
			return
		}
	}
	d.mu.Unlock()
	
	conn, err := net.DialTimeout("tcp", address, 3*time.Second)
	if err != nil {
		// 主机在线但连接失败，添加为未知设备
		d.mu.Lock()
		if _, exists := d.devices[ip]; !exists {
			d.devices[ip] = &Device{
				Name:     "Unknown Device",
				IP:       ip,
				Port:     port,
				Type:     "Unknown",
				LastSeen: time.Now(),
				Online:   true,
				IsESP32:  false,
				Manual:   false,
			}
			log.Printf("Found unknown device at %s (connection failed)", ip)
		} else {
			// 更新已存在设备的时间戳
			d.devices[ip].LastSeen = time.Now()
			d.devices[ip].Online = true
		}
		d.mu.Unlock()
		return
	}
	defer conn.Close()
	
	// 给ESP32一点时间准备
	time.Sleep(100 * time.Millisecond)
	
	// 发送设备发现命令 0xA6
	_, err = conn.Write([]byte{0xA6})
	if err != nil {
		log.Printf("Failed to send discovery command to %s: %v", ip, err)
		d.updateDeviceAsUnknown(ip, port)
		return
	}
	
	// 设置较长的读取超时（ESP32可能需要时间处理）
	conn.SetReadDeadline(time.Now().Add(5 * time.Second))
	
	// 读取响应
	buf := make([]byte, 64)
	n, err := conn.Read(buf)
	if err != nil {
		log.Printf("No valid response from %s: %v (may not be ESP32)", ip, err)
		d.updateDeviceAsUnknown(ip, port)
		return
	}
	
	if n < 1 {
		log.Printf("Empty response from %s", ip)
		d.updateDeviceAsUnknown(ip, port)
		return
	}
	
	// 检查响应码 0xD5
	if buf[0] == 0xD5 && n > 1 {
		deviceName := string(buf[1:n])
		log.Printf("✓ Found ESP32 device: %s at %s", deviceName, ip)
		
		d.mu.Lock()
		d.devices[ip] = &Device{
			Name:     deviceName,
			IP:       ip,
			Port:     port,
			Type:     "ESP32-S3",
			LastSeen: time.Now(),
			Online:   true,
			IsESP32:  true,
			Manual:   false,
		}
		d.mu.Unlock()
	} else {
		log.Printf("Unexpected response from %s: 0x%02X (length: %d)", ip, buf[0], n)
		d.updateDeviceAsUnknown(ip, port)
	}
}

// ==================== 更新设备为未知类型 ====================
func (d *DeviceDiscovery) updateDeviceAsUnknown(ip string, port int) {
	d.mu.Lock()
	defer d.mu.Unlock()
	
	if device, exists := d.devices[ip]; exists {
		// 保留设备，只更新时间戳
		device.LastSeen = time.Now()
		device.Online = true
		// 不改变IsESP32标志，可能之前识别成功过
	} else {
		// 添加为新的未知设备
		d.devices[ip] = &Device{
			Name:     "Device (port 8080)",
			IP:       ip,
			Port:     port,
			Type:     "Unknown",
			LastSeen: time.Now(),
			Online:   true,
			IsESP32:  false,
			Manual:   false,
		}
	}
}

// ==================== 清理离线设备 ====================
func (d *DeviceDiscovery) cleanupOfflineDevices() {
	d.mu.Lock()
	defer d.mu.Unlock()
	
	timeout := 30 * time.Second
	now := time.Now()
	
	for ip, device := range d.devices {
		// 手动添加的设备不自动标记为离线
		if device.Manual {
			continue
		}
		
		if now.Sub(device.LastSeen) > timeout {
			if device.Online {
				log.Printf("Device %s (%s) marked as offline", device.Name, ip)
				device.Online = false
			}
			// 离线设备保留60秒后删除（给它重新上线的机会）
			if now.Sub(device.LastSeen) > 60*time.Second && !device.IsESP32 {
				log.Printf("Removing stale device: %s (%s)", device.Name, ip)
				delete(d.devices, ip)
			}
		}
	}
}

// ==================== 手动添加设备 ====================
func (d *DeviceDiscovery) AddDevice(ip string, port int, name string) error {
	// 验证IP格式
	if net.ParseIP(ip) == nil {
		return fmt.Errorf("invalid IP address: %s", ip)
	}
	
	// 尝试连接验证
	address := fmt.Sprintf("%s:%d", ip, port)
	conn, err := net.DialTimeout("tcp", address, 3*time.Second)
	if err != nil {
		return fmt.Errorf("failed to connect to %s: %v", address, err)
	}
	conn.Close()
	
	d.mu.Lock()
	defer d.mu.Unlock()
	
	if name == "" {
		name = "Manual Device"
	}
	
	d.devices[ip] = &Device{
		Name:     name,
		IP:       ip,
		Port:     port,
		Type:     "Manual",
		LastSeen: time.Now(),
		Online:   true,
		IsESP32:  false,
		Manual:   true,
	}
	
	log.Printf("Manually added device: %s at %s:%d", name, ip, port)
	return nil
}

// ==================== 删除设备 ====================
func (d *DeviceDiscovery) RemoveDevice(ip string) {
	d.mu.Lock()
	defer d.mu.Unlock()
	
	if device, exists := d.devices[ip]; exists {
		log.Printf("Removing device: %s at %s", device.Name, ip)
		delete(d.devices, ip)
	}
}

// ==================== 获取设备列表 ====================
func (d *DeviceDiscovery) GetDevices() []*Device {
	d.mu.RLock()
	defer d.mu.RUnlock()
	
	devices := make([]*Device, 0, len(d.devices))
	for _, device := range d.devices {
		devices = append(devices, device)
	}
	
	return devices
}

// ==================== 获取本机IP ====================
func getLocalIP() string {
	addrs, err := net.InterfaceAddrs()
	if err != nil {
		return ""
	}
	
	for _, addr := range addrs {
		if ipnet, ok := addr.(*net.IPNet); ok && !ipnet.IP.IsLoopback() {
			if ipnet.IP.To4() != nil {
				return ipnet.IP.String()
			}
		}
	}
	
	return ""
}

