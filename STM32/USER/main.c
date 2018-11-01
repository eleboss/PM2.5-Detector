#include "delay.h"
#include "sys.h"
#include "lcd.h"			 //显示模块
#include "usart.h"
#include "string.h" 
#include "spi.h" 
#include "led.h" 
#include "adc.h" 
#include "gp2y1010au.h"
//////////////////////////////////////////////////////////

/*******************************
*连线说明：

************************************/
/*全局变量*/
u8 t;
u8 len=3;//读写错误重试次数
float density;
u16 hcho_data=0;
/*函数声明*/
float HCHO_DataGet()//串口二发送通讯指令0x42 0x4d 0x01 0x00 0x00 0x00 0x90
{
	u16 data;
	USART_SendData(USART2,0x42);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	USART_SendData(USART2,0x4d);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	USART_SendData(USART2,0x01);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	USART_SendData(USART2,0x00);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	USART_SendData(USART2,0x00);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	USART_SendData(USART2,0x00);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	USART_SendData(USART2,0x90);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
	
	len=USART_RX_STA2&0x3f;
	if(len==10)
	{
		if(1)
		data=((USART_RX_BUF2[6]<<8)+USART_RX_BUF2[7]);//实际数据等于测量数据除当量
	}	
	USART_RX_STA2=0;
	return data;
}
int main(void)
{		

 	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	LED_Init();	
	uart_init(9600);
	uart_init2(9600);
	SPI1_Init();
	LCD_Init();
	GP2Y_Init();
	beep=1;
	LCD_ShowString(0,30,200,16,16,"Init ok");
	delay_ms(1000);
	LCD_Clear(WHITE);
  while(1) 
	{	
		if(PBin(1)==0)//CO2报警
		{
			delay_ms(5);
			if(PBin(1)==0)
			{
				beep=1;
				LCD_Clear(WHITE);
				LCD_ShowString(0,160,200,16,16,"Dangerous!!!");
				LCD_ShowString(0,180,200,16,16,"CO is too high!!!");
				LCD_ShowString(0,200,200,16,16,"Leave here now!");
				delay_ms(1000);
				LCD_Clear(WHITE);
			}
		}
		else
		{
			beep=0;
			LCD_ShowString(0,160,200,16,16,"Liquid gas concentration：");
			LCD_ShowString(0,180,200,16,16,"Safety place");
		}
		
		density = GP2Y_GetDensity();//PM2.5
		LCD_ShowString(0,10,200,16,16,"PM2.5 concentration:");
		LCD_ShowNum(0,30,density,5,16);
		LCD_ShowString(0,50,200,16,16,"Ug/m3");
	
		hcho_data=HCHO_DataGet();
		LCD_ShowString(0,90,200,16,16,"HCHO concentration:");
		LCD_ShowString(0,110,200,16,16,"0.00");
		LCD_ShowxNum(33,110,hcho_data,3,16,0xfe);
		LCD_ShowString(0,130,200,16,16,"g/m3");
		
		if(hcho_data>=100)
		{
			beep=1;
			LCD_ShowString(30,210,200,20,16,"ATTENTION!");
			LCD_ShowString(0,240,200,16,16,"This HCHO concentration");
			LCD_ShowString(0,260,200,16,16,"is harm to your body!!!");
			delay_ms(1000);
			delay_ms(1000);
			delay_ms(1000);
			beep=0;
			LCD_Clear(WHITE);
		}

		
		printf("%4.1f",density);

		delay_ms(500);
		
		//LCD_Clear(WHITE);
//		//if(USART_RX_STA2&0x8000)  //串口接受代码，保留以后看
//			//{ 
//				len=USART_RX_STA2&0x3f; 
//				for(t=0;t<len;t++)
//			{ 
//				USART_SendData(USART2, USART_RX_BUF2[t]); 
//				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);
//			}
//				USART_RX_STA2=0;
//			//}
			
	}
}
