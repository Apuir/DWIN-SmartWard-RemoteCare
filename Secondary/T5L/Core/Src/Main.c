/***
 * @file Main.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-10-24
 * @brief 
 * 
 * @version 0.2
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-10-23
 * @filePath Main.c
 * @projectType Embedded
 */

#include "Dwin_T5L1H.h"
#include "i2c.h"
#include "crc16.h"
#include "Sys.h"
#include "Uart.h"
#include "save_data_dgus.h"

// ==================== GPIO引脚定义 ====================
sbit FAN = P2^0;  // 风扇控制引脚（P2.0）

// ==================== 温度阈值定义 ====================
#define TEMP_FAN_THRESHOLD      280   // 28.0℃，风扇开启阈值
#define TEMP_BUZZER_THRESHOLD   350   // 35.0℃，蜂鸣器报警阈值

// ==================== ESP32温度阈值（用于WiFi报警） ====================
#define TEMP_ESP32_THRESHOLD1   280   // 28.0℃，ESP32阈值1
#define TEMP_ESP32_THRESHOLD2   350   // 35.0℃，ESP32阈值2

// ==================== DGUS变量地址定义 ====================
#define ADDR_TEMP_INT       0x5000  // 温度整数部分地址
#define ADDR_TEMP_DEC       0x5001  // 温度小数部分地址

// ==================== T5L→C51 控制命令（仅用于蜂鸣器） ====================
#define CMD_BUZZER_ON   0xB1   // 开蜂鸣器
#define CMD_BUZZER_OFF  0xB0   // 关蜂鸣器

// ==================== T5L→ESP32 温度阈值命令 ====================
#define CMD_ESP32_THRESHOLD1    0xE1   // 温度达到阈值1
#define CMD_ESP32_THRESHOLD2    0xE2   // 温度达到阈值2
#define CMD_ESP32_NORMAL        0xE0   // 温度恢复正常
#define CMD_ESP32_TEMP_UPDATE   0xE3   // 温度定期更新（用于显示）

// ==================== 设备状态标志 ====================
u8 fan_status = 0;     // 风扇状态：0=关，1=开
u8 buzzer_status = 0;  // 蜂鸣器状态：0=关，1=开
u8 esp32_status = 0;   // ESP32通知状态：0=正常，1=阈值1，2=阈值2
u16 temp_update_counter = 0;  // 温度更新计数器（每3秒发送一次温度）
/****************************************************************************
 * @brief 发送简单控制命令到C51（仅用于蜂鸣器控制）
 * @param cmd 控制命令字节
 * 
 * 说明：T5L→C51使用简单的单字节命令协议（C51端无固定协议要求）
 *       风扇已改为T5L直接GPIO控制，不再通过C51
 */
void Send_Control_Command_To_C51(u8 cmd)
{
    uart_send_byte(2, cmd);  // 通过UART2发送单字节命令到C51
}

/****************************************************************************
 * @brief 直接控制风扇（T5L GPIO控制）
 * @param state 1=开启风扇，0=关闭风扇
 * 
 * 硬件连接：
 * P2.0 → ULN2003输入端 → 风扇
 * 
 * 控制逻辑（ULN2003反相输出）：
 * - FAN = 0（低电平） → 风扇启动
 * - FAN = 1（高电平） → 风扇关闭
 */
void Control_Fan_Direct(u8 state)
{
    if (state)
    {
        FAN = 0;  // 低电平 → 风扇启动（ULN2003反相）
    }
    else
    {
        FAN = 1;  // 高电平 → 风扇关闭
    }
}

/****************************************************************************
 * @brief 发送温度阈值命令到ESP32
 * @param cmd 控制命令字节
 * @param temp 温度值（如25.3℃ = 253）
 * 
 * 说明：T5L通过UART5发送3字节数据到ESP32：命令+温度高字节+温度低字节
 */
void Send_Command_To_ESP32_With_Temp(u8 cmd, u16 temp)
{
    u8 temp_data[3];
    temp_data[0] = cmd;                // 命令字节
    temp_data[1] = (u8)(temp >> 8);    // 温度高字节
    temp_data[2] = (u8)(temp & 0xFF);  // 温度低字节
    uart_send_str(5, temp_data, 3);    // 通过UART5发送3字节数据到ESP32
}

/****************************************************************************
 * @brief 检测温度并控制风扇和蜂鸣器
 * 
 * 功能说明：
 * 1. 从DGUS变量读取C51发来的温度数据
 * 2. 判断温度是否达到阈值
 * 3. 向C51发送开关控制命令
 * 4. 向ESP32发送温度阈值通知
 * 
 * 温度控制：
 * - 温度 < 28℃：风扇关闭、ESP32正常
 * - 温度 ≥ 28℃：风扇开启、ESP32阈值1
 * - 温度 ≥ 35℃：蜂鸣器报警、ESP32阈值2
 */
