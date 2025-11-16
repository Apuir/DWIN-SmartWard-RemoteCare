/***
 * @file handlers.go
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief HTTP请求处理器
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath handlers.go
 * @projectType Backend
 */

package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

// ==================== 全局连接管理 ====================
var (
	esp32Conn   net.Conn
	esp32ConnMu sync.Mutex
	
	// 温度警告订阅者
	temperatureSubscribers   = make(map[chan TemperatureAlert]bool)
	temperatureSubscribersMu sync.RWMutex
	
	// 设备发现服务引用
	globalDiscoveryService *DeviceDiscovery
	discoveryServiceMu     sync.RWMutex
)

// ==================== 温度警告结构 ====================
type TemperatureAlert struct {
	Type        string  `json:"type"`        // "threshold1", "threshold2", "normal"
	Threshold   int     `json:"threshold"`   // 温度阈值
	Temperature float64 `json:"temperature"` // 实际温度值
	Message     string  `json:"message"`     // 警告消息
	Timestamp   int64   `json:"timestamp"`   // 时间戳
}

// ==================== 响应结构 ====================
type Response struct {
	Success bool        `json:"success"`
	Message string      `json:"message"`
	Data    interface{} `json:"data,omitempty"`
}

// ==================== 设置设备发现服务引用 ====================
func setDiscoveryService(ds *DeviceDiscovery) {
	discoveryServiceMu.Lock()
	defer discoveryServiceMu.Unlock()
	globalDiscoveryService = ds
}

// ==================== 获取设备发现服务引用 ====================
func getDiscoveryService() *DeviceDiscovery {
	discoveryServiceMu.RLock()
	defer discoveryServiceMu.RUnlock()
	return globalDiscoveryService
}

// ==================== 获取设备列表 ====================
func handleDeviceList(discovery *DeviceDiscovery) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		
		devices := discovery.GetDevices()
		
		// 按照ESP32设备优先排序
		esp32Devices := []*Device{}
		otherDevices := []*Device{}
		
		for _, device := range devices {
			if device.IsESP32 {
				esp32Devices = append(esp32Devices, device)
			} else {
				otherDevices = append(otherDevices, device)
			}
		}
		
		// 合并列表，ESP32设备在前
		allDevices := append(esp32Devices, otherDevices...)
		
		response := Response{
			Success: true,
			Message: "Device list retrieved",
			Data:    allDevices,
		}
		
		json.NewEncoder(w).Encode(response)
	}
}

// ==================== 手动添加设备 ====================
func handleDeviceAdd(discovery *DeviceDiscovery) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		
		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		
		var req struct {
			IP   string `json:"ip"`
			Port int    `json:"port"`
			Name string `json:"name"`
		}
		
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			response := Response{
				Success: false,
				Message: "Invalid request body",
			}
			json.NewEncoder(w).Encode(response)
			return
		}
		
		if req.Port == 0 {
			req.Port = 8080
		}
		
		err := discovery.AddDevice(req.IP, req.Port, req.Name)
		if err != nil {
			log.Printf("Failed to add device %s: %v", req.IP, err)
			response := Response{
				Success: false,
				Message: "Failed to add device: " + err.Error(),
			}
			json.NewEncoder(w).Encode(response)
			return
		}
		
		response := Response{
			Success: true,
			Message: "Device added successfully",
			Data: map[string]interface{}{
				"ip":   req.IP,
				"port": req.Port,
				"name": req.Name,
			},
		}
		
		json.NewEncoder(w).Encode(response)
	}
}

// ==================== 删除设备 ====================
func handleDeviceRemove(discovery *DeviceDiscovery) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		
		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		
		var req struct {
			IP string `json:"ip"`
		}
		
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			response := Response{
				Success: false,
				Message: "Invalid request body",
			}
			json.NewEncoder(w).Encode(response)
			return
		}
		
		discovery.RemoveDevice(req.IP)
		
		response := Response{
			Success: true,
			Message: "Device removed successfully",
		}
		
		json.NewEncoder(w).Encode(response)
	}
}

