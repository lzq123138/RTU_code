#include "sdhandle.h"

#define SD_HEADER_SIZE 4
static uint8_t SD_header[] = {0x4A,0x58,0x4A,0x53,0x4B,0x4A};

#define SD_PAGE_NUM		100
#define SD_LINE_NUM		100
#define SD_MENU_NUM		100
#define SD_CONFIG_NUM	400000
#define SD_HISTORY_NUM	200000

#define SD_WRITE_CONFIG	0x00
#define SD_PUSH_HISTORY	0x01
#define SD_POP_HISTORY	0x02
#define SD_CLEAR_HISTORY	0x03

#define SD_HIS_CHANNEL	6

/*
内存分布方式
-----------------------------------------------------------------------------------------------------------
|  page   |    line  |	menu |	config	|	his1	|	his2	|	his3	|	his4	|  his5  |  his6  |
-----------------------------------------------------------------------------------------------------------
sector0------------------------------------------------------------------------------->sectorx
*/

SD_menu_t SD_Menu;


static const uint32_t configStartAddr = (SD_LINE_NUM + 1) * SD_PAGE_NUM + SD_PAGE_NUM * SD_LINE_NUM * SD_MENU_NUM;
static const uint32_t historyStartAddr = configStartAddr + SD_CONFIG_NUM;

void initSDMenu(SD_menu_t *menu)
{
	uint8_t i;
	
	//index
	menu->_index._current_page = 0;
	menu->_index._current_line = 0;
	menu->_index._current_menu = 0;
	
	//config
	menu->_config._write_flag = 0;
	menu->_config._config_sector = 0;
	
	//history
	for(i = 0;i < SD_HIS_CHANNEL;i++)
	{
		menu->_history[i]._size = 0;
		menu->_history[i]._pop_sector = 0;
		menu->_history[i]._push_sector = 0;
	}
}


uint16_t appendSDHeader(uint8_t *data)
{
	uint8_t i;
	for(i = 0; i< SD_HEADER_SIZE;i++)
	{
		data[i] = SD_header[i];
	}
	return SD_HEADER_SIZE;
}

//检查数据头 0-->从未在这里写过 1-->最新数据 2-->写过 但已经失效的数据
uint8_t SD_check_header(uint8_t *data,uint16_t dataSize)
{
	uint8_t i;

	if(dataSize < SD_HEADER_SIZE + 1)
		return 0xFF;
	
	for(i = 0;i < SD_HEADER_SIZE;i++)
	{
		if(data[i] != SD_header[i])
			return 0;
	}
	
	return data[SD_HEADER_SIZE] == 0x01 ? 0x01 : 0x02;
}


//返回值 0-->获取失败 证明没有SD卡位初始化过
uint8_t getSdMenu(SD_menu_t *menu)
{	
	uint8_t data[512] = {0};
	uint8_t outFlag = 0;	
	uint8_t j = 0;
	
	
	uint16_t index = 0;
	
	uint32_t i;
	uint32_t byte32 = 0;
	
	//get page
	SD_ReadDisk(data,0,1);
	switch(SD_check_header(data,512))
	{
		case 0x01:
			menu->_index._current_page = 0;//读取到具体的page
			break;
		case 0x02:
			{
				for(i = 1;i < SD_PAGE_NUM;i++)
				{
					SD_ReadDisk(data,i,1);
					if(SD_check_header(data,512) == 0x01)
					{
						menu->_index._current_page = i;//读取到具体的page
						break;
					}
				}
			}
			break;
		default:
			outFlag = 1;
			break;
	}
	
	if(outFlag)
		return 0;//证明读取page失败
	
	//get line
	byte32 = SD_PAGE_NUM + menu->_index._current_page * SD_LINE_NUM;//开始地址
	for(i = 0;i < SD_LINE_NUM;i++)
	{
		SD_ReadDisk(data,byte32 + i,1);
		if(SD_check_header(data,512) == 0x01)
		{
			menu->_index._current_line = i;//读取到具体的line
			break;
		}
	}
	
	//get menu
	//------------page 和 line的内存-----------------第几页的menu地址基准-----------------------------------第几行的menu地址基准------------------------
	byte32 = (SD_LINE_NUM + 1) * SD_PAGE_NUM + menu->_index._current_page * SD_LINE_NUM * SD_MENU_NUM + menu->_index._current_line * SD_MENU_NUM;
	
	for(i = 0;i < SD_MENU_NUM;i++)
	{
		SD_ReadDisk(data,byte32 + i,1);
		if(SD_check_header(data,512) == 0x01)
		{
			menu->_index._current_menu = i;
			//开始读取具体数据
			index = SD_HEADER_SIZE + 1;
			//config
			menu->_config._write_flag = data[index++];
			menu->_config._config_sector = stringToUint32(data,&index);
			//history
			
			for(j = 0;j < SD_HIS_CHANNEL;j++)
			{
				menu->_history[j]._size = stringToUint32(data,&index);
				menu->_history[j]._pop_sector = stringToUint32(data,&index);
				menu->_history[j]._push_sector = stringToUint32(data,&index);
			}
			
			break;
		}
	}
	
	return 0x01;	
}

