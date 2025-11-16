#!/usr/bin/env python3
"""
VoxCPM TTS Helper Script
直接调用 VoxCPM 进行语音合成
"""

import sys
import os
import json
import warnings

# 禁用警告
warnings.filterwarnings('ignore')
os.environ['PYTHONWARNINGS'] = 'ignore'
os.environ["TOKENIZERS_PARALLELISM"] = "false"

def synthesize_tts(text, prompt_wav=None, prompt_text=None, output_path=None):
    """
    使用 VoxCPM 合成语音
    
    Args:
        text: 要合成的文本
        prompt_wav: 参考音频路径（可选）
        prompt_text: 参考文本（可选）
        output_path: 输出音频路径
    
    Returns:
        成功返回输出路径，失败返回 None
    """
    try:
        import voxcpm
        import soundfile as sf
        import numpy as np
        
        # 创建 VoxCPM 实例
        model = voxcpm.VoxCPM(
            voxcpm_model_path="./model/VoxCPM-0.5B",
            zipenhancer_model_path="./model/speech_zipenhancer_ans_multiloss_16k_base"
        )
        
        # 生成音频 - VoxCPM.generate() 直接返回音频数据
        audio_data = model.generate(
            text=text,
            prompt_text=prompt_text,
            prompt_wav_path=prompt_wav,
            cfg_value=2.0,
            inference_timesteps=10,
            normalize=False,
            denoise=False,
        )
        
        # VoxCPM 默认使用 16kHz 采样率
        sample_rate = 16000
        
        # 保存音频
        if output_path is None:
            import time
            output_path = f"/tmp/tts_result_{int(time.time())}.wav"
        
        # 确保音频数据是正确的格式
        if isinstance(audio_data, np.ndarray):
            sf.write(output_path, audio_data, sample_rate)
        else:
            raise ValueError(f"Unexpected audio data type: {type(audio_data)}")
        
        return output_path
        
    except Exception as e:
        import traceback
        error_msg = f"{str(e)}\n{traceback.format_exc()}"
        print(json.dumps({"error": error_msg}), file=sys.stderr)
        return None

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "No text provided"}))
        sys.exit(1)
    
    text = sys.argv[1]
    prompt_wav = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] != "" else None
    prompt_text = sys.argv[3] if len(sys.argv) > 3 and sys.argv[3] != "" else None
    output_path = sys.argv[4] if len(sys.argv) > 4 else None
    
    result_path = synthesize_tts(text, prompt_wav, prompt_text, output_path)
    
    if result_path:
        print(json.dumps({"success": True, "path": result_path}))
    else:
        print(json.dumps({"success": False, "error": "Synthesis failed"}))
        sys.exit(1)

