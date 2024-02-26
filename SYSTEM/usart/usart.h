#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

void uart2_init(uint32_t bound);
void Uart2_SendStr(char*SendBuf,char len);
#endif


