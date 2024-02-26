#include "flash.h"
#include "delay.h"


#define HEADER_SIZE 56

uint8_t flashIsEraseCode[] = 
{0x4c,0x49,0x55,0x20,0x4c,0x4f,0x56,0x45,0x20,0x59,0x49,0x4e,0x59,0x49,0x20,0x46,0x4f,0x52,0x45,0x56,0x45,0x52,0x0a,0x41,0x42,0x43,0x44,0x45,
0x46,0x47,0x0a,0x32,0x30,0x32,0x33,0x2e,0x30,0x35,0x2e,0x32,0x31,0x0a,0x49,0x20,0x4d,0x41,0x52,0x52,0x59,0x45,0x44,0x20,0x59,0x4f,0x55,0x0a};


//��flash��ȡ���� ÿ�ζ�ȡһ���ֽ�
uint8_t STMFLASH_ReadByte(uint32_t addr)
{
	return *(uint8_t*)addr;
}

uint8_t eraseUpdateProSector(void)
{
	uint8_t i;
	
	FLASH_Status status = FLASH_COMPLETE;
	
	FLASH_Unlock();
	FLASH_DataCacheCmd(DISABLE);
	for(i = 0;i < 3;i++)
	{
		//�Ȳ���6.7����
		status = FLASH_EraseSector(FLASH_Sector_6,VoltageRange_3);
		status = FLASH_EraseSector(FLASH_Sector_7,VoltageRange_3);
		if(status == FLASH_COMPLETE)
			break;		
		delay_ms(10);
	}
	
	FLASH_DataCacheCmd(ENABLE);
	FLASH_Lock();
	if(status != FLASH_COMPLETE)
		return 1;
	
	return 0;

}

uint8_t writeUpdateFlash(uint32_t startaddr,uint8_t *data,uint16_t datasize,uint16_t startIndex)
{
	uint16_t i;
	
	uint32_t realAddr = ADDR_FLASH_SECTOR6 + startaddr;
	FLASH_Unlock();
	FLASH_DataCacheCmd(DISABLE);
	for(i = 0;i < datasize;i++)
	{
		FLASH_ProgramByte(realAddr + i,data[startIndex + i]);
	}
	FLASH_DataCacheCmd(ENABLE);
	FLASH_Lock();
	return 0;
}

uint8_t writeFlash(uint32_t addr,uint8_t *data,uint16_t datasize)
{
	uint16_t i;
	
	FLASH_Unlock();
	FLASH_DataCacheCmd(DISABLE);
	for(i = 0;i < datasize;i++)
	{
		FLASH_ProgramByte(addr + i,data[i]);
	}
	FLASH_DataCacheCmd(ENABLE);
	FLASH_Lock();
	return 0;
}


//���ṹ��ת��Ϊflash�洢������
static uint8_t flash_packet_to_data(uint8_t *data,uint32_t dataSize,flash_save_packet_t packet)
{
	uint8_t i,j,dataIndex = 0;
	uint8_t doubledata[8];
	
	double f;
	if(dataSize < PACKET_SIZE)
		return 0;
	
	//����socket�� ��ip �˿�
	data[dataIndex++] = packet._platform_num;
	for(i = 0;i < MAX_IP_NUM;i++)
	{
		data[dataIndex++] = packet._platforms[i]._ip1;
		data[dataIndex++] = packet._platforms[i]._ip2;
		data[dataIndex++] = packet._platforms[i]._ip3;
		data[dataIndex++] = packet._platforms[i]._ip4;
		data[dataIndex++] = packet._platforms[i]._port >> 8;
		data[dataIndex++] = packet._platforms[i]._port;
		data[dataIndex++] = packet._platforms[i]._type;
	}
	
	//�¹��豸�� �Լ��豸������ ���
	data[dataIndex++] = packet._device_num;
	for(i = 0;i < MAX_DEVICE_NUM;i++)
	{
		data[dataIndex++] = packet._devices[i]._device_type;
		data[dataIndex++] = packet._devices[i]._device_number;
	}

	//ˮ�Ĳ���
	for(i = 0;i < DOUBLE_NUMS;i++)
	{
		switch(i)
		{
			case 0:
				f = packet._water_param._n;
				break;
			case 1:
				f = packet._water_param._J;
				break;
			case 2:
				f = packet._water_param._topwidth;
				break;
			case 3:
				f = packet._water_param._bottomwidth;
				break;
			case 4:
				f = packet._water_param._insheight;
				break;
			case 5:
				f = packet._water_param._canheight;
				break;
			case 6:
				f = packet._water_param._r;
				break;
			case 7:
				f = packet._water_param._ur;
				break;
			case 8:
				f = packet._water_param._a;
				break;
			case 9:
				f = packet._water_param._H;
				break;
			default:
				break;
		}
		
		jxjs_double2data(f,doubledata,8);
		for(j = 0;j < 8;j++)
		{
			data[dataIndex++] = doubledata[j];
		}
	}
	data[dataIndex++] = packet._water_param._type;
	data[dataIndex++] = packet._water_param._insheight_mm >> 8;
	data[dataIndex++] = packet._water_param._insheight_mm;
/*	
	while(dataIndex < 128)
	{
		data[dataIndex++] = 0x00;
	}
*/
	return 1;
}

