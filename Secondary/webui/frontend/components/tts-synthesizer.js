/***
 * @file tts-synthesizer.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-06
 * @brief TTS 语音合成组件
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-06
 * @filePath tts-synthesizer.js
 * @projectType Frontend
 */

import { AudioProcessor } from '../utils/audio-processor.js';

class TTSSynthesizer extends HTMLElement {
    constructor() {
        super();
        this._isConnected = false;
        this._isSynthesizing = false;
        this._isSending = false;
        this._isRecognizing = false;
        this._promptAudioFile = null;
        this._promptText = '';
        this._targetText = '';
        this._synthesisStatus = 'idle';
        this._errorMessage = '';
        this._audioProcessor = new AudioProcessor();
        this._statusCheckInterval = null;
    }

    connectedCallback() {
        if (!this.eventListenerSetup) {
            this.setupEventListeners();
            this.eventListenerSetup = true;
        }
        this.render();
        // 初始化按钮状态
        this.updateButtonStates();
    }

    disconnectedCallback() {
        if (this._statusCheckInterval) {
            clearInterval(this._statusCheckInterval);
            this._statusCheckInterval = null;
        }
    }

    // ==================== 设置事件监听 ====================
    setupEventListeners() {
        // 监听设备连接状态
        document.addEventListener('device-connected', () => {
            this._isConnected = true;
            this.render();
            this.updateButtonStates();
        });

        document.addEventListener('device-disconnected', () => {
            this._isConnected = false;
            this.render();
            this.updateButtonStates();
        });
        
        // 使用事件委托处理按钮点击
        this.addEventListener('click', (e) => {
            const target = e.target;
            
            if (target.classList.contains('btn-synthesize')) {
                this.startSynthesis();
            }
            
            if (target.classList.contains('btn-send-to-esp32')) {
                this.sendToESP32();
            }
        });
        
        // 处理文件选择
        this.addEventListener('change', (e) => {
            if (e.target.classList.contains('prompt-audio-input')) {
                this.handlePromptAudioSelect(e);
            }
        });
        
        // 处理文本输入
        this.addEventListener('input', (e) => {
            if (e.target.classList.contains('prompt-text-input')) {
                this._promptText = e.target.value;
            }
            
            if (e.target.classList.contains('target-text-input')) {
                this._targetText = e.target.value;
                // 更新按钮状态
                this.updateButtonStates();
            }
        });
    }

    // ==================== 处理参考音频选择 ====================
    async handlePromptAudioSelect(event) {
        const file = event.target.files[0];
        if (file) {
            this._promptAudioFile = file;
            this.addLog('info', `已选择参考音频: ${file.name}`);
            this.render();
            
            // 自动识别音频中的文本
            await this.recognizePromptAudio(file);
        }
    }

    // ==================== 识别参考音频文本 ====================
    async recognizePromptAudio(file) {
        this._isRecognizing = true;
        this.render();

        try {
            this.addLog('info', '正在识别参考音频文本...');

            const formData = new FormData();
            formData.append('audio', file);

            const response = await fetch('http://localhost:8088/api/tts/recognize', {
                method: 'POST',
                body: formData
            });

            const result = await response.json();

            if (result.success) {
                this._promptText = result.data.text || '';
                this.addLog('success', '识别完成: ' + this._promptText);
                
                // 更新输入框的值
                const promptTextInput = this.querySelector('.prompt-text-input');
                if (promptTextInput) {
                    promptTextInput.value = this._promptText;
                }
            } else {
                this.addLog('error', '识别失败: ' + result.message);
            }

        } catch (error) {
            console.error('Failed to recognize audio:', error);
            this.addLog('error', '识别失败: ' + error.message);
        } finally {
            this._isRecognizing = false;
            this.render();
            this.updateButtonStates();
        }
    }

    // ==================== 开始合成 ====================
    async startSynthesis() {
        if (!this._targetText.trim()) {
            alert('请输入要合成的文本');
            return;
        }

        this._isSynthesizing = true;
        this._synthesisStatus = 'processing';
        this._errorMessage = '';
        this.render();

        try {
            const formData = new FormData();
            formData.append('text', this._targetText);
            
            if (this._promptText) {
                formData.append('prompt_text', this._promptText);
            }
            
            if (this._promptAudioFile) {
                formData.append('prompt_audio', this._promptAudioFile);
            }

            this.addLog('info', '正在请求 TTS 合成...');

            const response = await fetch('http://localhost:8088/api/tts/synthesize', {
                method: 'POST',
                body: formData
            });

            const result = await response.json();

            if (!result.success) {
                throw new Error(result.message);
            }

            this.addLog('success', 'TTS 合成已启动，请等待...');

            // 开始轮询状态
            this.startStatusPolling();

        } catch (error) {
            console.error('Failed to start synthesis:', error);
            this.addLog('error', '启动合成失败: ' + error.message);
            this._isSynthesizing = false;
            this._synthesisStatus = 'error';
            this._errorMessage = error.message;
            this.render();
        }
    }

