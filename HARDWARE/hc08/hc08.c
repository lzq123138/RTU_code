#include "hc08.h"
#include "liuqueue.h"
#include "protocol.h"
#include "math.h"
#include "lcdhandle.h"
#include "sdhandle.h"

#define HC08SIZE 4096
uint8_t hc08_data[4096];
uint16_t hc08_size = 0;
uint8_t TIM7_timeout;
uint8_t hc08_mutex = 0;//������ �ڴ�����һ��ָ��ǰ �������������
uint8_t hc08_data_mutex = 0;

extern EC20_msg_t 			EC20_msg;
extern jxjs_time_t 			sys_time;
extern platform_manager_t	platform_manager;
extern device_manager_t		device_manager;
extern LCD_msg_t			LCD_msg;
extern battery_data_t		battery_data;
extern uint8_t 				SDCardInitSuccessFlag;
extern uint8_t				ChangeFlag;
extern rs485_state_t 		rs485State;
extern uint8_t 				debugFlag;

static const bounds[] = {1200,2400,4800,9600,14400,19200,38400,56000,57600,115200,128000,230400,256000,460800};
//								8b     9b
static const wordLengths[] = {0x0000,0x1000};

//                            1      0.5    2     1.5
static const stopbits[] = {0x0000,0x1000,0x2000,0x3000};

//                          ��      ż    ��
static const paritys[] = {0x0000,0x0400,0x0600};

void hc08_init(uint32_t bound)
{
	//GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//ʹ��ʱ��
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //ʹ��GPIOAʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//ʹ��USART3ʱ��
	
	//����3��Ӧ���Ÿ���ӳ��
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3); //GPIOA2����ΪUSART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); //GPIOA3����ΪUSART3
	
	//USART3�˿�����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //GPIOA2��GPIOA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOB,&GPIO_InitStructure); //��ʼ��PA2��PA3
	
   //USART3 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
    USART_Init(USART3, &USART_InitStructure); //��ʼ������2
    USART_Cmd(USART3, ENABLE);  //ʹ�ܴ���2 
	USART_ClearFlag(USART3, USART_FLAG_TC);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//��������ж�
	
	//Usart3 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//����3�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����	
}

void Uart3_SendStr(uint8_t *buf,uint16_t size)//����3��������
{
    uint16_t i;
    for (i = 0;i < size;i++)
	{
        while((USART3->SR&0X40)==0);//�ȴ��������
        USART3->DR = (uint8_t) buf[i]; 
	}
}

void clear_hc08_data(void)
{
	uint16_t i;
	for(i = 0;i < hc08_size;i++)
	{
		hc08_data[i] = 0;
	}
	hc08_size = 0;
}


/*
*׷������ͷ��Ŀǰ���ڶ������ݻش���׷��
*/
static void append_header_to_data(uint8_t *data,uint16_t *size,uint8_t mode)
{
	data[*size] = 0x4A;
	*size += 1;
	data[*size] = 0x53;
	*size += 1;
	data[*size] = 0x01;
	*size += 1;
	
	if(mode >=1 && mode <= 2)
	{
		data[*size] = 0x01;
		*size += 1;
		data[*size] = mode;
		*size += 1;
	}	
	else if(mode >= 3 && mode <= 4)
	{
		data[*size] = mode - 1;
		*size += 1;
	}	
}

static void appendStringToData(uint8_t *data,uint16_t *index,char *string)
{
	uint16_t i = 0;
	do
	{
		data[*index] = string[i];
		*index += 1;
	}
	while(string[i++] != 0);
}

static uint16_t data_to_device(uint16_t index,uint8_t *data)
{
	uint8_t i;
	uint16_t byte16;
	uint32_t byte32;
	
	device_manager._device_count = data[index++];//����������
	if(device_manager._device_count > 4)
		device_manager._device_count = 4;//Ŀǰ���֧��8·
	
	//ֻҪ�������ù��豸���ݣ���ǿ�н�����ǰ485�¼�(�����)
	rs485State._flag = 0;
	
	for(i = 0;i < device_manager._device_count;i++)
	{
		//ͨ������
		device_manager._devices[i]._device_type = data[index++];//�豸����
		device_manager._devices[i]._device_protocol = data[index++];//�豸Э��
		device_manager._devices[i]._device_number = data[index++];//�豸���
		device_manager._devices[i]._device_com_state = 0x00;//��������״̬
		byte16 = stringToUint16(data,&index);
		if(byte16 < 20)
			byte16 = 20;//��̻�ȡ����ʱ����Ϊ20s
		device_manager._devices[i]._device_get_time = byte16;//�ɼ�ʱ��
		device_manager._devices[i]._device_formula = data[index++];//���㹫ʽ
		
		//��������
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._deep_water = byte32 / 1000.0;//��ˮ��
		byte16 = stringToUint16(data,&index);
		device_manager._devices[i]._water_param._n = byte16 / 10000.0;//����
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._J = byte32 / 10000000.0;//�Ƚ�
		device_manager._devices[i]._water_param._range = data[index++];
		
		//��������
		device_manager._devices[i]._water_param._type = data[index++];//����
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._canheight = byte32 / 1000.0;//����
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._topwidth = byte32 / 1000.0;//������
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._bottomwidth = byte32 / 1000.0;//���׿�
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._H = byte32 / 1000.0;//ֱ����
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._r = byte32 / 1000.0;//�뾶
		device_manager._devices[i]._water_param._ur = byte32 / 1000.0;//�뾶
		device_manager._devices[i]._water_param._a = data[index++];//�����
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._insheight = byte32 / 1000.0;//��װ�߶�
		device_manager._devices[i]._water_param._insheight_mm = byte32;//��װ�߶�
		
		//�ۼ�����
		if(data[index++] != 0x00)
		{//�����ۼ����� ����������֮���û������
			byte32 = stringToUint32(data,&index);
			device_manager._devices[i]._water_data._flow_backup = device_manager._devices[i]._water_data._flow;//����
			device_manager._devices[i]._water_data._flow = byte32;//�޸��ۼ�����
			device_manager._devices[i]._water_data._flow_change_backup = byte32;//�޸���㱸��
		}		
	}
	return index;
}

