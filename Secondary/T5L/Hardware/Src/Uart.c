#include "Uart.h"


//**********************************************************
#if UART2_ENABLE
u8          xdata         R_u2[UART2_RX_LENTH];	  //串口2接受数组
#else
u8          xdata         R_u2[20];	  //串口2接受数组
#endif
//u8          xdata         W_u2[135];	  //串口2发送数组
u8          xdata         R_OD2=0;	      //串口2收到数据
u16         xdata         R_CN2=0;		    //口2长度计数器
u8          xdata         T_O2=0;		      //口2超时计数器
bit			                  Busy2=0;	      //串口2发送标志
//**********************************************************
#if UART3_ENABLE
u8          xdata         R_u3[UART3_RX_LENTH];	  //串口3接受数组
#else
u8          xdata         R_u3[20];	  //串口3接受数组
#endif
u8          xdata         R_OD3=0;	      //串口3收到数据
u16         xdata         R_CN3=0;		    //口3长度计数器
u8          xdata         T_O3=0;		      //口3超时计数器
bit			                  Busy3=0;	      //串口3发送标志
// ========================================================================
#if UART4_ENABLE
u8          xdata        R_u4[UART4_RX_LENTH];	        //串口4接受数组
#else
u8          xdata        R_u4[20];	        //串口4接受数组
#endif
//u8        xdata        W_u4[255];	        //串口4发送数组
u8          xdata        R_OD4=0;	          //串口4收到数据
u16         xdata        R_CN4=0;		        //口4长度计数器
u8         xdata         T_O4=0;		        //口4超时计数器
bit			                 Busy4=0;	        //串口4发送标志
//**********************************************************
#if UART5_ENABLE
u8          xdata         R_u5[UART5_RX_LENTH];	  //串口5接受数组
#else
u8          xdata         R_u5[20];	  //串口5接受数组
#endif
//u8          xdata         W_u5[135];	  //串口5发送数组
u8          xdata         R_OD5=0;	      //串口5收到数据
u16         xdata         R_CN5=0;		    //口5长度计数器
u8          xdata         T_O5=0;		      //口5超时计数器
bit			                  Busy5=0;	      //串口5发送标志
//**********************************************************


bit                 Response_flog=0;  //应答用
bit                 Auto_data_upload=0;//数据自动上传用
bit                 Crc_check_flog=0;//Crc校验标记

/****************************************************************************/
#if UART2_ENABLE
void Uart2_Init(u32 baud_rate){
	     u16 i=0;
      i=1024-FOSC/64/baud_rate;
	    SREL2H = (u8)(i>>8);		//波特率 = FCLK/64*(1024-SREL)
	    SREL2L = (u8)i;    // 
	    MUX_SEL|=0x40;                         //UART2引出,UART3不引出，WDT关闭	
			ADCON = 0x80;            //选择SREL0H:L作为波特率发生器
			SCON2 = 0x50;            //接受使能和模式设置
			PCON &= 0x7F;            //SMOD=0
			IEN0 |= 0X10;                    //ES0=1	 串口2 接受+发送中断
			EA  = 1;	
}
#endif
/****************************************************************************/
#if UART3_ENABLE
void Uart3_Init(u32 baud_rate){
    	u16 i=0;
      i=1024-FOSC/32/baud_rate;
	    SREL3H = (u8)(i>>8);		//波特率 = FCLK/64*(1024-SREL)
	    SREL3L = (u8)i;    // S_2/115200=224=0X03C8 1024-206438400/(32*115200)=0X03C8
			MUX_SEL|=0x20;    //UART3引出
			P0MDOUT|=0x40;	  //P0.6 TXD 推挽				 
			SCON3 = 0x90;     //接受使能和模式设置		 
			IEN2 |= 0x01;	  //中断使能		
     	EA  = 1;	
}
#endif
/****************************************************************************/
#if UART4_ENABLE
void Uart4_Init(u32 baud_rate){
	    u16 i=0;
      i=FOSC/8/baud_rate;
    	BODE4_DIV_H = (u8)(i>>8);		//
      BODE4_DIV_L = (u8)i;    // 
		  SCON4T= 0x80	;//发送使能和模式设置
		  SCON4R= 0x80;//接受使能和模式设置 
			ES4R = 1;//中断接受使能
		  ES4T = 1;//中断发送使能
	    P0MDOUT|=(1<<0); //p0^0 强推
			TR4=0;
			EA  = 1;	
}
#endif
/****************************************************************************/
#if UART5_ENABLE
void Uart5_Init(u32 baud_rate){
    	 u16 i=0;
       i=FOSC/8/baud_rate;
       BODE5_DIV_H = (u8)(i>>8);		//
       BODE5_DIV_L = (u8)i;    // 
		   SCON5T= 0x80;//发送使能和模式设置,8bit模式
//		 SCON5T= 0xC0;//发送使能和模式设置,9bit模式
		   SCON5R= 0x80;//接受使能和模式设置 
		   ES5R = 1;//中断接受使能
		   ES5T = 1;//中断发送使能
	     P0MDOUT|=(1<<1); //p0^1 强推
			 TR5=0;	
		   EA  = 1;	
}
#endif
/*************************************************************************************************/
void uart_init(){
#if UART2_ENABLE
	          Uart2_Init(BAUD_UART2);
#endif	
#if UART3_ENABLE
            Uart3_Init(BAUD_UART3);
#endif	
#if UART4_ENABLE	
	          Uart4_Init(BAUD_UART4);
#endif	
#if UART5_ENABLE	
	          Uart5_Init(BAUD_UART5);
#endif	
}