// ==================== 连接设备 ====================
func handleDeviceConnect(w http.ResponseWriter, r *http.Request) {
	log.Printf("handleDeviceConnect called from %s", r.RemoteAddr)
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		log.Printf("Invalid method: %s", r.Method)
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	var req struct {
		IP   string `json:"ip"`
		Port int    `json:"port"`
	}
	
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		log.Printf("Failed to decode request body: %v", err)
		response := Response{
			Success: false,
			Message: "Invalid request body",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	log.Printf("Connecting to device: %s:%d", req.IP, req.Port)
	
	// 关闭现有连接
	esp32ConnMu.Lock()
	if esp32Conn != nil {
		esp32Conn.Close()
		esp32Conn = nil
	}
	esp32ConnMu.Unlock()
	
	// 建立新连接
	address := fmt.Sprintf("%s:%d", req.IP, req.Port)
	if req.Port == 0 {
		address = req.IP + ":8080"
	}
	
	log.Printf("Attempting to connect to %s...", address)
	conn, err := net.DialTimeout("tcp", address, 5*time.Second)
	if err != nil {
		log.Printf("❌ Failed to connect to device %s: %v", address, err)
		response := Response{
			Success: false,
			Message: "Failed to connect to device: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	log.Printf("✓ TCP connection established to %s", address)
	
	// 暂时禁用自动设备识别（避免ESP32重启）
	// TODO: 找到ESP32重启的根本原因后再启用
	deviceInfo := "Connected Device"
	isESP32 := false
	
	log.Printf("Device identification temporarily disabled to prevent ESP32 restart")
	
	esp32ConnMu.Lock()
	esp32Conn = conn
	esp32ConnMu.Unlock()
	
	// 启动温度监控goroutine
	go monitorESP32Temperature(conn)
	
	log.Printf("✓ Successfully connected to device at %s", address)
	log.Printf("Local addr: %s, Remote addr: %s", conn.LocalAddr(), conn.RemoteAddr())
	log.Printf("✓ Temperature monitoring started")
	
	response := Response{
		Success: true,
		Message: "Successfully connected to device",
		Data: map[string]interface{}{
			"address":     address,
			"device_info": deviceInfo,
			"is_esp32":    isESP32,
		},
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 断开设备连接 ====================
func handleDeviceDisconnect(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	esp32ConnMu.Lock()
	defer esp32ConnMu.Unlock()
	
	if esp32Conn != nil {
		esp32Conn.Close()
		esp32Conn = nil
		log.Println("Disconnected from ESP32 device")
	}
	
	response := Response{
		Success: true,
		Message: "Disconnected from device",
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 音频流开始 ====================
func handleAudioStreamStart(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	esp32ConnMu.Lock()
	conn := esp32Conn
	esp32ConnMu.Unlock()
	
	if conn == nil {
		response := Response{
			Success: false,
			Message: "Not connected to ESP32 device",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 暂停设备扫描，减少网络负载
	if ds := getDiscoveryService(); ds != nil {
		ds.Pause()
	}
	
	// 发送音频流开始命令 0xA3
	_, err := conn.Write([]byte{0xA3})
	if err != nil {
		log.Printf("Failed to send stream start command: %v", err)
		// 恢复设备扫描
		if ds := getDiscoveryService(); ds != nil {
			ds.Resume()
		}
		response := Response{
			Success: false,
			Message: "Failed to start audio stream: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	log.Println("Audio stream started")
	
	response := Response{
		Success: true,
		Message: "Audio stream started",
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 音频流数据 ====================
func handleAudioStreamData(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	esp32ConnMu.Lock()
	conn := esp32Conn
	esp32ConnMu.Unlock()
	
	if conn == nil {
		http.Error(w, "Not connected to ESP32 device", http.StatusBadRequest)
		return
	}
	
	// 读取音频数据
	audioData, err := io.ReadAll(r.Body)
	if err != nil {
		log.Printf("Failed to read audio data: %v", err)
		http.Error(w, "Failed to read audio data", http.StatusBadRequest)
		return
	}
	
	// 发送音频流数据命令 0xA4 + 数据
	packet := make([]byte, len(audioData)+1)
	packet[0] = 0xA4
	copy(packet[1:], audioData)
	
	_, err = conn.Write(packet)
	if err != nil {
		log.Printf("Failed to send audio data: %v", err)
		http.Error(w, "Failed to send audio data", http.StatusInternalServerError)
		return
	}
	
	// 简单的200响应，不需要JSON
	w.WriteHeader(http.StatusOK)
}

// ==================== 音频流结束 ====================
func handleAudioStreamEnd(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	esp32ConnMu.Lock()
	conn := esp32Conn
	esp32ConnMu.Unlock()
	
	if conn == nil {
		response := Response{
			Success: false,
			Message: "Not connected to ESP32 device",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 发送音频流结束命令 0xA5
	_, err := conn.Write([]byte{0xA5})
	if err != nil {
		log.Printf("Failed to send stream end command: %v", err)
		response := Response{
			Success: false,
			Message: "Failed to end audio stream: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 恢复设备扫描
	if ds := getDiscoveryService(); ds != nil {
		ds.Resume()
	}
	
	log.Println("Audio stream ended")
	
	response := Response{
		Success: true,
		Message: "Audio stream ended",
	}
	
	json.NewEncoder(w).Encode(response)
}


// ==================== 停止音频播放 ====================
func handleAudioStop(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	esp32ConnMu.Lock()
	conn := esp32Conn
	esp32ConnMu.Unlock()
	
	if conn == nil {
		response := Response{
			Success: false,
			Message: "Not connected to ESP32 device",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 发送停止命令
	_, err := conn.Write([]byte{0xA0})
	if err != nil {
		response := Response{
			Success: false,
			Message: "Failed to send stop command: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 恢复设备扫描
	if ds := getDiscoveryService(); ds != nil {
		ds.Resume()
	}
	
	log.Println("Audio playback stopped")
	
	response := Response{
		Success: true,
		Message: "Audio playback stopped",
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 获取状态 ====================
func handleStatus(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	esp32ConnMu.Lock()
	conn := esp32Conn
	esp32ConnMu.Unlock()
	
	connected := conn != nil
	
	response := Response{
		Success: true,
		Message: "Status retrieved",
		Data: map[string]interface{}{
			"connected": connected,
			"timestamp": time.Now().Unix(),
		},
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 广播温度警告 ====================
func broadcastTemperatureAlert(alert TemperatureAlert) {
	temperatureSubscribersMu.RLock()
	defer temperatureSubscribersMu.RUnlock()
	
	for ch := range temperatureSubscribers {
		select {
		case ch <- alert:
		default:
			// 如果channel已满，跳过
		}
	}
}

// ==================== 监听ESP32温度数据 ====================
func monitorESP32Temperature(conn net.Conn) {
	log.Println("Started ESP32 temperature monitoring")
	buffer := make([]byte, 3)  // 修改为3字节：命令 + 温度高字节 + 温度低字节
	
	for {
		conn.SetReadDeadline(time.Now().Add(30 * time.Second))
		n, err := conn.Read(buffer)
		if err != nil {
			log.Printf("ESP32 monitoring stopped: %v", err)
			return
		}
		
		if n >= 3 {
			cmd := buffer[0]
			tempValue := (uint16(buffer[1]) << 8) | uint16(buffer[2])  // 组合温度值
			temperature := float64(tempValue) / 10.0                     // 转换为实际温度（0.1°C精度）
			
			var alert TemperatureAlert
			alert.Timestamp = time.Now().Unix()
			alert.Temperature = temperature
			
			switch cmd {
			case 0xD1: // RESP_THRESHOLD1_REACHED (修正：0xF1->0xD1)
				alert.Type = "threshold1"
				alert.Threshold = 28
				alert.Message = fmt.Sprintf("温度已达到 %.1f°C，已启动风扇（阈值: 28°C）", temperature)
				log.Printf("Temperature Alert: Threshold 1 reached at %.1f°C", temperature)
				broadcastTemperatureAlert(alert)
				
			case 0xD2: // RESP_THRESHOLD2_REACHED (修正：0xF2->0xD2)
				alert.Type = "threshold2"
				alert.Threshold = 35
				alert.Message = fmt.Sprintf("温度已达到 %.1f°C，蜂鸣器报警（阈值: 35°C）", temperature)
				log.Printf("Temperature Alert: Threshold 2 reached at %.1f°C", temperature)
				broadcastTemperatureAlert(alert)
				
			case 0xD0: // RESP_TEMP_NORMAL
				alert.Type = "normal"
				alert.Threshold = 0
				alert.Message = fmt.Sprintf("温度已恢复正常（当前: %.1f°C）", temperature)
				log.Printf("Temperature Status: Returned to normal at %.1f°C", temperature)
				broadcastTemperatureAlert(alert)
				
			case 0xD3: // RESP_TEMP_UPDATE (定期温度更新)
				alert.Type = "update"
				alert.Threshold = 0
				alert.Message = fmt.Sprintf("当前温度: %.1f°C", temperature)
				log.Printf("Temperature Update: %.1f°C", temperature)
				broadcastTemperatureAlert(alert)
			}
		}
	}
}

// ==================== 温度警告SSE端点 ====================
func handleTemperatureEvents(w http.ResponseWriter, r *http.Request) {
	// 设置SSE头部
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")
	w.Header().Set("Connection", "keep-alive")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	
	// 创建订阅channel
	alertChan := make(chan TemperatureAlert, 10)
	
	// 注册订阅者
	temperatureSubscribersMu.Lock()
	temperatureSubscribers[alertChan] = true
	temperatureSubscribersMu.Unlock()
	
	// 清理函数
	defer func() {
		temperatureSubscribersMu.Lock()
		delete(temperatureSubscribers, alertChan)
		temperatureSubscribersMu.Unlock()
		close(alertChan)
	}()
	
	// 发送初始连接成功消息
	fmt.Fprintf(w, "data: {\"type\":\"connected\",\"message\":\"温度监控已连接\"}\n\n")
	if f, ok := w.(http.Flusher); ok {
		f.Flush()
	}
	
	// 持续发送温度警告
	for {
		select {
		case alert := <-alertChan:
			data, err := json.Marshal(alert)
			if err != nil {
				continue
			}
			fmt.Fprintf(w, "data: %s\n\n", data)
			if f, ok := w.(http.Flusher); ok {
				f.Flush()
			}
			
		case <-r.Context().Done():
			return
		}
	}
}

// ==================== TTS 合成状态 ====================
var (
	ttsStatus       = "idle" // idle, processing, completed, error
	ttsStatusMu     sync.RWMutex
	ttsResultPath   string
	ttsErrorMessage string
)

// ==================== TTS 合成请求 ====================
func handleTTSSynthesize(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	// 检查当前状态
	ttsStatusMu.Lock()
	if ttsStatus == "processing" {
		ttsStatusMu.Unlock()
		response := Response{
			Success: false,
			Message: "TTS synthesis already in progress",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	ttsStatus = "processing"
	ttsErrorMessage = ""
	ttsResultPath = ""
	ttsStatusMu.Unlock()
	
	// 解析multipart form
	err := r.ParseMultipartForm(32 << 20) // 32MB max
	if err != nil {
		log.Printf("Failed to parse multipart form: %v", err)
		ttsStatusMu.Lock()
		ttsStatus = "error"
		ttsErrorMessage = "Failed to parse form data"
		ttsStatusMu.Unlock()
		
		response := Response{
			Success: false,
			Message: "Failed to parse form data: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 获取参数
	targetText := r.FormValue("text")
	promptText := r.FormValue("prompt_text")
	
	if targetText == "" {
		ttsStatusMu.Lock()
		ttsStatus = "error"
		ttsErrorMessage = "Target text is required"
		ttsStatusMu.Unlock()
		
		response := Response{
			Success: false,
			Message: "Target text is required",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 获取上传的音频文件
	file, handler, err := r.FormFile("prompt_audio")
	var promptAudioPath string
	
	if err == nil && file != nil {
		defer file.Close()
		
		// 创建临时目录
		tmpDir := filepath.Join(os.TempDir(), "tts_temp")
		os.MkdirAll(tmpDir, 0755)
		
		// 保存上传的文件
		promptAudioPath = filepath.Join(tmpDir, handler.Filename)
		dst, err := os.Create(promptAudioPath)
		if err != nil {
			log.Printf("Failed to create temp file: %v", err)
			ttsStatusMu.Lock()
			ttsStatus = "error"
			ttsErrorMessage = "Failed to save prompt audio"
			ttsStatusMu.Unlock()
			
			response := Response{
				Success: false,
				Message: "Failed to save prompt audio: " + err.Error(),
			}
			json.NewEncoder(w).Encode(response)
			return
		}
		defer dst.Close()
		
		_, err = io.Copy(dst, file)
		if err != nil {
			log.Printf("Failed to save prompt audio: %v", err)
			ttsStatusMu.Lock()
			ttsStatus = "error"
			ttsErrorMessage = "Failed to save prompt audio"
			ttsStatusMu.Unlock()
			
			response := Response{
				Success: false,
				Message: "Failed to save prompt audio: " + err.Error(),
			}
			json.NewEncoder(w).Encode(response)
			return
		}
		
		log.Printf("Saved prompt audio to: %s", promptAudioPath)
	}
	
	// 清理上一次的结果文件
	ttsStatusMu.Lock()
	oldResultPath := ttsResultPath
	ttsResultPath = ""
	ttsStatusMu.Unlock()
	
	if oldResultPath != "" {
		os.Remove(oldResultPath)
	}
	
	// 异步调用 VoxCPM 进行合成
	go func() {
		err := callVoxCPMAPI(targetText, promptText, promptAudioPath)
		
		ttsStatusMu.Lock()
		if err != nil {
			ttsStatus = "error"
			ttsErrorMessage = err.Error()
			log.Printf("TTS synthesis failed: %v", err)
		} else {
			ttsStatus = "completed"
			log.Printf("TTS synthesis completed successfully")
		}
		ttsStatusMu.Unlock()
		
		// 清理临时文件
		if promptAudioPath != "" {
			os.Remove(promptAudioPath)
		}
	}()
	
	response := Response{
		Success: true,
		Message: "TTS synthesis started",
	}
	json.NewEncoder(w).Encode(response)
}

// ==================== 调用 VoxCPM 合成语音 ====================
func callVoxCPMAPI(text, promptText, promptAudioPath string) error {
	// 获取 Python 脚本路径
	helperPath, err := filepath.Abs(filepath.Join(".", "voxcpm_tts.py"))
	if err != nil {
		return fmt.Errorf("failed to get helper script path: %v", err)
	}
	
	// 获取 VoxCPM 目录
	voxcpmPath := filepath.Join("..", "..", "..", "VoxCPM")
	absVoxcpmPath, err := filepath.Abs(voxcpmPath)
	if err != nil {
		return fmt.Errorf("failed to get VoxCPM path: %v", err)
	}
	
	// 准备输出路径
	tmpDir := filepath.Join(os.TempDir(), "tts_temp")
	os.MkdirAll(tmpDir, 0755)
	resultPath := filepath.Join(tmpDir, fmt.Sprintf("tts_result_%d.wav", time.Now().Unix()))
	
	// 获取参考音频的绝对路径
	absPromptPath := ""
	if promptAudioPath != "" {
		absPromptPath, _ = filepath.Abs(promptAudioPath)
	}
	
	// 构建命令参数
	args := []string{
		"run", 
		"--directory", absVoxcpmPath, 
		"python", 
		helperPath,
		text,
		absPromptPath,
		promptText,
		resultPath,
	}
	
	log.Printf("Calling VoxCPM TTS with text: %s", text[:min(50, len(text))])
	
	// 使用 uv run 执行 Python 脚本
	cmd := exec.Command("uv", args...)
	
	output, err := cmd.CombinedOutput()
	if err != nil {
		return fmt.Errorf("failed to execute TTS: %v, output: %s", err, string(output))
	}
	
	// 解析 JSON 输出
	outputStr := string(output)
	lines := strings.Split(strings.TrimSpace(outputStr), "\n")
	lastLine := lines[len(lines)-1]
	
	var result struct {
		Success bool   `json:"success"`
		Path    string `json:"path"`
		Error   string `json:"error"`
	}
	
	err = json.Unmarshal([]byte(lastLine), &result)
	if err != nil {
		return fmt.Errorf("failed to parse TTS result: %v, output: %s", err, lastLine)
	}
	
	if !result.Success {
		return fmt.Errorf("TTS synthesis failed: %s", result.Error)
	}
	
	// 设置结果路径
	ttsStatusMu.Lock()
	ttsResultPath = result.Path
	ttsStatusMu.Unlock()
	
	log.Printf("TTS result saved to: %s", result.Path)
	
	return nil
}

// 辅助函数：返回较小的整数
func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

// ==================== 获取 TTS 状态 ====================
func handleTTSStatus(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	ttsStatusMu.RLock()
	status := ttsStatus
	errorMsg := ttsErrorMessage
	hasResult := ttsResultPath != ""
	resultPath := ttsResultPath
	ttsStatusMu.RUnlock()
	
	response := Response{
		Success: true,
		Message: "TTS status retrieved",
		Data: map[string]interface{}{
			"status":      status,
			"error":       errorMsg,
			"has_result":  hasResult,
			"result_path": resultPath,
		},
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 下载 TTS 结果 ====================
func handleTTSDownload(w http.ResponseWriter, r *http.Request) {
	ttsStatusMu.RLock()
	resultPath := ttsResultPath
	status := ttsStatus
	ttsStatusMu.RUnlock()
	
	if status != "completed" || resultPath == "" {
		w.Header().Set("Content-Type", "application/json")
		response := Response{
			Success: false,
			Message: "No TTS result available",
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 读取文件
	file, err := os.Open(resultPath)
	if err != nil {
		log.Printf("Failed to open TTS result: %v", err)
		w.Header().Set("Content-Type", "application/json")
		response := Response{
			Success: false,
			Message: "Failed to open TTS result: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	defer file.Close()
	
	// 设置响应头，允许重复下载
	w.Header().Set("Content-Type", "audio/wav")
	w.Header().Set("Content-Disposition", "inline; filename=tts_result.wav")
	w.Header().Set("Cache-Control", "no-cache")
	
	// 发送文件内容
	_, err = io.Copy(w, file)
	if err != nil {
		log.Printf("Failed to send TTS result: %v", err)
	}
	
	// 不再自动重置状态和删除文件，允许重复下载和发送
	// 只有在新的合成开始时才清理旧文件
}

// ==================== 识别参考音频文本 ====================
func handleTTSRecognize(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	
	// 解析 multipart form
	err := r.ParseMultipartForm(32 << 20)
	if err != nil {
		response := Response{
			Success: false,
			Message: "Failed to parse form data: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	// 获取上传的音频文件
	file, handler, err := r.FormFile("audio")
	if err != nil {
		response := Response{
			Success: false,
			Message: "No audio file provided: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	defer file.Close()
	
	// 保存临时文件
	tmpDir := filepath.Join(os.TempDir(), "tts_temp")
	os.MkdirAll(tmpDir, 0755)
	
	audioPath := filepath.Join(tmpDir, handler.Filename)
	dst, err := os.Create(audioPath)
	if err != nil {
		response := Response{
			Success: false,
			Message: "Failed to save audio file: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	defer dst.Close()
	defer os.Remove(audioPath)
	
	_, err = io.Copy(dst, file)
	if err != nil {
		response := Response{
			Success: false,
			Message: "Failed to save audio file: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	dst.Close()
	
	// 调用 VoxCPM ASR API 识别文本
	recognizedText, err := callVoxCPMRecognitionAPI(audioPath)
	if err != nil {
		log.Printf("Failed to recognize audio: %v", err)
		response := Response{
			Success: false,
			Message: "Failed to recognize audio: " + err.Error(),
		}
		json.NewEncoder(w).Encode(response)
		return
	}
	
	response := Response{
		Success: true,
		Message: "Audio recognized successfully",
		Data: map[string]interface{}{
			"text": recognizedText,
		},
	}
	
	json.NewEncoder(w).Encode(response)
}

// ==================== 调用 VoxCPM ASR API ====================
func callVoxCPMRecognitionAPI(audioPath string) (string, error) {
	// 获取绝对路径
	absAudioPath, err := filepath.Abs(audioPath)
	if err != nil {
		return "", fmt.Errorf("failed to get absolute path: %v", err)
	}
	
	// 获取 helper 脚本的绝对路径
	helperPath, err := filepath.Abs(filepath.Join(".", "voxcpm_helper.py"))
	if err != nil {
		return "", fmt.Errorf("failed to get helper script path: %v", err)
	}
	
	// 获取 VoxCPM 目录以使用其环境
	voxcpmPath := filepath.Join("..", "..", "..", "VoxCPM")
	absVoxcpmPath, err := filepath.Abs(voxcpmPath)
	if err != nil {
		return "", fmt.Errorf("failed to get VoxCPM path: %v", err)
	}
	
	// 使用 uv run 以确保使用正确的 Python 环境
	// --directory 设置工作目录到 VoxCPM，这样可以加载模型
	cmd := exec.Command("uv", "run", "--directory", absVoxcpmPath, "python", helperPath, absAudioPath)
	
	output, err := cmd.CombinedOutput()
	if err != nil {
		return "", fmt.Errorf("failed to execute recognition: %v, output: %s", err, string(output))
	}
	
	// 只取最后一行作为 JSON 输出（忽略可能的警告信息）
	outputStr := string(output)
	lines := strings.Split(strings.TrimSpace(outputStr), "\n")
	lastLine := lines[len(lines)-1]
	
	// 解析 JSON 输出
	var result struct {
		Text  string `json:"text"`
		Error string `json:"error"`
	}
	
	err = json.Unmarshal([]byte(lastLine), &result)
	if err != nil {
		return "", fmt.Errorf("failed to parse recognition result: %v, last line: %s, full output: %s", err, lastLine, outputStr)
	}
	
	if result.Error != "" {
		return "", fmt.Errorf("recognition error: %s", result.Error)
	}
	
	return result.Text, nil
}

