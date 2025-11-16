#!/usr/bin/env python3
"""
VoxCPM Helper Script for ASR Recognition
用于从 Go 后端调用 VoxCPM 的 ASR 功能
"""

import sys
import os
import json
import warnings
import logging

# 禁用所有警告
warnings.filterwarnings('ignore')
os.environ['PYTHONWARNINGS'] = 'ignore'

# 设置环境变量
os.environ["TOKENIZERS_PARALLELISM"] = "false"

# 禁用所有日志输出
logging.disable(logging.CRITICAL)

# 重定向 stderr 到 null（抑制进度条）
class DevNull:
    def write(self, msg):
        pass
    def flush(self):
        pass

def recognize_audio(audio_path):
    """识别音频中的文本"""
    try:
        # 临时重定向 stderr
        old_stderr = sys.stderr
        sys.stderr = DevNull()
        
        from funasr import AutoModel
        
        # 使用相对路径加载 ASR 模型（相对于 VoxCPM 目录）
        asr_model_path = "./model/SenseVoiceSmall"
        
        asr_model = AutoModel(
            model=asr_model_path,
            disable_update=True,
            log_level='ERROR',
            device="cpu",  # 使用 CPU 以避免 GPU 资源竞争
        )
        
        # 识别音频
        result = asr_model.generate(input=audio_path, language="auto", use_itn=True)
        
        # 恢复 stderr
        sys.stderr = old_stderr
        
        if result and len(result) > 0:
            # 提取文本，去除语言标记
            text = result[0]["text"]
            if '|>' in text:
                text = text.split('|>')[-1]
            return text.strip()
        
        return ""
        
    except Exception as e:
        # 恢复 stderr
        sys.stderr = old_stderr
        return ""

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "No audio file provided"}))
        sys.exit(1)
    
    audio_path = sys.argv[1]
    
    if not os.path.exists(audio_path):
        print(json.dumps({"error": "Audio file not found"}))
        sys.exit(1)
    
    # 识别音频
    text = recognize_audio(audio_path)
    
    # 输出 JSON 结果
    print(json.dumps({"text": text}))

