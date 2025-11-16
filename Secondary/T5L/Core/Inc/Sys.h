#ifndef __SYS_H__
#define __SYS_H__

#include "Dwin_T5L1H.h"
#include "Uart.h"
extern u16    xdata        SysTick_RTC;
extern bit                 RTC_Flog;
extern u16    xdata        Count_num1;
extern u16    xdata        TimVal;


void Sys_Cpu_Init();
void T5L_Flash(unsigned char mod,unsigned int addr,long addr_flash,unsigned int len);

#endif