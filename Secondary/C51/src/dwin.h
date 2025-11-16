#ifndef __DWIN_H__
#define __DWIN_H__

#include "public.h"
#include "uart.h"

void Send_DWIN_Int16(unsigned int vp, u16 value);
void Read_DWIN_Data(unsigned int vp);
void Process_Received_Data(void);

#endif