//����flash������data���� ת��Ϊ�Լ�����Ľṹ��
static flash_save_packet_t flash_data_to_packet(uint8_t *data,uint32_t dataSize)
{
	flash_save_packet_t packet;
	uint8_t dataIndex = 0,i;
	if(dataSize < PACKET_SIZE)
		return packet;
	packet._platform_num = data[dataIndex++];
	for(i = 0;i < MAX_IP_NUM;i++)
	{
		packet._platforms[i]._ip1 = data[dataIndex++];
		packet._platforms[i]._ip2 = data[dataIndex++];
		packet._platforms[i]._ip3 = data[dataIndex++];
		packet._platforms[i]._ip4 = data[dataIndex++];
		packet._platforms[i]._port = data[dataIndex++];
		packet._platforms[i]._port <<= 8;
		packet._platforms[i]._port |= data[dataIndex++];
		packet._platforms[i]._type = data[dataIndex++];
	}
	
	packet._device_num = data[dataIndex++];
	for(i = 0;i < MAX_DEVICE_NUM;i++)
	{
		packet._devices[i]._device_type = data[dataIndex++];
		packet._devices[i]._device_number = data[dataIndex++];
	}
	
	for(i = 0;i < DOUBLE_NUMS;i++)
	{
		switch(i)
		{
			case 0:
				packet._water_param._n = jxjs_data2double(data + dataIndex);
				break;
			case 1:
				packet._water_param._J = jxjs_data2double(data + dataIndex);
				break;
			case 2:
				packet._water_param._topwidth = jxjs_data2double(data + dataIndex);
				break;
			case 3:
				packet._water_param._bottomwidth = jxjs_data2double(data + dataIndex);
				break;
			case 4:
				packet._water_param._insheight = jxjs_data2double(data + dataIndex);
				break;
			case 5:
				packet._water_param._canheight = jxjs_data2double(data + dataIndex);
				break;
			case 6:
				packet._water_param._r = jxjs_data2double(data + dataIndex);
				break;
			case 7:
				packet._water_param._ur = jxjs_data2double(data + dataIndex);
				break;
			case 8:
				packet._water_param._a = jxjs_data2double(data + dataIndex);
				break;
			case 9:
				packet._water_param._H = jxjs_data2double(data + dataIndex);
				break;
			default:
				break;
		}
		dataIndex += 8;
	}
	packet._water_param._type = data[dataIndex];
	packet._water_param._insheight_mm = data[dataIndex++] << 8 | data[dataIndex++];
	return packet;
}

/*
������д��־��
Ŀǰ�������128byte��� �ߵ�ַҲ��128byte������
ÿ������������ ��д�ϲ�����־�� �ж��Ѿ�����
���أ�0-->����ʧ�ܣ����������С��С   1-->�ɹ�
*/
static uint8_t create_erase_pack(uint8_t *data,uint32_t dataSize)
{
	uint8_t i;
	
	if(dataSize < HEADER_SIZE)
		return 0;
	
	for(i = 0;i < HEADER_SIZE;i++)
	{
		data[i] = flashIsEraseCode[i];
	}
	return 1;
}

//�жϸõ�ַ�Ƿ�Ϊĳ������������ͷ
static uint8_t addr_is_sector_header(uint32_t addr)
{
	uint8_t res = 0;
	switch(addr)
	{
		case ADDR_FLASH_SECTOR0:
		case ADDR_FLASH_SECTOR1:
		case ADDR_FLASH_SECTOR2:
		case ADDR_FLASH_SECTOR3:
		case ADDR_FLASH_SECTOR4:
		case ADDR_FLASH_SECTOR5:
		case ADDR_FLASH_SECTOR6:
		case ADDR_FLASH_SECTOR7:
		case ADDR_FLASH_SECTOR8:
		case ADDR_FLASH_SECTOR9:
		case ADDR_FLASH_SECTOR10:
		case ADDR_FLASH_SECTOR11:
			res = 1;
			break;
		default:
			res = 0;
			break;
	}
	return  res;
}

