 #include "modbus.h"
 #include "rs485.h"	
 #include "string.h"	
 #include "delay.h"
 #include "protocol.h"

 extern uint8_t RS485_RX_BUF[128],RS485_RX_CNT;   

uint8_t Modbus_Send_cmd(uint8_t *cmd,uint16_t cmdSize,uint8_t device_number)
{
	uint8_t data[32] = {0};
	uint16_t index = 0;
	uint16_t crc_byte = 0;
	uint16_t i;
	
	if(cmdSize > 29)
		return 0;
	
	data[index++] = device_number;
	for(i = 0;i < cmdSize;i++)
	{
		data[index++] = cmd[i];
	}
	
	crc_byte = modBusCRC16(data,index);
	data[index++] = crc_byte & 0xFF;
	data[index++] = (crc_byte >> 8) & 0xFF;
	
	RS485_Send_Data(data,index);
	
	return 1;
}
