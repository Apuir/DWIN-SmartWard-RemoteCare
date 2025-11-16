#ifndef __WENYU_T5L51_H__
#define __WENYU_T5L51_H__
//====================================================
#define  u8   unsigned char  
#define  s8     signed char
#define  u16  unsigned int 
#define  s16    signed int
#define  u32  unsigned long 
#define  s32    signed long
//=====================================================
#include  "Parameter_Config.h"
//=====================================================
sfr	P0		=	0x80;		/********PO口*******/
sfr	SP		=	0x81;		/********堆栈指针*******/
sfr DPL		=	0x82;		/********DPTR数据指针*******/
sfr DPH		=	0x83;		/********DPTR数据指针*******/
sfr PCON	=	0x87;		/********.7 UART2波特率设置*******/
sfr TCON	=	0x88;		/********T0 T1控制寄存器*******/

sbit	TF1 =	TCON^7;		/********T1中断触发*******/
sbit	TR1	=	TCON^6;		
sbit	TF0	=	TCON^5;		/********T0中断触发*******/
sbit	TR0	=	TCON^4;
sbit	IE1	=	TCON^3;		/********外部中断1*******/
sbit	IT1	=	TCON^2;		/********外部中断1触发方式	0：低电平触发		1：下降沿触发*******/
sbit	IE0	=	TCON^1;		/********外部中断0*******/
sbit	IT0	=	TCON^0;		/********外部中断0触发方式	0：低电平触发		1：下降沿触发*******/

sfr	TMOD	=	0x89;		/********T0 T1模式选择,同8051*******/
sfr	TH0 	=	0x8C;		
sfr TL0 	=	0x8A;
sfr TH1 	=	0x8D;
sfr TL1 	=	0x8B;

sfr CKCON	=	0x8E;		/********CPU运行=0, 1T*******/
sfr	P1		=	0x90;
sfr	DPC		=	0x93;		/********MOVX指令后，DPTR的变化模式	0：不变 1：+1  2：-1*******/
sfr PAGESEL	=	0x94;		/********必须是0x01*******/
sfr	D_PAGESEL	=	0x95;	/********必须是0x02*******/

sfr SCON2	=	0x98;		/********UART2控制接口，同8051*******/

sbit	TI2	=	SCON2^1;
sbit	RI2	=	SCON2^0;
sfr	SBUF2	=	0x99;		/********UART2收发数据接口*******/
sfr	SREL2H	=	0xBA;		/********设置波特率，当ADCON为0x80时*******/
sfr	SREL2L	=	0xAA;

sfr	SCON3	=	0x9B;		/********UART3控制接口*******/
sfr	SBUF3	=	0x9C;
sfr	SREL3H	=	0xBB;
sfr	SREL3L	=	0x9D;

sfr	IEN2	=	0x9A;		/********中断使能控制器SFR  .7~.1必须写0		.0 UART3中断使能控制位*******/
sfr	P2		=	0xA0;

sbit    Red_LED1     =   P2^6;
sbit    Green_LED2   =   P2^7;

sfr	IEN0	=	0xA8;		/********中断使能控制器0*******/
sbit	EA	=	IEN0^7;		/********中断总控制位*******/
sbit	ET2	=	IEN0^5;		/********定时器2中断控制位*******/
sbit	ES2	=	IEN0^4;		/********UART2*******/
sbit	ET1	=	IEN0^3;		/********T1*******/
sbit	EX1	=	IEN0^2;		/********外部中断1*******/
sbit	ET0	=	IEN0^1;		/********T0*******/
sbit	EX0	=	IEN0^0;		/********外部中断0*******/

sfr	IP0		=	0xA9;				/********中断优先级控制器0*******/
sfr	P3		=	0xB0;

sbit RTC_SDA	=	P3^3;		   /**************时钟***************/
sbit RTC_SCL	=	P3^2;

sfr	IEN1	=	0xB8;				/********中断使能接受控制器******/
sbit	ES5R	=	IEN1^5;			/*****UART5接受中断使能控制位****/
sbit	ES5T	=	IEN1^4;			/*****UART5接受中断使能控制位****/
sbit	ES4R	=	IEN1^3;			/*****UART4接受中断使能控制位****/
sbit	ES4T	=	IEN1^2;			/*****UART4接受中断使能控制位****/
sbit	ECAN	=	IEN1^1;			/********CAN中断使能控制位******/

sfr	IP1		=	0xB9;				/********中断优先级控制器0*******/
sfr	IRCON2	=	0xBF;
sfr	IRCON 	=	0xC0;
sbit	TF2	=	IRCON^6;			/********T2中断触发标志*******/
sfr	T2CON	=	0xC8;				/********T2控制寄存器********/
sbit	TR2	=	T2CON^0;			/***********T2使能***********/
sfr	TRL2H	=	0xCB;
sfr	TRL2L	=	0xCA;
sfr	TH2 	=	0xCD;
sfr	TL2 	=	0xCC;

