#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_conf.h"
#include "protocol.h"
#include "math.h" //不包含也不会编译报错 但需要包含 不然pow函数 总返回0

#define PI acos(-1)
#define DEC (PI / 180)

uint8_t crcCalc(unsigned char *data,unsigned int dataLen)
{
	unsigned int i;
	unsigned char j;
	unsigned char crc = 0x00;
	for(i = 0;i < dataLen;i++)
	{
		crc ^= data[i];
		for(j = 8;j > 0;j--)
		{
			if(crc & 0x80)
			{
				crc <<= 1;
				crc ^= 0xe5;//x7 + x6 + x5 + x2 + 1
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

uint16_t modBusCRC16(uint8_t *data,uint16_t dataSize)
{
	uint16_t i,j,tmp,CRC16;
	
	CRC16 = 0xFFFF;
	for(i = 0;i < dataSize;i++)
	{
		CRC16 ^= data[i];
		for(j = 0;j < 8;j++)
		{
			tmp = CRC16 & 0x0001;
			CRC16 >>= 1;
			if(tmp == 1)
			{
				CRC16 ^= 0xA001;
			}
		}
	}
	return CRC16;
}

uint32_t crc32_in_c(uint8_t *p, int32_t len)
{
	uint8_t i;
    uint32_t crcByte = 0xffffffff;
    while (--len >= 0)
    {
        crcByte = crcByte ^ *p++;
        for (i = 0; i < 8;i++)
        {
            crcByte = (crcByte >> 1) ^ (0xedb88320 & -(crcByte & 1));
        }
    }
    return ~crcByte;
}


uint8_t CheckDataLegality(uint8_t *data,uint32_t dataSize,uint8_t mode)
{
	uint16_t crcByte;
	
	if(dataSize < 5)//数据过短
		return 0;
	
	if(mode != 0xFF)
	{
		if(data[0] != 0x4A || data[1] != 0x53)//数据不符合协议头
			return 0xFE;
	}
	
	crcByte = modBusCRC16(data,dataSize - 2);	
	if((crcByte & 0xFF) != data[dataSize - 2] || (crcByte >> 8 & 0xFF) != data[dataSize - 1])//数据不符合crc
		return 0;
	
	if(mode != 0xFF)
		return data[2];
	
	return 1;
}

uint8_t byteToBcd(uint8_t byte)
{
	uint8_t bcdhigh = 0;
  
	while (byte >= 10)
	{
		bcdhigh++;
		byte -= 10;
	}
  
	return  ((uint8_t)(bcdhigh << 4) | byte);
}

uint16_t byte16ToBcd(uint16_t byte16)
{
	uint16_t sum = 0, c = 1;  // sum返回十进制，c每次翻10倍
    uint16_t i;
    for(i = 1; byte16 > 0; i++)
    {
        if( i >= 2)
        {
            c*=16;
        }
        sum += (byte16%10) * c;
        byte16 /= 10;  // 除以16同理与十进制除10将小数点左移一次，取余16也同理
    }
    return sum;
}

uint32_t bcdToByte(uint32_t bcd)
{
    uint32_t sum = 0, c = 1;  // sum返回十进制，c每次翻10倍
    uint32_t i;
    for(i = 1; bcd > 0; i++)
    {
        if( i >= 2)
        {
            c*=10;
        }
        sum += (bcd%16) * c;
        bcd /= 16;  // 除以16同理与十进制除10将小数点左移一次，取余16也同理
    }
    return sum;
}

//通过日期换算星期几
uint8_t getWeekdayByYearDay(uint32_t year,uint8_t month,uint8_t day)
{
    uint8_t iWeekDay = 0;

    if(month == 1 || month == 2)
    {
        month += 12;

        year -= 1;
    }

    iWeekDay = (day + 1 + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;
    if(iWeekDay ==0)
        iWeekDay = 7;
    return iWeekDay;
}

void UTCToBeijing(jxjs_time_t* time)
{
	uint8_t days = 0;
	if (time->_month == 1 || time->_month == 3 || time->_month == 5 || time->_month == 7 || time->_month == 8 || time->_month == 10 || time->_month == 12)
	{
		days = 31;
	}
	else if (time->_month == 4 || time->_month == 6 || time->_month == 9 || time->_month == 11)
	{
		days = 30;
	}
	else if (time->_month == 2)
	{
		if ((time->_year % 400 == 0) || ((time->_year % 4 == 0) && (time->_year % 100 != 0))) /* 判断平年还是闰年 */
		{
			days = 29;
		}
		else
		{
			days = 28;
		}
	}
	time->_hour += 8;                 /* 北京时间比格林威治时间快8小时 */
	if (time->_hour >= 24)            /* 跨天 */
	{
		time->_hour -= 24;
		time->_day++;
		if (time->_day > days)        /* 跨月 */
		{
			time->_day = 1;
			time->_month++;
			if (time->_month > 12)    /* 跨年 */
			{
				time->_year++;
			}
		}
	}
	time->_weekday = getWeekdayByYearDay(time->_year, time->_month, time->_day);  /* 重新计算周 */
}

void init_deviceStruct(jxjs_device_t *device)
{
	device->_device_type = DEVICE_WATERLEVEL;
	device->_device_protocol = WATERLEVEL_PROTOCOL1;
	device->_failed_num = 0;
	device->_device_number = 0x01;
	device->_device_com_state = 0x00;
	device->_device_formula = 0x01;
	init_waterData(&device->_water_data);
	init_waterParam(&device->_water_param);
	initDeviceUartConfig(&device->_uart_config);
}

void init_waterData(jxjs_water_data_t *data)
{
	data->_flow = 0;
	data->_flowRate = 0;
	data->_flowspeed = 0;
	data->_waterlevel = 0;
	data->_flow_backup = 0;
	data->_flow_change_backup = 0;
	data->_airHeight = 0;
}

void initDeviceUartConfig(device_uart_config_t *config)
{
	config->_bound = 9600;
	config->_parity = 0x0000;//USART_Parity_No
	config->_stopbit = 0x0000;//USART_StopBits_1
	config->_wordlength = 0x0000;//USART_WordLength_8b
}

uint8_t checkDeviceUartConfigIsEqual(device_uart_config_t *config1,device_uart_config_t *config2)
{
	if( (config1->_bound == config2->_bound) && (config1->_parity == config2->_parity)
		&& (config1->_stopbit == config2->_stopbit) && (config1->_wordlength == config2->_wordlength) )
		return 1;
	
	return 0;
}

void init_waterParam(water_param_t *param)
{
	param->_type = RECT_CANAL;
	param->_deep_water = 0;
	param->_n = 0.0225;
	param->_a = 0;
	param->_bottomwidth = 0;
	param->_canheight = 0;
	param->_H = 0;
	param->_insheight = 10;
	param->_insheight_mm = 10000;
	param->_J = 0.0001;
	param->_r = 0.5;
	param->_topwidth = 0;
	param->_ur = 0;
	param->_range = 10;
}

void init_device_manager(device_manager_t	*manager)
{
	uint8_t i;
	manager->_device_count = 0;
	for(i = 0;i < 6;i++)
	{
		init_deviceStruct(&manager->_devices[i]);
	}
}

//矩形渠谢才公式计算
static uint8_t chezy_formula_rec(water_param_t param,double airHeight,double *flowSpeed,double *flowRate)
{
	double C,R,S;
	double topwidth,bottomwidth,realHeight,realLong,k,countHeight;
	
	if(param._n == 0 || param._canheight == 0 || (param._bottomwidth == param._topwidth && param._bottomwidth == 0))
		return 1;
	
	if(param._deep_water < 0)
		param._deep_water = 0;
	
	realHeight  = param._insheight - airHeight;
	if(realHeight < param._deep_water || realHeight > param._canheight)
		return 2;
	
	countHeight = realHeight - param._deep_water;
	
	if(param._bottomwidth == param._topwidth)
	{
		topwidth = param._topwidth;
		bottomwidth = param._bottomwidth;
		realLong = countHeight;
	}
	else
	{
		k = (param._bottomwidth - param._topwidth) / param._canheight;
		bottomwidth = param._bottomwidth - k * param._deep_water;
		topwidth = param._bottomwidth - k * realHeight;
		R = (bottomwidth - topwidth) * (bottomwidth - topwidth) / 4.0 + param._canheight * param._canheight;
		realLong = pow(R,0.5) * countHeight / param._canheight;
	}
	
	S = (bottomwidth + topwidth) * countHeight / 2.0;
	R = S / (bottomwidth + realLong * 2);
	C = (1.0 / param._n) * pow(R,1.0 / 6);
	*flowSpeed = C * pow(R * param._J,0.5);
	*flowRate = *flowSpeed * S;
	return 0;
}

//圆形渠谢才公式计算
static uint8_t chezy_formula_cir(water_param_t param,double airHeight,double *flowSpeed,double *flowRate)
{
	double C,R,S,L;
	double r,radian,realHeight,chordLength,height,canheight;
	double radian_deep,chordLength_deep,heigth_deep,s_deep,l_deep;
	if(param._n == 0)
		return 1;
	
	r = param._r;
	canheight = param._canheight;
	
	if(param._deep_water < 0)
		param._deep_water = 0;
		
	realHeight  = param._insheight - airHeight;
	if(realHeight < param._deep_water || realHeight > canheight || realHeight > r)
		return 2;
	
	if(param._r <= 0)
		return 3;
	
	//实际水位扇形
	height = r - realHeight;
	chordLength = 2 * pow((r * r - height * height),0.5);
	radian = 2 * acos(height / r);
	L = radian * param._r;
	S = 0.5 * (L * r - chordLength * height);
	
	//积水深扇形
	if(param._deep_water > 0)
	{
		heigth_deep = r - param._deep_water;
		chordLength_deep = 2 * pow((r * r - heigth_deep * heigth_deep),0.5);
		radian_deep = 2 * acos(heigth_deep / r);
		l_deep = radian_deep * r;
		s_deep = 0.5 * (l_deep * r - chordLength_deep * heigth_deep);
	}
	else
	{
		chordLength_deep = 0;
		s_deep = 0;
		l_deep = 0;
	}
	
	S = S - s_deep;
	L = L - l_deep + chordLength_deep;	
	R = S / L;
	C = (1.0 / param._n) * pow(R,1.0 / 6);
	*flowSpeed = C * pow(R * param._J,0.5);
	*flowRate = *flowSpeed * S;
	return 0;
}

//U形渠谢才公式计算
static uint8_t chezy_formula_u(water_param_t param,double airHeight,double *flowSpeed,double *flowRate)
{
	double C,R,S,L;
	double h1,h2,r,a,H;
	
	if(param._n == 0)
		return 1;
	
	H = param._insheight - airHeight;
	if(H < 0 || H > param._canheight)
		return 2;
	
	r = param._ur;
	if(r <= 0)
		return 3;
	
	a = param._a;
	h1  = r * (1 - sin(a * DEC));
	if(H <= h1)
		return chezy_formula_cir(param,airHeight,flowSpeed,flowRate);
	
	h2 = H - h1;
	
	S = (r * r / 2 ) * (PI * (1 - a/90) - sin(2 * a * DEC)) + h2 * (2 * r * cos(a * DEC) + h2 * tan(a * DEC));
	L = PI * r * (1 - a/90) + 2 * h2 / cos(a * DEC);
	R = S / L;
	C = (1.0 / param._n) * pow(R,1.0 / 6);
	*flowSpeed = C * pow(R * param._J,0.5);
	*flowRate = *flowSpeed * S;
	return 0;
}

//谢才公式 返回0成功 流速赋值到flowspeed 流量赋值到flowrate 返回其余 失败
uint8_t Chezy_formula(water_param_t param,double airHeight,double *flowSpeed,double *flowRate)
{
	if(param._type == RECT_CANAL)
		return chezy_formula_rec(param,airHeight,flowSpeed,flowRate);
	else if(param._type == CIR_CANAL)
		return chezy_formula_cir(param,airHeight,flowSpeed,flowRate);
	else if(param._type == U_CANAL)
		return chezy_formula_u(param,airHeight,flowSpeed,flowRate);
	
	return 1;
}

void init_platform(jxjs_platform_t	*platform)
{
	platform->_type = JXJS_PLATFORM_SELF;
	platform->_enable = 1;
	platform->_mode = NO_RESPONSE_MODE;
	platform->_connect_state = 0x00;
	platform->_sendFlag = 0;
	platform->_ip1 = 127;
	platform->_ip2 = 0;
	platform->_ip3 = 0;
	platform->_ip4 = 1;
	platform->_port = 8080;
	platform->_interval_time = 5 * 60;
	platform->_last_send_time = 0;
	platform->_last_save_time = 0;
	platform->_address_type = ADDRESSTYPE_NORMAL;
	platform->_data_type = DATATYPE_FLOW_BEHIND;
	platform->_send_data_type = JXJS_CONTROL_FUNC_FLOW_BIT;
	platform->_ip_type = IPTYPE_IPV4;
	platform->_protocol = PROTOCOL_WATER2016;
	platform->_cont_failed_num = 0;
}

void init_platform_manager(platform_manager_t	*manager)
{
	uint8_t	i;
	manager->_platform_count = 0;
	for(i = 0;i < 8;i++)
	{
		init_platform(&(manager->_platforms[i]));
		manager->_platforms[i]._last_send_time = i * 10;
	}
	manager->_platform_number_a1 = 0;
	manager->_platform_number_a2 = 0;
	for(i = 0;i < 7;i++)
	{
		manager->_device_number[i] = 0;
	}
	//\"47.97.200.120\",2347
	manager->_ipv4[0] = 218;
	manager->_ipv4[1] = 95;
	manager->_ipv4[2] = 67;
	manager->_ipv4[3] = 107;
	manager->_port = 2347;
}

void init_battery_data(battery_data_t *data)
{
	uint8_t i;
	data->_version = 1;
	data->_have_power = 1;
	data->_power_mode = 1;
	data->_voltage = 12 * 10;
	for(i = 0;i < 8;i++)
	{
		data->_extend_data[i] = 0;
	}
}

float Hex2Float(uint8_t *data)
{
#if 0
	uint8_t i;
	uint32_t number = 0;
	int8_t sign;
	int8_t exponent;
	float mantissa;
	for(i = 0;i < 4;i++)
	{
		number <<= 8;
		number += data[i];
	}
	
	if(((number >> 23) & 0xff) == 0)
		return 0;
	
	sign = (number & 0x80000000) ? -1 : 1;
	exponent = ((number >> 23) & 0xff) - 127;
	mantissa = 1 + ((float)(number & 0x7fffff) / 0x7fffff);
	return sign * mantissa * pow(2,exponent);
#else
	uint8_t i;	
	uint32_t number = 0;
	
    for(i = 0;i < 4;i++)
    {
        number <<= 8;
        number += data[i];
    }
	
    return *(float *)&number;
#endif
}


uint8_t jxjs_double2data(double f1,uint8_t *data,uint8_t datasize)
{
	uint8_t i;
	double *pf1 = &f1;
	
	if(datasize != 8)
		return 1;
	
	for(i = 0;i < 8;i++)
	{
		data[i] = *((uint8_t*)pf1 + i);
	}
	return 0;
}

double jxjs_data2double(uint8_t *data)
{
	return *(double *)data;
}

uint16_t append_byte4_to_data(uint8_t *data,uint32_t byte,uint16_t size)
{
	data[size++] = byte >> 24 & 0xFF;
	data[size++] = byte >> 16 & 0xFF;
	data[size++] = byte >> 8 & 0xFF;
	data[size++] = byte & 0xFF;
	return size;
}
void initRs485State(rs485_state_t *state)
{
	state->_failed_num =0;
	state->_flag = 0;
	state->_index = 0;
	state->_stage = 0;
	state->_cmd_time = 0;
	state->_last_res = 0x00;
}

void init_lcd_msg(LCD_msg_t *msg)
{
	msg->_menu = MENU_MAIN;
	msg->_menuIndex = 0;
	
	msg->_lineTop = 0;
	msg->_currentLine = 0;
	msg->_currentLineHeight = 1;
	
	msg->_backState = LCD_BACK_ON;
	msg->_lastBackOnTime = 0;
}

/*
函数描述：根据固定参数组建水文控制域 控制域为一个字节
参数：dir:方向 spilt:拆分标志 fcb:帧计数 func:功能码 详情可参考枚举 enum JXJS_CONTROL_FUNC
返回值：uint8_t 拼接好的水文控制域
*/
static uint8_t get_control_byte(uint8_t dir,uint8_t spilt,uint8_t fcb,uint8_t func)
{
	uint8_t byte = 0;
	
	byte |= dir << 7;
	
	byte |= spilt << 6;
	
	byte |= fcb << 4;
	
	byte |= func;
	
	return byte;
}

/*
函数描述：根据参数组建水文地址域包，长度为5个字节
参数：addressBytes传来的数组指针 包将拷贝进这里 a1:行政区划 a2:测站地址
返回：成功：包的长度 失败：0
*/
uint8_t get_address_bytes(uint8_t *addressBytes,uint8_t bytesLength,uint32_t a1,uint16_t a2,uint16_t port)
{
	uint32_t a2_byte;
	//行政区划码A1
	addressBytes[0] = a1 >> 16 & 0xFF;
	addressBytes[1] = a1 >> 8 & 0xFF;
	addressBytes[2] = a1 & 0xFF;
	
	if(port == 5055)
	{
		addressBytes[3] = a2 & 0xFF;
		addressBytes[4] = a2 >> 8 & 0xFF;
	}
	else if(port == 5069)
	{
		a2_byte = a2;
		a2_byte = bcdToByte(a2_byte);
		addressBytes[3] = a2_byte & 0xFF;
		addressBytes[4] = a2_byte >> 8;
	}
	else
	{
		addressBytes[3] = a2 >> 8 & 0xFF;
		addressBytes[4] = a2 & 0xFF;
	}
	
	return 0;
}

uint8_t data_fcb_sub(uint8_t *data,uint32_t dataSize,uint32_t fcbIndex,uint8_t bitIndex)
{
	uint8_t byte,fcb;
	
	byte = data[fcbIndex];
	fcb = byte >> bitIndex & 0x03;
	
	if(fcb <= 0)
		return 0;
	
	if(dataSize < 7)
		return 0;
	
	fcb -= 1;
	byte = byte & 0xCF;
	byte |= (fcb << bitIndex);

	data[fcbIndex] = byte;
	data[dataSize - 2] = crcCalc(data + 3,dataSize - 5);
	return fcb;
}

uint8_t EC20DataFcb(EC20_send_data_t *pack,uint32_t fcbIndex,uint8_t bitIndex)
{
	uint8_t data[128] = {0};
	
	uint16_t startIndex = 0;
	uint16_t index;
	uint16_t i;
	uint8_t fcb;
	
	for(i = 0;i < pack->_current_sector;i++)
	{
		startIndex += pack->_sector_size[i];
	}
	
	for(i = startIndex;i < startIndex + pack->_sector_size[pack->_current_sector];i++)
	{
		data[i - startIndex] = pack->_data[i];
	}
	
	fcb = data_fcb_sub(data,pack->_sector_size[pack->_current_sector],fcbIndex,bitIndex);
	
	for(i = startIndex;i < startIndex + pack->_sector_size[pack->_current_sector];i++)
	{
		pack->_data[i] = data[i - startIndex];
	}
	
	return fcb;
}

/*
函数描述：组建时间标签包
参数：包 包长度 秒 分 时 日 延迟
返回：成功：包的长度 失败：0
*/
uint8_t get_time_tag_pack(uint8_t *timeTags,uint8_t len,uint8_t dd,uint8_t hh,uint8_t mm,uint8_t ss,uint8_t delay)
{
	//时间标签顺序按照 秒 分 时 日 延迟
	timeTags[0] = 0;//按照省平台的协议 秒数固定为0
	timeTags[1] = ((mm / 10)) << 4 | (mm % 10);
	timeTags[2] = ((hh / 10)) << 4 | (hh % 10);
	timeTags[3] = ((dd / 10)) << 4 | (dd % 10);
	timeTags[4] = delay;
	return 5;
}

uint8_t create_waterLevel_data(uint8_t *data,uint8_t *dataLen,uint32_t ullage_mm)
{
	data[0] = ullage_mm % 10;
	data[1] = (ullage_mm /10) % 10;
	data[2] = (ullage_mm /100) % 10;
	data[3] = (ullage_mm /1000) % 10;
	*dataLen = 4;
	return 4;
}


/*
函数描述：组建符合水文协议的水文包
参数:data 外部传入指针，最终是赋值好的水文协议包
dataLen 外部传入长度指针 最终是水文包的长度值
packet 结构体 需要组建的值
返回：0 成功 其余 失败
*/
uint8_t make_hydrology_pack(unsigned char *data,uint32_t *dataLen,hydrology_packet_t *packet,uint16_t port)
{
	uint8_t res = 0;
	uint8_t UserDataLen = 0;
	uint8_t controlByte;
	int i;
	uint8_t j;
	uint8_t crcBytes;
	uint32_t allLength = 0;
	uint8_t addressBytes[5] = {0};
	uint8_t timeTag[5] = {0};
	unsigned char* pData;
	unsigned char header[3] = {0x68,0x00,0x68};//报文头 中间需要改为长度 
	*dataLen = 0;
	
	if(packet->_control_func != JXJS_CONTROL_FUNC_CMD)
	{
		UserDataLen = packet->_user_data_length + 1 + 5 + 1 + 5;//control1 + address5 + AFN1 + TP5
	}
	else
	{
		//确认帧不需要附加信息域
		UserDataLen = packet->_user_data_length + 1 + 5 + 1;//control1 + address5 + AFN1
	}
	
	header[1] = UserDataLen;
	//将头数据拷贝进去
	for(i = 0;i < 3;i++)
	{
		data[allLength++] = (unsigned char)header[i];
	}
	//封装控制域
	controlByte = 
	get_control_byte(packet->_control_dir,packet->_control_spilt,packet->_control_fcb,packet->_control_func);
	//拷贝控制域
	data[allLength++] = (unsigned char)controlByte;
	
	//封装地址域
	get_address_bytes(addressBytes,5,packet->_address_a1,packet->_address_a2,port);
	//拷贝地址域
	for(i = 0;i < 5;i++)
	{
		data[allLength++] = (unsigned char)addressBytes[i];
	}
	//拷贝应用层功能码
	data[allLength++] = (unsigned char)packet->_application_func;
	//拷贝应用层用户数据
	for(j = 0; j < packet->_user_data_length;j++)
	{
		data[allLength++] = (unsigned char)packet->_user_data[j];
	}
	
	
	if(packet->_control_func != JXJS_CONTROL_FUNC_CMD)//确认帧不需要附加信息
	{
		/*目前不需要pw
		//拷贝附加信息域中的pw
		data[allLength++] = packet._app_pw >> 8;
		data[allLength++] = packet._app_pw;
		*/
	
		//组建附加信息域时间标签		
		get_time_tag_pack(timeTag,5,packet->_app_day,packet->_app_hour,
		packet->_app_min,packet->_app_sec,packet->_app_delay);
		//拷贝时间标签
		for(i = 0; i < 5;i++)
		{
			data[allLength++] = (unsigned char)timeTag[i];
		}
	}
	
	
	//计算CRC校验
	pData = (data + 3);//crc 计算控制 + 地址 + 应用层 +3 跳过header
	crcBytes = crcCalc(pData,UserDataLen);//前面已经计算过crc计算所需数据长度
	//拷贝crc校验码
	data[allLength++] = (unsigned char)crcBytes;
	//拷贝结束符号
	data[allLength++] = 0x16;
	data[allLength] = 0;//考虑可能作为字符串直接传输 所以最后加上'\0'结尾
	*dataLen = allLength;
	return res;
}


/*
*十六进制的数组转化为字符串 --> data {0x01,0x02,0x03} -->string "010203"
*返回: 转化完成之后string的size  如果返回0 代表转换失败
*/
uint16_t hexdata2hexstring(uint8_t *data,uint32_t dataSize,unsigned char *string,uint32_t stringSize)
{
	uint16_t tmpSize;
	uint8_t i;
	char hexByte[3] = {0};
	
	if(dataSize * 2 >= stringSize)
		return 0;
	
	tmpSize = 0;
	for(i = 0;i < dataSize;i++)
	{
		sprintf(hexByte,"%02x",data[i]);
		string[tmpSize++] = hexByte[0];
		string[tmpSize++] = hexByte[1];
	}
	string[tmpSize] = 0;
	
	return tmpSize;
}

uint8_t create_flow_data(uint8_t *data,device_manager_t *manager,uint8_t data_type)
{
	uint8_t tmpsize = 0;
	uint8_t i,j;
	int32_t flow,flowRate;
	uint8_t flag;
	
	for(i = 0;i < manager->_device_count;i++)
	{
		flowRate = manager->_devices[i]._water_data._flowRate * 1000 * 3600;
		flow = manager->_devices[i]._water_data._flow;
		
		if(flow < 0)
			flow = 0;
		
		if(flowRate < 0)
		{
			flowRate *= -1;
			flag = 1;
		}
		else
		{
			flag = 0;
		}
		if(data_type == DATATYPE_FLOW_BEHIND)
		{//瞬时在前 累计在后
			for(j = 0;j < 5;j++)
			{
				data[tmpsize++] = byteToBcd(flowRate % 100);
				flowRate /= 100;
			}
			if(flag)
			{
				data[tmpsize] |= 0x80;
			}
		
			for(j = 0;j < 5;j++)
			{
				data[tmpsize++] = byteToBcd(flow % 100);
				flow /= 100;
			}
		}
		else if(data_type == DATATYPE_FLOW_IN_FRONT)
		{//累计在前 瞬时在后
			for(j = 0;j < 5;j++)
			{
				data[tmpsize++] = byteToBcd(flow % 100);
				flow /= 100;
			}
			
			for(j = 0;j < 5;j++)
			{
				data[tmpsize++] = byteToBcd(flowRate % 100);
				flowRate /= 100;
			}
			if(flag)
			{
				data[tmpsize] |= 0x80;
			}			
		}
		else
		{//其他数据类型
			for(j = 0;j < 5;j++)
			{
				data[tmpsize++] = byteToBcd(flowRate % 100);
				flowRate /= 100;
			}
			if(flag)
			{
				data[tmpsize] |= 0x80;
			}
		
			for(j = 0;j < 5;j++)
			{
				data[tmpsize++] = byteToBcd(flow % 100);
				flow /= 100;
			}
		}
		
	}
	data[tmpsize++] = 0x00;
	data[tmpsize++] = 0x40;
	data[tmpsize++] = 0x40;
	data[tmpsize++] = 0x00;
	
	return tmpsize;
}

static uint8_t createWaterLevelDataPro1(uint8_t *data,device_manager_t *manager,uint8_t pro)
{
	uint8_t tmpSize = 0;
	uint8_t i;
	uint8_t j;
	uint32_t water_level;
	uint8_t byte;
	uint8_t res = 0;
	
	for(i = 0;i < manager->_device_count;i++)
	{
		if(manager->_devices[i]._device_type == DEVICE_WATERLEVEL)
		{
			water_level = manager->_devices[i]._water_data._waterlevel * 1000;//毫米值
			for(j = 0;j < 4;j++)
			{
				byte = water_level % 100;
				data[tmpSize++] = byteToBcd(byte);
				water_level /= 100;
			}
			break;
		}
	}
	
	if(tmpSize == 0)
	{
		for(j = 0;j < 4;j++)
		{
			data[tmpSize++] = 0;
		}
	}
	
	data[tmpSize++] = 0x00;
	if(pro == 0x01)
	{
		data[tmpSize++] = 0x40;
		data[tmpSize++] = 0x40;
	}
	else if(pro == 0x02)
	{
		data[tmpSize++] = 0x00;
		data[tmpSize++] = 0x00;
	}
	else
	{
		data[tmpSize++] = 0x00;
		data[tmpSize++] = 0x00;
	}	
	data[tmpSize++] = 0x00;
	
	return tmpSize;
}

static uint8_t createFlowSpeedDataPro1(uint8_t *data,device_manager_t *manager,uint8_t pro)
{
	uint8_t tmpSize = 0;
	uint8_t i;
	uint8_t j;
	uint32_t flowSpeed;
	uint8_t byte;
	uint8_t res = 0;
	
	for(i = 0;i < manager->_device_count;i++)
	{
		if((manager->_devices[i]._device_type == DEVICE_WATERLEVEL && manager->_devices[i]._device_formula != FORMULA_NONE)
			|| manager->_devices[i]._device_type == DEVICE_FLOWSPEED)
		{
			flowSpeed = manager->_devices[i]._water_data._flowspeed * 1000;//毫米值
			for(j = 0;j < 3;j++)
			{
				byte = flowSpeed % 100;
				data[tmpSize++] = byteToBcd(byte);
				flowSpeed /= 100;
			}
			break;
		}
	}
	
	if(tmpSize == 0)
	{
		for(j = 0;j < 3;j++)
		{
			data[tmpSize++] = 0;
		}
	}
	
	data[tmpSize++] = 0x00;	
	if(pro == 0x01)
	{
		data[tmpSize++] = 0x40;
		data[tmpSize++] = 0x40;
	}
	else if(pro == 0x02)
	{
		data[tmpSize++] = 0x00;
		data[tmpSize++] = 0x00;
	}
	else
	{
		data[tmpSize++] = 0x00;
		data[tmpSize++] = 0x00;
	}
	data[tmpSize++] = 0x00;
	
	return tmpSize;
}

static uint8_t create_flow_data_new(uint8_t *data,device_manager_t *manager,uint8_t data_type)
{
	uint8_t tmpsize = 0;
	uint8_t i,j;
	int32_t byte_front;
	uint8_t flag;
	
	for(i = 0;i < manager->_device_count;i++)
	{

		byte_front = manager->_devices[i]._water_data._flowRate * 1000;

		if(byte_front < 0)
		{
			byte_front = 0;
		}

		for(j = 0;j < 5;j++)
		{
			data[tmpsize++] = byteToBcd(byte_front % 100);
			byte_front /= 100;
		}
		if(flag)
		{
			data[tmpsize] |= 0x80;
		}
	}
	data[tmpsize++] = 0x00;
	data[tmpsize++] = 0x00;
	data[tmpsize++] = 0x00;
	data[tmpsize++] = 0x00;
	
	return tmpsize;
}

uint8_t	create_user_data(uint8_t *data,uint8_t *size,uint8_t maxsize,device_manager_t manager,uint8_t func)
{
	if(func == JXJS_CONTROL_FUNC_FLOW)
	{
		//create_flow_data(data,size,maxsize,manager,func);
	}
	
	return 0;
}

void create_hydrology_packet(hydrology_packet_t *packet,jxjs_time_t *time,device_manager_t *manager,platform_manager_t *platform_manager,uint8_t func,uint8_t index)
{
	uint8_t data[128];
	uint8_t	dataSize,i;
	//平台编号
	packet->_index = index;
	//控制域
	packet->_control_dir = JXJS_CONTROL_DIR_UP;
	packet->_control_spilt = JXJS_CONTROL_NO_SPILT;
	packet->_control_fcb = 3;
	packet->_control_func = func;
	
	//地址域
	packet->_address_a1 = platform_manager->_platform_number_a1;
	packet->_address_a2 = platform_manager->_platform_number_a2;
	
	//用户数据域
	packet->_application_func = JXJS_APP_FUNC_TERMINAL_SELF_REPORT_DATA;
	//create_user_data(packet->_user_data,&(packet->_user_data_length),128,manager,func);
		
	if(func == JXJS_CONTROL_FUNC_FLOW)
	{
		dataSize = create_flow_data(data,manager,platform_manager->_platforms[index]._data_type);		
	}
	else
	{
		//这里可以做扩展
	}
	
	packet->_user_data_length = 0;
	for(i = 0;i < dataSize;i++)
	{
		packet->_user_data[packet->_user_data_length++] = data[i];
	}
	
	//附加信息域
	packet->_app_day = time->_day;
	packet->_app_hour = time->_hour;
	packet->_app_min = time->_min;
	packet->_app_sec = time->_sec;
	packet->_app_delay = 0;
}


uint16_t stringToUint16(uint8_t* data,uint16_t *index)
{
    uint16_t half_word = 0;
    uint8_t i;

    for(i = 0;i < 2;i++)
    {
        half_word = (half_word << 8) | data[*index];
        *index += 1;
    }
    return half_word;
}

uint32_t stringToUint32(uint8_t* data,uint16_t *index)
{
    uint32_t word = 0;
    uint8_t i;

    for(i = 0;i < 4;i++)
    {
        word = (word << 8) | data[*index];
        *index += 1;
    }
    return word;
}

//读byte_count字节的数组 赋值给uint32 并做下标移动
uint32_t stringToUint32ByBytes(uint8_t* data,uint16_t *index,uint8_t byte_count)
{
	uint32_t word = 0;
    uint8_t i;

    for(i = 0;i < byte_count;i++)
    {
        word = (word << 8) | data[*index];
        *index += 1;
    }
    return word;
}


void appendUint16ToData(uint8_t *data,uint16_t *index,uint16_t halfByte)
{
    uint8_t i;
    for(i = 0;i < 2;i++)
    {
        data[*index] = halfByte >> ((1 - i) * 8) & 0xFF;
        *index += 1;
    }
}

void appendUint32ToData(uint8_t *data,uint16_t *index,uint32_t byte)
{
    uint8_t i;
    for(i = 0;i < 4;i++)
    {
        data[*index] = byte >> ((3 - i) * 8) & 0xFF;
        *index += 1;
    }
}


static uint8_t createPackPro1(uint8_t *sector,platform_manager_t *pmanager,device_manager_t *dmanager,jxjs_time_t *time,uint8_t index,uint16_t port,uint8_t func)
{
	uint8_t sector_size = 0;
	uint8_t i;
	uint8_t header[] = {0x68,0x00,0x68};
	uint8_t address[8] = {0};
	uint8_t user_data[64];
	uint8_t user_data_length = 0;
	uint8_t *pdata;
	
	for(i = 0;i < 3;i++)
	{
		sector[sector_size++] = header[i];
	}
	//控制域
	sector[sector_size++] = 
	get_control_byte(JXJS_CONTROL_DIR_UP,JXJS_CONTROL_NO_SPILT,3,func);
	//地址域		
	get_address_bytes(address,5,pmanager->_platform_number_a1,pmanager->_platform_number_a2,port);
	for(i = 0;i < 5;i++)
	{
		sector[sector_size++] = address[i];
	}
	//应用层功能码
	sector[sector_size++] = JXJS_APP_FUNC_TERMINAL_SELF_REPORT_DATA;//自报数据
	//用户数据域
	if(func == JXJS_CONTROL_FUNC_FLOW)
	{
		if(pmanager->_platforms[index]._protocol == 0x01)
		{
			user_data_length = create_flow_data(user_data,dmanager,pmanager->_platforms[index]._data_type);
		}
		else if(pmanager->_platforms[index]._protocol == 0x02)
		{
			user_data_length = create_flow_data_new(user_data,dmanager,pmanager->_platforms[index]._data_type);
		}
		
	}
	else if(func == JXJS_CONTROL_FUNC_WATERLEVEL)
	{
		user_data_length = createWaterLevelDataPro1(user_data,dmanager,pmanager->_platforms[index]._protocol);
	}
	else if(func == JXJS_CONTROL_FUNC_FLOWSPEED)
	{
		user_data_length = createFlowSpeedDataPro1(user_data,dmanager,pmanager->_platforms[index]._protocol);
	}
	
	for(i = 0;i < user_data_length;i++)
	{
		sector[sector_size++] = user_data[i];
	}
	//重新给到长度位
	sector[1] = user_data_length + 1 + 5 + 1 + 5;//control1 + address5 + AFN1 + TP5
	//时间标签
	get_time_tag_pack(address,5,time->_day,time->_hour,time->_min,0,0);
	for(i = 0;i < 5;i++)
	{
		sector[sector_size++] = address[i];
	}
	//计算CRC校验
	pdata = (sector + 3);//crc 计算控制 + 地址 + 应用层 +3 跳过header
	i = crcCalc(pdata,sector[1]);//前面已经计算过crc计算所需数据长度
	sector[sector_size++] = i;
	sector[sector_size++] = 0x16;
	return sector_size;
}
/*
*创建水文协议包 所属协议 SZY_2016
*
*/
uint8_t craeteHygPackPro1(EC20_send_data_t *pack,platform_manager_t *pmanager,device_manager_t *dmanager,jxjs_time_t *time,uint8_t index)
{
	uint16_t dataSize = 0;
	uint8_t sector_size = 0;
	uint8_t sector[128] = {0};
	uint8_t i;
	uint16_t port;
	pack->_sector_num = 0;
	
	if(pmanager->_platforms[index]._address_type == ADDRESSTYPE_NORMAL)
	{
		port = 9999;	
	}
	else if(pmanager->_platforms[index]._address_type == ADDRESSTYPE_EXCHANGE)
	{
		port = 5055;
	}
	else if(pmanager->_platforms[index]._address_type == ADDRESSTYPE_CONVERT_FORMAT)
	{
		port = 5069;
	}
	else
	{
		port = 9999;
	}
	
	if(pmanager->_platforms[index]._send_data_type & JXJS_CONTROL_FUNC_FLOW_BIT)
	{
		pack->_sector_num += 1;
		//组建流量包
		sector_size = createPackPro1(sector,pmanager,dmanager,time,index,port,JXJS_CONTROL_FUNC_FLOW);
		//sector长度
		pack->_sector_size[pack->_sector_num -1] = sector_size;
		
		for(i = 0;i < sector_size;i++)
		{
			pack->_data[dataSize++] = sector[i];
		}
	}
	if(pmanager->_platforms[index]._send_data_type & JXJS_CONTROL_FUNC_WATERLEVEL_BIT)
	{
		pack->_sector_num += 1;
		//组建水位包
		sector_size = createPackPro1(sector,pmanager,dmanager,time,index,port,JXJS_CONTROL_FUNC_WATERLEVEL);
		//sector长度
		pack->_sector_size[pack->_sector_num -1] = sector_size;
		
		for(i = 0;i < sector_size;i++)
		{
			pack->_data[dataSize++] = sector[i];
		}
	}
	
	if(pmanager->_platforms[index]._send_data_type & JXJS_CONTROL_FUNC_FLOWSPEED_BIT)
	{
		pack->_sector_num += 1;
		//组建流速包
		sector_size = createPackPro1(sector,pmanager,dmanager,time,index,port,JXJS_CONTROL_FUNC_FLOWSPEED);
		//sector长度
		pack->_sector_size[pack->_sector_num -1] = sector_size;
		
		for(i = 0;i < sector_size;i++)
		{
			pack->_data[dataSize++] = sector[i];
		}
	}
	
	pack->_size = dataSize;
	pack->_current_sector = 0;
	
	return 1;
}