static uint16_t data_to_platform(uint16_t index,uint8_t *data,uint8_t setUser)
{
	uint8_t i,j;
	uint32_t byte32;
	uint16_t byte16;
	
	//վ�����
	byte32 = stringToUint32ByBytes(data,&index,3);//վ�����a1 3���ֽ�
	platform_manager._platform_number_a1 = byte32;
	byte16 = stringToUint16(data,&index);//վ�����a2 2���ֽ�
	platform_manager._platform_number_a2 = byte16;
	
	if(setUser == 0x02)//����Զ��ƽ̨������
	{
		i = data[index++];
		if(i == 0x01)
		{
			if(SDCardInitSuccessFlag)
			{
				clearHistoryData();
			}
		}
	}
	
	platform_manager._platform_count = data[index++];//ƽ̨����
	if(platform_manager._platform_count > 6)
		platform_manager._platform_count = 6;
	
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		platform_manager._platforms[i]._enable = data[index++];//ƽ̨����
		platform_manager._platforms[i]._ip_type = data[index++];
		if(platform_manager._platforms[i]._ip_type == 0x01)
		{//ipv4
			platform_manager._platforms[i]._ip1 = data[index++];
			platform_manager._platforms[i]._ip2 = data[index++];
			platform_manager._platforms[i]._ip3 = data[index++];
			platform_manager._platforms[i]._ip4 = data[index++];
			platform_manager._platforms[i]._port = stringToUint16(data,&index);
		}
		else
		{//ipv6
			for(j = 0;j < 16;j++)
			{
				platform_manager._platforms[i]._ipv6[j] = data[index++];
			}
		}
		platform_manager._platforms[i]._protocol = data[index++];//ˮ�Ĺ�Լ
		platform_manager._platforms[i]._address_type = data[index++];//��ַ����
		platform_manager._platforms[i]._data_type = data[index++];//��������
		platform_manager._platforms[i]._interval_time = stringToUint16(data,&index);//����ʱ��
		if(platform_manager._platforms[i]._interval_time < 30)
			platform_manager._platforms[i]._interval_time = 30;
		platform_manager._platforms[i]._mode = data[index++];//Ӧ��ģʽ
		if(setUser == 0x02)
		{
			byte16 = stringToUint16(data,&index);
			platform_manager._platforms[i]._send_data_type = byte16;
		}
	}
	return index;
}

//�����ʷ����
static void extendDataCase1(uint8_t *data)
{
	uint8_t needClearFlag = 1;
	uint8_t i;
	
	for(i = 1;i < 8;i++)
	{
		if(battery_data._extend_data[i] != 0)
		{
			needClearFlag = 0;
			break;
		}			
	}
	
	if(SDCardInitSuccessFlag)
	{
		if(needClearFlag)
		{
			clearHistoryData();
		}
	}
}

//�޸ĺ�̨ip��ַ
static void extendDataCase2(uint8_t *data)
{
	uint8_t i;
	if(battery_data._extend_data[7] == 0xFF)
	{
		for(i = 1;i < 5;i++)
		{
			platform_manager._ipv4[i - 1] = battery_data._extend_data[i];
		}
		platform_manager._port = battery_data._extend_data[5] << 8 | battery_data._extend_data[6];
	}
}

//�޸�ƽ̨��������
static void extendDataCase3(uint8_t *data)
{
	uint8_t i;
	if(battery_data._extend_data[6] = 0xFF && battery_data._extend_data[7] == 0xFF)
	{
		i = battery_data._extend_data[1];
		platform_manager._platforms[i]._send_data_type = (battery_data._extend_data[2] << 8) | battery_data._extend_data[3]; 
	}
}

//�Ƿ���debugģʽ
static void extendDataCase4(uint8_t *data)
{
	if(battery_data._extend_data[6] = 0xFF && battery_data._extend_data[7] == 0xFF)
	{
		if(battery_data._extend_data[1] == 0x01)
		{
			debugFlag = 1;
		}
		else
		{
			debugFlag = 0;
		}
	}
}

//�޸Ĵ������Ĳ���
static void extendDataCase5(uint8_t *data)
{
	uint8_t index;
	if(data[6] == 0xFF && data[7] == 0xFF)
	{
		index = data[1];
		
		if(index >= device_manager._device_count)
			return;
		
		device_manager._devices[index]._uart_config._bound = bounds[data[2]];
		device_manager._devices[index]._uart_config._wordlength = wordLengths[data[3]];
		device_manager._devices[index]._uart_config._stopbit = stopbits[data[4]];
		device_manager._devices[index]._uart_config._parity = paritys[data[5]];
	}
}