sfr	PSW		=	0xD0;
sbit	CY	=	PSW^7;
sbit	AC	=	PSW^6;
sbit	F0	=	PSW^5;
sbit	RS1	=	PSW^4;
sbit	RS0	=	PSW^3;
sbit	OV	=	PSW^2;
sbit	F1	=	PSW^1;
sbit	P  	=	PSW^0;
sfr	ADCON	=	0xD8;
sfr	ACC		=	0xE0;
sfr	B 		=	0xF0;

/******硬件扩展定义*********/
/******DGUS变量存储器访问*********/
sfr	RAMMODE	=	0xF8;				/******DGUS变量存储器访问接口控制寄存器*********/
sbit	APP_REQ	=	RAMMODE^7;
sbit	APP_EN	=	RAMMODE^6;
sbit	APP_RW	=	RAMMODE^5;
sbit	APP_ACK	=	RAMMODE^4;
sfr ADR_H	=	0xF1;
sfr ADR_M	=	0xF2;
sfr ADR_L	=	0xF3;
sfr ADR_INC	=	0xF4;
sfr DATA3	=	0xFA;
sfr DATA2	=	0xFB;
sfr DATA1	=	0xFC;
sfr DATA0	=	0xFD;


//UART4
sfr	SCON4T	=	0x96;					/******UART4发送控制********/
sfr	SCON4R	=	0x97;					/******UART4接收控制*********/
sfr	BODE4_DIV_H	=	0xD9;				/******波特率设置********/
sfr	BODE4_DIV_L	=	0xE7;
sfr	SBUF4_TX	=	0x9E;				/******UART4发送数据接口********/
sfr	SBUF4_RX	=	0x9F;				/******UART4接收数据接口*********/
sbit    TR4     =   P0^0; 				/******口4的485方向控制**********/            
//UART5
sfr	SCON5T	=	0xA7;
sfr	SCON5R	=	0xAB;
sfr	BODE5_DIV_H	=	0xAE;
sfr	BODE5_DIV_L	=	0xAF;
sfr	SBUF5_TX	=	0xAC;
sfr	SBUF5_RX	=	0xAD;
sbit    TR5     =   P0^1; 				/******口5的485方向控制**********/ 
//CAN通信
sfr	CAN_CR	=	0x8F;
sfr	CAN_IR	=	0x91;
sfr	CAN_ET	=	0xE8;

//GPIO
sfr	P0MDOUT	=	0xB7;
sfr	P1MDOUT	=	0xBC;
sfr	P2MDOUT	=	0xBD;
sfr	P3MDOUT	=	0xBE;
sfr	MUX_SEL	=	0xC9;
sfr	PORTDRV	=	0xF9;				   /******输出驱动强度*********/

//MAC&DIV
sfr	MAC_MODE	=	0xE5;
sfr	DIV_MODE	=	0xE6;

//SFR扩展接口
sfr	EXADR	=	0xFE;
sfr	EXDATA	=	0xFF;

//看门狗宏定义
#define	WDT_ON()				MUX_SEL|=0x02		//开启看门狗
#define	WDT_OFF()				MUX_SEL&=0xFD		//关闭看门狗
#define	WDT_RST()				MUX_SEL|=0x01		//喂狗
//系统主频和1ms定时数值定义
//#define FOSC     				206438400UL
//#define T1MS    				(65536-FOSC/12/1000)

#define       FOSC          206438400UL
#define       FRAME_LEN     255		    	//帧长
//===========================================
void Write_Dgus(u16 Dgus_Addr,u16 Val);
u16 Read_Dgus(u16 Dgus_Addr);
void write_dgus_vp(unsigned int addr,unsigned char *buf,unsigned int len);
void read_dgus_vp(unsigned int addr,unsigned char *buf,unsigned int len);
void Write_Dgusii_Vp_byChar(unsigned int addr,unsigned char *buf,unsigned int len);