uint8_t writeSdPage(SD_menu_t *menu,uint8_t state)
{
	uint8_t data[512] = {0};
	uint16_t index;
	//组数据
	index = appendSDHeader(data);
	data[index++] = state;
	//写磁盘
	SD_WriteDisk(data,menu->_index._current_page,1);
	
	return 1;
}

uint8_t writeSDLine(SD_menu_t *menu,uint8_t state)
{
	uint8_t data[512] = {0};
	uint16_t index;
	uint32_t sector_addr;
	//组数据
	index = appendSDHeader(data);
	data[index++] = state;
	//写磁盘
	sector_addr = SD_PAGE_NUM + menu->_index._current_page * SD_LINE_NUM + menu->_index._current_line;
	SD_WriteDisk(data,sector_addr,1);
	
	return 1;
}

uint8_t writeSdMenu(SD_menu_t *menu,uint8_t state)
{
	uint8_t i;
	uint8_t data[512] = {0};
	uint16_t index = 0;
	uint32_t sector_addr;
	
	index = appendSDHeader(data);
	data[index++] = state;

	//write config
	data[index++] = menu->_config._write_flag;
	appendUint32ToData(data,&index,menu->_config._config_sector);
	//write history
	for(i = 0;i < SD_HIS_CHANNEL;i++)
	{
		appendUint32ToData(data,&index,SD_Menu._history[i]._size);
		appendUint32ToData(data,&index,SD_Menu._history[i]._pop_sector);
		appendUint32ToData(data,&index,SD_Menu._history[i]._push_sector);
	}
	//写入的具体地址
	sector_addr = SD_PAGE_NUM * (SD_LINE_NUM + 1) + menu->_index._current_page * SD_LINE_NUM * SD_MENU_NUM 
				+ menu->_index._current_line * SD_MENU_NUM + menu->_index._current_menu;
	//往地址写入数据
	SD_WriteDisk(data,sector_addr,1);
	
	return 1;	
}

