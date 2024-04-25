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
uint8_t hc08_mutex = 0;//蓝牙锁 在处理完一条指令前 不理会来的数据
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

//                          无      偶    奇
static const paritys[] = {0x0000,0x0400,0x0600};

void hc08_init(uint32_t bound)
{
	//GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//使能时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //使能GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//使能USART3时钟
	
	//串口3对应引脚复用映射
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3); //GPIOA2复用为USART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); //GPIOA3复用为USART3
	
	//USART3端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //GPIOA2与GPIOA3
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOB,&GPIO_InitStructure); //初始化PA2，PA3
	
   //USART3 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(USART3, &USART_InitStructure); //初始化串口2
    USART_Cmd(USART3, ENABLE);  //使能串口2 
	USART_ClearFlag(USART3, USART_FLAG_TC);
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断
	
	//Usart3 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//串口3中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、	
}

void Uart3_SendStr(uint8_t *buf,uint16_t size)//串口3发送数据
{
    uint16_t i;
    for (i = 0;i < size;i++)
	{
        while((USART3->SR&0X40)==0);//等待发送完成
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
*追加数据头，目前用于对拿数据回传的追加
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
	
	device_manager._device_count = data[index++];//传感器数量
	if(device_manager._device_count > 4)
		device_manager._device_count = 4;//目前最多支持8路
	
	//只要重新设置过设备数据，就强行结束当前485事件(如果有)
	rs485State._flag = 0;
	
	for(i = 0;i < device_manager._device_count;i++)
	{
		//通用设置
		device_manager._devices[i]._device_type = data[index++];//设备类型
		device_manager._devices[i]._device_protocol = data[index++];//设备协议
		device_manager._devices[i]._device_number = data[index++];//设备编号
		device_manager._devices[i]._device_com_state = 0x00;//重置连接状态
		byte16 = stringToUint16(data,&index);
		if(byte16 < 20)
			byte16 = 20;//最短获取数据时间间隔为20s
		device_manager._devices[i]._device_get_time = byte16;//采集时间
		device_manager._devices[i]._device_formula = data[index++];//计算公式
		
		//参数设置
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._deep_water = byte32 / 1000.0;//积水深
		byte16 = stringToUint16(data,&index);
		device_manager._devices[i]._water_param._n = byte16 / 10000.0;//糙率
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._J = byte32 / 10000000.0;//比降
		device_manager._devices[i]._water_param._range = data[index++];
		
		//渠形设置
		device_manager._devices[i]._water_param._type = data[index++];//渠形
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._canheight = byte32 / 1000.0;//渠高
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._topwidth = byte32 / 1000.0;//渠顶宽
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._bottomwidth = byte32 / 1000.0;//渠底宽
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._H = byte32 / 1000.0;//直段深
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._r = byte32 / 1000.0;//半径
		device_manager._devices[i]._water_param._ur = byte32 / 1000.0;//半径
		device_manager._devices[i]._water_param._a = data[index++];//外倾角
		byte32 = stringToUint32(data,&index);
		device_manager._devices[i]._water_param._insheight = byte32 / 1000.0;//安装高度
		device_manager._devices[i]._water_param._insheight_mm = byte32;//安装高度
		
		//累计流量
		if(data[index++] != 0x00)
		{//包含累计流量 不包含流量之后就没有数据
			byte32 = stringToUint32(data,&index);
			device_manager._devices[i]._water_data._flow_backup = device_manager._devices[i]._water_data._flow;//备份
			device_manager._devices[i]._water_data._flow = byte32;//修改累计流量
			device_manager._devices[i]._water_data._flow_change_backup = byte32;//修改起点备份
		}		
	}
	return index;
}

static uint16_t data_to_platform(uint16_t index,uint8_t *data,uint8_t setUser)
{
	uint8_t i,j;
	uint32_t byte32;
	uint16_t byte16;
	
	//站点编码
	byte32 = stringToUint32ByBytes(data,&index,3);//站点编码a1 3个字节
	platform_manager._platform_number_a1 = byte32;
	byte16 = stringToUint16(data,&index);//站点编码a2 2个字节
	platform_manager._platform_number_a2 = byte16;
	
	if(setUser == 0x02)//代表远程平台的设置
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
	
	platform_manager._platform_count = data[index++];//平台数量
	if(platform_manager._platform_count > 6)
		platform_manager._platform_count = 6;
	
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		platform_manager._platforms[i]._enable = data[index++];//平台启用
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
		platform_manager._platforms[i]._protocol = data[index++];//水文规约
		platform_manager._platforms[i]._address_type = data[index++];//地址类型
		platform_manager._platforms[i]._data_type = data[index++];//数据类型
		platform_manager._platforms[i]._interval_time = stringToUint16(data,&index);//报存时间
		if(platform_manager._platforms[i]._interval_time < 30)
			platform_manager._platforms[i]._interval_time = 30;
		platform_manager._platforms[i]._mode = data[index++];//应答模式
		if(setUser == 0x02)
		{
			byte16 = stringToUint16(data,&index);
			platform_manager._platforms[i]._send_data_type = byte16;
		}
	}
	return index;
}

