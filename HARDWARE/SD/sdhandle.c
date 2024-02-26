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
�ڴ�ֲ���ʽ
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

//�������ͷ 0-->��δ������д�� 1-->�������� 2-->д�� ���Ѿ�ʧЧ������
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


//����ֵ 0-->��ȡʧ�� ֤��û��SD��λ��ʼ����
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
			menu->_index._current_page = 0;//��ȡ�������page
			break;
		case 0x02:
			{
				for(i = 1;i < SD_PAGE_NUM;i++)
				{
					SD_ReadDisk(data,i,1);
					if(SD_check_header(data,512) == 0x01)
					{
						menu->_index._current_page = i;//��ȡ�������page
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
		return 0;//֤����ȡpageʧ��
	
	//get line
	byte32 = SD_PAGE_NUM + menu->_index._current_page * SD_LINE_NUM;//��ʼ��ַ
	for(i = 0;i < SD_LINE_NUM;i++)
	{
		SD_ReadDisk(data,byte32 + i,1);
		if(SD_check_header(data,512) == 0x01)
		{
			menu->_index._current_line = i;//��ȡ�������line
			break;
		}
	}
	
	//get menu
	//------------page �� line���ڴ�-----------------�ڼ�ҳ��menu��ַ��׼-----------------------------------�ڼ��е�menu��ַ��׼------------------------
	byte32 = (SD_LINE_NUM + 1) * SD_PAGE_NUM + menu->_index._current_page * SD_LINE_NUM * SD_MENU_NUM + menu->_index._current_line * SD_MENU_NUM;
	
	for(i = 0;i < SD_MENU_NUM;i++)
	{
		SD_ReadDisk(data,byte32 + i,1);
		if(SD_check_header(data,512) == 0x01)
		{
			menu->_index._current_menu = i;
			//��ʼ��ȡ��������
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
	//������
	index = appendSDHeader(data);
	data[index++] = state;
	//д����
	SD_WriteDisk(data,menu->_index._current_page,1);
	
	return 1;
}

uint8_t writeSDLine(SD_menu_t *menu,uint8_t state)
{
	uint8_t data[512] = {0};
	uint16_t index;
	uint32_t sector_addr;
	//������
	index = appendSDHeader(data);
	data[index++] = state;
	//д����
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
	//д��ľ����ַ
	sector_addr = SD_PAGE_NUM * (SD_LINE_NUM + 1) + menu->_index._current_page * SD_LINE_NUM * SD_MENU_NUM 
				+ menu->_index._current_line * SD_MENU_NUM + menu->_index._current_menu;
	//����ַд������
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
		//ͨ������
		dmanager->_devices[i]._device_type = data[index++];//�豸����
		dmanager->_devices[i]._device_protocol = data[index++];//�豸Э��
		dmanager->_devices[i]._device_number = data[index++];//�豸���
		byte16 = stringToUint16(data,&index);
		dmanager->_devices[i]._device_get_time = byte16;//�ɼ�ʱ��
		dmanager->_devices[i]._device_com_state = data[index++];//ͨ��״̬		
		dmanager->_devices[i]._device_formula = data[index++];//���㹫ʽ
		
		dmanager->_devices[i]._device_com_state = 0x00;//�տ�����Ĭ���쳣
		
		//��������
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._deep_water = byte32 / 1000.0;//��ˮ��
		byte16 = stringToUint16(data,&index);
		dmanager->_devices[i]._water_param._n = byte16 / 10000.0;//����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._J = byte32 / 10000000.0;//�Ƚ�
		dmanager->_devices[i]._water_param._range = data[index++];
		
		//��������
		dmanager->_devices[i]._water_param._type = data[index++];//����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._canheight = byte32 / 1000.0;//����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._topwidth = byte32 / 1000.0;//������
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._bottomwidth = byte32 / 1000.0;//���׿�
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._H = byte32 / 1000.0;//ֱ����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._r = byte32 / 1000.0;//�뾶
		dmanager->_devices[i]._water_param._ur = byte32 / 1000.0;//�뾶
		dmanager->_devices[i]._water_param._a = data[index++];//�����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_param._insheight = byte32 / 1000.0;//��װ�߶�
		dmanager->_devices[i]._water_param._insheight_mm = byte32;//��װ�߶�
		
		//�ۼ�����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_data._flow = byte32 * 1.0;
		//�ۼ���������
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_data._flow_backup = byte32 * 1.0;
		//�ۼ������޸�ֵ����
		byte32 = stringToUint32(data,&index);
		dmanager->_devices[i]._water_data._flow_change_backup = byte32 * 1.0;
	}
	
	//վ������
	byte32 = stringToUint32ByBytes(data,&index,3);//վ�����a1 3���ֽ�
	pManager->_platform_number_a1 = byte32;
	byte16 = stringToUint16(data,&index);//վ�����a2 2���ֽ�
	pManager->_platform_number_a2 = byte16;
	
	if(type == 0x10)//�°�Ĵ��� ����Զ�̹���ƽ̨��ip�Ͷ˿�
	{
		for(i = 0;i < 4;i++)
		{
			pManager->_ipv4[i] = data[index++];
		}
		pManager->_port = stringToUint16(data,&index);
	}
	
	pManager->_platform_count = data[index++];//ƽ̨����
	if(pManager->_platform_count > 6)
		pManager->_platform_count = 6;
		
	for(i = 0;i < pManager->_platform_count;i++)
	{
		pManager->_platforms[i]._enable = data[index++];//ƽ̨����
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
		pManager->_platforms[i]._protocol = data[index++];//ˮ�Ĺ�Լ
		pManager->_platforms[i]._address_type = data[index++];//��ַ����
		pManager->_platforms[i]._data_type = data[index++];//��������
		pManager->_platforms[i]._interval_time = stringToUint16(data,&index);//����ʱ��
		pManager->_platforms[i]._mode = data[index++];//Ӧ��ģʽ
		if(type == 0x10)
		{//���͵���������
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
		needUpdatePopIndex = 1;//���������ʷ������ ��Ҫpop������λ ʵ�ָ���
	
	menu->_history[index]._push_sector += 1;
	if(menu->_history[index]._push_sector >= SD_HISTORY_NUM)
		menu->_history[index]._push_sector = 0;
	
	//popҲ��Ҫ������ ʵ�ָ���
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
	
	//ÿpop��һ���ͼ���size
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
	
	//config ���� history�ĸ���
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
	{//���Ǵ�δд��� ��Ҫ��ʼ�����õĵط�
		writeSdPage(menu,1);
		writeSDLine(menu,1);
		writeSdMenu(menu,1);
		return 1;
	}
	
	//SD_menu index�ĸ���
	newPage = menu->_index._current_page;
	newLine = menu->_index._current_line;
	newMenu = menu->_index._current_menu + 1;
	//���
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
	//�Ѿɲ˵�״̬��0
	if(pageflag)
		writeSdPage(menu,0);
	
	if(lineFlag)
		writeSDLine(menu,0);
	
	writeSdMenu(menu,0);
	
	menu->_index._current_menu = newMenu;
	menu->_index._current_line = newLine;
	menu->_index._current_page = newPage;
	
	//д���²˵�
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
	uint8_t pageFlag;//�Ƿ���Ҫ
	uint8_t addressByte[8] = {0};
	uint8_t initFlag = 0;
	
	if(getSdMenu(&SD_Menu) == 0)
	{
		initSDMenu(&SD_Menu);
		initFlag = 1;
	}
	
	if(SDWriteManager(&SD_Menu,SD_WRITE_CONFIG,0,initFlag) == 0)
		return 0;
	
	//���úò˵��ɹ� ��ʼд����ʽ���� �����齨����
	data[index++] = 0x10;//��������°�� ���ڿ�ͷ���ϱ�־λ
	if(dmanager->_device_count > 4)
		dmanager->_device_count = 4;
	data[index++] = dmanager->_device_count;//�豸����
	for(i = 0;i < dmanager->_device_count;i++)
	{
		//ͨ������
		data[index++] = dmanager->_devices[i]._device_type;//�豸����
		data[index++] = dmanager->_devices[i]._device_protocol;//�豸Э��
		data[index++] = dmanager->_devices[i]._device_number;//�豸���
		data[index++] = dmanager->_devices[i]._device_get_time >> 8 & 0xFF;
		data[index++] = dmanager->_devices[i]._device_get_time & 0xFF; //�ɼ�ʱ��
		data[index++] = dmanager->_devices[i]._device_com_state;//ͨ��״̬
		data[index++] = dmanager->_devices[i]._device_formula;//���㹫ʽ
		//��������
		byte32 = dmanager->_devices[i]._water_param._deep_water * 1000;//��ˮ��
		appendUint32ToData(data,&index,byte32);
		byte16 = dmanager->_devices[i]._water_param._n * 10000;//����
		appendUint16ToData(data,&index,byte16);
		byte32 = dmanager->_devices[i]._water_param._J * 10000000;//�Ƚ�
		appendUint32ToData(data,&index,byte32);
		data[index++] = dmanager->_devices[i]._water_param._range;
		//��������
		data[index++] = dmanager->_devices[i]._water_param._type;//����
		byte32 = dmanager->_devices[i]._water_param._canheight * 1000;//����
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._topwidth * 1000;//������
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._bottomwidth * 1000;//���׿�
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._H * 1000;//ֱ����
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_param._r * 1000;//�뾶
		appendUint32ToData(data,&index,byte32);
		data[index++] = dmanager->_devices[i]._water_param._a / 1;//�����
		byte32 = dmanager->_devices[i]._water_param._insheight * 1000;//��װ�߶�
		appendUint32ToData(data,&index,byte32);
		
		//ʵʱ����
		byte32 = dmanager->_devices[i]._water_data._flow;//����
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_data._flow_backup;//��������
		appendUint32ToData(data,&index,byte32);
		byte32 = dmanager->_devices[i]._water_data._flow_change_backup;//�����޸�ֵ����
		appendUint32ToData(data,&index,byte32);				
	}
	
	get_address_bytes(addressByte,8,pManager->_platform_number_a1,pManager->_platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[index++] = addressByte[i];
	}
	
	//����Զ��ƽ̨��ip �˿�
	for(i = 0;i < 4;i++)
	{
		data[index++] = pManager->_ipv4[i];
	}
	appendUint16ToData(data,&index,pManager->_port);
	
	//ƽ̨����
	if(pManager->_platform_count > 6)
		pManager->_platform_count = 6;
	data[index++] = pManager->_platform_count;
	
	for(i = 0;i < pManager->_platform_count;i++)
	{
		data[index++] = pManager->_platforms[i]._enable;//ƽ̨����
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
		
		data[index++] = pManager->_platforms[i]._protocol;//ˮ�Ĺ�Լ
		data[index++] = pManager->_platforms[i]._address_type;//��ַ����
		data[index++] = pManager->_platforms[i]._data_type;//��������
		data[index++] = pManager->_platforms[i]._interval_time >> 8 & 0xFF;
		data[index++] = pManager->_platforms[i]._interval_time & 0xFF;//�� ��ʱ��
		data[index++] = pManager->_platforms[i]._mode;//Ӧ��ģʽ
		appendUint16ToData(data,&index,pManager->_platforms[i]._send_data_type);//�������� ��0x10�汾����
	}
	
	data[index++] = bdata->_power_mode;
	for(i = 0;i < 8;i++)
	{
		data[index++] = bdata->_extend_data[i];
	}

	byte32 = configStartAddr + SD_Menu._config._config_sector;
	//дconfig����
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
	//�����ʷ����		
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
	
	//��ȡ�˵���	
	if(getSdMenu(&SD_Menu) == 0)
	{
		initSDMenu(&SD_Menu);
		initFlag = 1;
	}
		
	
	//�������ʷ����size
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
	//д���������
	SD_WriteDisk(data,sector_addr,1);
	//д��˵���
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
