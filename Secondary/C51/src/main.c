/***
 * @file main.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-10-24
 * @brief 
 * 
 * @version 0.3
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-10-23
 * @filePath main.c
 * @projectType Embedded
 */

#include "public.h"
#include "smg.h"
#include "ds18b20.h"
#include "uart.h"
#include "dwin.h"

// 外设控制引脚定义
sbit BUZZER = P2^5;        // 蜂鸣器控制引脚

// 串口接收缓冲区
u8 rx_buffer[20];
u8 rx_count = 0;
bit data_received = 0;

// T5L→C51 控制指令定义（单字节命令）
#define CMD_BUZZER_ON   0xB1   // 开蜂鸣器
#define CMD_BUZZER_OFF  0xB0   // 关蜂鸣器


/***
 * @brief 蜂鸣器控制函数
 * @param state 1-开启蜂鸣器，0-关闭蜂鸣器
 */
void Control_Buzzer(u8 state)
{
    if (state)
    {
        BUZZER = 1;  // 开启蜂鸣器
    }
    else
    {
        BUZZER = 0;  // 关闭蜂鸣器
    }
}

/***
 * @brief 处理来自T5L的控制指令
 * 
 * 指令格式（单字节）：
 * 0xB1 - 开蜂鸣器
 * 0xB0 - 关蜂鸣器
 */
void Process_T5L_Command(void)
{
    u8 i;
    
    // 在接收缓冲区中查找控制命令
    for (i = 0; i < rx_count; i++)
    {
        switch (rx_buffer[i])
        {
            case CMD_BUZZER_ON:
                Control_Buzzer(1);  // 开启蜂鸣器
                break;
                
            case CMD_BUZZER_OFF:
                Control_Buzzer(0);  // 关闭蜂鸣器
                break;
                
            default:
                break;
        }
    }
}

/***
 * @brief 串口中断
 * 接收T5L发来的简单控制命令（单字节）
 */
void UART_Interrupt() interrupt 4
{
    if (RI)
    {
        RI = 0;
        rx_buffer[rx_count] = SBUF;
        rx_count++;
        
        // 标记有数据接收
        data_received = 1;
        
        // 缓冲区满则重置
        if (rx_count >= sizeof(rx_buffer))
        {
            rx_count = 0;
        }
    }
}

/***
 * @brief 主函数
 * 
 * 功能说明：
 * 1. C51负责：温度采集、温度显示、蜂鸣器控制、执行T5L蜂鸣器控制指令
 * 2. T5L负责：温度判断、风扇直接控制（P2.0）、发送蜂鸣器控制指令
 */
void main()
{
    u8 i = 0;
    int temp_value;
    u8 temp_buf[5];
    
    // 初始化外设
    BUZZER = 0;        // 蜂鸣器关闭
    ds18b20_init();    // 初始化温度传感器
    uart_init();       // 初始化串口
    
    while (1)
    {
        i++;
        
        // 每50个循环读取一次温度
        if (i % 50 == 0)
            temp_value = ds18b20_read_temperture() * 10;

        // ==================== 温度显示处理 ====================
        if (temp_value < 0)
            temp_buf[0] = 0x40;  // 负温度标志
        else
            temp_buf[0] = 0x00;
        temp_buf[1] = gsmg_code[temp_value / 100];
        temp_buf[2] = gsmg_code[temp_value / 10];
        temp_buf[3] = gsmg_code[temp_value % 10] | 0x80;
        
        // 数码管显示
        smg_display(temp_buf, 3);
        
        // ==================== 发送温度到T5L ====================
        // 使用DWIN协议发送温度数据
        Send_DWIN_Int16(0x5000, temp_value / 10);  // 温度整数部分
        delay_ms(15);
        Send_DWIN_Int16(0x5001, temp_value % 10);  // 温度小数部分
        
        // ==================== 处理T5L控制指令 ====================
        if (data_received)
        {
            Process_T5L_Command();  // 处理控制指令
            data_received = 0;      // 清除接收标志
            rx_count = 0;           // 重置缓冲区计数
        }
    }
}