//清空历史数据
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

//修改后台ip地址
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

//修改平台发送类型
static void extendDataCase3(uint8_t *data)
{
	uint8_t i;
	if(battery_data._extend_data[6] = 0xFF && battery_data._extend_data[7] == 0xFF)
	{
		i = battery_data._extend_data[1];
		platform_manager._platforms[i]._send_data_type = (battery_data._extend_data[2] << 8) | battery_data._extend_data[3]; 
	}
}

//是否开启debug模式
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

//修改传感器的参数
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

	
	battery_data._power_mode = data[index++];//外部供电方式
	//扩展数据
	for(i = 0;i < 8;i++)
	{
		battery_data._extend_data[i] = data[index++];
	}
	
	//作者通道
	
	switch(battery_data._extend_data[0])
	{
		case 0x01:
			extendDataCase1(battery_data._extend_data);//代表清空历史数据判断
			break;
		case 0x02:
			extendDataCase2(battery_data._extend_data);//修改后台ip地址
			break;
		case 0x03:
			extendDataCase3(battery_data._extend_data);//代表修改平台发送类型
			break;
		case 0x04:
			extendDataCase4(battery_data._extend_data);//代表是否开启debug模式
			break;
		case 0x05:
			extendDataCase5(battery_data._extend_data);//代表修改传感器波特率设置
			break;
		default:
			break;
	}
	
	return index;
}

//根据传来的数据 设置相应参数 并返回设置结果
uint8_t hc08_data_to_param(uint8_t *data,uint8_t setUser)
{
	uint8_t have_device = 0,have_platform = 0,have_local = 0;
	uint32_t byte32;
	uint16_t byte16;
	uint16_t index = 3;
	
	//涉及到lcd的操作，因为设置参数可能导致详细页面的菜单栏发生改变，所以直接回到初始页面
	goFirstPage();
	
	//解析包含何种参数设置
	if(data[index] & 0x01)
		have_device = 1;
	
	if(data[index] & 0x02)
		have_platform = 1;
	
	if(data[index] & 0x04)
		have_local = 1;
	
	index++;
	
	//顺序非常重要 不可乱改
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
	
	//头
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//数据内容
	data[size++] = 0x02;
	data[size++] = 0x01;
	//CRC校验码
	CRCByte = modBusCRC16(data,size);
	data[size++] = CRCByte & 0xFF;
	data[size++] = CRCByte >> 8;
	return size;
}

