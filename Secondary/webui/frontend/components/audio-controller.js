/***
 * @file audio-controller.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 音频控制Web Component
 * 
 * @version 0.2
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath audio-controller.js
 * @projectType Frontend
 */

import { AudioProcessor, MicrophoneProcessor, AUDIO_CONFIG } from '../utils/audio-processor.js';

class AudioController extends HTMLElement {
    constructor() {
        super();
        console.log('AudioController constructor called');
        // 使用私有属性名避免冲突
        this._isConnected = false;
        this._isRecording = false;
        this._isPlaying = false;
        this._audioProcessor = new AudioProcessor();
        this._micProcessor = null;
        this._currentFile = null;
        this._uploadProgress = 0;
    }

    connectedCallback() {
        console.log('AudioController connected to DOM');
        if (!this.eventListenerSetup) {
            this.setupEventListeners();
            this.eventListenerSetup = true;
        }
        this.render();
        console.log('AudioController rendered');
    }

    // ==================== 设置事件监听 ====================
    setupEventListeners() {
        // 监听设备连接状态
        document.addEventListener('device-connected', () => {
            console.log('Audio controller: device connected');
            this._isConnected = true;
            this.render();
        });

        document.addEventListener('device-disconnected', () => {
            console.log('Audio controller: device disconnected');
            this._isConnected = false;
            this.stopRecording();
            this.render();
        });
        
        // 使用事件委托处理按钮点击
        this.addEventListener('click', (e) => {
            const target = e.target;
            
            // 开始录音按钮
            if (target.classList.contains('btn-start-recording')) {
                console.log('Start recording button clicked');
                this.startRecording();
            }
            
            // 停止录音按钮
            if (target.classList.contains('btn-stop-recording')) {
                console.log('Stop recording button clicked');
                this.stopRecording();
            }
            
            // 停止播放按钮
            if (target.classList.contains('btn-stop-playback')) {
                console.log('Stop playback button clicked');
                this.stopPlayback();
            }
        });
        
        // 处理文件选择
        this.addEventListener('change', (e) => {
            if (e.target.type === 'file' && e.target.accept === 'audio/*') {
                this.handleFileSelect(e);
            }
        });
    }

    // ==================== 上传音频文件 ====================
    async uploadAudioFile(file) {
        if (!this._isConnected) {
            alert('请先连接设备');
            return;
        }

        this._currentFile = file;
        this._isPlaying = true;
        this._uploadProgress = 0;
        this.render();

        try {
            // 处理音频文件（解码、重采样、转PCM）
            this.addLog('info', `正在处理音频文件: ${file.name}`);
            const audioData = await this._audioProcessor.processAudioFile(file);
            
            this.addLog('success', `音频处理完成: ${audioData.chunks.length} 个数据包, 总大小 ${(audioData.totalSize / 1024).toFixed(2)} KB`);
            this.addLog('info', `音频参数: ${audioData.sampleRate}Hz, ${audioData.channels}通道, 时长 ${audioData.duration.toFixed(2)}秒`);

            // 发送音频流
            await this.sendAudioStream(audioData.chunks);

            this.addLog('success', '音频文件播放完成');
            this._isPlaying = false;
            this._currentFile = null;
            this.render();

        } catch (error) {
            console.error('Failed to upload audio:', error);
            this.addLog('error', '音频处理失败: ' + error.message);
            this._isPlaying = false;
            this._currentFile = null;
            this.render();
            alert('音频处理失败: ' + error.message);
        }
    }

    // ==================== 发送音频流 ====================
    async sendAudioStream(chunks) {
        const totalChunks = chunks.length;
        
        // 发送开始命令
        await fetch('http://localhost:8088/api/audio/stream/start', {
            method: 'POST'
        });

        this.addLog('info', '开始传输音频数据...');

        // 逐块发送音频数据
        for (let i = 0; i < totalChunks; i++) {
            const chunk = chunks[i];
            
            try {
                await fetch('http://localhost:8088/api/audio/stream/data', {
                    method: 'POST',
                    body: chunk
                });

                // 更新进度
                this._uploadProgress = Math.round(((i + 1) / totalChunks) * 100);
                
                if ((i + 1) % 10 === 0) {
                    this.addLog('info', `传输进度: ${this._uploadProgress}% (${i + 1}/${totalChunks})`);
                }
                
                this.render();

                // 控制发送速率，避免网络拥塞
                await new Promise(resolve => setTimeout(resolve, 10));

            } catch (error) {
                console.error('Failed to send chunk', i, error);
                throw error;
            }
        }

        // 发送结束命令
        await fetch('http://localhost:8088/api/audio/stream/end', {
            method: 'POST'
        });

        this._uploadProgress = 100;
        this.render();
    }