#endif 
/*
T5   S4_S5_15667200/9600=1632=0X0660    S4_S5_115200=136=0X0088
T5L  s4_s5_25804800/9600=2688=0X0A80    S4_S5_ 115200=00E0   s2_115200=03E4 S2_9600=02B0  
T5L  s3 cpu主频=晶体频率*56/3	 11.0592mhz=206.4384  SREL3H_L=1024-(cpu主频/（32*波特率)）	1024-206438400/（32*115200）=1024-56=968=03c8
#define     BaudH	    0X03            
#define     BaudL	    0XE4           
#define     DTHD1       0X5A			  //帧头1
#define     DTHD2       0XA5			  //帧头2


u16         xdata         TimVal=1000 ;
u8          xdata         TimVa2=0 ;
u8          xdata         DMTwt=0  ; 	  //视频芯片初始化指令发送后=1  应答完成=2
u8          xdata         DrawL_Val[13]={0xAA,0xBB,0x0A,0x61,0x02,0x00,0x00,0x00,0x00,0x03,0x20,0x03,0x20}; //页面擦除
//**********************************************************
u8          xdata         R_u2[255];	  //串口2接受数组
u8          xdata         R_OD2=0;	      //串口2收到数据
u16         xdata         R_CN2=0;		  //口2长度计数器
u8          xdata         T_O2=0;		  //口2超时计数器
bit			              Busy2=0;	      //串口2发送标志
//**********************************************************
u8          xdata         R_u3[255];	  //串口3接受数组
u8          xdata         R_OD3=0;	      //串口3收到数据
u16         xdata         R_CN3=0;		  //口3长度计数器
u8          xdata         T_O3=0;		  //口3超时计数器
bit			              Busy3=0;	      //串口3发送标志
/**********************************************************
u8          xdata         R_u4[255];	  //串口4接受数组
u8          xdata         R_OD4=0;	      //串口4收到数据
u16         xdata         R_CN4=0;		  //口4长度计数器
u8          xdata         T_O4=0;		  //口4超时计数器
bit			              Busy4=0;	      //串口4发送标志
/**********************************************************
u8          xdata         R_u5[255];	  //串口5接受数组
u8          xdata         R_OD5=0;	      //串口5收到数据
u16         xdata         R_CN5=0;		  //口5长度计数器
u8          xdata         T_O5=0;		  //口5超时计数器
bit			              Busy5=0;	      //串口5发送标志

/*======================================== 串口2初始化
			ADCON = 0x80;            //选择SREL0H:L作为波特率发生器
			SCON2 = 0x50;            //接受使能和模式设置
			PCON &= 0x7F;            //SMOD=0
			SREL2H = BaudH;  
			SREL2L = BaudL;
			IEN0 = 0X10;             //ES0=1	 串口2 接受+发送中断
/*========================================串口3初始化
			MUX_SEL|=0x20;    //UART3引出
			P0MDOUT|=0x40;	  //P0.6 TXD 推挽
			SREL3H = 0X03;    //115200=0X03c8	         cpu主频=晶体频率*56/3	 11.0592mhz=206.4384  SREL3H_L=1024-(cpu主频/（32*波特率)）	1024-206438400/（32*115200）=1024-56=968=03c8
	        SREL3L = 0Xc8;	  //						 
			SCON3 = 0x90;     //接受使能和模式设置		 
			IEN2 |= 0x01;	  //中断使能
//========================================串口4初始化  
		    BODE4_DIV_H =BaudH	;
			BODE4_DIV_L =BaudL;
		    SCON4T= 0x80;     //发送使能和模式设置
		    SCON4R= 0x80;     //接受使能和模式设置 
			ES4R = 1;         //中断接受使能
		    ES4T = 1;         //中断发送使能
			TR4=0;
//========================================串口5初始化
			BODE5_DIV_H =Baud_H	;
			BODE5_DIV_L =Baud_L;
		    SCON5T= 0x80	;//发送使能和模式设置
		    SCON5R= 0x80;//接受使能和模式设置 
			ES5R = 1;//中断接受使能
		    ES5T = 1;//中断发送使能
			TR5=0;
//**************************************************************************** 
void  OneSendData3(u8 Dat)
{           while (Busy3);               
			Busy3 = 1;
			SBUF3 = Dat; 
}
/****************************************************************************
void  OneSendData4(u8 Dat)
{           while (Busy4);               
			Busy4 = 1;
			SBUF4_TX = Dat; 
}
/****************************************************************************
void  ReceiveDate2(u8 Dat)
{			while (Busy2);               
			Busy2 = 1;
			SBUF2 = Dat; 
}
****************************************************************************
void  ReceiveDate5()
{			while (Busy5);               
			Busy5 = 1;
			SBUF5_TX = Dat; 
}
/*********************************************************
void uart2_Risr()	    interrupt 4   //串口2收发中断
{           
           if(RI2==1)
		   {        R_u2[R_CN2]=SBUF2;
		            SCON2&=0xFE;
		            R_OD2=1;
		            R_CN2++;
		            T_O2=4;    //Cs=2;
		   }
		   if(TI2==1)
		   { 
		            SCON2&=0xFD ; 
			        Busy2=0;
		   }
}  
//*********************************************************
void uart3_Risr()	    interrupt 16   //串口3收发中断
{           
           if(SCON3&0x01==1)
		   {        
		            R_u3[R_CN3]=SBUF3;
		            SCON3&=0xFE;
		            R_OD3=1;
		            R_CN3++;
		            T_O3=4;    
		   }
		   if(SCON3&0x02==2)
		   { 
		            SCON3&=0xFD ; 
			        Busy3=0;
		   }
} 
/********************************************************* 
void uart4_Tisr()	    interrupt 10  //串口4发送中断
{           SCON4T&=0xFE ; 
			Busy4=0;
}
/*********************************************************
void uart4_Risr()	    interrupt 11   //串口4中断接收
{           R_u4[R_CN4]=SBUF4_RX;
            SCON4R&=0xFE;
            R_OD4=1;
            R_CN4++;
            T_O4=4;   
}
//*********************************************************
void uart5_Risr()	    interrupt 13   //串口5中断接收
{           
            R_u5[R_CN5]=SBUF5_RX;
            SCON5R&=0xFE;
            R_OD5=1;
            R_CN5++;
            T_O5=4;   
}
//========================
void uart5_Tisr()	    interrupt 12
{           
	        SCON5T&=0xFE ; 
			Busy5=0;
}
//=========================	*/


