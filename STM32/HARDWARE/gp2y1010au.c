/*********************************************************************************************************
*
* File                : gp2y1010au.c
* Hardware Environment: 
* Build Environment   : RealView MDK-ARM  Version: 4.74
* Version             : V1.0
* By                  : 
*
*                                  (c) Copyright 2005-2011, WaveShare
*                                       http://www.waveshare.net
*                                          All Rights Reserved
*
*********************************************************************************************************/
#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
//#include "arm_cortex_m.h"


#define            COV_RATIO                            0.2            // ug/mmm / mv
#define            NO_DUST_VOLTAGE                      500            //mv


                  

/*
Private Function
*/
static void _GP2Y_HardInit(void);
static void _GP2Y_Wait(void);
static vu32 _GP2Y_GetADCValue(void);
static float _GP2Y_ADCValue2Voltage(vu32 Value);
static float _GP2Y_DataProcess(float Voltage);
static int _filter(int m);



/*******************************************************************************
* Function Name  : static void _GP2Y_HardInit(void)
* Description    : Initialize Hardware
                   1. PA.6    ADC
									 2. PA.7    ILED
* Input          : None
* Output         : None
* Return         : None
* Attention		   : None
*******************************************************************************/
static void _GP2Y_HardInit(void)
{
  ADC_InitTypeDef ADC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

	/*Clock enable*/
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

  /* Configure PA.00 (ADC Channel6) as analog input -------------------------*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);   
	
	/* Configure PA.08 as digital output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure); 	
	GPIO_ResetBits(GPIOB, GPIO_Pin_9);                                             //default low, ILED closed
	
    
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel6 configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));		
}
/*******************************************************************************
* Function Name  : static void _GP2Y_Wait(void)
* Description    : wait 0.28ms
* Input          : 
* Output         : None
* Return         : ADC value
* Attention		   : None
*******************************************************************************/
static void _GP2Y_Wait(void)
{
	delay_us(280);
}
/*******************************************************************************
* Function Name  : static vu32 _GP2Y_GetADCValue(void)
* Description    : 
* Input          : 
* Output         : None
* Return         : ADC value
* Attention		   : None
*******************************************************************************/
static vu32 _GP2Y_GetADCValue(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
	_GP2Y_Wait();
	
	/*Start once cov*/
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != SET)
		;
	ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
	
  GPIO_ResetBits(GPIOB, GPIO_Pin_9);                                             //default low, ILED closed
	
	return _filter(ADC_GetConversionValue(ADC1));
}
/*******************************************************************************
* Function Name  : static float _GP2Y_ADCValue2Voltage(vu32 Value)
* Description    : value -> Voltage
* Input          : ADC value
* Output         : None
* Return         : voltage, unit: v
* Attention		   : None
*******************************************************************************/
static float _GP2Y_ADCValue2Voltage(vu32 Value)
{
	float Temp;
	
	Temp = (3300 / 4096.0) * Value * 11;
	
	return Temp;
}
/*******************************************************************************
* Function Name  : static float _GP2Y_DataProcess(float Voltage)
* Description    : Voltage -> Dust density
* Input          : Voltage: Sensor output voltage, unit: v
* Output         : None
* Return         : Dust density, unit: ug/m*m*m
* Attention		   : None
*******************************************************************************/
static float _GP2Y_DataProcess(float Voltage)
{
	if(Voltage >= NO_DUST_VOLTAGE)
	{
		Voltage -= NO_DUST_VOLTAGE;
		
		return (Voltage * COV_RATIO);
	}
	else
		return 0;
}
/*******************************************************************************
* Function Name  : static int _filter(int m)
* Description    : 
* Input          : 
* Output         : None
* Return         : 
* Attention		   : None
*******************************************************************************/
static int _filter(int m)
{
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  
  if(flag_first == 0)
  {
    flag_first = 1;

    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];
    
    i = sum / 10.0;
    return i;
  }
}








/*******************************************************************************
* Function Name  : void GP2Y_Init(void)
* Description    : 1. Initialize Hardware
* Input          : 
* Output         : None
* Return         : None
* Attention		   : None
*******************************************************************************/
void GP2Y_Init(void)
{
	_GP2Y_HardInit();
}
/*******************************************************************************
* Function Name  : float GP2Y_GetDensity(void)
* Description    : get density
* Input          : 
* Output         : None
* Return         : density
* Attention		   : None
*******************************************************************************/
float GP2Y_GetDensity(void)
{
	float Voltage;
	
	Voltage = _GP2Y_ADCValue2Voltage(_GP2Y_GetADCValue());
	
	Voltage = _GP2Y_DataProcess(Voltage);
	
	return Voltage;
}


















