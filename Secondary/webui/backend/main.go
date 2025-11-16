/***
 * @file main.go
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief ESP32音频控制Web服务器主程序
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath main.go
 * @projectType Backend
 */

package main

import (
	"flag"
	"log"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"syscall"
	"time"

	"github.com/rs/cors"
)

// ==================== 配置参数 ====================
var (
	httpAddr = flag.String("http", ":8088", "HTTP server address")
	debug    = flag.Bool("debug", false, "Enable debug logging")
	voxcpmCmd *exec.Cmd
)

func main() {
	flag.Parse()

	// ==================== 初始化日志 ====================
	if *debug {
		log.SetFlags(log.LstdFlags | log.Lshortfile)
		log.Println("Debug mode enabled")
	}

	// ==================== 启动 VoxCPM 服务 ====================
	if err := startVoxCPM(); err != nil {
		log.Printf("Warning: Failed to start VoxCPM service: %v", err)
	} else {
		log.Println("VoxCPM service started successfully")
	}

	// ==================== 设置信号处理 ====================
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-sigChan
		log.Println("Shutting down...")
		stopVoxCPM()
		os.Exit(0)
	}()

	// ==================== 初始化设备发现 ====================
	discoveryService := NewDeviceDiscovery()
	setDiscoveryService(discoveryService)  // 设置全局引用
	go discoveryService.Start()

	// ==================== 初始化路由 ====================
	mux := http.NewServeMux()
	
	// 静态文件服务
	fs := http.FileServer(http.Dir("../frontend"))
	mux.Handle("/", fs)
	
	// API路由
	mux.HandleFunc("/api/devices", handleDeviceList(discoveryService))
	mux.HandleFunc("/api/devices/add", handleDeviceAdd(discoveryService))
	mux.HandleFunc("/api/devices/remove", handleDeviceRemove(discoveryService))
	mux.HandleFunc("/api/connect", handleDeviceConnect)
	mux.HandleFunc("/api/disconnect", handleDeviceDisconnect)
	mux.HandleFunc("/api/audio/stream/start", handleAudioStreamStart)
	mux.HandleFunc("/api/audio/stream/data", handleAudioStreamData)
	mux.HandleFunc("/api/audio/stream/end", handleAudioStreamEnd)
	mux.HandleFunc("/api/audio/stop", handleAudioStop)
	mux.HandleFunc("/api/status", handleStatus)
	mux.HandleFunc("/api/temperature/events", handleTemperatureEvents)
	
	// TTS 相关路由
	mux.HandleFunc("/api/tts/synthesize", handleTTSSynthesize)
	mux.HandleFunc("/api/tts/status", handleTTSStatus)
	mux.HandleFunc("/api/tts/download", handleTTSDownload)
	mux.HandleFunc("/api/tts/recognize", handleTTSRecognize)

	// ==================== 请求日志中间件 ====================
	loggingMux := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		log.Printf("[%s] %s %s", r.Method, r.URL.Path, r.RemoteAddr)
		mux.ServeHTTP(w, r)
	})

	// ==================== CORS配置 ====================
	c := cors.New(cors.Options{
		AllowedOrigins:   []string{"*"},
		AllowedMethods:   []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowedHeaders:   []string{"*"},
		AllowCredentials: true,
	})

	handler := c.Handler(loggingMux)

	// ==================== 启动HTTP服务器 ====================
	server := &http.Server{
		Addr:           *httpAddr,
		Handler:        handler,
		ReadTimeout:    30 * time.Second,
		WriteTimeout:   30 * time.Second,
		MaxHeaderBytes: 1 << 20,
	}

	log.Printf("========================================")
	log.Printf("ESP32 Audio Control Web Server")
	log.Printf("Version: 0.2")
	log.Printf("========================================")
	log.Printf("Starting HTTP server on %s", *httpAddr)
	log.Printf("Device discovery service started")
	log.Printf("VoxCPM TTS service integrated")
	log.Printf("========================================")

	if err := server.ListenAndServe(); err != nil {
		log.Fatal("HTTP server error:", err)
	}
}

// ==================== 启动 VoxCPM 服务 ====================
func startVoxCPM() error {
	// 获取 VoxCPM 目录路径
	voxcpmPath := filepath.Join("..", "..", "..", "VoxCPM")
	
	// 检查目录是否存在
	if _, err := os.Stat(voxcpmPath); os.IsNotExist(err) {
		return err
	}
	
	// 使用 uv 启动 VoxCPM 服务
	voxcpmCmd = exec.Command("uv", "run", "python", "app.py")
	voxcpmCmd.Dir = voxcpmPath
	voxcpmCmd.Stdout = os.Stdout
	voxcpmCmd.Stderr = os.Stderr
	
	if err := voxcpmCmd.Start(); err != nil {
		return err
	}
	
	// 等待服务启动（VoxCPM 需要较长时间加载模型）
	log.Println("Waiting for VoxCPM service to start (this may take a while)...")
	time.Sleep(5 * time.Second)
	
	return nil
}

// ==================== 停止 VoxCPM 服务 ====================
func stopVoxCPM() {
	if voxcpmCmd != nil && voxcpmCmd.Process != nil {
		log.Println("Stopping VoxCPM service...")
		voxcpmCmd.Process.Kill()
	}
}