/*************************************************************
函数名：void  uart_send_byte(u8 Uart_number,u8 Dat)
参数：Uart_number=串口号=2，3，4，5；Dat为需要发送的byte
作者：cuijia
日期：20220526
***********************************************************/
void  uart_send_byte(u8 Uart_number,u8 Dat)
{
     if(Uart_number==2){
    	    while (Busy2);               
			    Busy2 = 1;
		    	SBUF2 = Dat; 		 		 
		 }else if(Uart_number==3){
		     while (Busy3);               
			   Busy3 = 1;
		     SBUF3 = Dat; 		 		 
		 }else if(Uart_number==4){
		     while (Busy4);               
			   Busy4 = 1;
			   SBUF4_TX = Dat; 		 		 
		 }else if(Uart_number==5){
		     while (Busy5);               
		   	 Busy5 = 1;
			   SBUF5_TX = Dat; 		 		 
		 }
}
#if 1
/*************************************************************
函数名：void  uart_send_str(u8 U_number,u8* Str,u8 Len)
参数：U_number=串口号=2，3，4，5；Str需要发送的字符串，发送长度Len
作者：cuijia
日期：20230331
***********************************************************/
void  uart_send_str(u8 U_number,u8* Str,u8 Len)
{
    u8 i=0;
	       if(U_number==4)       TR4=1;
         else if(U_number==5)  TR5=1; 
	       for(i=0;i<Len;i++)
	      {
	             uart_send_byte(U_number,*Str++);
	      }
				if(U_number==4) {
								  while(Busy4);TR4=0;
				}else if(U_number==5){
								while(Busy5);TR5=0; 
				}
}
#endif
/****************************数据自动上传************************************************/
void uart_data_send(u8* Arr,u8 Uartnum,bit Outo_send,bit Crc_ck )
{
	        u8   i=0; 
	        u16  V1=Read_Dgus(0x0f01);	        
          Auto_data_upload=Outo_send;
					if(Auto_data_upload) 
					{
						 Crc_check_flog=Crc_ck;						 
						 if(Crc_check_flog) //带Crc校验
						 {
							   u16 Crc=0;	
							   Arr[2] = (((u8)V1)<<1)+6;								   					 						 								   
   							 Crc=crc16table(Arr+3,Arr[2]-2);
		             Arr[Arr[2]+1]=Crc& 0x00FF;
		             Arr[Arr[2]+2]=Crc>> 8;	
                uart_send_str(Uartnum,Arr,Arr[2]+3);						 
							   Arr[2] = (((u8)V1)<<1)+4;
						 }else
             {
                  uart_send_str(Uartnum,Arr,Arr[2]+3);												 
						 }						 						 
					}
}
/****************************数据自动上传************************************************/
void  Sw_Data_Send ()
{
		  u16  Va=Read_Dgus(0x0f00);
			u16  V1=Read_Dgus(0x0f01);
			if(((u8)(Va>>8))==0x5A)
			{	   u8 i=0;u16 Temp=0;
				   u8 Val[100]={0}; 	   //5A A5 06 83 ADDR Len XX XX 
					 Val[0] = DTHD1;
		       Val[1] = DTHD2;
					 Val[2] = (((u8)V1)<<1)+4;
		       Val[3] = 0x83;
				   Val[4]=(u8)Va; Val[5]=(u8)(V1>>8);
					 Val[6]=(u8)V1;
					 for(i=0;i<(u8)V1;i++)
					{
						  Temp=Read_Dgus(((Val[4]<<8)+Val[5]+i));
						  Val[7+2*i]=(u8)(Temp>>8);
						  Val[8+2*i]=(u8)(Temp);
					 }
#if UART2_ENABLE							
	        uart_data_send(Val,2,DATA_UPLOAD_UART2,CRC_CHECK_UART2);
#endif	
#if UART3_ENABLE					 
          uart_data_send(Val,3,DATA_UPLOAD_UART3,CRC_CHECK_UART3);
#endif	
#if UART4_ENABLE					 
					uart_data_send(Val,4,DATA_UPLOAD_UART4,CRC_CHECK_UART4);
#endif	
#if UART5_ENABLE					 
          uart_data_send(Val,5,DATA_UPLOAD_UART5,CRC_CHECK_UART5);	
#endif					
					Write_Dgus(0x0f00,0);Write_Dgus(0x0f01,0);
			}
}
	