uint8_t SDReadConfig(device_manager_t *dmanager,platform_manager_t *pManager,battery_data_t *bdata)
{
	uint8_t i,j;
	uint8_t data[512] = {0};
	uint16_t index = 0;
	uint16_t byte16;
	uint32_t byte32;
	uint8_t type;
	
	if(getSdMenu(&SD_Menu) == 0)
		return 0;
	
	byte32 = configStartAddr + SD_Menu._config._config_sector;
	SD_ReadDisk(data,byte32,1);
	
	type = data[0];
	if(type == 0x10)
	{
		index = 1;
	}
	dmanager->_device_count = data[index++];
	for(i = 0;i < dmanager->_device_count;i++)
	{
		//通用设置
		dmanager->_devices[i]._device_type = data[index++];//设备类型
		dmanager->_devices[i]._device_protocol = data[index++];//设备协议
		dmanager->_devices[i]._device_number = data[index++];//设备编号
		byte16 = stringToUint16(data,&index);
		dmanager->_devices[i]._device_get_time = byte16;//采集时间
		dmanager->_devices[i]._device_com_state = data[index++];//通信状态		
		dmanager->_devices[i]._device_formula = data[index++];//计算公式
		
		dmanager->_devices[i]._device_com_state = 0x00;//刚开机就默认异常
		
		//参数设置
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._deep_water = byte32 / 1000.0;//积水深
		byte16 = stringToUint16(data,&index);
		dmanager->_devices[i]._water_param._n = byte16 / 10000.0;//糙率
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._J = byte32 / 10000000.0;//比降
		dmanager->_devices[i]._water_param._range = data[index++];
		
		//渠形设置
		dmanager->_devices[i]._water_param._type = data[index++];//渠形
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._canheight = byte32 / 1000.0;//渠高
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._topwidth = byte32 / 1000.0;//渠顶宽
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._bottomwidth = byte32 / 1000.0;//渠底宽
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._H = byte32 / 1000.0;//直段深
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._r = byte32 / 1000.0;//半径
		dmanager->_devices[i]._water_param._ur = byte32 / 1000.0;//半径
		dmanager->_devices[i]._water_param._a = data[index++];//外倾角
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._insheight = byte32 / 1000.0;//安装高度
		dmanager->_devices[i]._water_param._insheight_mm = byte32;//安装高度
		
		//累计流量
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_data._flow = byte32 * 1.0;
		//累计流量备份
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_data._flow_backup = byte32 * 1.0;
		//累计流量修改值备份
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_data._flow_change_backup = byte32 * 1.0;
	}
	
	//站点数据
	byte32 = stringToUint32ByBytes(data,&index,3);//站点编码a1 3个字节
	pManager->_platform_number_a1 = byte32;
	byte16 = stringToUint16(data,&index);//站点编码a2 2个字节
	pManager->_platform_number_a2 = byte16;
	
	if(type == 0x10)//新版的存贮 加上远程管理平台的ip和端口
	{
		for(i = 0;i < 4;i++)
		{
			pManager->_ipv4[i] = data[index++];
		}
		pManager->_port = stringToUint16(data,&index);
	}
	
	pManager->_platform_count = data[index++];//平台数量
	if(pManager->_platform_count > 6)
		pManager->_platform_count = 6;
		
	for(i = 0;i < pManager->_platform_count;i++)
	{
		pManager->_platforms[i]._enable = data[index++];//平台启用
		pManager->_platforms[i]._ip_type = data[index++];
		if(pManager->_platforms[i]._ip_type == 0x01)
		{//ipv4
			pManager->_platforms[i]._ip1 = data[index++];
			pManager->_platforms[i]._ip2 = data[index++];
			pManager->_platforms[i]._ip3 = data[index++];
			pManager->_platforms[i]._ip4 = data[index++];
			pManager->_platforms[i]._port = stringToUint16(data,&index);
		}
		else
		{//ipv6
			for(j = 0;j < 16;j++)
			{
				pManager->_platforms[i]._ipv6[j] = data[index++];
			}
		}
		pManager->_platforms[i]._protocol = data[index++];//水文规约
		pManager->_platforms[i]._address_type = data[index++];//地址类型
		pManager->_platforms[i]._data_type = data[index++];//数据类型
		pManager->_platforms[i]._interval_time = stringToUint16(data,&index);//报存时间
		pManager->_platforms[i]._mode = data[index++];//应答模式
		if(type == 0x10)
		{//发送的数据类型
			pManager->_platforms[i]._send_data_type = stringToUint16(data,&index);
		}
	}
	
	
	bdata->_power_mode = data[index++];

	for(i = 0;i < 8;i++)
	{
		bdata->_extend_data[i] = data[index++];
	}

	/*
	bdata->_extend_data[0] = SD_Menu._index._current_page & 0xFF;
	bdata->_extend_data[1] = SD_Menu._index._current_line & 0xFF;
	bdata->_extend_data[2] = SD_Menu._index._current_menu & 0xFF;
	bdata->_extend_data[3] = SD_Menu._config._config_sector & 0xFF;
	*/
	return 1;
	
}

uint8_t SD_update_config(SD_menu_t *menu)
{
	if(menu->_config._write_flag)
	{
		menu->_config._config_sector += 1;
		if(menu->_config._config_sector >= SD_CONFIG_NUM)
		{
			menu->_config._config_sector = 0;
		}
	}
	menu->_config._write_flag = 1;
	return 1;
}

