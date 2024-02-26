#ifndef __FLASH_H
#define __FLASH_H

#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_conf.h"
#include "protocol.h"

#define MAX_IP_NUM  6
#define MAX_DEVICE_NUM 6
#define PACKET_SIZE 512


//定义好扇区位置
#define ADDR_FLASH_SECTOR0		((uint32_t)0x08000000)  //16KBytes
#define ADDR_FLASH_SECTOR1		((uint32_t)0x08004000)  //16KBytes
#define ADDR_FLASH_SECTOR2		((uint32_t)0x08008000)  //16KBytes
#define ADDR_FLASH_SECTOR3		((uint32_t)0x0800C000)  //16KBytes
#define ADDR_FLASH_SECTOR4		((uint32_t)0x08010000)  //64KBytes
#define ADDR_FLASH_SECTOR5		((uint32_t)0x08020000)  //128KBytes
#define ADDR_FLASH_SECTOR6		((uint32_t)0x08040000)  //128KBytes
#define ADDR_FLASH_SECTOR7		((uint32_t)0x08060000)  //128KBytes
#define ADDR_FLASH_SECTOR8		((uint32_t)0x08080000)  //128KBytes
#define ADDR_FLASH_SECTOR9		((uint32_t)0x080A0000)  //128KBytes
#define ADDR_FLASH_SECTOR10		((uint32_t)0x080C0000)  //128KBytes
#define ADDR_FLASH_SECTOR11		((uint32_t)0x080E0000)  //128KBytes
#define ADDR_FLASH_END			((uint32_t)0x08100000)



typedef enum
{
	FLASH_READ = 0,
	FLASH_WRITE
}FLASH_USE_MODE;


typedef struct
{
	uint8_t 			_platform_num;
	jxjs_platform_t		_platforms[MAX_IP_NUM];//最多支持8路网络连接
	uint8_t				_device_num;
	jxjs_device_t		_devices[MAX_DEVICE_NUM];
	water_param_t		_water_param;
}flash_save_packet_t;

uint8_t STMFLASH_ReadByte(uint32_t addr);
uint8_t eraseUpdateProSector(void);
uint8_t writeUpdateFlash(uint32_t startaddr,uint8_t *data,uint16_t datasize,uint16_t startIndex);
uint8_t writeFlash(uint32_t addr,uint8_t *data,uint16_t datasize);
uint8_t init_flash_packet(flash_save_packet_t *packet);
flash_save_packet_t read_packet_from_flash(void);
uint8_t write_packet_to_flash(flash_save_packet_t packet,uint32_t *addr);

#endif

