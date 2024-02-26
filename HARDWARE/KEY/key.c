#include "key.h"
#include "delay.h" 
#include "stm32f4xx_conf.h"
	 

//按键初始化函数
void KEY_Init(void)
{	
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);//GPIOE时钟
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4 | GPIO_Pin_5; //KEY0 KEY1 KEY2对应引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOE, &GPIO_InitStructure);//初始化GPIOE2,3,4,5
} 

uint8_t KEY_Scan(uint8_t mode)
{	 	
	uint8_t KeyNum = 0;


	if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) == 0)
	{
		delay_ms(20);
		while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) == 0);
		delay_ms(20);
		KeyNum = KEY_ENTER_PRES;
	}
	else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0)
	{
		delay_ms(20);
		while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) == 0);
		delay_ms(20);
		KeyNum = KEY_ESC_PRES;
	}
	else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == 0)
	{
		delay_ms(20);
		while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_4) == 0);
		delay_ms(20);
		KeyNum = KEY_DOWN_PRES;
	}
	else if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5) == 0)
	{
		delay_ms(20);
		while (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_5) == 0);
		delay_ms(20);
		KeyNum = KEY_UP_PRES;
	}
	return KeyNum;
}




