uint8_t SD_update_push(SD_menu_t *menu,uint8_t index)
{
	uint8_t needUpdatePopIndex = 0;
	
	if(index > 5)
		return 0;
	
	if(menu->_history[index]._size < SD_HISTORY_NUM)
		menu->_history[index]._size += 1;
	else
		needUpdatePopIndex = 1;//到达最大历史数据量 需要pop往后移位 实现覆盖
	
	menu->_history[index]._push_sector += 1;
	if(menu->_history[index]._push_sector >= SD_HISTORY_NUM)
		menu->_history[index]._push_sector = 0;
	
	//pop也需要往后移 实现覆盖
	if(needUpdatePopIndex)
	{
		menu->_history[index]._pop_sector += 1;
		if(menu->_history[index]._pop_sector >= SD_HISTORY_NUM)
			menu->_history[index]._pop_sector = 0;
	}

	return 1;
}

uint8_t SD_update_pop(SD_menu_t *menu,uint8_t index)
{
	if(index > 5)
		return 0;
	
	if(menu->_history[index]._size == 0)
		return 0;
	
	//每pop出一条就减少size
	menu->_history[index]._size -= 1;
	
	menu->_history[index]._pop_sector += 1;
	if(menu->_history[index]._pop_sector >= SD_HISTORY_NUM)
		menu->_history[index]._pop_sector = 0;
	
	return 1;
	
}

uint8_t SD_update_clear(SD_menu_t *menu)
{
	uint8_t i = 0;
	
	for(i = 0;i < SD_HIS_CHANNEL;i++)
	{
		menu->_history[i]._size = 0;
		menu->_history[i]._pop_sector = menu->_history[i]._push_sector;
	}
	
	return 1;
}

uint8_t SDWriteManager(SD_menu_t *menu,uint8_t mode,uint8_t index,uint8_t initFlag)
{
	uint8_t i = 0,pageflag = 0,lineFlag = 0,res = 0;	
	uint32_t newPage,newLine,newMenu;
	
	if(index > 5)
		return 0;
	
	//config 或者 history的更新
	switch(mode)
	{
		case SD_WRITE_CONFIG:
			res = SD_update_config(menu);
			break;
		case SD_POP_HISTORY:
			res = SD_update_pop(menu,index);
			break;
		case SD_PUSH_HISTORY:
			res = SD_update_push(menu,index);
			break;
		case SD_CLEAR_HISTORY:
			res = SD_update_clear(menu);
			break;
		default:
			res = 0;
			break;
	}
	
	if(res == 0)
		return 0;

	if(initFlag)
	{//这是从未写入过 需要初始化设置的地方
		writeSdPage(menu,1);
		writeSDLine(menu,1);
		writeSdMenu(menu,1);
		return 1;
	}
	
	//SD_menu index的更新
	newPage = menu->_index._current_page;
	newLine = menu->_index._current_line;
	newMenu = menu->_index._current_menu + 1;
	//检测
	if(newMenu >= SD_MENU_NUM)
	{
		newMenu = 0;
		newLine += 1;
		lineFlag = 1;
		if(newLine >= SD_LINE_NUM)
		{
			pageflag = 1;
			newLine = 0;
			newPage += 1;
			if(newPage >= SD_PAGE_NUM)
				newPage = 0;
		}
	}
	//把旧菜单状态置0
	if(pageflag)
		writeSdPage(menu,0);
	
	if(lineFlag)
		writeSDLine(menu,0);
	
	writeSdMenu(menu,0);
	
	menu->_index._current_menu = newMenu;
	menu->_index._current_line = newLine;
	menu->_index._current_page = newPage;
	
	//写入新菜单
	if(pageflag)
		writeSdPage(menu,1);
	
	if(lineFlag)
		writeSDLine(menu,1);
	
	writeSdMenu(menu,1);
	
	return 1;
}