    // ==================== 轮询合成状态 ====================
    startStatusPolling() {
        if (this._statusCheckInterval) {
            clearInterval(this._statusCheckInterval);
        }

        this._statusCheckInterval = setInterval(async () => {
            try {
                const response = await fetch('http://localhost:8088/api/tts/status');
                const result = await response.json();

                if (result.success) {
                    const { status, error, has_result } = result.data;
                    
                    this._synthesisStatus = status;
                    this._errorMessage = error || '';

                    if (status === 'completed') {
                        this.addLog('success', 'TTS 合成完成！');
                        this._isSynthesizing = false;
                        clearInterval(this._statusCheckInterval);
                        this._statusCheckInterval = null;
                        this.render();
                        this.updateButtonStates();
                    } else if (status === 'error') {
                        this.addLog('error', '合成失败: ' + error);
                        this._isSynthesizing = false;
                        clearInterval(this._statusCheckInterval);
                        this._statusCheckInterval = null;
                        this.render();
                        this.updateButtonStates();
                    } else if (status === 'processing') {
                        // 继续轮询，只更新状态显示不重新渲染整个组件
                        const prevStatus = this._synthesisStatus;
                        if (prevStatus !== status) {
                            this.render();
                        }
                    }
                }
            } catch (error) {
                console.error('Failed to check status:', error);
            }
        }, 1000);
    }

    // ==================== 发送到 ESP32 ====================
    async sendToESP32() {
        if (!this._isConnected) {
            alert('请先连接 ESP32 设备');
            return;
        }

        this._isSending = true;
        this.render();

        try {
            this.addLog('info', '正在下载合成音频...');

            // 下载生成的音频文件
            const audioResponse = await fetch('http://localhost:8088/api/tts/download');
            
            if (!audioResponse.ok) {
                throw new Error('下载音频失败');
            }

            const audioBlob = await audioResponse.blob();
            const audioFile = new File([audioBlob], 'tts_result.wav', { type: 'audio/wav' });

            this.addLog('info', '正在处理音频数据...');

            // 使用 AudioProcessor 处理音频
            const audioData = await this._audioProcessor.processAudioFile(audioFile);
            
            this.addLog('success', `音频处理完成: ${audioData.chunks.length} 个数据包`);
            this.addLog('info', '开始发送到 ESP32...');

            // 发送音频流
            await this.sendAudioStream(audioData.chunks);

            this.addLog('success', '音频已成功发送到 ESP32');
            
            // 保持 completed 状态，允许重复发送
            // this._synthesisStatus = 'idle';  // 不重置状态
            this._isSending = false;
            this.render();
            this.updateButtonStates();

        } catch (error) {
            console.error('Failed to send to ESP32:', error);
            this.addLog('error', '发送失败: ' + error.message);
            this._isSending = false;
            this.render();
            this.updateButtonStates();
            alert('发送到 ESP32 失败: ' + error.message);
        }
    }