/****************************************************************************/
void  deal_82_cmd(u8 Uart,u8* arr)
{    
	   u8 i=0;
     if(Crc_check_flog==0) //不含CRC检验
     {		 
			      if(arr[4]==0&&arr[5]==6) //更新c代码
						{
							 arr[7]=0xA5;
						}
            Write_Dgusii_Vp_byChar((arr[4]<<8)+arr[5],arr+6,arr[2]-3);												 
            if(Response_flog)   
            {
                 u8 Temp_arr[]={DTHD1,DTHD2,0X03,0X82,0X4F,0X4B}; 	
                 uart_send_str(Uart,Temp_arr,6);					                    	 
            }
      }else //带CRC校验
      {
           u16 Crc=0,Crc_check=0;
           Crc=crc16table((u8*)(&arr[3]),arr[2]-2);
           Crc_check=(u16)(arr[3+arr[2]-1]<<8)+(u16)(arr[3+arr[2]-2]);  
           if(Crc==Crc_check)
           {
                Write_Dgusii_Vp_byChar((arr[4]<<8)+arr[5],arr+6,arr[2]-5);																				
                if(Response_flog)   
                { 
                   u8 Temp_arr[]={DTHD1,DTHD2,0X05,0X82,0X4F,0X4B,0XA5,0XEF}; 	
	                 uart_send_str(Uart,Temp_arr,8);			
                }												 
           }											 
			 }
}
/****************************************************************************/
void  deal_83_cmd(u8 Uart,u8* arr,u8* arr1)
{
	     u8 i=0;
       if(Crc_check_flog==0) //不含CRC检验
		 	{		
              for(i=0;i<7;i++)
									 arr[i]=arr1[i];
						 read_dgus_vp((arr[4]<<8)+arr[5],(u8*)&arr[7],arr[6]);
						 arr[2]=(2*arr[6])+4;
             uart_send_str(Uart,arr,arr[2]+3);									 
		   }else   //带Crc检验
		  {
						 u16 Crc=0,Crc_check=0;
						 for(i=0;i<9;i++)
									 arr[i]=arr1[i];
						 Crc=crc16table((u8*)(&arr[3]),arr[2]-2);
						 Crc_check=(u16)(arr[3+arr[2]-1]<<8)+(u16)(arr[3+arr[2]-2]);  
					  if(Crc==Crc_check)
						{
								  read_dgus_vp((arr[4]<<8)+arr[5],(u8*)&arr[7],arr[6]);
								  arr[2]=(2*arr[6])+4+2;
								  Crc=crc16table(arr+3,arr[2]-2);
								  arr[arr[2]+1]=Crc& 0x00FF;
		              arr[arr[2]+2]=Crc>> 8;
	                uart_send_str(Uart,arr,arr[2]+3);	
						}									     
			 }
}
/*************************************************************************************************************
函数名：deal_uart_data(u8* Arr_rece,u16* Data_len,u8 Uart_num,bit Response,bit Crc_ck)
参数：Arr_rece为串口接收数组，Data_len接收数据长度地址，Uart_num串口号，Response是否带应答，Crc_ck是否带校验
作者：cuijia
日期：20220526
*************************************************************************************************************/
void  deal_uart_data(u8* Arr_rece,u16* Data_len,u8 Uart_num,bit Response,bit Crc_ck)
{
     u16 N=0;	bit Flog=1;
			while(Flog)
			{		if((Arr_rece[N]==DTHD1)&&(Arr_rece[N+1]==DTHD2))   //5A A5 07 82 1000 0001 0002
					{	     if(Arr_rece[N+3]==0x82)     
						     {	   
									     Response_flog=Response;//应答标记
									     Crc_check_flog=Crc_ck;//Crc标记
                       deal_82_cmd(Uart_num,Arr_rece+N);//处理82指令
									     Arr_rece[N]=0;
			                 N=N+Arr_rece[N+2]+2;
						     }								   // 0	 1	2  3  4	5  6  7	8  9 10
						     else if(Arr_rece[N+3]==0x83) 		   //5A A5 08 83 1000 02 0001 0002
						     {	   
									    
								   		 u8 Val[FRAME_LEN]={0}; 
										   Crc_check_flog=Crc_ck;//Crc标记
                       deal_83_cmd(Uart_num,Val,Arr_rece+N);//处理83指令
											 N=N+Arr_rece[N+2]+2;
						     }else
						      N++;
					}
					if(N<(*Data_len-4)) N++;
					else            Flog=0; 
			}
}

