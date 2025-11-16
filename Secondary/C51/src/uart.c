/***
 * @file uart.c
 * @author bearbox <apuirbox@gmail.com>
 * @date 2025-09-24
 * @brief 设置串口波特率，实现相关函数
 * 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2025 by AmBearBox, All Rights Reserved.
 * 
 * @lastEditors bearbox <apuirbox@gmail.com>
 * @lastEditTime 2025-09-24
 * @filePath uart.c
 * @projectType Embedded
 */

#include "uart.h"

#define MAIN_Fosc 11059200UL

/***
 * @brief 串口初始化
 * 
 */
void uart_init(void)
{
    SCON = 0x50;    // 设置串口工作方式1，8位UART，可变波特率
    T2CON = 0x34;   // 设置定时器2为波特率发生器
    RCAP2H = 0xFF;
    RCAP2L = 0xFD;
    TR2 = 1;        // 启动定时器2
    
    ES = 1;         // 开启串口中断
    EA = 1;         // 开启总中断
}

/***
 * @brief 发送一个字节
 * 
 * @param byteToSend 要发送的字节
 */
void Uart_SendByte(uchar byteToSend)
{
    SBUF = byteToSend;
    while (!TI)
        ;
    TI = 0;
}

/***
 * @brief 发送字符串
 * 
 * @param str 要发送的字符串
 */
void Uart_SendString(uchar *str)
{
    while (*str)
    {
        Uart_SendByte(*str++);
    }
}

/***
 * @brief 发送一个字节数组
 * 
 * @param buffer 要发送的字节数组
 * @param length 要发送的字节数组长度
 */
void Uart_SendBuffer(uchar *buffer, unsigned int length)
{
    while (length--)
    {
        Uart_SendByte(*buffer++);
    }
}