/***
 * @file audio-processor.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief 音频处理工具类（格式转换、重采样等）
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath audio-processor.js
 * @projectType Frontend
 */

// ==================== 音频处理配置 ====================
const AUDIO_CONFIG = {
    sampleRate: 44100,      // 采样率 44.1kHz (MAX98357支持)
    channels: 2,            // 立体声
    bitDepth: 16,           // 16位
    chunkSize: 3000,        // 每次发送3KB数据
};

// ==================== 音频文件转PCM ====================
class AudioProcessor {
    constructor() {
        this.audioContext = null;
    }

    // ==================== 初始化AudioContext ====================
    initAudioContext() {
        if (!this.audioContext) {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)({
                sampleRate: AUDIO_CONFIG.sampleRate
            });
        }
        return this.audioContext;
    }

    // ==================== 从文件加载音频 ====================
    async loadAudioFile(file) {
        try {
            const arrayBuffer = await file.arrayBuffer();
            const ctx = this.initAudioContext();
            
            console.log('Decoding audio file:', file.name, 'size:', file.size);
            const audioBuffer = await ctx.decodeAudioData(arrayBuffer);
            
            console.log('Audio decoded:', {
                duration: audioBuffer.duration,
                sampleRate: audioBuffer.sampleRate,
                channels: audioBuffer.numberOfChannels,
                length: audioBuffer.length
            });
            
            return audioBuffer;
        } catch (error) {
            console.error('Failed to load audio file:', error);
            throw new Error('Audio file decode failed: ' + error.message);
        }
    }

    // ==================== 重采样音频到目标采样率 ====================
    resampleAudio(audioBuffer, targetSampleRate) {
        if (audioBuffer.sampleRate === targetSampleRate) {
            return audioBuffer;
        }

        const ctx = this.audioContext;
        const ratio = targetSampleRate / audioBuffer.sampleRate;
        const newLength = Math.round(audioBuffer.length * ratio);
        
        // 创建新的AudioBuffer
        const offlineCtx = new OfflineAudioContext(
            audioBuffer.numberOfChannels,
            newLength,
            targetSampleRate
        );

        const source = offlineCtx.createBufferSource();
        source.buffer = audioBuffer;
        source.connect(offlineCtx.destination);
        source.start(0);

        return offlineCtx.startRendering();
    }

    // ==================== 转换AudioBuffer为PCM数据 ====================
    audioBufferToPCM(audioBuffer) {
        const channels = audioBuffer.numberOfChannels;
        const length = audioBuffer.length;
        const sampleRate = audioBuffer.sampleRate;

        console.log('Converting to PCM:', {
            channels,
            length,
            sampleRate
        });

        // 获取所有通道数据
        const channelData = [];
        for (let i = 0; i < channels; i++) {
            channelData.push(audioBuffer.getChannelData(i));
        }

        // 转换为16位PCM交错立体声
        const pcmData = new Int16Array(length * channels);
        
        for (let i = 0; i < length; i++) {
            for (let c = 0; c < channels; c++) {
                // 将浮点数[-1.0, 1.0]转换为16位整数[-32768, 32767]
                const sample = Math.max(-1, Math.min(1, channelData[c][i]));
                pcmData[i * channels + c] = sample < 0 
                    ? sample * 0x8000 
                    : sample * 0x7FFF;
            }
        }

        // 如果是单声道，复制到双声道
        if (channels === 1) {
            const stereoData = new Int16Array(length * 2);
            for (let i = 0; i < length; i++) {
                stereoData[i * 2] = pcmData[i];
                stereoData[i * 2 + 1] = pcmData[i];
            }
            return stereoData;
        }

        return pcmData;
    }

    // ==================== 分块PCM数据 ====================
    chunkPCMData(pcmData, chunkSize = AUDIO_CONFIG.chunkSize) {
        const chunks = [];
        const totalBytes = pcmData.length * 2; // Int16Array，每个元素2字节
        const samplesPerChunk = Math.floor(chunkSize / 2);

        for (let i = 0; i < pcmData.length; i += samplesPerChunk) {
            const chunk = pcmData.slice(i, i + samplesPerChunk);
            chunks.push(new Uint8Array(chunk.buffer, chunk.byteOffset, chunk.byteLength));
        }

        console.log('PCM data chunked:', {
            totalSize: totalBytes,
            chunkCount: chunks.length,
            chunkSize: chunkSize
        });

        return chunks;
    }

    // ==================== 处理音频文件（完整流程） ====================
    async processAudioFile(file) {
        try {
            // 1. 加载音频文件
            let audioBuffer = await this.loadAudioFile(file);

            // 2. 重采样到44.1kHz（如果需要）
            if (audioBuffer.sampleRate !== AUDIO_CONFIG.sampleRate) {
                console.log('Resampling from', audioBuffer.sampleRate, 'to', AUDIO_CONFIG.sampleRate);
                audioBuffer = await this.resampleAudio(audioBuffer, AUDIO_CONFIG.sampleRate);
            }

            // 3. 转换为PCM
            const pcmData = this.audioBufferToPCM(audioBuffer);

            // 4. 分块
            const chunks = this.chunkPCMData(pcmData);

            return {
                chunks,
                sampleRate: AUDIO_CONFIG.sampleRate,
                channels: AUDIO_CONFIG.channels,
                duration: audioBuffer.duration,
                totalSize: pcmData.length * 2
            };
        } catch (error) {
            console.error('Failed to process audio file:', error);
            throw error;
        }
    }

    // ==================== 清理资源 ====================
    cleanup() {
        if (this.audioContext && this.audioContext.state !== 'closed') {
            this.audioContext.close();
            this.audioContext = null;
        }
    }
}

