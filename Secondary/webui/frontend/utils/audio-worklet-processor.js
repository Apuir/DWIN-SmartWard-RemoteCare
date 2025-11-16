/***
 * @file audio-worklet-processor.js
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-11-05
 * @brief AudioWorklet处理器（现代方案，备用）
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-11-05
 * @filePath audio-worklet-processor.js
 * @projectType Frontend
 */

// 注意：此文件在AudioWorklet线程中运行，不能访问DOM

class PCMProcessor extends AudioWorkletProcessor {
    constructor() {
        super();
        this.bufferSize = 4096;
        this.buffer = new Float32Array(this.bufferSize * 2);
        this.bufferIndex = 0;
    }

    process(inputs, outputs, parameters) {
        const input = inputs[0];
        
        if (!input || input.length === 0) {
            return true;
        }

        const leftChannel = input[0];
        const rightChannel = input.length > 1 ? input[1] : leftChannel;
        const frameCount = leftChannel.length;

        // 将音频数据添加到缓冲区
        for (let i = 0; i < frameCount; i++) {
            if (this.bufferIndex >= this.bufferSize) {
                // 缓冲区满，发送数据
                this.sendPCMData();
                this.bufferIndex = 0;
            }

            this.buffer[this.bufferIndex * 2] = leftChannel[i];
            this.buffer[this.bufferIndex * 2 + 1] = rightChannel[i];
            this.bufferIndex++;
        }

        return true;
    }

    sendPCMData() {
        // 转换为16位PCM
        const pcmData = new Int16Array(this.bufferIndex * 2);
        
        for (let i = 0; i < this.bufferIndex; i++) {
            const left = Math.max(-1, Math.min(1, this.buffer[i * 2]));
            const right = Math.max(-1, Math.min(1, this.buffer[i * 2 + 1]));
            
            pcmData[i * 2] = left < 0 ? left * 0x8000 : left * 0x7FFF;
            pcmData[i * 2 + 1] = right < 0 ? right * 0x8000 : right * 0x7FFF;
        }

        // 发送到主线程
        this.port.postMessage(pcmData.buffer, [pcmData.buffer]);
    }
}

registerProcessor('pcm-processor', PCMProcessor);