static uint16_t data_to_local(uint16_t index,uint8_t *data)
{
	uint8_t i;

	
	battery_data._power_mode = data[index++];//�ⲿ���緽ʽ
	//��չ����
	for(i = 0;i < 8;i++)
	{
		battery_data._extend_data[i] = data[index++];
	}
	
	//����ͨ��
	
	switch(battery_data._extend_data[0])
	{
		case 0x01:
			extendDataCase1(battery_data._extend_data);//���������ʷ�����ж�
			break;
		case 0x02:
			extendDataCase2(battery_data._extend_data);//�޸ĺ�̨ip��ַ
			break;
		case 0x03:
			extendDataCase3(battery_data._extend_data);//�����޸�ƽ̨��������
			break;
		case 0x04:
			extendDataCase4(battery_data._extend_data);//�����Ƿ���debugģʽ
			break;
		case 0x05:
			extendDataCase5(battery_data._extend_data);//�����޸Ĵ���������������
			break;
		default:
			break;
	}
	
	return index;
}

//���ݴ��������� ������Ӧ���� ���������ý��
uint8_t hc08_data_to_param(uint8_t *data,uint8_t setUser)
{
	uint8_t have_device = 0,have_platform = 0,have_local = 0;
	uint32_t byte32;
	uint16_t byte16;
	uint16_t index = 3;
	
	//�漰��lcd�Ĳ�������Ϊ���ò������ܵ�����ϸҳ��Ĳ˵��������ı䣬����ֱ�ӻص���ʼҳ��
	goFirstPage();
	
	//�����������ֲ�������
	if(data[index] & 0x01)
		have_device = 1;
	
	if(data[index] & 0x02)
		have_platform = 1;
	
	if(data[index] & 0x04)
		have_local = 1;
	
	index++;
	
	//˳��ǳ���Ҫ �����Ҹ�
	if(have_device)
		index = data_to_device(index,data);
	
	if(have_platform)
		index = data_to_platform(index,data,setUser);
	
	if(have_local)
		index = data_to_local(index,data);
	
	return 2;
}

uint16_t createSuccessData(uint8_t *data)
{
	uint16_t size = 0;
	uint16_t CRCByte;
	
	//ͷ
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//��������
	data[size++] = 0x02;
	data[size++] = 0x01;
	//CRCУ����
	CRCByte = modBusCRC16(data,size);
	data[size++] = CRCByte & 0xFF;
	data[size++] = CRCByte >> 8;
	return size;
}

uint16_t createFailedData(uint8_t *data)
{
	uint16_t size = 0;
	uint16_t CRCByte;
	
	//ͷ
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//��������
	data[size++] = 0x02;
	data[size++] = 0x02;
	//CRCУ����
	CRCByte = modBusCRC16(data,size);
	data[size++] = CRCByte & 0xFF;
	data[size++] = CRCByte >> 8;
	
	return size;
}
uint16_t createDeviceProtocolData(uint8_t *data)
{
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	
	//ͷ
	append_header_to_data(data,&size,1);//1-->�豸Э������

	//�豸����
	data[size++] = 0x02;//Ŀǰ�������豸������
	
	data[size++] = DEVICE_WATERLEVEL;//�豸����һ���
	sprintf(tmp_data,"ˮλ��");//�豸һ����
	appendStringToData(data,&size,tmp_data);//������׷�ӵ�������
	
	data[size++] = 0x02;//�豸һ��������Э��
	data[size++] = WATERLEVEL_PROTOCOL1;//Э��һ���
	sprintf(tmp_data,"�Ŵ��״�");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = WATERLEVEL_PROTOCOL2;//Э������
	sprintf(tmp_data,"4��20mAˮλ��");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = DEVICE_FLOWMETER;//�豸���Ͷ����
	sprintf(tmp_data,"������");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//�豸�� ֻ����һ��Э��
	data[size++] = FLOWMETER_PROTOCOL1;//Э��һ���
	sprintf(tmp_data,"ʩ�͵µ��");//Э��һ����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL2;//Э������
	sprintf(tmp_data,"�������峬��");//Э�������
	appendStringToData(data,&size,tmp_data);
	
	//�豸�������� ���㹫ʽ����
	data[size++] = 0x01;//Ŀǰֻ��һ�ּ��㹫ʽ
	data[size++] = FORMULA_XIECAI;//���㹫ʽһ���
	sprintf(tmp_data,"л�Ź�ʽ");
	appendStringToData(data,&size,tmp_data);
	
	//���㹫ʽ���꣬����
	data[size++] = 0x03;//Ŀǰ������������
	
	data[size++] = RECT_CANAL;//����һ���
	sprintf(tmp_data,"���λ�����");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = CIR_CANAL;//���ζ����
	sprintf(tmp_data,"Բ��");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = U_CANAL;//���������
	sprintf(tmp_data,"U��");
	appendStringToData(data,&size,tmp_data);
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	return size;
}

