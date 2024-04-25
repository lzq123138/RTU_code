#ifndef __RS485_H
#define __RS485_H			 
#include "sys.h"	 								  
	  	
extern uint8_t RS485_RX_BUF[128]; 		//���ջ���,���64���ֽ�
extern uint8_t RS485_RX_CNT;   			//���յ������ݳ���

//ģʽ����
#define RS485_TX_EN		PAout(8)	//485ģʽ����.0,����;1,����.
#define RS485_MODE_EN	PAout(11)   //485�ڿ��� 0-->��й���� 1-->��Ӵ�������
														 
void RS485_Init(uint32_t bound);
void RS485_Send_Data(uint8_t *buf,uint8_t len);
void RS485_Receive_Data(uint8_t *buf,uint8_t *len);
void switchRS485Mode(uint8_t mode);
void RS485Config(uint32_t bound,uint16_t wordLength,uint16_t stopbits,uint16_t parity);
#endif	   
















