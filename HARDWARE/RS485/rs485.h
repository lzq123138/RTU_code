#ifndef __RS485_H
#define __RS485_H			 
#include "sys.h"	 								  
	  	
extern uint8_t RS485_RX_BUF[128]; 		//接收缓冲,最大64个字节
extern uint8_t RS485_RX_CNT;   			//接收到的数据长度

//模式控制
#define RS485_TX_EN		PAout(8)	//485模式控制.0,接收;1,发送.
#define RS485_MODE_EN	PAout(11)   //485口控制 0-->下泄流量 1-->外接传感器等
														 
void RS485_Init(uint32_t bound);
void RS485_Send_Data(uint8_t *buf,uint8_t len);
void RS485_Receive_Data(uint8_t *buf,uint8_t *len);
void switchRS485Mode(uint8_t mode);
void RS485Config(uint32_t bound,uint16_t wordLength,uint16_t stopbits,uint16_t parity);
#endif	   
















