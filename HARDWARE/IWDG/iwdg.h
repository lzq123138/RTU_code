#ifndef _IWDG_H
#define _IWDG_H
#include "sys.h"
#include "stm32f4xx_conf.h"

void IWDG_Init(uint8_t prer,uint16_t rlr);//IWDG³õÊ¼»¯
void IWDG_Feed(void);  //Î¹¹·º¯Êý
#endif
