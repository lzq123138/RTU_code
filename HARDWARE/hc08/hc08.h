#ifndef __HC08_H
#define __HC08_H

#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_conf.h"

void hc08_init(uint32_t bound);
void Uart3_SendStr(uint8_t *buf,uint16_t size);
void read_uart3(void);
void clear_hc08_data(void);

uint16_t createSuccessData(uint8_t *data);
uint16_t createFailedData(uint8_t *data);
uint16_t createDeviceProtocolData(uint8_t *data);
uint16_t createDeviceRealData(uint8_t *data);
uint16_t createPlatformData(uint8_t *data);
uint16_t createRTUData(uint8_t *data);

uint8_t hc08_data_to_param(uint8_t *data,uint8_t setUser);
uint8_t parse_hc08_data(uint8_t *mode,uint8_t *param,uint8_t *data,uint16_t dataSize);

void TIM7_Int_Init(uint16_t arr,uint16_t psc);


void getDeviceNumber(void);
void setBluetoothName(void);
#endif