uint8_t SDWriteConfig(device_manager_t *dmanager,platform_manager_t *pManager,battery_data_t *bdata)
{
	uint8_t i,j;
	uint8_t data[512] = {0};
	uint16_t index = 0;
	uint16_t byte16;
	uint32_t byte32;
	uint8_t pageFlag;//是否需要
	uint8_t addressByte[8] = {0};
	uint8_t initFlag = 0;
	
	if(getSdMenu(&SD_Menu) == 0)
	{
		initSDMenu(&SD_Menu);
		initFlag = 1;
	}
	
	if(SDWriteManager(&SD_Menu,SD_WRITE_CONFIG,0,initFlag) == 0)
		return 0;
	
	//设置好菜单成功 开始写入正式数据 首先组建数据
	data[index++] = 0x10;//这个是最新版的 先在开头加上标志位
	if(dmanager->_device_count > 4)
		dmanager->_device_count = 4;
	data[index++] = dmanager->_device_count;//设备数量
	for(i = 0;i < dmanager->_device_count;i++)
	{
		//通用设置
		data[index++] = dmanager->_devices[i]._device_type;//设备类型
		data[index++] = dmanager->_devices[i]._device_protocol;//设备协议
		data[index++] = dmanager->_devices[i]._device_number;//设备编号
		data[index++] = dmanager->_devices[i]._device_get_time >> 8 & 0xFF;
		data[index++] = dmanager->_devices[i]._device_get_time & 0xFF; //采集时间
		data[index++] = dmanager->_devices[i]._device_com_state;//通信状态
		data[index++] = dmanager->_devices[i]._device_formula;//计算公式
		//参数数据
		byte32 = dmanager->_devices[i]._water_param._deep_water * 1000;//积水深
		appendUint32ToData(data,&index,byte32);
		byte16 = dmanager->_devices[i]._water_param._n * 10000;//糙率
		appendUint16ToData(data,&index,byte16);
		byte32 = dmanager->_devices[i]._water_param._J * 10000000;//比降
		appendUint32ToData(data,&index,byte32);
		data[index++] = dmanager->_devices[i]._water_param._range;
		//渠形数据
		data[index++] = dmanager->_devices[i]._water_param._type;//渠形
		byte32 = dmanager->_devices[i]._water_param._canheight * 1000;//渠高
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._topwidth * 1000;//渠顶宽
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._bottomwidth * 1000;//渠底宽
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._H * 1000;//直段深
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._r * 1000;//半径
		appendUint32ToData(data,&index,byte32);
		data[index++] = dmanager->_devices[i]._water_param._a / 1;//外倾角
		byte32 = dmanager->_devices[i]._water_param._insheight * 1000;//安装高度
		appendUint32ToData(data,&index,byte32);
		
		//实时数据
		byte32 = dmanager->_devices[i]._water_data._flow;//流量
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_data._flow_backup;//流量备份
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_data._flow_change_backup;//流量修改值备份
		appendUint32ToData(data,&index,byte32);				
	}
	
	get_address_bytes(addressByte,8,pManager->_platform_number_a1,pManager->_platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[index++] = addressByte[i];
	}
	
	//加上远程平台的ip 端口
	for(i = 0;i < 4;i++)
	{
		data[index++] = pManager->_ipv4[i];
	}
	appendUint16ToData(data,&index,pManager->_port);
	
	//平台数量
	if(pManager->_platform_count > 6)
		pManager->_platform_count = 6;
	data[index++] = pManager->_platform_count;
	
	for(i = 0;i < pManager->_platform_count;i++)
	{
		data[index++] = pManager->_platforms[i]._enable;//平台启用
		data[index++] = pManager->_platforms[i]._ip_type;
		if(data[index] = pManager->_platforms[i]._ip_type == 0x01)
		{//ipv4
			data[index++] = pManager->_platforms[i]._ip1;
			data[index++] = pManager->_platforms[i]._ip2;
			data[index++] = pManager->_platforms[i]._ip3;
			data[index++] = pManager->_platforms[i]._ip4;
			data[index++] = pManager->_platforms[i]._port >> 8 & 0xFF;
			data[index++] = pManager->_platforms[i]._port & 0xFF;
		}
		else
		{
			for(j = 0;j < 16;j++)
			{
				data[index++] = pManager->_platforms[i]._ipv6[j];
			}
		}
		
		data[index++] = pManager->_platforms[i]._protocol;//水文规约
		data[index++] = pManager->_platforms[i]._address_type;//地址类型
		data[index++] = pManager->_platforms[i]._data_type;//数据类型
		data[index++] = pManager->_platforms[i]._interval_time >> 8 & 0xFF;
		data[index++] = pManager->_platforms[i]._interval_time & 0xFF;//报 存时间
		data[index++] = pManager->_platforms[i]._mode;//应答模式
		appendUint16ToData(data,&index,pManager->_platforms[i]._send_data_type);//发送类型 在0x10版本加上
	}
	
	data[index++] = bdata->_power_mode;
	for(i = 0;i < 8;i++)
	{
		data[index++] = bdata->_extend_data[i];
	}

	byte32 = configStartAddr + SD_Menu._config._config_sector;
	//写config数据
	SD_WriteDisk(data,byte32,1);
	return 1;
}


