#ifndef __UART_H__
#define __UART_H__

#include <reg52.h>

#ifndef uchar
#define uchar unsigned char
#endif

void uart_init(void);

void Uart_SendByte(uchar byteToSend);

void Uart_SendString(uchar *str);

void Uart_SendBuffer(uchar *buffer, unsigned int length);

#endif