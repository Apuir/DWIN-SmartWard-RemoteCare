#ifndef __UART_H__
#define __UART_H__

#include "Dwin_T5L1H.h"
#include "Sys.h"
#include "crc16.h"


extern u8     xdata        T_O2;		  //口2超时计数器
extern u8     xdata        T_O3;		  //口3超时计数器
extern u8     xdata        T_O4;		  //口4超时计数器
extern u8     xdata        T_O5;		  //口5超时计数器


extern bit    Response_flog;  //应答用
extern bit    Auto_data_upload;//数据自动上传用
extern bit    Crc_check_flog;//Crc校验标记

void  uart_init();
void  Sw_Data_Send ();
void  uart_frame_deal();
void  uart_send_str(u8 U_number,u8* Str,u8 Len);
void  uart_send_byte(u8 Uart_number,u8 Dat);

#endif