// ==================== 麦克风音频处理器 ====================
class MicrophoneProcessor {
    constructor() {
        this.audioContext = null;
        this.mediaStream = null;
        this.mediaRecorder = null;
        this.onDataCallback = null;
        this.isRecording = false;
    }

    // ==================== 启动麦克风采集 ====================
    async start(onDataCallback) {
        try {
            this.onDataCallback = onDataCallback;
            this.isRecording = true;

            // 请求麦克风权限
            this.mediaStream = await navigator.mediaDevices.getUserMedia({
                audio: {
                    channelCount: 2,
                    sampleRate: AUDIO_CONFIG.sampleRate,
                    echoCancellation: true,
                    noiseSuppression: true,
                    autoGainControl: true
                }
            });

            // 创建AudioContext用于处理
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)({
                sampleRate: AUDIO_CONFIG.sampleRate
            });

            // 使用MediaRecorder录制音频
            const options = {
                mimeType: 'audio/webm;codecs=opus',
                audioBitsPerSecond: 128000
            };

            // 检查浏览器支持的格式
            if (!MediaRecorder.isTypeSupported(options.mimeType)) {
                options.mimeType = 'audio/webm';
            }

            this.mediaRecorder = new MediaRecorder(this.mediaStream, options);

            this.mediaRecorder.ondataavailable = async (event) => {
                if (event.data.size > 0 && this.isRecording) {
                    try {
                        // 将录制的音频数据转换为PCM
                        const audioData = await event.data.arrayBuffer();
                        const audioBuffer = await this.audioContext.decodeAudioData(audioData);
                        
                        // 转换为PCM
                        const pcmData = this.audioBufferToPCM(audioBuffer);
                        const uint8Data = new Uint8Array(pcmData.buffer);
                        
                        // 发送数据
                        if (this.onDataCallback) {
                            this.onDataCallback(uint8Data);
                        }
                    } catch (error) {
                        console.error('Failed to process audio chunk:', error);
                    }
                }
            };

            // 每100ms生成一个数据块
            this.mediaRecorder.start(100);

            console.log('Microphone started:', {
                sampleRate: this.audioContext.sampleRate,
                mimeType: options.mimeType
            });

            return true;
        } catch (error) {
            console.error('Failed to start microphone:', error);
            throw new Error('Microphone access failed: ' + error.message);
        }
    }

    // ==================== 转换AudioBuffer为PCM ====================
    audioBufferToPCM(audioBuffer) {
        const channels = Math.min(audioBuffer.numberOfChannels, 2);
        const length = audioBuffer.length;

        // 获取通道数据
        const leftChannel = audioBuffer.getChannelData(0);
        const rightChannel = channels > 1 ? audioBuffer.getChannelData(1) : leftChannel;

        // 转换为16位PCM立体声
        const pcmData = new Int16Array(length * 2);
        
        for (let i = 0; i < length; i++) {
            const left = Math.max(-1, Math.min(1, leftChannel[i]));
            const right = Math.max(-1, Math.min(1, rightChannel[i]));
            
            pcmData[i * 2] = left < 0 ? left * 0x8000 : left * 0x7FFF;
            pcmData[i * 2 + 1] = right < 0 ? right * 0x8000 : right * 0x7FFF;
        }

        return pcmData;
    }

    // ==================== 停止麦克风采集 ====================
    stop() {
        this.isRecording = false;

        if (this.mediaRecorder && this.mediaRecorder.state !== 'inactive') {
            this.mediaRecorder.stop();
            this.mediaRecorder = null;
        }

        if (this.mediaStream) {
            this.mediaStream.getTracks().forEach(track => track.stop());
            this.mediaStream = null;
        }

        if (this.audioContext && this.audioContext.state !== 'closed') {
            this.audioContext.close();
            this.audioContext = null;
        }

        console.log('Microphone stopped');
    }
}

// ==================== 导出 ====================
export { AudioProcessor, MicrophoneProcessor, AUDIO_CONFIG };