uint16_t createFailedData(uint8_t *data)
{
	uint16_t size = 0;
	uint16_t CRCByte;
	
	//头
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//数据内容
	data[size++] = 0x02;
	data[size++] = 0x02;
	//CRC校验码
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
	
	//头
	append_header_to_data(data,&size,1);//1-->设备协议数据

	//设备数据
	data[size++] = 0x02;//目前是两种设备类型数
	
	data[size++] = DEVICE_WATERLEVEL;//设备类型一编号
	sprintf(tmp_data,"水位计");//设备一名称
	appendStringToData(data,&size,tmp_data);//把名字追加到数组里
	
	data[size++] = 0x02;//设备一包含两种协议
	data[size++] = WATERLEVEL_PROTOCOL1;//协议一编号
	sprintf(tmp_data,"古大雷达");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = WATERLEVEL_PROTOCOL2;//协议二编号
	sprintf(tmp_data,"4～20mA水位计");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = DEVICE_FLOWMETER;//设备类型二编号
	sprintf(tmp_data,"流量计");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//设备二 只包含一种协议
	data[size++] = FLOWMETER_PROTOCOL1;//协议一编号
	sprintf(tmp_data,"施耐德电磁");//协议一名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL2;//协议二编号
	sprintf(tmp_data,"大连海峰超声");//协议二名称
	appendStringToData(data,&size,tmp_data);
	
	//设备数据走完 计算公式数据
	data[size++] = 0x01;//目前只有一种计算公式
	data[size++] = FORMULA_XIECAI;//计算公式一编号
	sprintf(tmp_data,"谢才公式");
	appendStringToData(data,&size,tmp_data);
	
	//计算公式走完，渠形
	data[size++] = 0x03;//目前共有三种渠形
	
	data[size++] = RECT_CANAL;//渠形一编号
	sprintf(tmp_data,"矩形或梯形");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = CIR_CANAL;//渠形二编号
	sprintf(tmp_data,"圆形");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = U_CANAL;//渠形三编号
	sprintf(tmp_data,"U形");
	appendStringToData(data,&size,tmp_data);
	
	//crc校验
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
	
	//头
	append_header_to_data(data,&size,2);//模式2代表设备实际数据
	data[size++] = device_manager._device_count;//设备数量
	for(i = 0;i < device_manager._device_count;i++)
	{
		//通用设置
		data[size++] = device_manager._devices[i]._device_type;//设备类型
		data[size++] = device_manager._devices[i]._device_protocol;//设备协议
		data[size++] = device_manager._devices[i]._device_number;//设备编号
		data[size++] = device_manager._devices[i]._device_get_time >> 8 & 0xFF;
		data[size++] = device_manager._devices[i]._device_get_time & 0xFF; //采集时间
		data[size++] = device_manager._devices[i]._device_com_state;//通信状态
		data[size++] = device_manager._devices[i]._device_formula;//计算公式
		//参数数据
		byte = device_manager._devices[i]._water_param._deep_water * 1000;//积水深
		appendUint32ToData(data,&size,byte);
		half_byte = device_manager._devices[i]._water_param._n * 10000;//糙率
		appendUint16ToData(data,&size,half_byte);
		byte = device_manager._devices[i]._water_param._J * 10000000;//比降
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._range;
		//渠形数据
		data[size++] = device_manager._devices[i]._water_param._type;//渠形
		byte = device_manager._devices[i]._water_param._canheight * 1000;//渠高
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._topwidth * 1000;//渠顶宽
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._bottomwidth * 1000;//渠底宽
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._H * 1000;//直段深
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._r * 1000;//半径
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._a / 1;//外倾角
		byte = device_manager._devices[i]._water_param._insheight * 1000;//安装高度
		appendUint32ToData(data,&size,byte);		
		
		//实时数据
		byte = device_manager._devices[i]._water_data._airHeight * 1000;//空高
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._waterlevel * 1000;//水位
		appendUint32ToData(data,&size,byte);
		if(device_manager._devices[i]._device_type == DEVICE_FLOWSPEED && device_manager._devices[i]._device_protocol == FLOWSPEED_MSYX1)
			byte = device_manager._devices[i]._water_data._flowspeed * 1000;
		else
			byte = device_manager._devices[i]._water_data._flowRate * 1000 * 3600.0;//瞬时流量
		appendUint32ToData(data,&size,byte);
		if(device_manager._devices[i]._water_data._flow < 0)
			byte = 0;
		else
			byte = device_manager._devices[i]._water_data._flow;//流量
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_backup;//流量备份
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_change_backup;//流量修改值备份
		appendUint32ToData(data,&size,byte);				
	}
	
	//crc校验
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
	
	//头
	append_header_to_data(data,&size,3);
	
	//站点编码
	get_address_bytes(address_data,8,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[size++] = address_data[i];
	}
#if 0	
	//协议数据
	data[size++] = 0x01;//目前一条水文规约
	
	data[size++] = 0x01;//水文规约一编号
	sprintf(tmp_data,"SZY206-2016");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//目前三种地址类型
	
	data[size++] = 0x01;//地址类型一编号
	sprintf(tmp_data,"写1234报1234");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//地址类型二编号
	sprintf(tmp_data,"写1234报3412");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//地址类型三编号
	sprintf(tmp_data,"写1234报D204");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//目前两种数据类型
	
	data[size++] = 0x01;//数据类型一编号
	sprintf(tmp_data,"流量在前,累计在后");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//地址类型二编号
	sprintf(tmp_data,"累计在前,流量在后");
	appendStringToData(data,&size,tmp_data);
#endif	
	//平台数量
	data[size++] = platform_manager._platform_count;
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		data[size++] = platform_manager._platforms[i]._connect_state;//与平台通信状态
		data[size++] = platform_manager._platforms[i]._enable;//平台启用
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
		
		data[size++] = platform_manager._platforms[i]._protocol;//水文规约
		data[size++] = platform_manager._platforms[i]._address_type;//地址类型
		data[size++] = platform_manager._platforms[i]._data_type;//数据类型
		data[size++] = platform_manager._platforms[i]._interval_time >> 8 & 0xFF;
		data[size++] = platform_manager._platforms[i]._interval_time & 0xFF;//报 存时间
		data[size++] = platform_manager._platforms[i]._mode;//应答模式	
	}
	
	//crc校验
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
	
	//头
	append_header_to_data(data,&size,0xFF);
