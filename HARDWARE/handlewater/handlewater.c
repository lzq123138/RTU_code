#include "handlewater.h"
#include "rs485.h"

extern device_manager_t	device_manager;
extern uint8_t RS485Mode;
extern uint8_t RS485_RX_CNT;
extern device_uart_config_t	currentConfig;

extern rs485_state_t rs485State;

extern jxjs_time_t sys_time;

static uint8_t waterLevel_cmd1[] = {03,00,00,00,02};

static uint8_t flowRate_cmd1[] = {0x03,0x00,0x00,0x00,0x02};
static uint8_t flow_cmd1[] = {0x03,0x00,0x0B,0x00,0x04};

static uint8_t flowRate_cmd2[] = {0x03,0x00,0x00,0x00,0x02};
static uint8_t flow_cmd2[] = {0x03,0x00,0x08,0x00,0x04};

static uint8_t flowRate_cmd3[] = {0x03,0x00,0x00,0x00,0x02};
static uint8_t flow_cmd3[] = {0x03,0x00,0x08,0x00,0x04};

static uint8_t flowRate_cmd4[] = {0x03,0x07,0xD0,0x00,0x02};
static uint8_t flow_cmd4[] = {0x03,0x07,0xD6,0x00,0x04};

static uint8_t flowRate_cmd5[] = {0x03,0x00,0x00,0x00,0x02};
static uint8_t flow_cmd5[] = {0x03,0x00,0x03,0x00,0x04};

static uint8_t flowRate_cmd6[] = {0x03,0x00,0x04,0x00,0x02};
static uint8_t flow_cmd6[] = {0x03,0x00,0x08,0x00,0x03};

static uint8_t flow_cmd7[] = {0x04,0x00,0x00,0x00,0x14};

static uint8_t flow_cmd8[] = {0x03,0x00,0x5A,0x00,0x0A};

static uint8_t flow_cmd9[] = {0x04,0x10,0x10,0x00,0x12};

static uint8_t flow_cmdA[] = {0x03,0x00,0x00,0x00,0x04};

static uint8_t flowRate_cmdB[] = {0x03,0x02,0x52,0x00,0x02};
static uint8_t flow_cmdB[] = {0x03,0x03,0x08,0x00,0x04};

static uint8_t flowspeed_cmd1[] = {0x03,0x00,0x08,0x00,0x01};
static uint8_t flowspeed_byd[] = {0x03,0x00,0xA2,0x00,0x08};


static void handle_waterlevel_protocol1(uint8_t index)
{	
	if(index >= device_manager._device_count)
		return;
	
	Modbus_Send_cmd(waterLevel_cmd1,5,device_manager._devices[index]._device_number);
	rs485State._flag = 1;
	rs485State._cmd_time = sys_time._diff;	
}

static void handle_waterlevel_protocol2(uint8_t index)
{//模拟量 与485无关
	uint16_t ad_value;
	double airHeight;
	double real_ma;
	uint8_t ch = 8;
	
	if(index >= device_manager._device_count)
		return;
	
	if(device_manager._devices[index]._device_number == 1)
	{
		ch = 8;
	}
	else if(device_manager._devices[index]._device_number == 2)
	{
		ch = 9;
	}
	
	//取通道20次平均
	//ad_value = Get_Adc_Median_algorithm(ch);
	ad_value = Get_Adc_Median_algorithm(ch);
	
	if(ad_value == 0)
	{
		device_manager._devices[index]._device_com_state = 2;
		device_manager._devices[index]._water_data._flowspeed = 0;
		device_manager._devices[index]._water_data._flowRate = 0;
		device_manager._devices[index]._water_data._waterlevel = 0;
		device_manager._devices[index]._water_data._airHeight = 0;
	}
	else
	{
		device_manager._devices[index]._device_com_state = 1;
		//计算实际的ma值
		ad_value += 1;
		real_ma = (double)(ad_value * 3.3 * 10 / 4096);
		if(real_ma < 4.0 || real_ma > 20)
		{
			device_manager._devices[index]._water_data._airHeight = 0;
			device_manager._devices[index]._water_data._flowspeed = 0;
			device_manager._devices[index]._water_data._flowRate = 0;
			device_manager._devices[index]._water_data._waterlevel = 0;
		}
		else
		{			
			airHeight = (real_ma - 4.0) * (10 / 16.0);
			device_manager._devices[index]._water_data._airHeight = airHeight;
			device_manager._devices[index]._water_data._waterlevel = device_manager._devices[index]._water_param._insheight - airHeight;//device_manager._devices[index]._water_param._insheight - airHeight
			if(device_manager._devices[index]._water_data._waterlevel < 0)
				device_manager._devices[index]._water_data._waterlevel = 0;
			ch = Chezy_formula(device_manager._devices[index]._water_param,airHeight,&device_manager._devices[index]._water_data._flowspeed,
			&device_manager._devices[index]._water_data._flowRate);
			if(ch != 0)
			{
				device_manager._devices[index]._water_data._flowspeed = 0;
				device_manager._devices[index]._water_data._flowRate = 0;
			}
		}
						
	}
}