uint32_t getHistorySize(uint8_t platformIndex)
{
	if(getSdMenu(&SD_Menu) == 0)
		return 0;
	
	return SD_Menu._history[platformIndex]._size;
}

uint8_t SDPopHistoryData(EC20_send_data_t *pack,uint8_t platformIndex)
{
	uint8_t data[512];
	uint32_t sector_addr;
	uint16_t index = 0;
	uint16_t i;
	uint8_t byte;
	
	if(platformIndex < 0 || platformIndex > 5)
		return 0;
	
	if(getSdMenu(&SD_Menu) == 0)
		return 0;
	
	if(SD_Menu._history[platformIndex]._size <= 0)
		return 0;
	
	sector_addr = historyStartAddr + platformIndex * SD_HISTORY_NUM + SD_Menu._history[platformIndex]._pop_sector;
	SD_ReadDisk(data,sector_addr,1);
	
	byte = data[0];
	if(byte == 0xFE)
	{
		index++;
		pack->_sector_num = data[index++];
		for(i = 0;i < 8;i++)
		{
			pack->_sector_size[i] = data[index++];
		}
		pack->_size = stringToUint16(data,&index);
	}
	else
	{
		pack->_sector_num = 1;
		pack->_size = stringToUint16(data,&index);
		pack->_sector_size[0] = pack->_size;
	}
	//获得历史数据		
	for(i = 0;i < pack->_size;i++)
	{
		pack->_data[i] = data[index++];
	}	
	pack->_current_sector = 0;
	
	SDWriteManager(&SD_Menu,SD_POP_HISTORY,platformIndex,0);		
	return 1;
}

uint8_t SDPushHistoryData(EC20_send_data_t *pack,uint8_t platformIndex)
{
	uint8_t data[512] = {0};
	uint16_t index = 0;
	uint16_t i;
	uint32_t sector_addr;
	uint8_t initFlag = 0;
	
	if(platformIndex > 5)
		return 0;
	
	if(pack->_size > 400)
		return 0;
	
	//读取菜单栏	
	if(getSdMenu(&SD_Menu) == 0)
	{
		initSDMenu(&SD_Menu);
		initFlag = 1;
	}
		
	
	//存入的历史数据size
	data[index++] = 0xFE;
	data[index++] = pack->_sector_num;
	for(i = 0;i < 8;i++)
	{
		data[index++] = pack->_sector_size[i];
	}
	appendUint16ToData(data,&index,pack->_size);
	
	for(i = 0;i < pack->_size;i++)
	{
		data[index++] = pack->_data[i];
	}
	
	sector_addr = historyStartAddr + SD_HISTORY_NUM * platformIndex + SD_Menu._history[platformIndex]._push_sector;
	//写入具体数据
	SD_WriteDisk(data,sector_addr,1);
	//写入菜单栏
	SDWriteManager(&SD_Menu,SD_PUSH_HISTORY,platformIndex,initFlag);
	return 1;
}

uint8_t clearHistoryData(void)
{
	if(getSdMenu(&SD_Menu) == 0)
		return 0;
	
	SDWriteManager(&SD_Menu,SD_CLEAR_HISTORY,0,0);
	return 1;
}
