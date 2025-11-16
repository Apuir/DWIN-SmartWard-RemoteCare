#include "save_data_dgus.h"
u16  xdata  Power_Down_Save_Address[]={0x6000,0x6001,0x10DA,0x10DA,0x10DA,0x10DA,0x10DA,0x10DA,0x10DA,0x10DA,0x10D6,0x10D7,0x10D8,0x10D9,
                                       0x10DA,0x10DB,0x10DC,0x10DD,0x10DE};//���籣���ַ
u16  xdata  Power_Down_Save_Data[]={50,50,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//��һ���ϵ����ݳ�ʼ��
#define	    SAVE_NUMBER      2  //��Ҫ����ĵ�ַ����



#define     SEND_FINISH 3
#define     INITIALIZATION_MARK 	  0X5AAA
#define     START_ADD(a)            (0xE270+a)
#define     READ_MOD                 0x5A
#define     WRITE_MOD                0xA5


/*****************************************************************************
�������ܣ���ʼ��
��һ��ʹ�ñ���Ĭ�����ݣ����������ݿ��ȡ����
*/	
void data_save_init()   
{    
	    u16 Ladd=0,Va1=0,Va2=0,i=0; 
			T5L_Flash(READ_MOD,START_ADD(10),START_ADD(10),2);
			if(Read_Dgus(START_ADD(10))==INITIALIZATION_MARK)
			{                    
            	
                   for(i=0;i<SAVE_NUMBER;i++)//���籣���������
				   {
						T5L_Flash(READ_MOD,Power_Down_Save_Address[i],Power_Down_Save_Address[i],2);
						Power_Down_Save_Data[i]= Read_Dgus(Power_Down_Save_Address[i]);
				   }
			}
			else   //��һ��ʹ��				
			{	
				   	Write_Dgus(START_ADD(10),INITIALIZATION_MARK);
				   T5L_Flash(WRITE_MOD,START_ADD(10),START_ADD(10),2);
                   for(i=0;i<SAVE_NUMBER;i++)//���籣���������
				   {
						Write_Dgus(Power_Down_Save_Address[i],Power_Down_Save_Data[i]);//��ʼ������
                        T5L_Flash(WRITE_MOD,Power_Down_Save_Address[i],Power_Down_Save_Address[i],2);
				   }        					
			}                         
		
}
/*****************************************************************************
�������ܣ����ݵ��籣��
���ݸı䱣����
*/
void data_change_sava()
{
	  u16 Val=0;
      u16 i=0,Va2=0;			   
	  for(i=0;i<SAVE_NUMBER;i++)
	  {
			Va2= Read_Dgus(Power_Down_Save_Address[i]);//��ȡ��Ҫ���籣��õ�ַ��ֵ
			if(Power_Down_Save_Data[i]!=Va2)          //���籣���ַ��ֵ��ԭ��ֵ�Ա���ֵ�仯����
			{
				Power_Down_Save_Data[i]=Va2;            //��¼�ı���ֵ
				T5L_Flash(WRITE_MOD,Power_Down_Save_Address[i],Power_Down_Save_Address[i],2);//��������
			}
	  }		
}