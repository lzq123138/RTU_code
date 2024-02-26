#ifndef __HANDLE_WATER_H
#define __HANDLE_WATER_H

#include "protocol.h"
#include "modbus.h"
#include "adc.h"
#include "stm32f4xx_conf.h"


void handle_water_data(uint8_t index);

void readWaterData(uint8_t *data,uint16_t size,uint8_t index);

void failedWaterData(uint8_t index);

#endif