void Check_Temperature_And_Control_Devices(void)
{
    u8 temp_buf[4];  // 缓冲区，每个u16需要2字节
    u16 temp_int, temp_dec;
    u16 temp_total;
    u8 new_fan_status;
    u8 new_buzzer_status;
    u8 new_esp32_status;
    
    // ==================== 读取温度数据 ====================
    // 读取温度整数部分（地址0x5000，1个字）
    read_dgus_vp(ADDR_TEMP_INT, temp_buf, 1);
    temp_int = (temp_buf[0] << 8) | temp_buf[1];
    
    // 读取温度小数部分（地址0x5001，1个字）
    read_dgus_vp(ADDR_TEMP_DEC, temp_buf, 1);
    temp_dec = (temp_buf[0] << 8) | temp_buf[1];
    
    temp_total = temp_int * 10 + temp_dec;  // 如：25.3℃ = 253
    
    // ==================== 风扇开关控制 ====================
    if (temp_total >= TEMP_FAN_THRESHOLD)
    {
        new_fan_status = 1;  // ≥28℃，开启风扇
    }
    else
    {
        new_fan_status = 0;  // <28℃，关闭风扇
    }
    
    // 风扇状态变化时直接控制GPIO
    if (new_fan_status != fan_status)
    {
        fan_status = new_fan_status;
        Control_Fan_Direct(fan_status);  // 直接GPIO控制，不再通过UART发送到C51
    }
    
    // ==================== 蜂鸣器控制 ====================
    if (temp_total >= TEMP_BUZZER_THRESHOLD)
    {
        new_buzzer_status = 1;  // ≥35℃，报警
    }
    else
    {
        new_buzzer_status = 0;  // <35℃，关闭
    }
    
    if (new_buzzer_status != buzzer_status)
    {
        buzzer_status = new_buzzer_status;
        
        if (buzzer_status)
        {
            Send_Control_Command_To_C51(CMD_BUZZER_ON);   // 0xB1
        }
        else
        {
            Send_Control_Command_To_C51(CMD_BUZZER_OFF);  // 0xB0
        }
    }
    
    // ==================== ESP32温度阈值通知 ====================
    if (temp_total >= TEMP_ESP32_THRESHOLD2)
    {
        new_esp32_status = 2;  // ≥35℃，阈值2
    }
    else if (temp_total >= TEMP_ESP32_THRESHOLD1)
    {
        new_esp32_status = 1;  // ≥28℃，阈值1
    }
    else
    {
        new_esp32_status = 0;  // <28℃，正常
    }
    
    // ESP32状态变化时发送命令（带温度值）
    if (new_esp32_status != esp32_status)
    {
        esp32_status = new_esp32_status;
        
        if (esp32_status == 2)
        {
            Send_Command_To_ESP32_With_Temp(CMD_ESP32_THRESHOLD2, temp_total);  // 0xE2 + 温度
        }
        else if (esp32_status == 1)
        {
            Send_Command_To_ESP32_With_Temp(CMD_ESP32_THRESHOLD1, temp_total);  // 0xE1 + 温度
        }
        else
        {
            Send_Command_To_ESP32_With_Temp(CMD_ESP32_NORMAL, temp_total);      // 0xE0 + 温度
        }
        
        temp_update_counter = 0;  // 重置计数器
    }
    else
    {
        // 状态未变化，但定期发送温度更新（每3秒，即10次循环）
        temp_update_counter++;
        if (temp_update_counter >= 10)
        {
            Send_Command_To_ESP32_With_Temp(CMD_ESP32_TEMP_UPDATE, temp_total);  // 0xE3 + 温度
            temp_update_counter = 0;
        }
    }
}

/****************************************************************************
 * @brief 主函数
 * 
 * 功能说明：
 * 1. T5L负责：接收并显示C51发来的温度数据、判断温度阈值、直接控制风扇(P2.0)、发送蜂鸣器控制指令到C51
 * 2. C51负责：温度采集、执行蜂鸣器控制指令
 * 3. ESP32负责：WiFi通信、接收T5L温度阈值通知、向上位机发送报警、播放音频
 * 
 * 硬件变更：
 * - 风扇从C51 P1.0移至T5L P2.0，由T5L直接GPIO控制
 * - 蜂鸣器仍由C51控制
 */
void    Main()
{		  
        Sys_Cpu_Init();
        uart_init();
	    data_save_init();
	    
	    // 启动后立即发送一次温度，确保Web UI能立即显示
	    Check_Temperature_And_Control_Devices();
	    
        while(1)
			{			
				Clock();
				if(Count_num1==0) 
				{
					Sw_Data_Send ();                          // 数据自动上传
					data_change_sava();                       // 数据保存
					Check_Temperature_And_Control_Devices();  // 检测温度并控制风扇和蜂鸣器
					Count_num1=300;  // 300ms执行一次温度检测和控制
				}
                uart_frame_deal();  // 串口数据处理
			}	
}