    // ==================== 发送音频流到 ESP32 ====================
    async sendAudioStream(chunks) {
        const totalChunks = chunks.length;
        
        // 发送开始命令
        await fetch('http://localhost:8088/api/audio/stream/start', {
            method: 'POST'
        });

        // 逐块发送音频数据
        for (let i = 0; i < totalChunks; i++) {
            const chunk = chunks[i];
            
            try {
                await fetch('http://localhost:8088/api/audio/stream/data', {
                    method: 'POST',
                    body: chunk
                });

                if ((i + 1) % 10 === 0) {
                    const progress = Math.round(((i + 1) / totalChunks) * 100);
                    this.addLog('info', `发送进度: ${progress}% (${i + 1}/${totalChunks})`);
                }

                // 控制发送速率
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

    // ==================== 更新按钮状态（不重新渲染整个组件） ====================
    updateButtonStates() {
        // 使用 setTimeout 确保 DOM 更新后再查询元素
        setTimeout(() => {
            const synthesizeBtn = this.querySelector('.btn-synthesize');
            const sendBtn = this.querySelector('.btn-send-to-esp32');
            
            if (synthesizeBtn) {
                const canSynthesize = !this._isSynthesizing && this._targetText.trim() !== '';
                synthesizeBtn.disabled = !canSynthesize;
            }
            
            if (sendBtn) {
                const canSend = this._isConnected && !this._isSending && this._synthesisStatus === 'completed';
                sendBtn.disabled = !canSend;
            }
        }, 0);
    }

    // ==================== 渲染组件 ====================
    render() {
        const canSynthesize = !this._isSynthesizing && this._targetText.trim() !== '';
        const canSend = this._isConnected && !this._isSending && this._synthesisStatus === 'completed';

        this.innerHTML = `
            <div class="card tts-card">
                <h2>语音合成</h2>
                <p style="font-size: 12px; color: var(--text-secondary); margin-bottom: 16px;">
                    基于 VoxCPM 的 TTS 语音合成功能
                </p>
                
                <div style="display: flex; flex-direction: column; gap: 20px;">
                    <!-- 参考音频 -->
                    <div>
                        <h3 style="font-size: 14px; margin-bottom: 8px; font-weight: 500;">参考音频（可选）</h3>
                        <label class="file-input-label" style="${this._isSynthesizing || this._isRecognizing ? 'opacity: 0.5; cursor: not-allowed;' : ''}">
                            <input type="file" accept="audio/*" class="prompt-audio-input" ${this._isSynthesizing || this._isRecognizing ? 'disabled' : ''}>
                            ${this._isRecognizing ? '识别中...' : (this._promptAudioFile ? this._promptAudioFile.name : '选择音频文件')}
                        </label>
                        <p style="font-size: 12px; color: var(--text-secondary); margin-top: 4px;">
                            提供参考音频可以模仿其音色和风格，选择后会自动识别文本
                        </p>
                    </div>

                    <!-- 参考文本 -->
                    <div>
                        <h3 style="font-size: 14px; margin-bottom: 8px; font-weight: 500;">参考文本（自动识别）</h3>
                        <input 
                            type="text" 
                            class="prompt-text-input text-input"
                            placeholder="选择参考音频后自动识别，也可手动输入或修改..."
                            value="${this._promptText}"
                            ${this._isSynthesizing || this._isRecognizing ? 'disabled' : ''}
                            style="width: 100%; padding: 10px; border: 1px solid var(--border-color); font-size: 14px; font-family: inherit; ${this._isSynthesizing || this._isRecognizing ? 'opacity: 0.5; cursor: not-allowed;' : ''}"
                        >
                        <p style="font-size: 12px; color: var(--text-secondary); margin-top: 4px;">
                            ${this._isRecognizing ? '正在识别音频文本...' : '识别结果可以手动修改'}
                        </p>
                    </div>

                    <!-- 目标文本 -->
                    <div>
                        <h3 style="font-size: 14px; margin-bottom: 8px; font-weight: 500;">合成文本 *</h3>
                        <textarea 
                            class="target-text-input text-input"
                            placeholder="输入要合成的文本内容..."
                            ${this._isSynthesizing ? 'disabled' : ''}
                            style="width: 100%; min-height: 100px; padding: 10px; border: 1px solid var(--border-color); font-size: 14px; font-family: inherit; resize: vertical; ${this._isSynthesizing ? 'opacity: 0.5; cursor: not-allowed;' : ''}"
                        >${this._targetText}</textarea>
                    </div>

                    <!-- 状态显示 -->
                    ${this._synthesisStatus === 'processing' ? `
                        <div style="padding: 12px; background-color: #dbeafe; border-left: 3px solid #2563eb;">
                            <p style="color: #1e40af; font-size: 14px; font-weight: 500;">正在合成中，请稍候...</p>
                        </div>
                    ` : ''}

                    ${this._synthesisStatus === 'completed' ? `
                        <div style="padding: 12px; background-color: #d1fae5; border-left: 3px solid var(--success-color);">
                            <p style="color: #065f46; font-size: 14px; font-weight: 500; margin-bottom: 8px;">合成完成！可以试听或发送到 ESP32</p>
                            <audio controls style="width: 100%; max-width: 400px;">
                                <source src="http://localhost:8088/api/tts/download?t=${Date.now()}" type="audio/wav">
                                您的浏览器不支持音频播放
                            </audio>
                        </div>
                    ` : ''}

                    ${this._synthesisStatus === 'error' ? `
                        <div style="padding: 12px; background-color: #fee2e2; border-left: 3px solid var(--error-color);">
                            <p style="color: #991b1b; font-size: 14px; font-weight: 500;">合成失败: ${this._errorMessage}</p>
                        </div>
                    ` : ''}

                    <!-- 操作按钮 -->
                    <div style="display: flex; gap: 12px;">
                        <button 
                            class="btn-synthesize" 
                            ${!canSynthesize ? 'disabled' : ''}
                            style="flex: 1;"
                        >
                            ${this._isSynthesizing ? '合成中...' : '开始合成'}
                        </button>
                        
                        <button 
                            class="success btn-send-to-esp32" 
                            ${!canSend ? 'disabled' : ''}
                            style="flex: 1;"
                        >
                            ${this._isSending ? '发送中...' : '发送到 ESP32'}
                        </button>
                    </div>

                    ${!this._isConnected ? `
                        <div style="padding: 12px; background-color: #fef3c7; border-left: 3px solid var(--warning-color);">
                            <p style="color: var(--warning-color); font-size: 14px;">提示: 需要连接 ESP32 设备才能发送音频</p>
                        </div>
                    ` : ''}
                </div>
            </div>
        `;
    }
}

customElements.define('tts-synthesizer', TTSSynthesizer);

console.log('tts-synthesizer component registered');