uint16_t createDeviceRealData(uint8_t *data)
{
	uint16_t size = 0;
	uint16_t crc_byte = 0;
	uint8_t i;
	
	uint16_t half_byte;
	uint32_t byte;
	
	//ͷ
	append_header_to_data(data,&size,2);//ģʽ2�����豸ʵ������
	data[size++] = device_manager._device_count;//�豸����
	for(i = 0;i < device_manager._device_count;i++)
	{
		//ͨ������
		data[size++] = device_manager._devices[i]._device_type;//�豸����
		data[size++] = device_manager._devices[i]._device_protocol;//�豸Э��
		data[size++] = device_manager._devices[i]._device_number;//�豸���
		data[size++] = device_manager._devices[i]._device_get_time >> 8 & 0xFF;
		data[size++] = device_manager._devices[i]._device_get_time & 0xFF; //�ɼ�ʱ��
		data[size++] = device_manager._devices[i]._device_com_state;//ͨ��״̬
		data[size++] = device_manager._devices[i]._device_formula;//���㹫ʽ
		//��������
		byte = device_manager._devices[i]._water_param._deep_water * 1000;//��ˮ��
		appendUint32ToData(data,&size,byte);
		half_byte = device_manager._devices[i]._water_param._n * 10000;//����
		appendUint16ToData(data,&size,half_byte);
		byte = device_manager._devices[i]._water_param._J * 10000000;//�Ƚ�
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._range;
		//��������
		data[size++] = device_manager._devices[i]._water_param._type;//����
		byte = device_manager._devices[i]._water_param._canheight * 1000;//����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._topwidth * 1000;//������
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._bottomwidth * 1000;//���׿�
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._H * 1000;//ֱ����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._r * 1000;//�뾶
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._a / 1;//�����
		byte = device_manager._devices[i]._water_param._insheight * 1000;//��װ�߶�
		appendUint32ToData(data,&size,byte);		
		
		//ʵʱ����
		byte = device_manager._devices[i]._water_data._airHeight * 1000;//�ո�
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._waterlevel * 1000;//ˮλ
		appendUint32ToData(data,&size,byte);
		if(device_manager._devices[i]._device_type == DEVICE_FLOWSPEED && device_manager._devices[i]._device_protocol == FLOWSPEED_MSYX1)
			byte = device_manager._devices[i]._water_data._flowspeed * 1000;
		else
			byte = device_manager._devices[i]._water_data._flowRate * 1000 * 3600.0;//˲ʱ����
		appendUint32ToData(data,&size,byte);
		if(device_manager._devices[i]._water_data._flow < 0)
			byte = 0;
		else
			byte = device_manager._devices[i]._water_data._flow;//����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_backup;//��������
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_change_backup;//�����޸�ֵ����
		appendUint32ToData(data,&size,byte);				
	}
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
}

uint16_t createPlatformData(uint8_t *data)
{
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	uint8_t i,byte,j;
	uint8_t address_data[8] = {0};
	
	//ͷ
	append_header_to_data(data,&size,3);
	
	//վ�����
	get_address_bytes(address_data,8,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[size++] = address_data[i];
	}
#if 0	
	//Э������
	data[size++] = 0x01;//Ŀǰһ��ˮ�Ĺ�Լ
	
	data[size++] = 0x01;//ˮ�Ĺ�Լһ���
	sprintf(tmp_data,"SZY206-2016");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//Ŀǰ���ֵ�ַ����
	
	data[size++] = 0x01;//��ַ����һ���
	sprintf(tmp_data,"д1234��1234");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//��ַ���Ͷ����
	sprintf(tmp_data,"д1234��3412");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//��ַ���������
	sprintf(tmp_data,"д1234��D204");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//Ŀǰ������������
	
	data[size++] = 0x01;//��������һ���
	sprintf(tmp_data,"������ǰ,�ۼ��ں�");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//��ַ���Ͷ����
	sprintf(tmp_data,"�ۼ���ǰ,�����ں�");
	appendStringToData(data,&size,tmp_data);
#endif	
	//ƽ̨����
	data[size++] = platform_manager._platform_count;
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		data[size++] = platform_manager._platforms[i]._connect_state;//��ƽ̨ͨ��״̬
		data[size++] = platform_manager._platforms[i]._enable;//ƽ̨����
		data[size++] = platform_manager._platforms[i]._ip_type;
		if(platform_manager._platforms[i]._ip_type == 0x01)
		{//ipv4
			data[size++] = platform_manager._platforms[i]._ip1;
			data[size++] = platform_manager._platforms[i]._ip2;
			data[size++] = platform_manager._platforms[i]._ip3;
			data[size++] = platform_manager._platforms[i]._ip4;
			data[size++] = platform_manager._platforms[i]._port >> 8 & 0xFF;
			data[size++] = platform_manager._platforms[i]._port & 0xFF;
		}
		else
		{
			for(j = 0;j < 8;j++)
			{
				data[size++] = platform_manager._platforms[i]._ipv6[j];
			}
		}
		
		data[size++] = platform_manager._platforms[i]._protocol;//ˮ�Ĺ�Լ
		data[size++] = platform_manager._platforms[i]._address_type;//��ַ����
		data[size++] = platform_manager._platforms[i]._data_type;//��������
		data[size++] = platform_manager._platforms[i]._interval_time >> 8 & 0xFF;
		data[size++] = platform_manager._platforms[i]._interval_time & 0xFF;//�� ��ʱ��
		data[size++] = platform_manager._platforms[i]._mode;//Ӧ��ģʽ	
	}
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	return size;
}

