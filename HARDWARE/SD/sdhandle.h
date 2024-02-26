#ifndef _SD_HANDLE_H
#define _SD_HANDLE_H

#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_conf.h"
#include "protocol.h"
#include "sdio_sdcard.h"


typedef struct
{
	uint32_t 	_size;
	uint32_t 	_pop_sector;
	uint32_t 	_push_sector;
}history_data_index_t;

typedef struct
{
	uint8_t 	_write_flag;
	uint32_t 	_config_sector;
}sd_config_t;

typedef struct
{
	uint32_t	_current_page;
	uint32_t	_current_line;
	uint32_t	_current_menu;
}sd_menu_index_t;

typedef struct
{
	sd_menu_index_t			_index;
	sd_config_t				_config;
	history_data_index_t	_history[6];//最大支持六路平台 所以有六路平台的数据
}SD_menu_t;

uint8_t SDReadConfig(device_manager_t *dmanager,platform_manager_t *pManager,battery_data_t *bdata);

uint8_t SDWriteConfig(device_manager_t *dmanager,platform_manager_t *pManager,battery_data_t *bdata);

uint32_t getHistorySize(uint8_t platformIndex);

uint8_t SDPopHistoryData(EC20_send_data_t *pack,uint8_t platformIndex);

uint8_t SDPushHistoryData(EC20_send_data_t *pack,uint8_t platformIndex);

uint8_t clearHistoryData(void);

#endif