#if 0	
	data[size++] = 0x02;//目前有两种外部供电方式
	data[size++] = 0x01;//外部供电方式一编号
	sprintf(tmp_data,"市电");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//外部供电方式二编号
	sprintf(tmp_data,"太阳能电池");
	appendStringToData(data,&size,tmp_data);
#endif	
	//设备编号
	for(i = 0;i < 7;i++)
	{
		data[size++] = platform_manager._device_number[i];
	}
	
	data[size++] = battery_data._version;//程序版本
	data[size++] = battery_data._power_mode;//供电方式
	data[size++] = battery_data._have_power;//是否供电
	data[size++] = battery_data._voltage;//电压
	//网络状态
	if(EC20_msg._ec20_haveinit)
		data[size++] = 0x00;//正常
	else
	{
		if(EC20_msg._ec20_stage._type == EC20_ATCPIN && EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x01;//未识别到sim卡
		}
		else if((EC20_msg._ec20_stage._type == EC20_ATCREG || EC20_msg._ec20_stage._type == EC20_ATCGREG)&& EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x02;//组网失败
		}
		else
		{
			data[size++] = 0x03;//组网中...
		}
	}
	
	data[size++] = EC20_msg._csq1;//信号质量值

	if(battery_data._extend_data[0] == 0x01)
		battery_data._extend_data[0] = 0xFF;	
	for(i = 0;i < 8;i++)
	{//扩展数据		
		data[size++] = battery_data._extend_data[i];
	}

	//加上平台数据
	//站点编码
	get_address_bytes(address_data,8,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[size++] = address_data[i];
	}
	
	//平台数量
	data[size++] = platform_manager._platform_count;
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		data[size++] = platform_manager._platforms[i]._connect_state;//与平台通信状态
		data[size++] = platform_manager._platforms[i]._enable;//平台启用
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
		
		data[size++] = platform_manager._platforms[i]._protocol;//水文规约
		data[size++] = platform_manager._platforms[i]._address_type;//地址类型
		data[size++] = platform_manager._platforms[i]._data_type;//数据类型
		data[size++] = platform_manager._platforms[i]._interval_time >> 8 & 0xFF;
		data[size++] = platform_manager._platforms[i]._interval_time & 0xFF;//报 存时间
		data[size++] = platform_manager._platforms[i]._mode;//应答模式	
		half_byte = platform_manager._platforms[i]._send_data_type;
		appendUint16ToData(data,&size,half_byte);
	}
	
	//加上传感器数据	
	data[size++] = device_manager._device_count;//设备数量
	for(i = 0;i < device_manager._device_count;i++)
	{
		//通用设置
		data[size++] = device_manager._devices[i]._device_type;//设备类型
		data[size++] = device_manager._devices[i]._device_protocol;//设备协议
		data[size++] = device_manager._devices[i]._device_number;//设备编号
		data[size++] = device_manager._devices[i]._device_get_time >> 8 & 0xFF;
		data[size++] = device_manager._devices[i]._device_get_time & 0xFF; //采集时间
		data[size++] = device_manager._devices[i]._device_com_state;//通信状态
		data[size++] = device_manager._devices[i]._device_formula;//计算公式
		//参数数据
		byte = device_manager._devices[i]._water_param._deep_water * 1000;//积水深
		appendUint32ToData(data,&size,byte);
		half_byte = device_manager._devices[i]._water_param._n * 10000;//糙率
		appendUint16ToData(data,&size,half_byte);
		byte = device_manager._devices[i]._water_param._J * 10000000;//比降
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._range;
		//渠形数据
		data[size++] = device_manager._devices[i]._water_param._type;//渠形
		byte = device_manager._devices[i]._water_param._canheight * 1000;//渠高
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._topwidth * 1000;//渠顶宽
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._bottomwidth * 1000;//渠底宽
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._H * 1000;//直段深
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._r * 1000;//半径
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._a / 1;//外倾角
		byte = device_manager._devices[i]._water_param._insheight * 1000;//安装高度
		appendUint32ToData(data,&size,byte);		
		
		//实时数据
		byte = device_manager._devices[i]._water_data._airHeight * 1000;//空高
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._waterlevel * 1000;//水位
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flowRate * 1000 * 3600.0;//瞬时流量
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow;//流量
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_backup;//流量备份
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_change_backup;//流量修改值备份
		appendUint32ToData(data,&size,byte);				
	}
	
	//crc校验
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	return size;
}