//�жϸ������Ƿ������
static uint8_t sector_isErase(uint32_t addr)
{
	uint8_t res,i;
	res = addr_is_sector_header(addr);
	if(res == 0)
		return 0;
	
	res = 1;
	for(i = 0;i < HEADER_SIZE;i++)
	{
		if(STMFLASH_ReadByte(addr + i) != flashIsEraseCode[i])
		{
			res = 0;
			break;
		}
	}
	return res;
}

/*
��ȡ��ǰ��ȡ��ַ
���أ�0-->��ȡ��ַʧЧ,����ԭ��,��δ�������  �ɹ� ������һ������ͷ��ַ 
*/
static uint32_t getcurrent_addr(uint32_t *addr,FLASH_USE_MODE mode)
{
	uint8_t res,i;
	uint32_t tmpAddr,currentSector,nextSector;
	res = sector_isErase(ADDR_FLASH_SECTOR8);//�Զ������ݴ��� �ӵ���������ʼ
	if(res == 0)
	{
		return 0;
	}

	for(i = 0;i < 4;i++)
	{
		switch(i)
		{
			case 0:
				currentSector = ADDR_FLASH_SECTOR8;
				nextSector  = ADDR_FLASH_SECTOR9;
				break;
			case 1:
				currentSector = ADDR_FLASH_SECTOR9;
				nextSector = ADDR_FLASH_SECTOR10;
				break;
			case 2:
				currentSector = ADDR_FLASH_SECTOR10;
				nextSector = ADDR_FLASH_SECTOR11;
				break;
			case 3:
				currentSector = ADDR_FLASH_SECTOR11;
				nextSector = ADDR_FLASH_END;
				break;
			default:
				break;
		}
		
		tmpAddr = currentSector + PACKET_SIZE;
		while(tmpAddr < nextSector)
		{
			//�����ҵ��������Ե�ԭ�� ֻҪд�����ݵİ� ��һ���ֽڱ�Ȼ����0xFF,�����ڲ�������sector��
			//ÿ��ֻҪ��ͷ����0xFF�ʹ���д������ ��0xFF�ʹ���û��д����
			if(STMFLASH_ReadByte(tmpAddr) == 0xFF)
			{
				if(mode == FLASH_WRITE)
				{
					*addr = tmpAddr;
					return nextSector;
				}
				else if(mode == FLASH_READ)
				{
					if((tmpAddr - PACKET_SIZE) == currentSector)
					{
						if(currentSector > ADDR_FLASH_SECTOR8)
						{
							*addr = currentSector - PACKET_SIZE;
							return currentSector;
						}
						else if(currentSector == ADDR_FLASH_SECTOR8)
						{
							//������������� һ�ǳ�ʼ���� ����δд������ ��ʱ��û��readaddr����һ�����ϴ�д����sector11 Ȼ���ʼ����sector8
							//�������һ��д�����ݾ���sector5 ���һ��
							if(sector_isErase(ADDR_FLASH_SECTOR11) && STMFLASH_ReadByte(ADDR_FLASH_END - PACKET_SIZE) != 0xFF)
							{
								*addr = ADDR_FLASH_END - PACKET_SIZE;
								return currentSector;
							}
							else
								return 0;
						}
						else
							return 0;
					}
					else
					{
						*addr = tmpAddr - PACKET_SIZE;
						return nextSector;
					}
				}								
			}
			tmpAddr += PACKET_SIZE;
		}
	}
	
	return 0;	
}

uint8_t init_flash_packet(flash_save_packet_t *packet)
{
	packet->_platform_num = 0;
	packet->_device_num = 0;
	packet->_water_param._a = 0;
	packet->_water_param._bottomwidth = 0;
	packet->_water_param._canheight = 0;
	packet->_water_param._H = 0;
	packet->_water_param._insheight = 0;
	packet->_water_param._insheight_mm = 0;
	packet->_water_param._J = 0;
	packet->_water_param._n = 0;
	packet->_water_param._r = 0;
	packet->_water_param._topwidth = 0;
	packet->_water_param._type = RECT_CANAL;
	packet->_water_param._ur = 0;
	return 0;
}