    // ==================== 开始录音 ====================
    async startRecording() {
        if (!this._isConnected) {
            alert('请先连接设备');
            return;
        }

        try {
            this.addLog('info', '启动麦克风...');
            
            // 发送音频流开始命令
            await fetch('http://localhost:8088/api/audio/stream/start', {
                method: 'POST'
            });

            // 创建麦克风处理器
            this._micProcessor = new MicrophoneProcessor();
            
            await this._micProcessor.start(async (pcmData) => {
                // 实时发送PCM数据
                try {
                    await fetch('http://localhost:8088/api/audio/stream/data', {
                        method: 'POST',
                        body: pcmData
                    });
                } catch (error) {
                    console.error('Failed to send microphone data:', error);
                }
            });

            this._isRecording = true;
            this.addLog('success', '麦克风已启动，实时传输中...');
            this.render();

        } catch (error) {
            console.error('Failed to start recording:', error);
            this.addLog('error', '麦克风启动失败: ' + error.message);
            alert('无法访问麦克风: ' + error.message);
        }
    }

    // ==================== 停止录音 ====================
    async stopRecording() {
        if (this._micProcessor) {
            this._micProcessor.stop();
            this._micProcessor = null;
        }

        // 发送音频流结束命令
        try {
            await fetch('http://localhost:8088/api/audio/stream/end', {
                method: 'POST'
            });
        } catch (error) {
            console.error('Failed to send stream end command:', error);
        }

        this._isRecording = false;
        this.addLog('info', '麦克风已停止');
        this.render();
    }

    // ==================== 停止播放 ====================
    async stopPlayback() {
        if (!this._isConnected) {
            return;
        }

        try {
            const response = await fetch('http://localhost:8088/api/audio/stop', {
                method: 'POST'
            });

            const result = await response.json();
            
            if (result.success) {
                console.log('Playback stopped');
            }
        } catch (error) {
            console.error('Failed to stop playback:', error);
        }
    }

    // ==================== 处理文件选择 ====================
    handleFileSelect(event) {
        const file = event.target.files[0];
        if (file) {
            this.uploadAudioFile(file);
        }
        // 清空input，允许重复选择同一文件
        event.target.value = '';
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

    // ==================== 渲染组件 ====================
    render() {
        console.log('AudioController render called, isConnected:', this._isConnected);
        this.innerHTML = `
            <div class="card">
                <h2>音频控制</h2>
                <p style="font-size: 12px; color: var(--text-secondary); margin-bottom: 16px;">
                    状态: ${this._isConnected ? '<span style="color: var(--success-color);">已连接</span>' : '<span style="color: var(--error-color);">未连接</span>'}
                </p>
                
                <div style="display: flex; flex-direction: column; gap: 16px;">
                    <!-- 文件上传 -->
                    <div>
                        <h3 style="font-size: 16px; margin-bottom: 8px;">音频文件</h3>
                        <label class="file-input-label" style="${!this._isConnected || this._isPlaying ? 'opacity: 0.5; cursor: not-allowed;' : ''}">
                            <input type="file" accept="audio/*" ${!this._isConnected || this._isPlaying ? 'disabled' : ''}>
                            ${this._isPlaying ? '处理中...' : '选择音频文件'}
                        </label>
                        ${this._currentFile ? `
                            <p style="font-size: 12px; color: var(--text-primary); margin-top: 4px;">
                                文件: ${this._currentFile.name}
                            </p>
                        ` : ''}
                        ${this._isPlaying && this._uploadProgress > 0 ? `
                            <div style="margin-top: 8px;">
                                <div style="background-color: var(--border-color); height: 8px; border-radius: 4px; overflow: hidden;">
                                    <div style="background-color: var(--primary-color); height: 100%; width: ${this._uploadProgress}%; transition: width 0.3s;"></div>
                                </div>
                                <p style="font-size: 12px; color: var(--text-secondary); margin-top: 4px;">
                                    传输进度: ${this._uploadProgress}%
                                </p>
                            </div>
                        ` : ''}
                        <p style="font-size: 12px; color: var(--text-secondary); margin-top: 4px;">
                            支持 WAV, MP3, OGG 等格式，自动转换为PCM 44.1kHz
                        </p>
                    </div>

                    <!-- 实时录音 - 已禁用 -->
                    <div style="opacity: 0.5; pointer-events: none;">
                        <h3 style="font-size: 16px; margin-bottom: 8px;">实时对讲</h3>
                        <button disabled>
                            开始录音
                        </button>
                        <p style="font-size: 12px; color: var(--text-secondary); margin-top: 4px;">
                            此功能已暂时禁用
                        </p>
                    </div>

                    <!-- 控制按钮 -->
                    <div>
                        <button class="danger btn-stop-playback" ${!this._isConnected ? 'disabled' : ''}>
                            停止播放
                        </button>
                    </div>
                </div>

                ${!this._isConnected ? `
                    <div style="margin-top: 16px; padding: 12px; background-color: #fef3c7; border-left: 3px solid var(--warning-color);">
                        <p style="color: var(--warning-color); font-size: 14px;">警告: 请先连接ESP32设备</p>
                    </div>
                ` : ''}
            </div>
        `;
    }
}

customElements.define('audio-controller', AudioController);

console.log('audio-controller component registered');