//发送设置成功给蓝牙
static void send_success_hc08()
{
	uint8_t data[8] = {0};
	uint16_t size = 0;
	/*
	uint16_t CRCByte;
	
	//头
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//数据内容
	data[size++] = 0x02;
	data[size++] = 0x01;
	//CRC校验码
	CRCByte = modBusCRC16(data,size);
	data[size++] = CRCByte & 0xFF;
	data[size++] = CRCByte >> 8;
	*/
	size = createSuccessData(data);
	Uart3_SendStr(data,size);
}

//发送设置失败给蓝牙
static void send_failed_hc08()
{
	uint8_t data[8] = {0};
	uint16_t size = 0;
	/*
	uint16_t CRCByte;
	
	//头
	data[size++] = 0x4A;
	data[size++] = 0x53;
	//数据内容
	data[size++] = 0x02;
	data[size++] = 0x02;
	//CRC校验码
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
	
	//头
	append_header_to_data(data,&size,1);//1-->设备协议数据

	//设备数据
	data[size++] = 0x03;//目前是两种设备类型数
	
	data[size++] = DEVICE_WATERLEVEL;//设备类型一编号
	sprintf(tmp_data,"水位计");//设备一名称
	appendStringToData(data,&size,tmp_data);//把名字追加到数组里
	
	data[size++] = 0x02;//设备一包含两种协议
	data[size++] = WATERLEVEL_PROTOCOL1;//协议一编号
	sprintf(tmp_data,"古大雷达");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = WATERLEVEL_PROTOCOL2;//协议二编号
	sprintf(tmp_data,"4～20mA水位计");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = DEVICE_FLOWMETER;//设备类型二编号
	sprintf(tmp_data,"流量计");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x0B;//设备二 包含11种协议
	data[size++] = FLOWMETER_PROTOCOL1;//协议一编号
	sprintf(tmp_data,"施耐德电磁");//协议一名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL2;//协议二编号
	sprintf(tmp_data,"大连海丰超声");//协议二名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL3;//协议三编号
	sprintf(tmp_data,"安布雷拉超声");//协议三名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL4;//协议四编号
	sprintf(tmp_data,"安布雷拉电磁");//协议四名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL5;//协议四编号
	sprintf(tmp_data,"上海肯特电磁");//协议五名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL6;//协议四编号
	sprintf(tmp_data,"建恒超声");//协议六名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL7;//协议7编号
	sprintf(tmp_data,"百江通超声");//协议7名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_PROTOCOL8;//协议8编号
	sprintf(tmp_data,"华隆电磁");//协议8名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_JINYI;//协议9编号
	sprintf(tmp_data,"京仪电磁");//协议9名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_JDLY;//协议10编号
	sprintf(tmp_data,"佳德超声");//协议10名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FLOWMETER_HZZH;//协议11编号
	sprintf(tmp_data,"振华电磁");//协议11名称
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = DEVICE_FLOWSPEED;//设备类型三编号
	sprintf(tmp_data,"流速仪");//设备一名称
	appendStringToData(data,&size,tmp_data);//把名字追加到数组里
	
	data[size++] = 0x01;//设备三包含一种协议
	data[size++] = FLOWSPEED_MSYX1;//协议一编号
	sprintf(tmp_data,"京水流速仪");
	appendStringToData(data,&size,tmp_data);
	
	
	//设备数据走完 计算公式数据
	data[size++] = 0x02;//目前只有二种计算公式	
	data[size++] = FORMULA_XIECAI;//计算公式一编号
	sprintf(tmp_data,"谢才公式");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = FORMULA_NONE;//计算公式二编号
	sprintf(tmp_data,"无计算");
	appendStringToData(data,&size,tmp_data);
	
	
	//计算公式走完，渠形
	data[size++] = 0x03;//目前共有三种渠形
	
	data[size++] = RECT_CANAL;//渠形一编号
	sprintf(tmp_data,"矩形或梯形");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = CIR_CANAL;//渠形二编号
	sprintf(tmp_data,"圆形");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = U_CANAL;//渠形三编号
	sprintf(tmp_data,"U形");
	appendStringToData(data,&size,tmp_data);
	
	//crc校验
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	Uart3_SendStr(data,size);//将数据发送到串口
}

/*
* 发送传感器实际数据给蓝牙
*/
static void send_device_real_to_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	uint16_t crc_byte = 0;
	uint8_t i;
	
	uint16_t half_byte;
	uint32_t byte;
	
	//头
	append_header_to_data(data,&size,2);//模式2代表设备实际数据
	data[size++] = device_manager._device_count;//设备数量
	for(i = 0;i < device_manager._device_count;i++)
	{
		//通用设置
		data[size++] = device_manager._devices[i]._device_type;//设备类型
		data[size++] = device_manager._devices[i]._device_protocol;//设备协议
		data[size++] = device_manager._devices[i]._device_number;//设备编号
		data[size++] = device_manager._devices[i]._device_get_time >> 8 & 0xFF;
		data[size++] = device_manager._devices[i]._device_get_time & 0xFF; //采集时间
		if(device_manager._devices[i]._device_com_state != 1)
		{
			data[size++] = 0;
		}
		else
		{
			data[size++] = device_manager._devices[i]._device_com_state;//通信状态
		}		
		data[size++] = device_manager._devices[i]._device_formula;//计算公式
		//参数数据
		byte = device_manager._devices[i]._water_param._deep_water * 1000;//积水深
		appendUint32ToData(data,&size,byte);
		half_byte = device_manager._devices[i]._water_param._n * 10000;//糙率
		appendUint16ToData(data,&size,half_byte);
		byte = device_manager._devices[i]._water_param._J * 10000000;//比降
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._range;
		//渠形数据
		data[size++] = device_manager._devices[i]._water_param._type;//渠形
		byte = device_manager._devices[i]._water_param._canheight * 1000;//渠高
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._topwidth * 1000;//渠顶宽
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._bottomwidth * 1000;//渠底宽
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._H * 1000;//直段深
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_param._r * 1000;//半径
		appendUint32ToData(data,&size,byte);
		data[size++] = device_manager._devices[i]._water_param._a / 1;//外倾角
		byte = device_manager._devices[i]._water_param._insheight * 1000;//安装高度
		appendUint32ToData(data,&size,byte);
		
		//实时数据
		byte = device_manager._devices[i]._water_data._waterlevel * 1000;//水位
		appendUint32ToData(data,&size,byte);
		if(device_manager._devices[i]._device_type == DEVICE_FLOWSPEED && device_manager._devices[i]._device_protocol == FLOWSPEED_MSYX1)
			byte = device_manager._devices[i]._water_data._flowspeed * 1000;
		else
			byte = device_manager._devices[i]._water_data._flowRate * 1000 * 3600.0;//瞬时流量
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow;//流量
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_backup;//流量备份
		appendUint32ToData(data,&size,byte);
		byte = device_manager._devices[i]._water_data._flow_change_backup;//流量修改值备份
		appendUint32ToData(data,&size,byte);				
	}
	
	//crc校验
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	//发送数据
	Uart3_SendStr(data,size);//将数据发送到串口
}

