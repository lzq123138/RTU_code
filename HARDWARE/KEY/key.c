#include "key.h"
#include "delay.h" 
#include "stm32f4xx_conf.h"
	 

//������ʼ������
void KEY_Init(void)
{	
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);//GPIOEʱ��
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4 | GPIO_Pin_5; //KEY0 KEY1 KEY2��Ӧ����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//��ͨ����ģʽ
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOE, &GPIO_InitStructure);//��ʼ��GPIOE2,3,4,5
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




















