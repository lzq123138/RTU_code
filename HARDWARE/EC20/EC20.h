#ifndef __EC20_H
#define __EC20_H
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "protocol.h"

#define PWRKEY PEout(1)	// DS0

void PWER_Init(void);
void EC20_Restart(void);
void EC20_check(void);
void EC20_parse(void);
void EC20_Init(void);
void EC20Send_StrData(char *bufferdata);
void EC20Send_MultiStrData(uint8_t channel ,char *bufferdata);
void EC20Send_HexData(char *bufferdata);
void EC20Recv_Data(void);
void EC20Send_HexData_JxJS(uint8_t *data,uint32_t dataLen);
void EC20Send_RecAccessMode(void);
void EC20Send_ChangeMode(uint8_t data);
void EC20Send_RecAccessMode(void);
void EC20Send_HttpPkt(char *phead, char *pbody);
void EC20_MQTTCONNECTServer(void);
void EC20_MQTTCONNECTServer(void);
#endif