static void send_platform_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	uint8_t i,byte,j;
	uint8_t address_data[8] = {0};
	
	//头
	append_header_to_data(data,&size,3);
	
	//站点编码
	get_address_bytes(address_data,8,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
	for(i = 0;i < 5;i++)
	{
		data[size++] = address_data[i];
	}
	
	//协议数据
	data[size++] = 0x02;//目前一条水文规约
	
	data[size++] = 0x01;//水文规约一编号
	sprintf(tmp_data,"SZY206-2016");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//水文规约一编号
	sprintf(tmp_data,"江西生态泄流");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//目前三种地址类型
	
	data[size++] = 0x01;//地址类型一编号
	sprintf(tmp_data,"写1234报1234");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//地址类型二编号
	sprintf(tmp_data,"写1234报3412");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x03;//地址类型三编号
	sprintf(tmp_data,"写1234报D204");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//目前两种数据类型
	
	data[size++] = 0x01;//数据类型一编号
	sprintf(tmp_data,"流量在前,累计在后");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//地址类型二编号
	sprintf(tmp_data,"累计在前,流量在后");
	appendStringToData(data,&size,tmp_data);
	
	//平台数量
	data[size++] = platform_manager._platform_count;
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		data[size++] = 0x00;//填充数据 与蓝牙端通信问题
		data[size++] = platform_manager._platforms[i]._enable;//平台启用
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
		
		data[size++] = platform_manager._platforms[i]._protocol;//水文规约
		data[size++] = platform_manager._platforms[i]._address_type;//地址类型
		data[size++] = platform_manager._platforms[i]._data_type;//数据类型
		data[size++] = platform_manager._platforms[i]._interval_time >> 8 & 0xFF;
		data[size++] = platform_manager._platforms[i]._interval_time & 0xFF;//报 存时间
		data[size++] = platform_manager._platforms[i]._mode;//应答模式	
	}
	
	//crc校验
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	//发送数据
	Uart3_SendStr(data,size);//将数据发送到串口
}