uint16_t createRTUData(uint8_t *data)
{
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint8_t address_data[8] = {0};
	uint16_t crc_byte = 0;
	uint8_t i,j;
	uint16_t half_byte;
	uint32_t byte;
	
	//ͷ
	append_header_to_data(data,&size,0xFF);
#if 0	
	data[size++] = 0x02;//Ŀǰ�������ⲿ���緽ʽ
	data[size++] = 0x01;//�ⲿ���緽ʽһ���
	sprintf(tmp_data,"�е�");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//�ⲿ���緽ʽ�����
	sprintf(tmp_data,"̫���ܵ��");
	appendStringToData(data,&size,tmp_data);
#endif	
	//�豸���
	for(i = 0;i < 7;i++)
	{
		data[size++] = platform_manager._device_number[i];
	}
	
	data[size++] = battery_data._version;//����汾
	data[size++] = battery_data._power_mode;//���緽ʽ
	data[size++] = battery_data._have_power;//�Ƿ񹩵�
	data[size++] = battery_data._voltage;//��ѹ
	//����״̬
	if(EC20_msg._ec20_haveinit)
		data[size++] = 0x00;//����
	else
	{
		if(EC20_msg._ec20_stage._type == EC20_ATCPIN && EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x01;//δʶ��sim��
		}
		else if((EC20_msg._ec20_stage._type == EC20_ATCREG || EC20_msg._ec20_stage._type == EC20_ATCGREG)&& EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x02;//����ʧ��
		}
		else
		{
			data[size++] = 0x03;//������...
		}
	}
	
	data[size++] = EC20_msg._csq1;//�ź�����ֵ

	if(battery_data._extend_data[0] == 0x01)
		battery_data._extend_data[0] = 0xFF;	
	for(i = 0;i < 8;i++)
	{//��չ����		
		data[size++] = battery_data._extend_data[i];
	}

	//����ƽ̨����
	//վ�����
	get_address_bytes(address_data,8,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[size++] = address_data[i];
	}
	
	//ƽ̨����
	data[size++] = platform_manager._platform_count;
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		data[size++] = platform_manager._platforms[i]._connect_state;//��ƽ̨ͨ��״̬
		data[size++] = platform_manager._platforms[i]._enable;//ƽ̨����
		data[size++] = platform_manager._platforms[i]._ip_type;
		if(platform_manager._platforms[i]._ip_type == 0x01)
		{//ipv4
			data[size++] = platform_manager._platforms[i]._ip1;
			data[size++] = platform_manager._platforms[i]._ip2;
			data[size++] = platform_manager._platforms[i]._ip3;
			data[size++] = platform_manager._platforms[i]._ip4;
			data[size++] = platform_manager._platforms[i]._port >> 8 & 0xFF;
			data[size++] = platform_manager._platforms[i]._port & 0xFF;
		}
		else
		{
			for(j = 0;j < 8;j++)
			{
				data[size++] = platform_manager._platforms[i]._ipv6[j];
			}
		}
		
		data[size++] = platform_manager._platforms[i]._protocol;//ˮ�Ĺ�Լ
		data[size++] = platform_manager._platforms[i]._address_type;//��ַ����
		data[size++] = platform_manager._platforms[i]._data_type;//��������
		data[size++] = platform_manager._platforms[i]._interval_time >> 8 & 0xFF;
		data[size++] = platform_manager._platforms[i]._interval_time & 0xFF;//�� ��ʱ��
		data[size++] = platform_manager._platforms[i]._mode;//Ӧ��ģʽ	
		half_byte = platform_manager._platforms[i]._send_data_type;
		appendUint16ToData(data,&size,half_byte);
	}
	
	//���ϴ���������	
	data[size++] = device_manager._device_count;//�豸����
	for(i = 0;i < device_manager._device_count;i++)
	{
		//ͨ������
		data[size++] = device_manager._devices[i]._device_type;//�豸����
		data[size++] = device_manager._devices[i]._device_protocol;//�豸Э��
		data[size++] = device_manager._devices[i]._device_number;//�豸���
		data[size++] = device_manager._devices[i]._device_get_time >> 8 & 0xFF;
		data[size++] = device_manager._devices[i]._device_get_time & 0xFF; //�ɼ�ʱ��
		data[size++] = device_manager._devices[i]._device_com_state;//ͨ��״̬
		data[size++] = device_manager._devices[i]._device_formula;//���㹫ʽ
		//��������
		byte = device_manager._devices[i]._water_param._deep_water * 1000;//��ˮ��
		appendUint32ToData(data,&size,byte);
		half_byte = device_manager._devices[i]._water_param._n * 10000;//����
		appendUint16ToData(data,&size,half_byte);
		byte = device_manager._devices[i]._water_param._J * 10000000;//�Ƚ�
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._range;
		//��������
		data[size++] = device_manager._devices[i]._water_param._type;//����
		byte = device_manager._devices[i]._water_param._canheight * 1000;//����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._topwidth * 1000;//������
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._bottomwidth * 1000;//���׿�
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._H * 1000;//ֱ����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._r * 1000;//�뾶
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._a / 1;//�����
		byte = device_manager._devices[i]._water_param._insheight * 1000;//��װ�߶�
		appendUint32ToData(data,&size,byte);		
		
		//ʵʱ����
		byte = device_manager._devices[i]._water_data._airHeight * 1000;//�ո�
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._waterlevel * 1000;//ˮλ
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flowRate * 1000 * 3600.0;//˲ʱ����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow;//����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_backup;//��������
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_change_backup;//�����޸�ֵ����
		appendUint32ToData(data,&size,byte);				
	}
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	return size;
}

//�������óɹ�������
static void send_success_hc08()
{
	uint8_t data[8] = {0};
	uint16_t size = 0;
	/*
	uint16_t CRCByte;
	
	//ͷ
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//��������
	data[size++] = 0x02;
	data[size++] = 0x01;
	//CRCУ����
	CRCByte = modBusCRC16(data,size);
	data[size++] = CRCByte & 0xFF;
	data[size++] = CRCByte >> 8;
	*/
	size = createSuccessData(data);
	Uart3_SendStr(data,size);
}

