/***
 * @file dwin.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-09-24
 * @brief 
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-09-24
 * @filePath dwin.c
 * @projectType Embedded
 */

#include "dwin.h"

#define ADDR_TEMP_INT    0x5000 //T5L上温度的整数部分的地址  
#define ADDR_TEMP_DEC    0x5001 //小数部分的

extern u8 rx_buffer[20];
extern u8 rx_count;
extern bit data_received;

/***
 * @brief 向串口发送指令更改对应地址的数据，串口连接迪文屏
 * 
 * @param vp 在迪文屏上数据的地址
 * @param value 要更改为的值
 */
void Send_DWIN_Int16(unsigned int vp, u16 value)
{
    u8 tx_buffer[8];

    tx_buffer[0] = 0x5A;
    tx_buffer[1] = 0xA5;
    tx_buffer[2] = 0x05;
    tx_buffer[3] = 0x82;

    tx_buffer[4] = (vp >> 8) & 0xFF;
    tx_buffer[5] = vp & 0xFF;
    tx_buffer[6] = (value >> 8) & 0xFF;
    tx_buffer[7] = value & 0xFF;
    Uart_SendBuffer(tx_buffer, 8);
}

/***
 * @brief 读取迪文屏指定地址的数据
 * 
 * @param vp 要读取的地址
 */
void Read_DWIN_Data(unsigned int vp)
{
    u8 tx_buffer[6];
    
    tx_buffer[0] = 0x5A;
    tx_buffer[1] = 0xA5;
    tx_buffer[2] = 0x03;
    tx_buffer[3] = 0x83;
    tx_buffer[4] = (vp >> 8) & 0xFF;
    tx_buffer[5] = vp & 0xFF;
    Uart_SendBuffer(tx_buffer, 6);
}

/***
 * @brief 处理接收到的迪文屏数据
 * 
 */
void Process_Received_Data(void)
{
    // 处理接收数据的逻辑
    if (data_received)
    {
        data_received = 0;
        rx_count = 0;
        // 在这里添加对接收数据的处理
    }
}