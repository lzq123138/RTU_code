#ifndef __MODBUS_H
#define __MODBUS_H

#include <stm32f4xx.h>

uint8_t Modbus_Send_cmd(uint8_t *cmd,uint16_t cmdSize,uint8_t device_number);

#endif