//��������ʧ�ܸ�����
static void send_failed_hc08()
{
	uint8_t data[8] = {0};
	uint16_t size = 0;
	/*
	uint16_t CRCByte;
	
	//ͷ
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//��������
	data[size++] = 0x02;
	data[size++] = 0x02;
	//CRCУ����
	CRCByte = modBusCRC16(data,size);
	data[size++] = CRCByte & 0xFF;
	data[size++] = CRCByte >> 8;
	*/
	size = createFailedData(data);
	Uart3_SendStr(data,size);
}

static void send_device_protoclo_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	uint8_t i;
	
	//ͷ
	append_header_to_data(data,&size,1);//1-->�豸Э������

	//�豸����
	data[size++] = 0x03;//Ŀǰ�������豸������
	
	data[size++] = DEVICE_WATERLEVEL;//�豸����һ���
	sprintf(tmp_data,"ˮλ��");//�豸һ����
	appendStringToData(data,&size,tmp_data);//������׷�ӵ�������
	
	data[size++] = 0x02;//�豸һ��������Э��
	data[size++] = WATERLEVEL_PROTOCOL1;//Э��һ���
	sprintf(tmp_data,"�Ŵ��״�");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = WATERLEVEL_PROTOCOL2;//Э������
	sprintf(tmp_data,"4��20mAˮλ��");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = DEVICE_FLOWMETER;//�豸���Ͷ����
	sprintf(tmp_data,"������");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x0B;//�豸�� ����11��Э��
	data[size++] = FLOWMETER_PROTOCOL1;//Э��һ���
	sprintf(tmp_data,"ʩ�͵µ��");//Э��һ����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL2;//Э������
	sprintf(tmp_data,"�������ᳬ��");//Э�������
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL3;//Э�������
	sprintf(tmp_data,"������������");//Э��������
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL4;//Э���ı��
	sprintf(tmp_data,"�����������");//Э��������
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL5;//Э���ı��
	sprintf(tmp_data,"�Ϻ����ص��");//Э��������
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL6;//Э���ı��
	sprintf(tmp_data,"���㳬��");//Э��������
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL7;//Э��7���
	sprintf(tmp_data,"�ٽ�ͨ����");//Э��7����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL8;//Э��8���
	sprintf(tmp_data,"��¡���");//Э��8����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_JINYI;//Э��9���
	sprintf(tmp_data,"���ǵ��");//Э��9����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_JDLY;//Э��10���
	sprintf(tmp_data,"�ѵ³���");//Э��10����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_HZZH;//Э��11���
	sprintf(tmp_data,"�񻪵��");//Э��11����
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = DEVICE_FLOWSPEED;//�豸���������
	sprintf(tmp_data,"������");//�豸һ����
	appendStringToData(data,&size,tmp_data);//������׷�ӵ�������
	
	data[size++] = 0x01;//�豸������һ��Э��
	data[size++] = FLOWSPEED_MSYX1;//Э��һ���
	sprintf(tmp_data,"��ˮ������");
	appendStringToData(data,&size,tmp_data);
	
	
	//�豸�������� ���㹫ʽ����
	data[size++] = 0x02;//Ŀǰֻ�ж��ּ��㹫ʽ	
	data[size++] = FORMULA_XIECAI;//���㹫ʽһ���
	sprintf(tmp_data,"л�Ź�ʽ");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FORMULA_NONE;//���㹫ʽ�����
	sprintf(tmp_data,"�޼���");
	appendStringToData(data,&size,tmp_data);
	
	
	//���㹫ʽ���꣬����
	data[size++] = 0x03;//Ŀǰ������������
	
	data[size++] = RECT_CANAL;//����һ���
	sprintf(tmp_data,"���λ�����");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = CIR_CANAL;//���ζ����
	sprintf(tmp_data,"Բ��");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = U_CANAL;//���������
	sprintf(tmp_data,"U��");
	appendStringToData(data,&size,tmp_data);
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	Uart3_SendStr(data,size);//�����ݷ��͵�����
}

/*
* ���ʹ�����ʵ�����ݸ�����
*/
static void send_device_real_to_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	uint16_t crc_byte = 0;
	uint8_t i;
	
	uint16_t half_byte;
	uint32_t byte;
	
	//ͷ
	append_header_to_data(data,&size,2);//ģʽ2�����豸ʵ������
	data[size++] = device_manager._device_count;//�豸����
	for(i = 0;i < device_manager._device_count;i++)
	{
		//ͨ������
		data[size++] = device_manager._devices[i]._device_type;//�豸����
		data[size++] = device_manager._devices[i]._device_protocol;//�豸Э��
		data[size++] = device_manager._devices[i]._device_number;//�豸���
		data[size++] = device_manager._devices[i]._device_get_time >> 8 & 0xFF;
		data[size++] = device_manager._devices[i]._device_get_time & 0xFF; //�ɼ�ʱ��
		if(device_manager._devices[i]._device_com_state != 1)
		{
			data[size++] = 0;
		}
		else
		{
			data[size++] = device_manager._devices[i]._device_com_state;//ͨ��״̬
		}		
		data[size++] = device_manager._devices[i]._device_formula;//���㹫ʽ
		//��������
		byte = device_manager._devices[i]._water_param._deep_water * 1000;//��ˮ��
		appendUint32ToData(data,&size,byte);
		half_byte = device_manager._devices[i]._water_param._n * 10000;//����
		appendUint16ToData(data,&size,half_byte);
		byte = device_manager._devices[i]._water_param._J * 10000000;//�Ƚ�
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._range;
		//��������
		data[size++] = device_manager._devices[i]._water_param._type;//����
		byte = device_manager._devices[i]._water_param._canheight * 1000;//����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._topwidth * 1000;//������
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._bottomwidth * 1000;//���׿�
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._H * 1000;//ֱ����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._r * 1000;//�뾶
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._a / 1;//�����
		byte = device_manager._devices[i]._water_param._insheight * 1000;//��װ�߶�
		appendUint32ToData(data,&size,byte);
		
		//ʵʱ����
		byte = device_manager._devices[i]._water_data._waterlevel * 1000;//ˮλ
		appendUint32ToData(data,&size,byte);
		if(device_manager._devices[i]._device_type == DEVICE_FLOWSPEED && device_manager._devices[i]._device_protocol == FLOWSPEED_MSYX1)
			byte = device_manager._devices[i]._water_data._flowspeed * 1000;
		else
			byte = device_manager._devices[i]._water_data._flowRate * 1000 * 3600.0;//˲ʱ����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow;//����
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_backup;//��������
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_change_backup;//�����޸�ֵ����
		appendUint32ToData(data,&size,byte);				
	}
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	//��������
	Uart3_SendStr(data,size);//�����ݷ��͵�����
}