static void handle_water_flow_protocolOnce(uint8_t index,uint8_t protocol)
{
	uint8_t return_flag = 0;
	switch(protocol)
	{
		case FLOWMETER_PROTOCOL7:
			Modbus_Send_cmd(flow_cmd7,5,device_manager._devices[index]._device_number);
			break;
		case FLOWMETER_PROTOCOL8:
			Modbus_Send_cmd(flow_cmd8,5,device_manager._devices[index]._device_number);
			break;
		case FLOWMETER_JINYI:
			Modbus_Send_cmd(flow_cmd9,5,device_manager._devices[index]._device_number);
			break;
		case FLOWMETER_JDLY:
			Modbus_Send_cmd(flow_cmdA,5,device_manager._devices[index]._device_number);
			break;
		default:
			return_flag = 1;
			break;
			
	}
	if(return_flag == 1)
		return;
	rs485State._flag = 1;
	rs485State._cmd_time = sys_time._diff;	
}

static void handle_water_flow_protocol1and2(uint8_t index,uint8_t protocol)
{
	uint8_t error = 0;
	if((rs485State._stage & 0x01) == 0)
	{
		switch(protocol)
		{
			case FLOWMETER_PROTOCOL1:
				Modbus_Send_cmd(flowRate_cmd1,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL2:
				Modbus_Send_cmd(flowRate_cmd2,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL3:
				Modbus_Send_cmd(flowRate_cmd3,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL4:
				Modbus_Send_cmd(flowRate_cmd4,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL5:
				Modbus_Send_cmd(flowRate_cmd5,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL6:
				Modbus_Send_cmd(flowRate_cmd6,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_HZZH:
				Modbus_Send_cmd(flowRate_cmdB,5,device_manager._devices[index]._device_number);
				break;
			default:
				error = 1;
				break;
		}
	}
	else if((rs485State._stage & 0x02)  == 0)
	{
		switch(protocol)
		{
			case FLOWMETER_PROTOCOL1:
				Modbus_Send_cmd(flow_cmd1,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL2:
				Modbus_Send_cmd(flow_cmd2,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL3:
				Modbus_Send_cmd(flow_cmd3,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL4:
				Modbus_Send_cmd(flow_cmd4,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL5:
				Modbus_Send_cmd(flow_cmd5,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_PROTOCOL6:
				Modbus_Send_cmd(flow_cmd6,5,device_manager._devices[index]._device_number);
				break;
			case FLOWMETER_HZZH:
				Modbus_Send_cmd(flow_cmdB,5,device_manager._devices[index]._device_number);
				break;
			default:
				error = 1;
				break;
		}
	}
	else
		error = 1;
	
	if(error == 0)
	{
		rs485State._flag = 1;
		rs485State._cmd_time = sys_time._diff;
	}
		
}

static void handle_flowspeed_once(uint8_t index,uint8_t protocol)
{
	uint8_t return_flag = 0;
	switch(protocol)
	{
		case FLOWSPEED_MSYX1:
			Modbus_Send_cmd(flowspeed_cmd1,5,device_manager._devices[index]._device_number);
			break;
		case FLOWSPEED_BYD:
			Modbus_Send_cmd(flowspeed_byd,5,device_manager._devices[index]._device_number);
			break;
		default:
			return_flag = 1;
			break;
			
	}
	if(return_flag == 1)
		return;
	rs485State._flag = 1;
	rs485State._cmd_time = sys_time._diff;	
}

static void handle_waterlevel(uint8_t index)
{
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_protocol)
	{
		case WATERLEVEL_PROTOCOL1:
			handle_waterlevel_protocol1(index);
			break;
		case WATERLEVEL_PROTOCOL2:
			handle_waterlevel_protocol2(index);
			break;
		default:
			break;
	}
}

static void handle_flow(uint8_t index)
{
	//待补充
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_protocol)
	{
		case FLOWMETER_PROTOCOL1:
		case FLOWMETER_PROTOCOL2:
		case FLOWMETER_PROTOCOL3:
		case FLOWMETER_PROTOCOL4:
		case FLOWMETER_PROTOCOL5:
		case FLOWMETER_PROTOCOL6:
		case FLOWMETER_HZZH:
			handle_water_flow_protocol1and2(index,device_manager._devices[index]._device_protocol);
			break;
		case FLOWMETER_PROTOCOL7:
		case FLOWMETER_PROTOCOL8:
		case FLOWMETER_JINYI:
		case FLOWMETER_JDLY:
			handle_water_flow_protocolOnce(index,device_manager._devices[index]._device_protocol);
			break;
		default:
			break;
	}
}

static void handle_flowspeed(uint8_t index)
{
	//待补充
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_protocol)
	{
		case FLOWSPEED_MSYX1:
		case FLOWSPEED_BYD:
			handle_flowspeed_once(index,device_manager._devices[index]._device_protocol);
			break;
		default:
			break;
	}
}

void handle_water_data(uint8_t index)
{
	if(index >= device_manager._device_count)
		return;
	
	if(checkDeviceUartConfigIsEqual(&device_manager._devices[index]._uart_config,&currentConfig) == 0)
	{
		RS485Config(device_manager._devices[index]._uart_config._bound,device_manager._devices[index]._uart_config._wordlength,
		device_manager._devices[index]._uart_config._stopbit,device_manager._devices[index]._uart_config._parity);
	}
	
	//这里切换485口
	switchRS485Mode(1);
	switch(device_manager._devices[index]._device_type)
	{
		case DEVICE_WATERLEVEL:
			handle_waterlevel(index);
			break;
		case DEVICE_FLOWMETER:
			handle_flow(index);
			break;
		case DEVICE_FLOWSPEED:
			handle_flowspeed(index);
			break;
		default:
			break;
	}
	//这里切换回去
	switchRS485Mode(0);
}

static void readWaterLevelPro1(uint8_t *data,uint16_t size,uint8_t index)
{
	uint8_t ret;
	uint16_t ullage_mm,ullage_cm;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	ullage_cm = data[3] << 8 | data[4];
	
	if(data[2] == 0x04)
		ullage_mm = data[5] << 8 | data[6];


	if(ullage_mm != 0)
		device_manager._devices[index]._water_data._airHeight = ullage_mm / 1000.0;
	else
		device_manager._devices[index]._water_data._airHeight = ullage_cm / 100.0;
	device_manager._devices[index]._water_data._waterlevel = device_manager._devices[index]._water_param._insheight - device_manager._devices[index]._water_data._airHeight;
	
	if(device_manager._devices[index]._water_data._waterlevel < 0)
			device_manager._devices[index]._water_data._waterlevel = 0;
	if(device_manager._devices[index]._device_formula == FORMULA_XIECAI)
	{
		ret = Chezy_formula(device_manager._devices[index]._water_param,device_manager._devices[index]._water_data._airHeight,&device_manager._devices[index]._water_data._flowspeed,
		&device_manager._devices[index]._water_data._flowRate);	
		if(ret != 0)
		{
			device_manager._devices[index]._water_data._flowspeed = 0;
			device_manager._devices[index]._water_data._flowRate = 0;
		}
	}
	else
	{
		device_manager._devices[index]._water_data._flowspeed = 0;
		device_manager._devices[index]._water_data._flowRate = 0;
	}
	
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	device_manager._devices[index]._failed_num = 0;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
}

static void readWaterLevel(uint8_t *data,uint16_t size,uint8_t index)
{
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_protocol)
	{
		case WATERLEVEL_PROTOCOL1:
			readWaterLevelPro1(data,size,index);
			break;
		default:
			break;
	}
}

static void readFlowJingyi(uint8_t *data,uint16_t size,uint8_t index)
{
	uint32_t byte32 = 0;
	uint16_t byte16 = 0;
	float f1;
	uint8_t num[8] = {0};
	uint8_t i;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	if(size < 39)
		return;
	
	num[0] = data[3];
	num[1] = data[4];
	num[2] = data[5];
	num[3] = data[6];
		
	f1 = Hex2Float(num);
	device_manager._devices[index]._water_data._flowRate = f1 / 3600.0;
	
	num[0] = data[19];
	num[1] = data[20];
	num[2] = data[21];
	num[3] = data[22];	
	
	for(i = 0;i< 4;i++)
	{
		byte32 = (byte32 << 8) | num[i];
	}
	
	num[0] = data[23];
	num[1] = data[24];
	num[2] = data[25];
	num[3] = data[26];	
	
	f1 = Hex2Float(num);
	device_manager._devices[index]._water_data._flow = byte32 * 1.0 + f1;;
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
}

static void readFlowProHuaLong(uint8_t *data,uint16_t size,uint8_t index)
{
	uint32_t byte32 = 0;
	uint16_t byte16 = 0;
	float f1;
	uint8_t num[8] = {0};
	uint8_t i;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	if(size < 23)
		return;
	
	num[0] = data[3];
	num[1] = data[4];
	num[2] = data[5];
	num[3] = data[6];
		
	f1 = Hex2Float(num);
	device_manager._devices[index]._water_data._flow = f1;
	
	num[0] = data[19];
	num[1] = data[20];
	num[2] = data[21];
	num[3] = data[22];	
	
	f1 = Hex2Float(num);
	device_manager._devices[index]._water_data._flowRate = f1 / 3600.0;
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
}

static void readFlowPro7(uint8_t *data,uint16_t size,uint8_t index)
{
	uint32_t byte32 = 0;
	uint16_t byte16 = 0;
	float f1;
	uint8_t num[8] = {0};
	uint8_t i;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	if(size < 40)
		return;
	
	num[0] = data[5];
	num[1] = data[6];
	num[2] = data[3];
	num[3] = data[4];
		
	f1 = Hex2Float(num);
	device_manager._devices[index]._water_data._flowRate = f1 / 3600.0;
	
	num[0] = data[17];
	num[1] = data[18];
	num[2] = data[15];
	num[3] = data[16];
	
	for(i = 0;i < 4;i++)
	{
		byte32 = (byte32 << 8) | num[i];
	}
	
	byte16 = data[37];
	byte16 = num[0] << 8 | data[38];
	
	device_manager._devices[index]._water_data._flow = byte32 + (float)(byte16 * 0.001);
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
	
}

static void readFlowJianHeng(uint8_t *data,uint16_t size,uint8_t index)
{
	uint32_t byte32 = 0;
	uint16_t byte16 = 0;
	float f1;
	uint8_t num[8] = {0};
	uint8_t i;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	if(data[2] == 0x04)
	{
		num[0] = data[5];
		num[1] = data[6];
		num[2] = data[3];
		num[3] = data[4];
		
		f1 = Hex2Float(num);
		device_manager._devices[index]._water_data._flowRate = f1 / 3600.0;
		rs485State._stage |= 0x01;
	}
	else if(data[2] == 0x06)
	{
		num[0] = data[5];
		num[1] = data[6];
		num[2] = data[3];
		num[3] = data[4];

		for(i = 0;i < 4;i++)
		{
			byte32 = (byte32 << 8) | num[i];
		}
		
		byte16 = data[7];
		byte16 = (byte16 << 8) | data[8];
		
		switch(byte16)
		{
			case 0xFFFD:
				f1 = 0.001;
				break;
			case 0xFFFE:
				f1 = 0.01;
				break;
			case 0xFFFF:
				f1 = 0.1;
				break;
			case 0x0000:
				f1 = 1;
				break;
			case 0x0001:
				f1 = 10;
				break;
			case 0x0002:
				f1 = 100;
				break;
			case 0x0003:
				f1 = 1000;
				break;
			case 0x0004:
				f1 = 10000;
				break;
			default:
				f1 = 0;
		}
		
		device_manager._devices[index]._water_data._flow = byte32 * f1;
		rs485State._stage |= 0x02;
	}
	
	if((rs485State._stage & 0x01) == 0 || (rs485State._stage & 0x02) == 0)
	{
		handle_water_data(index);		
	}
	else
	{
		rs485State._flag = 0;
	}
	
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
}

static void readFlowMeterNormal(uint8_t *data,uint16_t size,uint8_t index,uint8_t reverseFlag)
{
	uint32_t byte32 = 0;
	float f1;
	uint8_t num[8] = {0};
	uint8_t i = 0;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	

	if(data[2] == 0x04)
	{//瞬时流量
		if(reverseFlag)
		{
			num[0] = data[5];
			num[1] = data[6];
			num[2] = data[3];
			num[3] = data[4];
		}
		else
		{
			num[0] = data[3];
			num[1] = data[4];
			num[2] = data[5];
			num[3] = data[6];
		}
				
		f1 = Hex2Float(num);
		//f1 = 3.468;
		device_manager._devices[index]._water_data._flowRate = f1 / 3600.0;
		rs485State._stage |= 0x01;
	}
	else if(data[2] == 0x08)
	{//累计流量
		//整数部分
		if(reverseFlag)
		{
			num[0] = data[5];
			num[1] = data[6];
			num[2] = data[3];
			num[3] = data[4];
		}
		else
		{
			num[0] = data[3];
			num[1] = data[4];
			num[2] = data[5];
			num[3] = data[6];
		}
		for(i = 0;i < 4;i++)
		{
			byte32 = (byte32 << 8) | num[i];
		}
		//小数部分
		if(reverseFlag)
		{
			num[0] = data[9];
			num[1] = data[10];
			num[2] = data[7];
			num[3] = data[8];
		}
		else
		{
			num[0] = data[7];
			num[1] = data[8];
			num[2] = data[9];
			num[3] = data[10];
		}
		
		f1 = Hex2Float(num);
		
		if(f1 >= 1.0 || f1 < -1.0)
			f1 = 0;
		
		device_manager._devices[index]._water_data._flow = byte32 * 1.0 + f1;
		rs485State._stage |= 0x02;
	}
	
	if((rs485State._stage & 0x01) == 0 || (rs485State._stage & 0x02) == 0)
	{
		handle_water_data(index);		
	}
	else
	{
		rs485State._flag = 0;
	}
	
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
}

static void readFlowHZZH(uint8_t *data,uint16_t size,uint8_t index)
{
	uint32_t byte32 = 0;
	float f1;
	uint8_t num[8] = {0};
	uint8_t i = 0;
	
	if(index >= device_manager._device_count)
		return;
	
	if(data[2] == 0x04)
	{
		num[0] = data[3];
		num[1] = data[4];
		num[2] = data[5];
		num[3] = data[6];
		
		f1 = Hex2Float(num);
		device_manager._devices[index]._water_data._flowRate = f1 / 3600.0;
		rs485State._stage |= 0x01;
		
	}
	else
	{
		byte32 = 0;
		
		for(i = 3;i < 7;i++)
		{
			byte32 = byte32 << 8 | data[i];
		}
		device_manager._devices[index]._water_data._flow = byte32 * 10000000;
		
		byte32 = 0;
		for(i = 7;i < 11;i++)
		{
			byte32 = byte32 << 8 | data[i];
		}
		device_manager._devices[index]._water_data._flow += byte32;
		rs485State._stage |= 0x02;
	}
	
	if((rs485State._stage & 0x01) == 0 || (rs485State._stage & 0x02) == 0)
	{
		handle_water_data(index);		
	}
	else
	{
		rs485State._flag = 0;
	}
	
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
}

static void readFlowJDLY(uint8_t *data,uint16_t size,uint8_t index)
{
	uint32_t byte32 = 0;
	uint8_t i;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	//累计流量
	for(i = 3;i < 7;i++)
	{
		byte32 = byte32 << 8 | data[i];
 	}
	device_manager._devices[index]._water_data._flow = byte32 * 0.001;
	
	//瞬时流量
	byte32 = 0;
	for(i = 7;i < 11;i++)
	{
		byte32 = byte32 << 8 | data[i];
	}
	device_manager._devices[index]._water_data._flowRate = (byte32 * 0.001) / 3600.0;
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
	device_manager._devices[index]._failed_num = 0;
}

static void readFlowMeter(uint8_t *data,uint16_t size,uint8_t index)
{
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_protocol)
	{
		case FLOWMETER_PROTOCOL1:
		case FLOWMETER_PROTOCOL2:
		case FLOWMETER_PROTOCOL3:
		case FLOWMETER_PROTOCOL4:
			readFlowMeterNormal(data,size,index,1);
			break;
		case FLOWMETER_PROTOCOL5:
			readFlowMeterNormal(data,size,index,0);
			break;
		case FLOWMETER_PROTOCOL6:
			readFlowJianHeng(data,size,index);
			break;
		case FLOWMETER_PROTOCOL7:
			readFlowPro7(data,size,index);
			break;
		case FLOWMETER_PROTOCOL8:
			readFlowProHuaLong(data,size,index);
			break;
		case FLOWMETER_JINYI:
			readFlowJingyi(data,size,index);
			break;
		case FLOWMETER_JDLY:
			readFlowJDLY(data,size,index);
			break;
		case FLOWMETER_HZZH:
			readFlowHZZH(data,size,index);
			break;
		default:
			break;
	}
	
	device_manager._devices[index]._water_data._airHeight = 0;
	device_manager._devices[index]._water_data._waterlevel = 0;
	device_manager._devices[index]._water_data._flowspeed = 0;
}


static void readFlowSpeedByd(uint8_t *data,uint16_t size,uint8_t index)
{
	uint8_t nums[8] = {0};
	uint8_t i = 0;
	float f1;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	if(size < 21)
		return;
	
	//waterlevel
	for(i = 3;i < 7;i++)
	{
		nums[i - 3] = data[i];
	}
	f1 = Hex2Float(nums);
	device_manager._devices[index]._water_data._waterlevel = f1;
	
	//flowspeed
	for(i = 7;i < 11;i++)
	{
		nums[i - 7] = data[i];
	}
	f1 = Hex2Float(nums);
	device_manager._devices[index]._water_data._flowspeed = f1;
	
	//flowrate
	for(i = 11;i < 15;i++)
	{
		nums[i - 11] = data[i];
	}
	f1 = Hex2Float(nums);
	device_manager._devices[index]._water_data._flowRate = f1;
	
	//flow
	for(i = 15;i < 19;i++)
	{
		nums[i - 15] = data[i];
	}
	f1 = Hex2Float(nums);
	device_manager._devices[index]._water_data._flow = f1;
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	device_manager._devices[index]._failed_num = 0;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
}

static void readFlowSpeedMsyx1(uint8_t *data,uint16_t size,uint8_t index)
{
	uint16_t byte16;
	
	if(index >= device_manager._device_count)
		return;
	
	if(CheckDataLegality(data,size,0xFF) == 0)
		return;
	
	byte16 = data[3] << 8 | data[4];
	
	device_manager._devices[index]._water_data._flowspeed = byte16 / 1000.0;
	
	rs485State._flag = 0;
	rs485State._cmd_time = sys_time._diff;
	device_manager._devices[index]._failed_num = 0;
	rs485State._last_res = 0;
	device_manager._devices[index]._device_com_state = 0x01;
}

static void readFlowSpeed(uint8_t *data,uint16_t size,uint8_t index)
{
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_protocol)
	{
		case FLOWSPEED_MSYX1:
			readFlowSpeedMsyx1(data,size,index);
			break;
		case FLOWSPEED_BYD:
			readFlowSpeedByd(data,size,index);
			break;
		default:
			break;
	}
}

void readWaterData(uint8_t *data,uint16_t size,uint8_t index)
{
	if(rs485State._flag)
	{
		switch(device_manager._devices[index]._device_type)
		{
			case DEVICE_WATERLEVEL:
				readWaterLevel(data,size,index);
				break;
			case DEVICE_FLOWMETER:
				readFlowMeter(data,size,index);
				break;
			case DEVICE_FLOWSPEED:
				readFlowSpeed(data,size,index);
				break;
			default:
				break;
		}
	}
}

static void failedWaterLevelPro1(uint8_t index)
{
	rs485State._failed_num += 1;
	
	if(rs485State._failed_num >= 3)
	{
		rs485State._last_res = 1;
		device_manager._devices[index]._device_com_state = 0x02;
		device_manager._devices[index]._water_data._airHeight = 0;
		device_manager._devices[index]._water_data._waterlevel = 0;
		device_manager._devices[index]._water_data._flowspeed = 0;
		device_manager._devices[index]._water_data._flowRate = 0;
		
		rs485State._flag = 0;
	}
	else
	{
		handle_water_data(index);
	}
}

static void failedWaterLevel(uint8_t index)
{
	switch(device_manager._devices[index]._device_protocol)
	{
		case WATERLEVEL_PROTOCOL1:
			failedWaterLevelPro1(index);
			break;
		default:
			break;
	}
}

static void failedFlowMeterReadOnce(uint8_t index)
{
	rs485State._failed_num += 1;
	
	if(rs485State._failed_num >= 3)
	{
		rs485State._last_res = 1;
		device_manager._devices[index]._device_com_state = 0x02;
		device_manager._devices[index]._water_data._airHeight = 0;
		device_manager._devices[index]._water_data._waterlevel = 0;
		device_manager._devices[index]._water_data._flowspeed = 0;
		device_manager._devices[index]._water_data._flowRate = 0;
		
		rs485State._flag = 0;
	}
	else
	{
		handle_water_data(index);
	}
}


static void failedFlowMeterNormal(uint8_t index)
{
	rs485State._failed_num += 1;
	
	if((rs485State._stage & 0x01) == 0)
	{
		if(rs485State._failed_num >= 3)
		{
			rs485State._last_res = 1;
			device_manager._devices[index]._device_com_state = 0x02;
			device_manager._devices[index]._water_data._airHeight = 0;
			device_manager._devices[index]._water_data._waterlevel = 0;
			device_manager._devices[index]._water_data._flowspeed = 0;
			device_manager._devices[index]._water_data._flowRate = 0;
			rs485State._stage |= 0x01;
			rs485State._failed_num = 0;
			rs485State._flag = 0;
		}
		else
		{
			handle_water_data(index);
		}
	}
	else if((rs485State._stage & 0x02) == 0)
	{
		if(rs485State._failed_num >= 3)
		{
			rs485State._last_res = 1;
			device_manager._devices[index]._device_com_state = 0x02;
			device_manager._devices[index]._water_data._airHeight = 0;
			device_manager._devices[index]._water_data._waterlevel = 0;
			device_manager._devices[index]._water_data._flowspeed = 0;
			device_manager._devices[index]._water_data._flowRate = 0;
			device_manager._devices[index]._water_data._flow = 0;
			rs485State._stage |= 0x02;
			rs485State._flag = 0;
		}
		else
		{
			handle_water_data(index);
		}
	}
	else
	{
		rs485State._flag = 0;
	}
}


static void failedFlowMeter(uint8_t index)
{
	switch(device_manager._devices[index]._device_protocol)
	{
		case FLOWMETER_PROTOCOL1:
		case FLOWMETER_PROTOCOL2:
		case FLOWMETER_PROTOCOL3:
		case FLOWMETER_PROTOCOL4:
		case FLOWMETER_PROTOCOL5:
		case FLOWMETER_PROTOCOL6:
		case FLOWMETER_HZZH:
			failedFlowMeterNormal(index);
			break;
		case FLOWMETER_PROTOCOL7:
		case FLOWMETER_PROTOCOL8:
		case FLOWMETER_JINYI:
		case FLOWMETER_JDLY:
			failedFlowMeterReadOnce(index);
			break;
		default:
			break;
	}
}

static void failedFlowSpeedOnce(uint8_t index)
{
	rs485State._failed_num += 1;
	
	if(rs485State._failed_num >= 3)
	{
		rs485State._last_res = 1;
		device_manager._devices[index]._device_com_state = 0x02;
		device_manager._devices[index]._water_data._airHeight = 0;
		device_manager._devices[index]._water_data._waterlevel = 0;
		device_manager._devices[index]._water_data._flowspeed = 0;
		device_manager._devices[index]._water_data._flowRate = 0;
		if(device_manager._devices[index]._device_protocol == FLOWSPEED_BYD)
			device_manager._devices[index]._water_data._flow = 0;
		
		rs485State._flag = 0;
	}
	else
	{
		handle_water_data(index);
	}
}

static void failedFlowSpeed(uint8_t index)
{
	switch(device_manager._devices[index]._device_protocol)
	{
		case FLOWSPEED_MSYX1:
		case FLOWSPEED_BYD:
			failedFlowSpeedOnce(index);
			break;
		default:
			break;
	}
}

void failedWaterData(uint8_t index)
{	
	rs485State._cmd_time = sys_time._diff;
	
	if(index >= device_manager._device_count)
		return;
	
	switch(device_manager._devices[index]._device_type)
	{
		case DEVICE_WATERLEVEL:
			failedWaterLevel(index);
			break;
		case DEVICE_FLOWMETER:
			failedFlowMeter(index);
			break;
		case DEVICE_FLOWSPEED:
			failedFlowSpeed(index);
		default:
			break;
	}
}