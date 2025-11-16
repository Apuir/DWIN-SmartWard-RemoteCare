/***
 * @file public.c
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
 * @filePath public.c
 * @projectType Embedded
 */

#include "public.h"

void delay_10us(u16 ten_us)
{
	while (ten_us--)
		;
}

void delay_ms(u16 ms)
{
	u16 i, j;
	for (i = ms; i > 0; i--)
		for (j = 110; j > 0; j--)
			;
}