static void send_platform_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	uint8_t i,byte,j;
	uint8_t address_data[8] = {0};
	
	//ͷ
	append_header_to_data(data,&size,3);
	
	//վ�����
	get_address_bytes(address_data,8,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[size++] = address_data[i];
	}
	
	//Э������
	data[size++] = 0x02;//Ŀǰһ��ˮ�Ĺ�Լ
	
	data[size++] = 0x01;//ˮ�Ĺ�Լһ���
	sprintf(tmp_data,"SZY206-2016");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//ˮ�Ĺ�Լһ���
	sprintf(tmp_data,"������̬й��");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//Ŀǰ���ֵ�ַ����
	
	data[size++] = 0x01;//��ַ����һ���
	sprintf(tmp_data,"д1234��1234");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//��ַ���Ͷ����
	sprintf(tmp_data,"д1234��3412");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//��ַ���������
	sprintf(tmp_data,"д1234��D204");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//Ŀǰ������������
	
	data[size++] = 0x01;//��������һ���
	sprintf(tmp_data,"������ǰ,�ۼ��ں�");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//��ַ���Ͷ����
	sprintf(tmp_data,"�ۼ���ǰ,�����ں�");
	appendStringToData(data,&size,tmp_data);
	
	//ƽ̨����
	data[size++] = platform_manager._platform_count;
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		data[size++] = 0x00;//������� ��������ͨ������
		data[size++] = platform_manager._platforms[i]._enable;//ƽ̨����
		data[size++] = platform_manager._platforms[i]._ip_type;
		if(platform_manager._platforms[i]._ip_type == IPTYPE_IPV4)
		{//ipv4
			data[size++] = platform_manager._platforms[i]._ip1;
			data[size++] = platform_manager._platforms[i]._ip2;
			data[size++] = platform_manager._platforms[i]._ip3;
			data[size++] = platform_manager._platforms[i]._ip4;
			data[size++] = platform_manager._platforms[i]._port >> 8 & 0xFF;
			data[size++] = platform_manager._platforms[i]._port & 0xFF;
		}
		else
		{
			for(j = 0;j < 16;j++)
			{
				data[size++] = platform_manager._platforms[i]._ipv6[j];
			}
		}
		
		data[size++] = platform_manager._platforms[i]._protocol;//ˮ�Ĺ�Լ
		data[size++] = platform_manager._platforms[i]._address_type;//��ַ����
		data[size++] = platform_manager._platforms[i]._data_type;//��������
		data[size++] = platform_manager._platforms[i]._interval_time >> 8 & 0xFF;
		data[size++] = platform_manager._platforms[i]._interval_time & 0xFF;//�� ��ʱ��
		data[size++] = platform_manager._platforms[i]._mode;//Ӧ��ģʽ	
	}
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	//��������
	Uart3_SendStr(data,size);//�����ݷ��͵�����
}

static void send_RTU_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	uint8_t i,byte,j;
	
	//ͷ
	append_header_to_data(data,&size,4);
	
	data[size++] = 0x02;//Ŀǰ�������ⲿ���緽ʽ
	data[size++] = 0x01;//�ⲿ���緽ʽһ���
	sprintf(tmp_data,"�е�");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//�ⲿ���緽ʽ�����
	sprintf(tmp_data,"̫���ܵ��");
	appendStringToData(data,&size,tmp_data);
	
	//�豸���
	for(i = 0;i < 7;i++)
	{
		data[size++] = platform_manager._device_number[i];
	}
	
	data[size++] = battery_data._power_mode;//���緽ʽ
	data[size++] = battery_data._have_power;//�Ƿ񹩵�
	data[size++] = battery_data._voltage;//��ѹ
	//����״̬
	if(EC20_msg._ec20_haveinit)
		data[size++] = 0x00;//����
	else
	{
		if(EC20_msg._ec20_stage._type == EC20_ATCPIN && EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x02;//δʶ��sim��
		}
		else if((EC20_msg._ec20_stage._type == EC20_ATCREG || EC20_msg._ec20_stage._type == EC20_ATCGREG)&& EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x03;//����ʧ��
		}
		else
		{
			data[size++] = 0x01;//������...
		}
	}
	
	data[size++] = EC20_msg._csq1;//�ź�����ֵ
	
	for(i = 0;i < 8;i++)
	{//��չ����
		data[size++] = battery_data._extend_data[i];
	}	
	
	//crcУ��
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	//��������
	Uart3_SendStr(data,size);//�����ݷ��͵�����
}