static void send_RTU_data(void)
{
	uint8_t data[1024] = {0};
	uint16_t size = 0;
	char tmp_data[128] = {0};
	uint16_t crc_byte = 0;
	uint8_t i,byte,j;
	
	//头
	append_header_to_data(data,&size,4);
	
	data[size++] = 0x02;//目前有两种外部供电方式
	data[size++] = 0x01;//外部供电方式一编号
	sprintf(tmp_data,"市电");
	appendStringToData(data,&size,tmp_data);
	
	data[size++] = 0x02;//外部供电方式二编号
	sprintf(tmp_data,"太阳能电池");
	appendStringToData(data,&size,tmp_data);
	
	//设备编号
	for(i = 0;i < 7;i++)
	{
		data[size++] = platform_manager._device_number[i];
	}
	
	data[size++] = battery_data._power_mode;//供电方式
	data[size++] = battery_data._have_power;//是否供电
	data[size++] = battery_data._voltage;//电压
	//网络状态
	if(EC20_msg._ec20_haveinit)
		data[size++] = 0x00;//正常
	else
	{
		if(EC20_msg._ec20_stage._type == EC20_ATCPIN && EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x02;//未识别到sim卡
		}
		else if((EC20_msg._ec20_stage._type == EC20_ATCREG || EC20_msg._ec20_stage._type == EC20_ATCGREG)&& EC20_msg._ec20_stage._failed_num >= 20)
		{
			data[size++] = 0x03;//组网失败
		}
		else
		{
			data[size++] = 0x01;//组网中...
		}
	}
	
	data[size++] = EC20_msg._csq1;//信号质量值
	
	for(i = 0;i < 8;i++)
	{//扩展数据
		data[size++] = battery_data._extend_data[i];
	}	
	
	//crc校验
	crc_byte = modBusCRC16(data,size);
    data[size++] = crc_byte & 0xFF;
    data[size++] = (crc_byte >> 8) & 0xFF;
	
	//发送数据
	Uart3_SendStr(data,size);//将数据发送到串口
}