/*************************************************************************************************/
void uart_frame_deal(){
				    if((1==R_OD2)&&(T_O2==0)){
								 deal_uart_data(R_u2,(u16*)(&R_CN2),2,RESPONSE_UART2,CRC_CHECK_UART2);
								 R_OD2=0;R_CN2=0;
						} //串口2数据收发
						if((1==R_OD3)&&(T_O3==0)){
						     deal_uart_data(R_u3,(u16*)(&R_CN3),3,RESPONSE_UART3,CRC_CHECK_UART3);
					       R_OD3=0;R_CN3=0;
						} //串口2数据收发
            if((1==R_OD4)&&(T_O4==0)){
							   deal_uart_data(R_u4,(u16*)(&R_CN4),4,RESPONSE_UART4,CRC_CHECK_UART4);
							   R_OD4=0;R_CN4=0;
						} //串口4数据收发
						if((1==R_OD5)&&(T_O5==0)){
						     deal_uart_data(R_u5,(u16*)(&R_CN5),5,RESPONSE_UART5,CRC_CHECK_UART5);
							   R_OD5=0;R_CN5=0;						
						} //串口5数据收发
}
//=========================//
//=========中断函数========//
//=========================//

//========================
void uart4_Risr()	    interrupt 11   //串口4中断接收
{           
            R_u4[R_CN4]=SBUF4_RX;
            SCON4R&=0xFE;
            R_OD4=1;
            if(R_CN4<UART4_RX_LENTH-1) R_CN4++;
            T_O4=5;   
}
//========================
void uart4_Tisr()	    interrupt 10
{           
	    SCON4T&=0xFE ; 
			Busy4=0;
}
//=========================	 
void uart5_Risr()	    interrupt 13   //串口5中断接收
{           
            R_u5[R_CN5]=SBUF5_RX;
            SCON5R&=0xFE;
            R_OD5=1;
            if(R_CN5<UART5_RX_LENTH-1) R_CN5++;
            T_O5=5;   
}
//========================
void uart5_Tisr()	    interrupt 12
{           
	        SCON5T&=0xFE ; 
			Busy5=0;
}
//========================
//*********************************************************
void uart3_Risr()	    interrupt 16   //串口3收发中断
{           
       if(SCON3&0x01)
		   {        
		            R_u3[R_CN3]=SBUF3;
		            SCON3&=0xFE;
//				        SCON3&=0xFE;
		            R_OD3=1;
		            if(R_CN3<UART3_RX_LENTH-1) R_CN3++;
		            T_O3=5;    
		   }
		   if(SCON3&0x02)
		   { 
		            SCON3&=0xFD ; 
				        SCON3&=0xFD ; 
			          Busy3=0;
		   }
} 
//*********************************************************
void uart2_Risr()	    interrupt 4   //串口2收发中断
{           
       if(RI2==1)
		   {        
		            R_u2[R_CN2]=SBUF2;
		            SCON2&=0xFE;
		            R_OD2=1;
		            if(R_CN2<UART2_RX_LENTH-1) R_CN2++;
		            T_O2=5;    
		   }
		   if(TI2==1)
		   { 
		            SCON2&=0xFD ; 
			          Busy2=0;
		   }
}
