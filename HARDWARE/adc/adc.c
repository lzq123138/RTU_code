#include "adc.h"
#include "sys.h"
#include "delay.h"

 
void Adc_Init(void) 				//ADC通道初始化
{	
	GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStruct;
	ADC_InitTypeDef ADC_InitStruct;
	
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);  
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	ADC_DeInit();//ADC复位
	
	ADC_CommonInitStruct.ADC_DMAAccessMode=ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStruct.ADC_Mode=ADC_Mode_Independent;
	ADC_CommonInitStruct.ADC_Prescaler=ADC_Prescaler_Div6;
	ADC_CommonInitStruct.ADC_TwoSamplingDelay=ADC_TwoSamplingDelay_5Cycles;
	
	ADC_CommonInit(&ADC_CommonInitStruct);
 
 
	ADC_InitStruct.ADC_ContinuousConvMode=DISABLE;
	ADC_InitStruct.ADC_DataAlign=ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ExternalTrigConvEdge=ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_NbrOfConversion=1;
	ADC_InitStruct.ADC_Resolution=ADC_Resolution_12b;
	ADC_InitStruct.ADC_ScanConvMode=DISABLE;
	
	ADC_Init(ADC1, &ADC_InitStruct);
 
	ADC_Cmd(ADC1, ENABLE);
}
 
 
uint16_t  Get_Adc(uint8_t ch) 				//获得某个通道值 
{
	
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );			    
  
	ADC_SoftwareStartConv(ADC1);	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));
 
	return ADC_GetConversionValue(ADC1);		
}
 
 
uint16_t Get_Adc_Average(uint8_t ch,uint8_t times)//得到某个通道给定次数采样的平均值  
{	
	uint32_t temp_val=0;
	uint8_t t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;		
}

//获取某个通道值 采用中位数平均算法 固定取15个数
uint16_t Get_Adc_Median_algorithm(uint8_t ch)
{
	uint16_t nums[16] = {0};
	uint8_t t,i;
	uint8_t size = 0;
	uint16_t tmp;

	uint32_t vaild_value = 0;//有效值总和
	uint8_t valid_number = 0;//有效值个数
	
	//采样数据
	for(t = 0;t < 10;t++)
	{		
		tmp = Get_Adc(ch);
		if(size != 0)
		{
			if((tmp - 50) > nums[size -1] || (tmp + 50) < nums[size - 1])
				return 0;
		} 
		nums[size++] = tmp;
		delay_ms(5);
	}
	
	//排序数据 少量数据直接冒泡排序
	for(i = 0;i<size;i++)
	{
		for(t = i + 1;t < size;t++)
		{
			if(nums[i] > nums[t])
			{
				tmp = nums[i];
				nums[i] = nums[t];
				nums[t] = tmp;
			}
		}
	}
	
	//筛选有效数据 做求和运算
	for(i = 0;i < size;i++)
	{
		if(i < size / 2 + 1)
		{
			if(nums[i] + 3 >= nums[size / 2])
			{
				vaild_value += nums[i];
				valid_number++;
			}
		}
		else
		{
			if(nums[i] - 3 <= nums[size / 2])
			{
				vaild_value += nums[i];
				valid_number++;
			}
		}
	}
	
	return vaild_value / valid_number;
}
 