//返回数据给hc08 目前直接是采用一包发送
static void send_data_to_hc08(uint8_t mode,uint8_t param)
{
	if(mode == 1)
	{
		if(param == 1)
		{//设备协议数据
			send_device_protoclo_data();
		}
		else if(param == 2)
		{//设备实际数据
			send_device_real_to_data();
		}
		else if(param == 3)
		{//平台数据
			send_platform_data();
		}
		else if(param == 4)
		{//本地数据
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
*设置蓝牙名称 规则: JSRTU_ + 设备编号后四位
*/
void setBluetoothName(void)
{
	uint8_t data[32] = {0};
	
	//设置名称
	sprintf(data,"AT+NAME=JSRTU_%02X%02X",platform_manager._device_number[5],platform_manager._device_number[6]);
	//发送指令
	Uart3_SendStr(data,18);//将数据发送到串口,固定是18个字节	
}

/*解析传过来的数据 并返回相应结果
*返回值:0-->非法数据 1-->请求数据 2-->设置参数成功 3-->设置参数失败 4 -->未知请求
*/
uint8_t parse_hc08_data(uint8_t *mode,uint8_t *param,uint8_t *data,uint16_t dataSize)
{
	uint16_t CRCByte;
	
	//检测数据是否合法
	if(dataSize < 5)
		return 0;
	
	//是否符合固定协议头
	if(data[0] != 0x4A || data[1] != 0x53)
		return 0;
	
	//是否符合modbusCRC校验
	CRCByte = modBusCRC16(data,dataSize - 2);
	if((CRCByte & 0xFF) != data[dataSize - 2] || (CRCByte >> 8 & 0xFF) != data[dataSize - 1])
		return 0;
	
	//解析合法数据
	if(data[2] == 0x01)
	{//请求数据
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
	{//解析数据成功
		send_data_to_hc08(mode,param);
	}
	
	clear_hc08_data();
	hc08_mutex = 0;
}

void USART3_IRQHandler(void)                	//串口3中断服务程序
{
	uint8_t Res;
	
	if(hc08_mutex)
		return;

	hc08_data_mutex = 1;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
	{
		TIM_Cmd(TIM7, ENABLE);
		TIM7_timeout = 0;
		Res =USART_ReceiveData(USART3);//(USART1->DR);	//读取接收到的数据
		if(hc08_size < HC08SIZE)
			hc08_data[hc08_size++] = Res;
	} 

}


void TIM7_Int_Init(uint16_t arr,uint16_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);  ///使能TIM3时钟
	
	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM7,&TIM_TimeBaseInitStructure);//初始化TIM7
	
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE); //允许定时器2更新中断
	TIM_Cmd(TIM7,DISABLE); //不使能定时器7
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM7_IRQn; //定时器2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//定时器7中断服务函数
void TIM7_IRQHandler(void)
{
	msg_event_t event;
	if(TIM_GetITStatus(TIM7,TIM_IT_Update)==SET) //溢出中断
	{	
		//60ms没有数据来认为一帧数据结束
		TIM7_timeout++;
		
		if(TIM7_timeout >= 10)
		{
			hc08_data_mutex = 0;
			event._msg_type = MSG_READ_USART3;
			push_event(event);
			TIM_Cmd(TIM7, DISABLE);  //不使能TIMx外设	
		}
		
	}
	TIM_ClearITPendingBit(TIM7,TIM_IT_Update);  //清除中断标志位
}

//附加部分功能  获取设备编号
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
	
	//获取设备ID完成
	crc_32 = crc32_in_c(cpu_id,12);
	crc_16 = modBusCRC16(cpu_id,12);
	platform_manager._device_number[index++] = 0x01;
	appendUint32ToData(platform_manager._device_number,&index,crc_32);
	appendUint16ToData(platform_manager._device_number,&index,crc_16);
}

