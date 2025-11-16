/***
 * @file ds18b20.c
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
 * @filePath ds18b20.c
 * @projectType Embedded
 */

#include "ds18b20.h"
#include "intrins.h"

/***
 * @brief 重置状态
 * 
 */
void ds18b20_reset(void)
{
	DS18B20_PORT = 0;
	delay_10us(75);
	DS18B20_PORT = 1;
	delay_10us(2);
}

/***
 * @brief 检测状态
 * 
 * @return u8 
 */
u8 ds18b20_check(void)
{
	u8 time_temp = 0;

	while (DS18B20_PORT && time_temp < 20)
	{
		time_temp++;
		delay_10us(1);
	}
	if (time_temp >= 20)
		return 1;
	else
		time_temp = 0;
	while ((!DS18B20_PORT) && time_temp < 20)
	{
		time_temp++;
		delay_10us(1);
	}
	if (time_temp >= 20)
		return 1;
	return 0;
}

/***
 * @brief 读取一个bit
 * 
 * @return u8 
 */
u8 ds18b20_read_bit(void)
{
	u8 dat = 0;

	DS18B20_PORT = 0;
	_nop_();
	_nop_();
	DS18B20_PORT = 1;
	_nop_();
	_nop_();
	if (DS18B20_PORT)
		dat = 1;
	else
		dat = 0;
	delay_10us(5);
	return dat;
}

/***
 * @brief 读取一个字节
 * 
 * @return u8 
 */
u8 ds18b20_read_byte(void)
{
	u8 i = 0;
	u8 dat = 0;
	u8 temp = 0;

	for (i = 0; i < 8; i++)
	{
		temp = ds18b20_read_bit();
		dat = (temp << 7) | (dat >> 1);
	}
	return dat;
}

/***
 * @brief 写一个字节
 * 
 * @param dat 
 */
void ds18b20_write_byte(u8 dat)
{
	u8 i = 0;
	u8 temp = 0;

	for (i = 0; i < 8; i++)
	{
		temp = dat & 0x01;
		dat >>= 1;
		if (temp)
		{
			DS18B20_PORT = 0;
			_nop_();
			_nop_();
			DS18B20_PORT = 1;
			delay_10us(6);
		}
		else
		{
			DS18B20_PORT = 0;
			delay_10us(6);
			DS18B20_PORT = 1;
			_nop_();
			_nop_();
		}
	}
}

/***
 * @brief 发送启动信号
 * 
 */
void ds18b20_start(void)
{
	ds18b20_reset();
	ds18b20_check();
	ds18b20_write_byte(0xcc);
	ds18b20_write_byte(0x44);
}

/***
 * @brief 初始化
 * 
 * @return u8 
 */
u8 ds18b20_init(void)
{
	ds18b20_reset();
	return ds18b20_check();
}

/***
 * @brief 读取温度
 * 
 * @return float 
 */
float ds18b20_read_temperture(void)
{
	float temp;
	u8 dath = 0;
	u8 datl = 0;
	u16 value = 0;

	ds18b20_start();
	ds18b20_reset();
	ds18b20_check();
	ds18b20_write_byte(0xcc);
	ds18b20_write_byte(0xbe);

	datl = ds18b20_read_byte();
	dath = ds18b20_read_byte();
	value = (dath << 8) + datl;

	if ((value & 0xf800) == 0xf800)
	{
		value = (~value) + 1;
		temp = value * (-0.0625);
	}
	else
	{
		temp = value * 0.0625;
	}
	return temp;
}