//�������ݸ�hc08 Ŀǰֱ���ǲ���һ������
static void send_data_to_hc08(uint8_t mode,uint8_t param)
{
	if(mode == 1)
	{
		if(param == 1)
		{//�豸Э������
			send_device_protoclo_data();
		}
		else if(param == 2)
		{//�豸ʵ������
			send_device_real_to_data();
		}
		else if(param == 3)
		{//ƽ̨����
			send_platform_data();
		}
		else if(param == 4)
		{//��������
			send_RTU_data();
		}
	}
	else if(mode == 2)
	{
		send_success_hc08();
	}
	else if(mode == 3)
	{
		send_failed_hc08();
	}
}

/*
*������������ ����: JSRTU_ + �豸��ź���λ
*/
void setBluetoothName(void)
{
	uint8_t data[32] = {0};
	
	//��������
	sprintf(data,"AT+NAME=JSRTU_%02X%02X",platform_manager._device_number[5],platform_manager._device_number[6]);
	//����ָ��
	Uart3_SendStr(data,18);//�����ݷ��͵�����,�̶���18���ֽ�	
}

/*���������������� ��������Ӧ���
*����ֵ:0-->�Ƿ����� 1-->�������� 2-->���ò����ɹ� 3-->���ò���ʧ�� 4 -->δ֪����
*/
uint8_t parse_hc08_data(uint8_t *mode,uint8_t *param,uint8_t *data,uint16_t dataSize)
{
	uint16_t CRCByte;
	
	//��������Ƿ�Ϸ�
	if(dataSize < 5)
		return 0;
	
	//�Ƿ���Ϲ̶�Э��ͷ
	if(data[0] != 0x4A || data[1] != 0x53)
		return 0;
	
	//�Ƿ����modbusCRCУ��
	CRCByte = modBusCRC16(data,dataSize - 2);
	if((CRCByte & 0xFF) != data[dataSize - 2] || (CRCByte >> 8 & 0xFF) != data[dataSize - 1])
		return 0;
	
	//�����Ϸ�����
	if(data[2] == 0x01)
	{//��������
		*mode = 1;
		if(data[3] == 0x01)
		{
			*param = data[4];
		}
		else
		{
			*param = data[3] + 1;
		}		
	}
	else if(data[2] == 0x02)
	{
		*mode = hc08_data_to_param(data,0);
		ChangeFlag = 1;
		
		if(SDCardInitSuccessFlag)
		{
			SDWriteConfig(&device_manager,&platform_manager,&battery_data);
		}
		
	}
	else if(data[2] == 0xFF)
	{
		*mode = 0xFF;
	}
	else
	{
		*mode = 0x64;
	}
	
	return 1;	
}

void read_uart3(void)
{
	uint8_t mode,param;
	hc08_mutex = 1;
	if(hc08_size < 5)
	{
		clear_hc08_data();
		hc08_mutex = 0;
		return;
	}
	
	if(parse_hc08_data(&mode,&param,hc08_data,hc08_size))
	{//�������ݳɹ�
		send_data_to_hc08(mode,param);
	}
	
	clear_hc08_data();
	hc08_mutex = 0;
}

void USART3_IRQHandler(void)                	//����3�жϷ������
{
	uint8_t Res;
	
	if(hc08_mutex)
		return;

	hc08_data_mutex = 1;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //�����ж�(���յ������ݱ�����0x0d 0x0a��β)
	{
		TIM_Cmd(TIM7, ENABLE);
		TIM7_timeout = 0;
		Res =USART_ReceiveData(USART3);//(USART1->DR);	//��ȡ���յ�������
		if(hc08_size < HC08SIZE)
			hc08_data[hc08_size++] = Res;
	} 

}


void TIM7_Int_Init(uint16_t arr,uint16_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);  ///ʹ��TIM3ʱ��
	
	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM7,&TIM_TimeBaseInitStructure);//��ʼ��TIM7
	
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE); //����ʱ��2�����ж�
	TIM_Cmd(TIM7,DISABLE); //��ʹ�ܶ�ʱ��7
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM7_IRQn; //��ʱ��2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//��ʱ��7�жϷ�����
void TIM7_IRQHandler(void)
{
	msg_event_t event;
	if(TIM_GetITStatus(TIM7,TIM_IT_Update)==SET) //����ж�
	{	
		//60msû����������Ϊһ֡���ݽ���
		TIM7_timeout++;
		
		if(TIM7_timeout >= 10)
		{
			hc08_data_mutex = 0;
			event._msg_type = MSG_READ_USART3;
			push_event(event);
			TIM_Cmd(TIM7, DISABLE);  //��ʹ��TIMx����	
		}
		
	}
	TIM_ClearITPendingBit(TIM7,TIM_IT_Update);  //����жϱ�־λ
}

//���Ӳ��ֹ���  ��ȡ�豸���
void getDeviceNumber(void)
{
	uint8_t	cpu_id[12] = {0};
	uint8_t	i;
	uint32_t crc_32;
	uint16_t crc_16;
	uint16_t index = 0;
	for(i = 0;i < 12;i++)
	{
		cpu_id[i] = *(uint8_t *)(0x1fff7a10 + i);
	}
	
	//��ȡ�豸ID���
	crc_32 = crc32_in_c(cpu_id,12);
	crc_16 = modBusCRC16(cpu_id,12);
	platform_manager._device_number[index++] = 0x01;
	appendUint32ToData(platform_manager._device_number,&index,crc_32);
	appendUint16ToData(platform_manager._device_number,&index,crc_16);
}

