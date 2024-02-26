#ifndef __ADC_H
#define __ADC_H
#include "stm32f4xx_conf.h"


void Adc_Init(void);
uint16_t Get_Adc(uint8_t ch);
uint16_t Get_Adc_Average(uint8_t ch,uint8_t times);
uint16_t Get_Adc_Median_algorithm(uint8_t ch);
#endif
