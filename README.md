# 智能病房监控与远程干预系统

嵌入式多模块协同控制系统，用于病房环境监测与远程语音交互。

## 系统架构

本系统采用分层架构设计，由显示层、控制层和应用层三部分组成：

```
Primary/          DWIN 屏幕界面设计（DGUS 项目）
Secondary/        多模块控制系统
  ├─ C51/         温度传感器采集（STC8 单片机）
  ├─ ESP32/       WiFi 通信与音频播放（ESP-IDF + MAX98357）
  ├─ T5L/         DWIN T5L 屏幕二次开发固件
  └─ webui/       Web 管理界面（Go + JavaScript）
VoxCPM/           语音合成引擎
```

## 主要功能

### 环境监测
- 实时温度采集与显示
- 多点位传感器数据汇总
- 数值异常预警

### 远程控制
- WiFi 无线连接
- Web 界面管理
- 设备状态监控

### 语音交互
- 零样本语音克隆
- 上下文感知语音合成
- 实时语音播报

## 技术栈

### 硬件平台
- **显示终端**: DWIN T5L 串口屏（带二次开发）
- **传感器单元**: STC8 系列单片机
- **通信与音频**: ESP32 + MAX98357 I2S 放大器
- **主控协议**: UART 串口通信

### 软件环境
- **嵌入式开发**: Keil C51, ESP-IDF v5.0+
- **界面设计**: DGUS Studio
- **后端服务**: Go 1.18+
- **前端界面**: 原生 JavaScript
- **语音引擎**: Python 3.10+, PyTorch, uv

## 快速开始

### 前置要求

```bash
# 嵌入式工具链
- Keil uVision 5 (C51 编译器)
- ESP-IDF v5.0+
- DWIN DGUS Designer

# 服务端环境
- Go 1.18+
- Python 3.10+ with uv
```

### 编译与部署

#### 1. DWIN 屏幕程序

```bash
cd Primary/
# 使用 DGUS Studio 打开 DGUS.dxpro 项目
# 生成配置文件到 DWIN_SET/ 目录
# 通过串口烧录至屏幕
```

#### 2. T5L 屏幕固件（可选）

```bash
cd Secondary/T5L/
# 使用 Keil C51 打开 NEW_C__S5_S4_S3_S2_8283_RTC.uvproj
# 编译生成 BIN 文件
# 通过 T5L 工具烧录到屏幕
```

#### 3. C51 温度采集程序

```bash
cd Secondary/C51/
# 使用 Keil 打开 temprature.uvproj
# 编译生成 HEX 文件
# 烧录至 STC8 单片机
```

#### 4. ESP32 固件

```bash
cd Secondary/ESP32/
idf.py menuconfig    # 配置 WiFi SSID/密码
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

#### 5. Web 管理界面

```bash
cd Secondary/webui/backend/
go build -o server main.go
./server

# 前端静态文件通过后端自动服务
# 默认访问地址: http://localhost:8088
```

#### 6. 语音合成服务

```bash
cd VoxCPM/
# 使用 uv 安装依赖并运行
uv sync
uv run python app.py
# Gradio 界面启动于 http://127.0.0.1:7860
```

## 系统集成

系统各模块通过以下方式协同工作：

1. **C51 → DWIN**: UART 协议传输传感器数据到屏幕显示
2. **T5L ↔ ESP32**: UART 串口双向通信
3. **ESP32 → Web**: TCP Socket 连接（端口 8080）
4. **Web ↔ VoxCPM**: HTTP API 调用语音合成（端口 7860）
5. **Web → ESP32 → MAX98357**: 音频数据流式传输与播放

## 配置说明

### WiFi 连接

编辑 `Secondary/ESP32/main/station_example_main.c`:

```c
#define EXAMPLE_ESP_WIFI_SSID      "your_ssid"
#define EXAMPLE_ESP_WIFI_PASS      "your_password"
```

### Web 服务端口

编辑 `Secondary/webui/backend/main.go`:

```go
httpAddr = flag.String("http", ":8088", "HTTP server address")  // 默认 8088
```

或使用命令行参数：

```bash
./server -http=:8080
```

### 语音模型路径

编辑 `VoxCPM/app.py`，或设置环境变量：

```bash
export HF_REPO_ID="openbmb/VoxCPM-0.5B"
# 模型默认从本地 ./model/VoxCPM-0.5B 加载
# 如不存在则自动从 HuggingFace 下载
```

## 目录结构

```
.
├── Primary/                  # 屏幕界面
│   ├── DGUS.dxpro           # DGUS 项目文件
│   ├── DWIN_SET/            # DGUS 编译输出（.bin 配置文件）
│   ├── Images/              # 界面素材
│   └── Pages/               # 页面定义
├── Secondary/               # 控制模块
│   ├── C51/                 # 温度采集单片机
│   │   ├── src/            # 源代码
│   │   └── temprature.uvproj
│   ├── ESP32/              # WiFi 与音频模块
│   │   ├── main/           # 主程序
│   │   ├── components/     # 组件（MAX98357 驱动）
│   │   └── CMakeLists.txt
│   ├── T5L/                # T5L 屏幕二次开发固件
│   │   ├── Core/           # 核心代码
│   │   ├── Hardware/       # 硬件驱动
│   │   └── NEW_C__S5_S4_S3_S2_8283_RTC.uvproj
│   └── webui/              # Web 界面
│       ├── backend/        # Go 后端（端口 8088）
│       └── frontend/       # JS 前端
└── VoxCPM/                 # 语音引擎
    ├── src/voxcpm/         # 核心库
    ├── model/              # 模型存放目录
    ├── pyproject.toml      # uv 依赖配置
    └── app.py              # Gradio 演示界面
```

## 开发指南

### 添加新传感器

1. 在 `Secondary/C51/src/` 中实现传感器驱动
2. 修改 `main.c` 集成数据采集逻辑
3. 更新与 DWIN 的串口通信协议
4. 在 DGUS Studio 中添加显示控件
5. 更新 T5L 固件以支持新数据格式（如需要）

### 扩展 Web API

1. 在 `Secondary/webui/backend/handlers.go` 添加路由
2. 实现对应的处理函数
3. 更新前端调用逻辑

### 自定义语音提示

1. 准备参考音频（3-10秒，16kHz）
2. 使用 VoxCPM 克隆音色
3. 集成到 `webui/backend/voxcpm_tts.py`

## 性能指标

- **温度采集精度**: ±0.5°C (DS18B20)
- **屏幕响应延迟**: <100ms
- **WiFi 连接时间**: <3s
- **TCP 通信端口**: ESP32:8080, Web:8088
- **语音合成 RTF**: 0.17 (RTX 4090下)

## 许可协议

- Apache-2.0 License

## 注意事项

- ESP32 需配置为 Station 模式，mDNS 服务名为 `_esp32temp._tcp`
- DWIN 屏幕与 C51 波特率统一为 115200
- ESP32 TCP 服务端口固定为 8080
- Web 后端默认端口为 8088，可通过 `-http` 参数修改
- VoxCPM 使用 uv 管理依赖，首次运行需下载依赖、模型（约 10GB）
- 语音合成需 GPU 支持，CPU 模式速度慢

## 联系方式

项目问题与建议请提交 Issue。