static uint8_t init_sector(uint32_t sectorAddr)
{
	FLASH_Status status = FLASH_COMPLETE;
	uint8_t i;
	uint8_t data[HEADER_SIZE];
	uint32_t writeAddr;
	uint32_t erase_sector_address;
	if(addr_is_sector_header(sectorAddr) == 0)
	{
		return 0;
	}
	
	switch (sectorAddr)
	{
		case ADDR_FLASH_SECTOR0:
			erase_sector_address = FLASH_Sector_0;
			break;
		case ADDR_FLASH_SECTOR1:
			erase_sector_address = FLASH_Sector_1;
			break;
		case ADDR_FLASH_SECTOR2:
			erase_sector_address = FLASH_Sector_2;
			break;
		case ADDR_FLASH_SECTOR3:
			erase_sector_address = FLASH_Sector_3;
			break;
		case ADDR_FLASH_SECTOR4:
			erase_sector_address = FLASH_Sector_4;
			break;
		case ADDR_FLASH_SECTOR5:
			erase_sector_address = FLASH_Sector_5;
			break;
		case ADDR_FLASH_SECTOR6:
			erase_sector_address = FLASH_Sector_6;
			break;
		case ADDR_FLASH_SECTOR7:
			erase_sector_address = FLASH_Sector_7;
			break;
		case ADDR_FLASH_SECTOR8:
			erase_sector_address = FLASH_Sector_8;
			break;
		case ADDR_FLASH_SECTOR9:
			erase_sector_address = FLASH_Sector_9;
			break;
		case ADDR_FLASH_SECTOR10:
			erase_sector_address = FLASH_Sector_10;
			break;
		case ADDR_FLASH_SECTOR11:
			erase_sector_address = FLASH_Sector_11;
			break;
		default:
			erase_sector_address = FLASH_Sector_0;
			break;
	}

	FLASH_Unlock();
	FLASH_DataCacheCmd(DISABLE);
	for(i = 0;i < 3;i++)
	{
		status = FLASH_EraseSector(erase_sector_address,VoltageRange_3);
		if(status == FLASH_COMPLETE)
			break;		
		delay_ms(10);
	}
	
	if(status != FLASH_COMPLETE)
	{
		FLASH_DataCacheCmd(ENABLE);
		FLASH_Lock();
		return 0;
	}
	
	create_erase_pack(data,HEADER_SIZE);
	writeAddr = sectorAddr;
	for(i = 0;i < HEADER_SIZE;i++)
	{
		FLASH_ProgramByte(writeAddr + i,data[i]);
	}
	FLASH_DataCacheCmd(ENABLE);
	FLASH_Lock();
	return 1;			
}

flash_save_packet_t read_packet_from_flash(void)
{
	flash_save_packet_t packet;
	uint32_t readAddr,readRes;
	uint8_t readData[PACKET_SIZE];
	uint16_t i;
	readRes = getcurrent_addr(&readAddr,FLASH_READ);
	if(readRes == 0)
	{
		init_flash_packet(&packet);
		return packet;
	}
	
	for(i = 0;i < PACKET_SIZE;i++)
	{
		readData[i] = STMFLASH_ReadByte(readAddr);
		readAddr++;
	}
	
	packet = flash_data_to_packet(readData,PACKET_SIZE);
	return packet;
}


uint8_t write_packet_to_flash(flash_save_packet_t packet,uint32_t *addr)
{
	uint32_t writeaddr,nextSector;
	uint8_t data[PACKET_SIZE];
	uint16_t i;
	
	nextSector = getcurrent_addr(&writeaddr,FLASH_WRITE);

	if(nextSector == 0)
	{
		init_sector(ADDR_FLASH_SECTOR8);
		writeaddr = ADDR_FLASH_SECTOR8 + PACKET_SIZE;
		nextSector = ADDR_FLASH_SECTOR9;
	}
	
	*addr = writeaddr;
	flash_packet_to_data(data,PACKET_SIZE,packet);

	FLASH_Unlock();
	FLASH_DataCacheCmd(DISABLE);
	for(i = 0;i < PACKET_SIZE;i++)
	{
		FLASH_ProgramByte(writeaddr + i,data[i]);
	}
	FLASH_DataCacheCmd(ENABLE);
	FLASH_Lock();
	
	if((writeaddr + PACKET_SIZE) == nextSector)
	{
		if(nextSector == ADDR_FLASH_END)
		{
			nextSector = ADDR_FLASH_SECTOR8;//Ŀǰֻ��8 9 10 11�ĸ����� ��11�������� ���³�ʼ����8���� ѭ����д
		}
		init_sector(nextSector);
	}
	return 1;
}
