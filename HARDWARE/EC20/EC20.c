#include "ec20.h"
#include "stdlib.h"
#include "string.h"
#include "usart.h"	
#include "iwdg.h"	
#include "stdio.h"
#include "liuqueue.h"
#include "hc08.h"
#include "sdhandle.h"
#include "rs485.h"
#include "flash.h"

#define FLASH_UPDATE_FLAG_ADDR 0x0807FF00 //是否需要升级程序标志位

extern EC20_msg_t	EC20_msg;
char *strx,*extstrx,*Readystrx;
extern uint8_t RxBuffer[2048];
extern uint16_t Rxcouter;
extern jxjs_time_t 			sys_time;
extern platform_manager_t	platform_manager;
extern device_manager_t		device_manager;
extern hydrology_packet_t	hydrology_packet;
extern battery_data_t		battery_data;
extern uint8_t 				SDCardInitSuccessFlag;
extern uint8_t 				ProUpdateFlag;
extern uint64_t 			connectServerDiff;

EC20_send_data_t	EC20_Send_Data[6];

//uint8_t pro_update_check[32];
uint8_t pro_update_packet_num = 0;
uint8_t current_packet;
uint8_t updateSuccess = 0;

uint8_t send_xiaxie_stage = 0;
uint8_t rebootFlag = 0;

uint8_t xiaxieTimeTag[20];

extern uint8_t 	debugFlag;


void Clear_Buffer(void)//清空缓存
{
	uint16_t i;
	if(debugFlag == 1)
		RS485_Send_Data(RxBuffer,Rxcouter);
	for(i=0;i<Rxcouter;i++)
		RxBuffer[i]=0;//缓存
	Rxcouter=0;
	IWDG_Feed();//喂狗
}

void PWER_Init(void)
{
	 //GPIOE1初始化设置 作为EC20的开关机引脚
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;//开漏输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOE,&GPIO_InitStructure);//初始化
	
	PWRKEY = 1;
}

void EC20_Restart(void)
{
	uint8_t restart_num;
	PWRKEY = 1; //EC20关机
	delay_ms(50);
	PWRKEY = 0;//EC20开机
	delay_ms(10);
	restart_num = EC20_msg._restart_num + 1;
	clear_ec20_event();//清空所有的ec20任务
	EC20_Init();
	EC20_msg._restart_num = restart_num;
}

uint8_t EC20_pushCheck(void)
{
	msg_event_t event;
	event._msg_type = MSG_CHECK_EC20;
	return push_event(event);
}

void EC20_init_num(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
	EC20_msg._ec20_stage._failed_num = 0;
	EC20_msg._ec20_stage._max_failed_num = 50;
}

void EC20_success_num(void)
{
	//EC20_msg._csq2 = 1;
	EC20_msg._ec20_stage._state = EC20_STATE_FINISHED;
	EC20_msg._ec20_stage._res = EC20_RES_SUCCESS;
	EC20_msg._ec20_stage._failed_num = 0;
	EC20_msg._ec20_stage._time_num = 1;
}

void EC20_failed_num(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_FINISHED;
	EC20_msg._ec20_stage._res = EC20_RES_FAILED;
	EC20_msg._ec20_stage._failed_num += 1;
	if(EC20_msg._ec20_stage._max_failed_num >= EC20_msg._ec20_stage._failed_num)
	{
		if(EC20_msg._ec20_stage._failed_num <= 10)
			EC20_msg._ec20_stage._time_num = 1;
		else if(EC20_msg._ec20_stage._failed_num <= 50)
			EC20_msg._ec20_stage._time_num = 5;
		else
			EC20_msg._ec20_stage._time_num = 100;
	}
	else
	{
		EC20_msg._ec20_stage._time_num = 0;
	}
	
}

//return  0-->ok  1-->error
void EC20_at(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT\r\n");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parse_at(void)
{
	char *strres;
	strres = strstr((const char*)RxBuffer,(const char*)"OK");
	if(strres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_ate0(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("ATE0\r\n");//关闭回显
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_csq(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+CSQ\r\n");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parsecsq(void)
{
	char *strres;
	uint8_t csq1 = 0,csq2 = 0,num = 0,i,canadd = 0;
	strres = strstr((const char*)RxBuffer,(const char*)"+CSQ:");
	if(strres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
		for( i = 0;i < Rxcouter;i++)
		{
			if(RxBuffer[i] >= 0x30 && RxBuffer[i] <= 0x39)
			{
				canadd = 1;
				if(num == 0)
				{
					csq1 = csq1 * 10 + (RxBuffer[i] - 0x30);
				}
				else if(num == 1)
				{
					csq2 = csq2 * 10 + (RxBuffer[i] - 0x30);
				}			
			}
			else
			{
				if(canadd)
					num += 1;
				if(num > 1)
					break;
				canadd = 0;
			}
		}
		EC20_msg._csq1 = csq1;
		EC20_msg._csq2 = csq2;
	}
	
	Clear_Buffer();
	EC20_check();
}

void EC20_ati(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("ATI\r\n");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_cpin(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+CPIN?\r\n");//检查SIM卡是否在位,卡的缺口朝外放置 
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parsecpin(void)
{
	char *strres;
	strres = strstr((const char*)RxBuffer,(const char*)"+CPIN: READY");
	if(strres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_creg(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+CREG?\r\n");//查看是否注册GSM网络
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parsecreg(void)
{
	char *strres,*extstres;
	strres = strstr((const char*)RxBuffer,(const char*)"+CREG: 0,1");//返回正常
	extstres = strstr((const char*)RxBuffer,(const char*)"+CREG: 0,5");//返回正常，漫游
	if(strres == NULL && extstres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_cgreg(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+CGREG?\r\n");//查看是否注册GPRS网络
}

void EC20_parsecgreg(void)
{
	char *strres,*extstres;
	strres = strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,1");//返回正常
	extstres = strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,5");//返回正常，漫游
	if(strres == NULL && extstres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_atcops(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+COPS?\r\n");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_atqiclose(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QICLOSE=0\r\n");//关闭socket连接
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_atqicsgp(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QICSGP=1,1,\042CMNET\042,\042\042,\042\042,0\r\n");//接入APN，无用户名和密码
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parseatqicsgp(void)
{
	char *strres;
	strres = strstr((const char*)RxBuffer,(const char*)"OK");
	if(strres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_atqideact(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QIDEACT=1\r\n");//去激活
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parseatqideact(void)
{
	char *strres;
	strres = strstr((const char*)RxBuffer,(const char*)"OK");
	if(strres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_atqiact(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QIACT=1\r\n");//激活
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parseatqiact(void)
{
	char *strres;
	strres = strstr((const char*)RxBuffer,(const char*)"OK");
	if(strres == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		EC20_msg._ec20_haveinit = 1;//分配ip完成 即算初始化完成
		EC20_success_num();
	}
	Clear_Buffer();
	EC20_check();
}

void EC20_parsedefault()
{
	EC20_success_num();
	Clear_Buffer();
	EC20_check();
}

void EC20_ctzu(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+CTZU=1\r\n");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}
void EC20_cclk(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+CCLK?\r\n");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parsecclk(void)
{
	char *strres;
	uint8_t	flag = 0,num = 0,yy = 0,MM = 0,dd = 0,hh = 0,mm = 0,ss = 0,i = 0,canadd = 0;
	jxjs_time_t js_time;
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time1;
	strres = strstr((const char*)RxBuffer,(const char*)"+CCLK:");
	if(strres == NULL)
	{
		sys_time._timing_state = TIMING_FAILED;
		EC20_failed_num();
	}
	else
	{
		sys_time._timing_state = TIMING_SUCCESS;
		EC20_success_num();
		for(i = 0;i < Rxcouter;i++)
		{
			if(flag == 0)
			{
				if(RxBuffer[i] >= 0x30 && RxBuffer[i] <= 0x39)
				{
					flag = 1;
					num = 1;
					canadd = 1;
					yy  = yy * 10 + (RxBuffer[i] - 0x30);
				}
			}
			else
			{
				if(RxBuffer[i] >= 0x30 && RxBuffer[i] <= 0x39)
				{
					canadd = 1;
					switch(num)
					{
						case 1:
							yy  = yy * 10 + (RxBuffer[i] - 0x30);
							break;
						case 2:
							MM  = MM * 10 + (RxBuffer[i] - 0x30);
							break;
						case 3:
							dd  = dd * 10 + (RxBuffer[i] - 0x30);
							break;
						case 4:
							hh  = hh * 10 + (RxBuffer[i] - 0x30);
							break;
						case 5:
							mm  = mm * 10 + (RxBuffer[i] - 0x30);
							break;
						case 6:
							ss = ss * 10 + (RxBuffer[i] - 0x30);
							break;
						default:
							break;
					}									
				}
				else
				{
					if(canadd)
					{
						num += 1;
						if(num > 6)
							break;
						canadd = 0;
					}
				}				
			}
		}
		
		//开始校准rtc时钟
		js_time._year = yy;
		js_time._month = MM;
		js_time._day = dd;
		js_time._hour = hh;
		js_time._min = mm;
		js_time._sec = ss;		
		UTCToBeijing(&js_time);//格林威治转北京时间
		
		PWR_BackupAccessCmd(ENABLE);//使能备份寄存器操作		
		if(hh < 8)
		{
			//设置时间
			time1.RTC_Hours = js_time._hour;
			time1.RTC_Minutes = js_time._min;
			time1.RTC_Seconds = js_time._sec;
			time1.RTC_H12 = (js_time._hour >= 12) ? RTC_H12_PM:RTC_H12_AM;
			RTC_SetTime(RTC_Format_BIN,&time1);
			
			RTC_GetDate(RTC_Format_BIN,&date);
			//再设置日期
			if(date.RTC_Year != js_time._year || date.RTC_Month != js_time._month | date.RTC_Date != js_time._day  || date.RTC_WeekDay != js_time._weekday)
			{
				date.RTC_Year = js_time._year ;
				date.RTC_Month = js_time._month ;
				date.RTC_Date = js_time._day;  
				date.RTC_WeekDay = js_time._weekday;
				RTC_SetDate(RTC_Format_BIN,&date);
			}
		}
			
		PWR_BackupAccessCmd(DISABLE);
	}
	
	Clear_Buffer();
	EC20_check();
}

void EC20_qiopen(void)
{		
	uint8_t	index = EC20_msg._ec20_stage._index;
	jxjs_platform_t platform;
	if(index >= 8)
		index = 0;
	platform = platform_manager._platforms[index];
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QIOPEN=1,0,\"TCP\",\"%u.%u.%u.%u\",%u,0,1\r\n",platform._ip1,platform._ip2,platform._ip3,platform._ip4,platform._port);
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;//建立tcp连接 开始时间
}

void EC20_parse_qiopen()
{
	char *str_res;
	str_res=strstr((const char*)RxBuffer,(const char*)"+QIOPEN: 0,0");
	if(str_res == NULL)
	{
		str_res=strstr((const char*)RxBuffer,(const char*)"CONNECT");
		if(str_res == NULL)
		{
			EC20_failed_num();
		}
		else
		{
			EC20_success_num();
		}
	}
	else
	{
		str_res = strstr((const char*)RxBuffer,(const char*)"closed");
		if(str_res == NULL)
		{
			EC20_success_num();
		}
		else
		{
			EC20_failed_num();
		}
		
	}
	Clear_Buffer();
	EC20_check();
	
}

void EC20_send(void)
{
	uint8_t data[128] = {0};
	unsigned char string[257] = {0};
	uint32_t stringSize = 257;
	uint8_t res;
	uint8_t index = EC20_msg._ec20_event._index;	
	uint16_t startIndex,i;
	uint8_t current_sector = EC20_Send_Data[index]._current_sector;
	
	for(i = 0;i < EC20_Send_Data[index]._current_sector;i++)
	{
		startIndex += EC20_Send_Data[index]._sector_size[i];
	}
	
	for(i = startIndex;i < startIndex + EC20_Send_Data[index]._sector_size[current_sector];i++)
	{
		data[i - startIndex] = EC20_Send_Data[index]._data[i];
	}
	
	res = hexdata2hexstring(data,EC20_Send_Data[index]._sector_size[current_sector],string,stringSize);
	
	printf("AT+QISENDEX=0,\"%s\"\r\n",string);//选择十六进制方式发送数据
	delay_ms(10);
    USART_SendData(USART2, (uint8_t) 0x1a);//发送完成函数

	EC20_msg._ec20_stage._cmd_time = sys_time._diff;	
}

void EC20_parse_send(void)
{
	char *strres;
	strres = strstr((const char*)RxBuffer,(const char*)"OK");
	if(strres == NULL)
	{
		strres = strstr((const char*)RxBuffer,(const char*)"closed");
		if(strres == NULL)
		{
			EC20_failed_num();
		}
		else
		{
			EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
			EC20_init_num();
			EC20_msg._ec20_event._time_num = 2;
		}
		
	}
	else
	{
		EC20_success_num();
	}
	
	Clear_Buffer();
	EC20_check();
}

void EC20_parse_waitreply(void)
{
	char *strres;
	uint8_t i,flag1 = 0,flag2 = 0;
	uint8_t data[1024] = {0};
	uint8_t dataSize = 0;
	strres = strstr((const char*)RxBuffer,(const char*)"+QIURC");
	
	EC20_success_num();
	Clear_Buffer();
/*
	if(strres != NULL)
	{
		for(i = 0;i < Rxcouter;i++)
		{
			if(flag1 == 0)
			{
				if(i >= 5)
				{
					if(RxBuffer[i - 5] == '+' && RxBuffer[i - 4] == 'Q' && RxBuffer[i - 3] == 'I' &&
						RxBuffer[i - 2] == 'U' && RxBuffer[i - 1] == 'R' && RxBuffer[i] == 'C')
					{
						flag1 = 1;
					}
				}
			}
			else
			{
				if(flag2 == 0)
				{
					if(i >= 1 && RxBuffer[i - 1] == '\r' && RxBuffer[i] == '\n')
					{
						flag2 = 1;
					}
				}
				else
				{
					data[dataSize++] = RxBuffer[i];
				}
			}
		}
		
	}
	*/
}

uint16_t createUpdateData(uint8_t *data,uint8_t success,uint16_t dataSize)
{
	uint16_t crcByte;
	
	if(dataSize < 5)
		return 0;
	
	data[0] = 0x4A;
	data[1] = 0x53;
	data[2] = 0x03;
	data[3] = success;
	
	crcByte = modBusCRC16(data,4);
	
	data[4] = crcByte & 0xFF;
	data[5] = crcByte >> 8 & 0xFF;
	
	return 6;
}

uint16_t createRebootData(uint8_t *data,uint8_t success)
{
	uint16_t crcByte;
	
	data[0] = 0x4A;
	data[1] = 0x53;
	data[2] = 0x04;
	data[3] = success;
	
	crcByte = modBusCRC16(data,4);
	
	data[4] = crcByte & 0xFF;
	data[5] = crcByte >> 8 & 0xFF;
	
	return 6;
}

void EC20_send_server(uint8_t stage)
{	
	uint8_t data[2048] = {0};
	uint16_t dataSize = 0;
	uint16_t i;
/*
#define EC20_CONNECT	0x30
#define	EC20_SEND_DEVICE_PROTOCOL	0x31
#define EC20_SEND_DEVICE_REAL		0x32
#define EC20_SEND_PLATFORM			0x33
#define EC20_SEND_LOCAL				0x34
#define EC20_SEND_SUCCESS			0x35
#define EC20_SEND_FAILED			0x36
#define EC20_WAIT_SERVER			0x37	
*/
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	
	switch (stage)
	{
		case EC20_SEND_DEVICE_REAL:
			dataSize = createDeviceRealData(data);
			break;
		case EC20_SEND_PLATFORM:
			dataSize = createPlatformData(data);
			break;
		case EC20_SEND_LOCAL:
			dataSize = createRTUData(data);
			break;
		case EC20_SEND_SUCCESS:
			dataSize = createSuccessData(data);
			break;
		case EC20_SEND_FAILED:
			dataSize = createFailedData(data);
			break;
		case EC20_SEND_UP_SUCCESS:
			dataSize = createUpdateData(data,1,1024);
			break;
		case EC20_SEND_UP_FAILED:
			dataSize = createUpdateData(data,2,1024);
			break;
		case EC20_SEND_REBOOT_SUCCESS:
			dataSize = createRebootData(data,1);
			break;
		case EC20_SEND_REBOOT_FAILED:
			dataSize = createRebootData(data,2);
			break;
	}
	
	for(i = 0;i < dataSize;i++)
	{
		while((USART2->SR&0X40)==0);//等待发送完成
        USART2->DR = (uint8_t) data[i]; 
	}
	
	EC20_msg._ec20_stage._type = EC20_WAIT_SERVER;
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;	
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_Parse_wait_server(void)
{
	uint8_t res;
	uint32_t addr;
	uint8_t data[] = {0x4A,0x53,0x03,0x01};
	char *strres = NULL,*strres1 = NULL;
	
	if(ProUpdateFlag)
	{		
		res = CheckDataLegality(RxBuffer,Rxcouter,0xFF);
		
		if(res == 0)
		{
			EC20_send_server(EC20_SEND_UP_FAILED);
		}
		else
		{
			addr = RxBuffer[0] * 1021;
			writeUpdateFlash(addr,RxBuffer,Rxcouter - 3,1);
			current_packet += 1;			
			EC20_send_server(EC20_SEND_UP_SUCCESS);
			
			if(current_packet >= pro_update_packet_num)
			{
				writeFlash(FLASH_UPDATE_FLAG_ADDR,data,4);
			}
		}				
	}
	else
	{
		res = CheckDataLegality(RxBuffer,Rxcouter,0x00);
		
		if(res == 0)
		{
			EC20_send_server(EC20_SEND_FAILED);
		}
		else if(res == 1)
		{
			EC20_send_server(EC20_SEND_LOCAL);
		}
		else if(res == 2)
		{
			hc08_data_to_param(RxBuffer,0x02);
			//写入内存
			if(SDCardInitSuccessFlag)
			{
				SDWriteConfig(&device_manager,&platform_manager,&battery_data);
			}
			EC20_send_server(EC20_SEND_SUCCESS);
		}
		else if(res == 3)
		{
			
			ProUpdateFlag = 1;
			
			pro_update_packet_num = RxBuffer[3];
			current_packet = 0;
			updateSuccess = 0;
			
			eraseUpdateProSector();
			
			EC20_send_server(EC20_SEND_UP_SUCCESS);			
		}
		else if(res == 4)
		{
			rebootFlag = 1;
			EC20_send_server(EC20_SEND_REBOOT_SUCCESS);
		}
	}
	
	if(res == 0x00 || res == 0xFE)
	{
		strres = strstr((const char*)RxBuffer,(const char*)"NO CARRIER");
		strres1 = strstr((const char*)RxBuffer,(const char*)"closed");
		if(strres != NULL || strres1 != NULL)
		{//代表远程主动断开连接
			EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
			EC20_init_num();
			EC20_msg._ec20_event._time_num = 2;
		}
	}
	
	if(res != 0xFE)//代表非法数据
		EC20_msg._ec20_stage._cmd_time = sys_time._diff;//不是非法数据 就刷新时间
	
	Clear_Buffer();
}

void EC20_connect(void)
{
	//设备管理的平台 固定ip
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QIOPEN=1,0,\"TCP\",\"%u.%u.%u.%u\",%u,0,2\r\n",platform_manager._ipv4[0],platform_manager._ipv4[1],
	platform_manager._ipv4[2],platform_manager._ipv4[3],platform_manager._port);//\"47.109.100.97\",18080
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;//建立tcp连接 开始时间
}


void EC20_quit_trans(void)
{
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("+++");
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

void EC20_parse_quit_trans(void)
{
	EC20_success_num();
	Clear_Buffer();
}

void EC20ConnectXiaXie(void)
{
	uint8_t index = EC20_msg._ec20_event._index;
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	printf("AT+QIOPEN=1,0,\"TCP\",\"%u.%u.%u.%u\",%u,0,2\r\n",platform_manager._platforms[index]._ip1,platform_manager._platforms[index]._ip2,
	platform_manager._platforms[index]._ip3,platform_manager._platforms[index]._ip4,platform_manager._platforms[index]._port);//\"47.109.100.97\",18080
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;//建立tcp连接 开始时间
}

uint8_t createXiaXieLoginData(uint8_t *data)
{
	uint8_t address[8] = {0};
	uint8_t header[] = {0x68,0x08,0x68,0xB0};
	uint8_t i;
	uint8_t index = 0;
	
	for(i = 0;i < 4;i++)
	{
		data[index++] = header[i];
	}
	get_address_bytes(address,5,platform_manager._platform_number_a1,platform_manager._platform_number_a2,9999);
	for(i = 0;i <  5;i++)
	{
		data[index++] = address[i];
	}
	data[index++] = 0x02;
	data[index++] = 0xF0;
	
	i = crcCalc(data + 3,8);
	data[index++] = i;
	data[index++] = 0x16;
	
	return index;
}

uint8_t createXiaXieKeepData(uint8_t *data)
{
	uint8_t address[8] = {0};
	uint8_t header[] = {0x68,0x08,0x68,0xB0};
	uint8_t i;
	uint8_t index = 0;
	
	for(i = 0;i < 4;i++)
	{
		data[index++] = header[i];
	}
	get_address_bytes(address,5,platform_manager._platform_number_a1,platform_manager._platform_number_a2,9999);
	for(i = 0;i <  5;i++)
	{
		data[index++] = address[i];
	}
	data[index++] = 0x02;
	data[index++] = 0xF2;
	
	i = crcCalc(data + 3,8);
	data[index++] = i;
	data[index++] = 0x16;
	
	return index;
}

uint8_t createXiaXieTimeData(uint8_t *data)
{
	uint8_t address[8] = {0};
	uint8_t header[] = {0x68,0x14,0x68,0xB0};
	uint8_t i;
	uint8_t index = 0;
	uint8_t byte;
	
	//头数据
	for(i = 0;i < 4;i++)
	{
		data[index++] = header[i];
	}
	//站点编号
	get_address_bytes(address,5,platform_manager._platform_number_a1,platform_manager._platform_number_a2,9999);
	for(i = 0;i <  5;i++)
	{
		data[index++] = address[i];
	}
	//功能吗
	data[index++] = 0x11;
	//数据域
	byte = sys_time._weekday;
	byte <<= 5;
	byte |= (sys_time._month / 10) << 4;
	byte |= sys_time._month % 10;
	get_time_tag_pack(address,5,sys_time._day,sys_time._hour,sys_time._min,sys_time._sec,byte);
	for(i = 0;i < 5;i++)
	{
		data[index++] = address[i];
	}
	data[index++] = byteToBcd(sys_time._year - 2000);
	//附加域
	data[index++] = 0x50;
	data[index++] = 0x80;
	get_time_tag_pack(address,5,sys_time._day,sys_time._hour,sys_time._min,sys_time._sec,0);
	for(i = 0;i < 5;i++)
	{
		data[index++] = address[i];
	}
	//crc
	i = crcCalc(data + 3,20);
	data[index++] = i;
	//end
	data[index++] = 0x16;
	
	return index;
}

void EC20SendXiaXieData(uint8_t stage)
{
	uint8_t data[128] = {0};
	uint8_t dataSize = 0;
	uint16_t i;
	uint8_t index = EC20_msg._ec20_event._index;
	uint16_t startIndex = 0;
	uint8_t return_flag = 0;
	
	send_xiaxie_stage |= stage;
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
	
	switch(stage)
	{
		case 0x01:
			dataSize = createXiaXieLoginData(data);
			break;
		case 0x02:
			dataSize = createXiaXieKeepData(data);
			break;
		case 0x04:
			dataSize = createXiaXieTimeData(data);
			break;
		case 0x08:
		{
			
			if(EC20_Send_Data[index]._current_sector >= EC20_Send_Data[index]._sector_num)
			{
				return_flag = 1;
			}
			
			if(return_flag == 0)
			{
				dataSize = EC20_Send_Data[index]._sector_size[EC20_Send_Data[index]._current_sector];
				
				for(i = 0;i < EC20_Send_Data[index]._current_sector;i++)
				{
					startIndex += EC20_Send_Data[index]._sector_size[i];
				}
			
				for(i = startIndex;i < startIndex + dataSize;i++)
				{
					data[i - startIndex] = EC20_Send_Data[index]._data[i];
				}
			}
			
		}
			break;
		default:
			return_flag = 1;
			break;
	}
	
	if(return_flag == 1)
		return;
	
	delay_ms(100);
	
	for(i = 0;i < dataSize;i++)
	{
		while((USART2->SR&0X40)==0);//等待发送完成
        USART2->DR = (uint8_t) data[i]; 
	}
	
	EC20_msg._ec20_stage._type = EC20_WAIT_XIAXIE;
	EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;	
	EC20_msg._ec20_stage._cmd_time = sys_time._diff;
}

static uint8_t checkXiaxieDataType(uint16_t startIndex)
{
	uint8_t flag = 0;
	
	if(RxBuffer[startIndex] == 0x68)
	{
		if(RxBuffer[startIndex + 9] == 0x02)
		{
			if(RxBuffer[startIndex + 10] == 0xF0)
				flag = 0x01;
			else if(RxBuffer[startIndex + 10] == 0xF2)
				flag = 0x02;
		}
		else if(RxBuffer[startIndex + 9] == 0x11)
		{//校时
			flag = 0x04;
		}
		else if(RxBuffer[startIndex + 9] == 0xC0)
		{
			flag = 0x08;
		}
	}
	return flag;
}

void EC20ParseWaitXiaXie(void)
{
	uint8_t flag = 0;
	uint8_t res = 0,i = 0,checkNum = 1;
	uint16_t startIndex = 0;
	uint8_t index = EC20_msg._ec20_event._index;
	char *str_res,*strres1 = NULL;
		
	if(Rxcouter <= 10)
	{
		Clear_Buffer();
		return;
	}
	
	if(RxBuffer[0] == 0x68)
	{
		if(Rxcouter > 23)
		{
			startIndex = RxBuffer[1] + 5;
			checkNum = 2;
		}
			
		
		for(i = 0;i < checkNum;i++)
		{
			res = checkXiaxieDataType(i * startIndex);
			
			if(res == 0x08)
			{
				EC20_Send_Data[index]._current_sector += 1;
		
				if(EC20_Send_Data[index]._current_sector >= EC20_Send_Data[index]._sector_num)
				{
					res = 0;
				}
			}
			
			flag |= res;
		}
		
	}

	str_res = strstr((const char*)RxBuffer,(const char*)"closed");
	strres1 = strstr((const char*)RxBuffer,(const char*)"NO CARRIER");
	if(str_res != NULL|| strres1 != NULL)
	{//代表远程主动断开连接 数据可能没有发上去 需要重新发送
		EC20_Send_Data[index]._current_sector = 0;
		if(SDCardInitSuccessFlag)
			SDPushHistoryData(&EC20_Send_Data[index],index);
		
		EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
		EC20_init_num();
		EC20_msg._ec20_event._time_num = 2;
	}
		
	Clear_Buffer();
	
	for(i = 1;i < 16;i = i * 2)
	{
		if(flag & i)
		{
			EC20SendXiaXieData(i);
		}
	}
}

void EC20ParseConnectXiaXie(void)
{
	char *str_res;
	uint8_t index = EC20_msg._ec20_event._index;
	uint8_t flag = 0;
	str_res=strstr((const char*)RxBuffer,(const char*)"CONNECT");
	if(str_res == NULL)
	{
		EC20_failed_num();
	}
	else
	{
		str_res = strstr((const char*)RxBuffer,(const char*)"closed");
		if(str_res == NULL)
		{
			if(SDCardInitSuccessFlag)
				SDPopHistoryData(&EC20_Send_Data[index],index);
			
			if(RxBuffer[Rxcouter -1] == 0x16)			
				flag = 1;

			send_xiaxie_stage = 0x00;
			platform_manager._platforms[index]._connect_state = 0x01;
			platform_manager._platforms[index]._cont_failed_num = 0;
			EC20_msg._ec20_stage._type = EC20_WAIT_XIAXIE;
			EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;	
			EC20_msg._ec20_stage._cmd_time = sys_time._diff;
		}
		else
		{
			EC20_failed_num();
		}
		
	}
	
	Clear_Buffer();
	
	if(flag)
		EC20SendXiaXieData(0x01);
	EC20_check();
}

void EC20_do(void)
{
	switch (EC20_msg._ec20_stage._type)
	{
		case EC20_NULL:
			EC20_at();
			break;
		case EC20_AT:
			EC20_at();
			break;
		case EC20_ATE0:
			EC20_ate0();
			break;
		case EC20_ATCSQ:
			EC20_csq();
			break;
		case EC20_ATI:
			EC20_ati();
			break;
		case EC20_ATZU:
			EC20_ctzu();
			break;
		case EC20_ATCPIN:
			EC20_cpin();
			break;
		case EC20_ATCREG:
			EC20_creg();
			break;
		case EC20_ATCGREG:
			EC20_cgreg();
			break;
		case EC20_ATCOPS:
			EC20_atcops();
			break;
		case EC20_ATQICLOSE:
		case EC20_DISCONNECT:
			EC20_atqiclose();
			break;
		case EC20_ATQICSGP:
			EC20_atqicsgp();
			break;
		case EC20_ATQIDEACT:
			EC20_atqideact();
			break;
		case EC20_ATQIACT:
			EC20_atqiact();
			break;
		case EC20_QIOPEN:
			EC20_qiopen();
			break;
		case EC20_ATCCLK:
			EC20_cclk();
			break;
		case EC20_QISEND:
			EC20_send();
			break;
		case EC20_CONNECT:
			EC20_connect();
			break;
		case EC20_SEND_DEVICE_REAL:
		case EC20_SEND_PLATFORM:
		case EC20_SEND_LOCAL:
		case EC20_SEND_SUCCESS:
		case EC20_SEND_FAILED:
		case EC20_SEND_UP_FAILED:
		case EC20_SEND_UP_SUCCESS:
			EC20_send_server(EC20_msg._ec20_stage._type);
			break;
		case EC20_QUIT_TRANS:
			EC20_quit_trans();
			break;
		case EC20_CONNECT_XIAXIE:
			EC20ConnectXiaXie();
			break;
		default:
			break;			
	}
}

void EC20_check_init(void)
{
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START)
	{
		EC20_msg._ec20_stage._type = EC20_AT;
		EC20_init_num();
		EC20_do();
		EC20_msg._ec20_event._state = EC20_STATE_IN_PROGRESS;
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_IN_PROGRESS)
	{
		if(EC20_msg._ec20_stage._state == EC20_STATE_WAIT_START)
		{
			EC20_init_num();
			EC20_do();
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_FINISHED)
		{
			if(EC20_msg._ec20_stage._res == EC20_RES_SUCCESS)
			{
				if(EC20_msg._ec20_stage._type == EC20_ATQIACT)
				{
					EC20_msg._ec20_haveinit = 1;
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
				}
				else
				{
					EC20_msg._ec20_stage._type += 1;
					EC20_init_num();
					EC20_do();
				}
			}
			else if(EC20_msg._ec20_stage._res == EC20_RES_FAILED)
			{
				if(EC20_msg._ec20_stage._max_failed_num >= EC20_msg._ec20_stage._failed_num)
				{
					if(EC20_msg._ec20_stage._time_num <= 0)
						EC20_do();
					else
						EC20_msg._ec20_stage._time_num -= 1;
				}
				else
				{
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_FAILED;
				}
			}
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_IN_PROGRESS)
		{
			if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 10)
			{
				if(EC20_msg._ec20_stage._failed_num <= 10)
				{
					EC20_failed_num();
					EC20_do();
				}
				else
				{
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_FAILED;
					EC20_msg._ec20_stage._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_stage._res = EC20_RES_FAILED;
				}
			}
		}
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(EC20_msg._ec20_event._res == EC20_RES_SUCCESS)
		{
			//do nothing
		}
		else if(EC20_msg._ec20_event._res == EC20_RES_FAILED)
		{
			if(EC20_msg._restart_num <= 1)
			{//尝试重启EC20模块2次
				EC20_Restart();
				EC20_msg._restart_num += 1;
			}
			else
			{//证明并不是EC20的问题 直接重新走初始化流程
				EC20_Init();
			}
		}
	}
}

void EC20_check_csq(void)
{
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START)
	{
		EC20_msg._ec20_stage._type = EC20_ATCSQ;
		EC20_init_num();
		EC20_do();
		EC20_msg._ec20_event._state = EC20_STATE_IN_PROGRESS;
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_IN_PROGRESS)
	{
		if(EC20_msg._ec20_stage._state == EC20_STATE_WAIT_START)
		{
			//EC20_msg._ec20_stage._type = EC20_ATCSQ;
			EC20_init_num();
			EC20_do();
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_FINISHED)
		{
			if(EC20_msg._ec20_stage._res == EC20_RES_SUCCESS)
			{
				EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
				EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
			}
			else if(EC20_msg._ec20_stage._res == EC20_RES_FAILED)
			{
				if(EC20_msg._ec20_stage._failed_num <= 5)
				{
					if(EC20_msg._ec20_stage._time_num <= 0)
						EC20_do();
					else
						EC20_msg._ec20_stage._time_num -= 1;
				}
				else
				{
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_FAILED;
				}
			}
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_IN_PROGRESS)
		{
			if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 10)
			{
				EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
				EC20_msg._ec20_event._res = EC20_RES_FAILED;
				EC20_msg._ec20_stage._state = EC20_STATE_FINISHED;
				EC20_msg._ec20_stage._res = EC20_RES_FAILED;
			}
		}
	}
	
	else if(EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(EC20_msg._ec20_event._res == EC20_RES_SUCCESS)
		{
			//do nothing
		}
		else if(EC20_msg._ec20_event._res == EC20_RES_FAILED)
		{
			//do nothing
		}
	}
}

void EC20_check_cclk(void)
{
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START)
	{
		EC20_msg._ec20_stage._type = EC20_ATCCLK;
		EC20_init_num();
		EC20_do();
		EC20_msg._ec20_event._state = EC20_STATE_IN_PROGRESS;
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_IN_PROGRESS)
	{
		if(EC20_msg._ec20_stage._state == EC20_STATE_WAIT_START)
		{
			EC20_init_num();
			EC20_do();
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_FINISHED)
		{
			if(EC20_msg._ec20_stage._res == EC20_RES_SUCCESS)
			{
				EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
				EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
			}
			else if(EC20_msg._ec20_stage._res == EC20_RES_FAILED)
			{
				if(EC20_msg._ec20_stage._max_failed_num >= EC20_msg._ec20_stage._failed_num)
				{
					if(EC20_msg._ec20_stage._time_num <= 0)
						EC20_do();
					else
						EC20_msg._ec20_stage._time_num -= 1;
				}
				else
				{
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_FAILED;
				}
			}
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_IN_PROGRESS)
		{
			if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 10)
			{
				EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
				EC20_msg._ec20_event._res = EC20_RES_FAILED;
				EC20_msg._ec20_stage._state = EC20_STATE_FINISHED;
				EC20_msg._ec20_stage._res = EC20_RES_FAILED;
			}
		}
	}
	
	else if(EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(EC20_msg._ec20_event._res == EC20_RES_SUCCESS)
		{
			//do nothing
		}
		else if(EC20_msg._ec20_event._res == EC20_RES_FAILED)
		{
			//do nothing
		}
	}
}


static void EC20_close_event()
{
	EC20_msg._ec20_event._time_num = 2;
	EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
	EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
	EC20_do();
}


void EC20_check_sendmsg(void)
{
	uint8_t fcb;
	uint8_t index = EC20_msg._ec20_event._index;
	
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START)
	{
		EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
		EC20_msg._ec20_event._state = EC20_STATE_IN_PROGRESS;
		EC20_init_num();
		EC20_msg._ec20_stage._max_failed_num = 10;
		EC20_msg._ec20_event._time_num = 1;//在该事件中 event里的time_num闲置,复用为第几次qiclose的计数
		EC20_do();
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_IN_PROGRESS)
	{
		if(EC20_msg._ec20_stage._state == EC20_STATE_WAIT_START)
		{
			EC20_init_num();
			EC20_msg._ec20_stage._max_failed_num = 10;
			EC20_do();
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_FINISHED)
		{
			if(EC20_msg._ec20_stage._res == EC20_RES_SUCCESS)
			{
				if(EC20_msg._ec20_stage._type == EC20_ATQICLOSE)
				{
					if(EC20_msg._ec20_event._time_num == 1)
					{
						EC20_msg._ec20_stage._type = EC20_QIOPEN;
						EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
						EC20_do();
					}
					else
					{
						EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
						EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
					}
				}
				else if(EC20_msg._ec20_stage._type == EC20_QIOPEN)
				{
					platform_manager._platforms[EC20_msg._ec20_event._index]._connect_state = 0x01;
					platform_manager._platforms[index]._cont_failed_num = 0;
					if(SDCardInitSuccessFlag)
					{
						SDPopHistoryData(&EC20_Send_Data[index],index);
					}
					EC20_msg._ec20_stage._type = EC20_QISEND;
					EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
					EC20_do();				
					
				}
				else if(EC20_msg._ec20_stage._type == EC20_QISEND)
				{
					EC20_msg._ec20_stage._cmd_time = sys_time._diff;
					EC20_msg._ec20_stage._type = EC20_QIWAITREPLY;
					EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;
				}
				else if(EC20_msg._ec20_stage._type == EC20_QIWAITREPLY)
				{
					//EC20_close_event();
					EC20_Send_Data[index]._current_sector += 1;
					if(EC20_Send_Data[index]._current_sector >= EC20_Send_Data[index]._sector_num)
					{
						EC20_close_event();
					}
					else
					{
						EC20_msg._ec20_stage._type = EC20_QISEND;
						EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
						EC20_do();
					}
				}
			}
			else if(EC20_msg._ec20_stage._res == EC20_RES_FAILED)
			{
				if(EC20_msg._ec20_stage._max_failed_num >= EC20_msg._ec20_stage._failed_num)
				{
					if(EC20_msg._ec20_stage._time_num <= 0)
					{
						EC20_do();
					}						
					else
					{
						EC20_msg._ec20_stage._time_num -= 1;
					}						
				}
				else
				{
					//等待修改
					EC20_close_event();
					platform_manager._platforms[index]._connect_state = 0x02;
					if(platform_manager._platforms[index]._cont_failed_num < 5)
					{
						platform_manager._platforms[index]._cont_failed_num += 1;
					}
					//EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					//EC20_msg._ec20_event._res = EC20_RES_FAILED;
				}
			}
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_IN_PROGRESS)
		{
			if(EC20_msg._ec20_stage._type == EC20_QIOPEN)
			{
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 20)
				{
					//可能还需要扩展 这里表示socket连接错误 之后这里可以加上储存历史数据的逻辑
					EC20_close_event();
					platform_manager._platforms[index]._connect_state = 0x02;
					if(platform_manager._platforms[index]._cont_failed_num < 5)
					{
						platform_manager._platforms[index]._cont_failed_num += 1;
					}
				}
			}
			else if(EC20_msg._ec20_stage._type == EC20_QIWAITREPLY)
			{
				if(platform_manager._platforms[index]._mode == RESPONSE_MODE)
				{
					if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 10)
					{
						fcb = EC20DataFcb(&EC20_Send_Data[index],3,4);
						if(fcb > 0)
						{
							
							EC20_msg._ec20_stage._type = EC20_QISEND;
							EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
							EC20_do();
						}
						else
						{		
							EC20_close_event();				
						}
					
					}
				}
				else
				{//不应答模式 等待发送完成1s后 再close 防止close发送太快导致指令失效
					if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 1)
					{
						EC20_Send_Data[index]._current_sector += 1;
						if(EC20_Send_Data[index]._current_sector >= EC20_Send_Data[index]._sector_num)
						{
							EC20_close_event();
						}
						else
						{	
							EC20_msg._ec20_stage._type = EC20_QISEND;
							EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
							EC20_do();
						}
					}
				}
				
			}
		}
	}	
	else if(EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(EC20_msg._ec20_event._res == EC20_RES_SUCCESS)
		{
			//do nothing
		}
		else if(EC20_msg._ec20_event._res == EC20_RES_FAILED)
		{
			//do nothing
		}
	}	
}

void EC20_check_updateData(void)
{
	msg_event_t event;
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START)
	{
		EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
		EC20_msg._ec20_event._state = EC20_STATE_IN_PROGRESS;
		EC20_msg._ec20_event._time_num = 1;
		EC20_init_num();
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_IN_PROGRESS)
	{
		if(EC20_msg._ec20_stage._state == EC20_STATE_WAIT_START)
		{
			EC20_init_num();
			EC20_do();
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_FINISHED)
		{
			if(EC20_msg._ec20_stage._res == EC20_RES_SUCCESS)
			{
				if(EC20_msg._ec20_stage._type == EC20_ATQICLOSE)
				{					
					if(EC20_msg._ec20_event._time_num == 1)
					{
						connectServerDiff = sys_time._diff;//记录连接上后台的时间，只要有连接动作 就不算卡住
						EC20_msg._ec20_stage._type = EC20_CONNECT;
						EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
						EC20_do();
					}
					else
					{
						EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
						EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
						if(ProUpdateFlag == 1 || rebootFlag == 1)
						{
							event._msg_type = MSG_REBOOT;
							push_event(event);
						}
					}
				}
				else if(EC20_msg._ec20_stage._type== EC20_CONNECT)
				{
					//在这里需要修改为等待服务器发命令
					if(EC20_msg._ec20_stage._time_num <= 0)
					{
						EC20_msg._ec20_stage._type = EC20_SEND_LOCAL;
						EC20_init_num();
						EC20_do();
					}						
					else
					{
						EC20_msg._ec20_stage._time_num -= 1;
					}		
					
				}
				else if(EC20_msg._ec20_stage._type == EC20_QUIT_TRANS)
				{
					EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
					EC20_init_num();
					EC20_msg._ec20_event._time_num = 2;
					EC20_do();
				}
			}
			else if(EC20_msg._ec20_stage._res == EC20_RES_FAILED)
			{
				if(EC20_msg._ec20_stage._failed_num <= 6)//在该事件中 最多失败6次
				{
					if(EC20_msg._ec20_stage._time_num <= 0)
					{
						EC20_do();
					}						
					else
					{
						EC20_msg._ec20_stage._time_num -= 1;
					}						
				}
				else
				{
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_FAILED;
				}
			}
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_IN_PROGRESS)
		{
			if(EC20_msg._ec20_stage._type == EC20_CONNECT)
			{
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 20)
				{
					EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
					EC20_init_num();
					EC20_msg._ec20_event._time_num = 2;
					EC20_do();
				}
			}
			else if(EC20_msg._ec20_stage._type == EC20_WAIT_SERVER)
			{
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 5)
				{//5秒钟没有任何通信 就断开连接
					EC20_msg._ec20_stage._type = EC20_QUIT_TRANS;
					EC20_init_num();
					//EC20_msg._ec20_event._time_num = 2;
					EC20_do();
				}
			}
			else if(EC20_msg._ec20_stage._type == EC20_QUIT_TRANS)
			{
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 3)
				{
					if(EC20_msg._ec20_stage._failed_num <= 2)
					{
						EC20_do();
						EC20_msg._ec20_stage._failed_num += 1;
					}
					else
					{
						EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
						EC20_init_num();
						EC20_msg._ec20_event._time_num = 2;
						EC20_do();
					}					
				}
			}
		}
	}	
	else if(EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(EC20_msg._ec20_event._res == EC20_RES_SUCCESS)
		{
			//do nothing
		}
		else if(EC20_msg._ec20_event._res == EC20_RES_FAILED)
		{
			//do nothing
		}
	}	
}

void EC20CheckSendXiaXie(void)
{
	uint8_t index = EC20_msg._ec20_event._index;
	
	uint8_t data[] = {0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68,0x68};
	uint8_t size = 12;
	
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START)
	{
		EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
		EC20_msg._ec20_event._state = EC20_STATE_IN_PROGRESS;
		EC20_msg._ec20_event._time_num = 1;
		EC20_init_num();
	}
	else if(EC20_msg._ec20_event._state == EC20_STATE_IN_PROGRESS)
	{
		if(EC20_msg._ec20_stage._state == EC20_STATE_WAIT_START)
		{
			EC20_init_num();
			EC20_do();
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_FINISHED)
		{
			if(EC20_msg._ec20_stage._res == EC20_RES_SUCCESS)
			{
				if(EC20_msg._ec20_stage._type == EC20_ATQICLOSE)
				{
					if(EC20_msg._ec20_event._time_num == 1)
					{
						EC20_msg._ec20_stage._type = EC20_CONNECT_XIAXIE;
						EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
						EC20_do();
					}
					else
					{
						EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
						EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
					}
				}
				else if(EC20_msg._ec20_stage._type== EC20_CONNECT_XIAXIE)
				{
					platform_manager._platforms[index]._connect_state = 0x01;
					platform_manager._platforms[index]._cont_failed_num = 0;
					EC20_msg._ec20_stage._type = EC20_WAIT_XIAXIE;
					EC20_msg._ec20_stage._state = EC20_STATE_IN_PROGRESS;	
					
				}
				else if(EC20_msg._ec20_stage._type == EC20_QUIT_TRANS)
				{
					EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
					EC20_init_num();
					EC20_msg._ec20_event._time_num = 2;
					EC20_do();
				}
			}
			else if(EC20_msg._ec20_stage._res == EC20_RES_FAILED)
			{
				if(EC20_msg._ec20_stage._failed_num <= 6)//在该事件中 最多失败6次
				{
					if(EC20_msg._ec20_stage._time_num <= 0)
					{
						EC20_do();
					}						
					else
					{
						EC20_msg._ec20_stage._time_num -= 1;
					}						
				}
				else
				{
					//platform_manager._platforms[index]._connect_state = 0x02;
					EC20_msg._ec20_event._state = EC20_STATE_FINISHED;
					EC20_msg._ec20_event._res = EC20_RES_FAILED;
				}
			}
		}
		else if(EC20_msg._ec20_stage._state == EC20_STATE_IN_PROGRESS)
		{
			if(EC20_msg._ec20_stage._type == EC20_CONNECT_XIAXIE)
			{
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 10)
				{
					platform_manager._platforms[index]._connect_state = 0x02;
					if(platform_manager._platforms[index]._cont_failed_num < 5)
					{
						platform_manager._platforms[index]._cont_failed_num += 1;
					}
					EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
					EC20_init_num();
					EC20_msg._ec20_event._time_num = 2;
					EC20_do();
				}
			}
			else if(EC20_msg._ec20_stage._type == EC20_WAIT_XIAXIE)
			{				
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 3)
				{
					if((send_xiaxie_stage & 0x01) == 0)
					{
						EC20SendXiaXieData(0x01);
					}
					else if((send_xiaxie_stage & 0x04) == 0)
					{
						EC20SendXiaXieData(0x04);
					}
					else if((send_xiaxie_stage & 0x08) == 0)
					{
						EC20SendXiaXieData(0x08);
					}
						
				}
			
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 5)
				{
					EC20_msg._ec20_stage._type = EC20_QUIT_TRANS;
					EC20_init_num();
					EC20_do();
				}				
				
			}
			else if(EC20_msg._ec20_stage._type == EC20_QUIT_TRANS)
			{
				if(sys_time._diff - EC20_msg._ec20_stage._cmd_time >= 3)
				{
					if(EC20_msg._ec20_stage._failed_num <= 2)
					{
						EC20_do();
						EC20_msg._ec20_stage._failed_num += 1;
					}
					else
					{
						EC20_msg._ec20_stage._type = EC20_ATQICLOSE;
						EC20_init_num();
						EC20_msg._ec20_event._time_num = 2;
						EC20_do();
					}					
				}
			}
		}
	}	
	else if(EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(EC20_msg._ec20_event._res == EC20_RES_SUCCESS)
		{
			//do nothing
		}
		else if(EC20_msg._ec20_event._res == EC20_RES_FAILED)
		{
			//do nothing
		}
	}	
}

void EC20_check(void)
{
	msg_event_t event;
	
	//这一步是确认EC20是否进入新的事件
	if(EC20_msg._ec20_event._state == EC20_STATE_WAIT_START || EC20_msg._ec20_event._state == EC20_STATE_FINISHED)
	{
		if(pop_ec20_event(&event) == 1)
		{
			//每次pop出的新EC20事件 必然是要从头开始
			EC20_msg._ec20_event._type = event._msg_type;
			EC20_msg._ec20_event._state = EC20_STATE_WAIT_START;
			EC20_msg._ec20_stage._index = event._index;
			EC20_msg._ec20_event._index = event._index;
		}
	}
	
	//根据相应的事件类型 做相应的事件检查
	switch(EC20_msg._ec20_event._type)
	{
		case EC20_EVENT_INIT:
			EC20_check_init();
			break;
		case EC20_EVENT_CSQ:
			EC20_check_csq();
			break;
		case EC20_EVENT_CCLK:
			EC20_check_cclk();
			break;
		case EC20_EVENT_SENDMSG:
			EC20_check_sendmsg();
			break;
		case EC20_EVENT_DATA_UPDATE:
			EC20_check_updateData();
			break;
		case EC20_EVENT_SEND_XIAXIE:
			EC20CheckSendXiaXie();
			break;
		default:
			break;
	}
}

void EC20_parse(void)
{
	switch (EC20_msg._ec20_stage._type)
	{
		case EC20_AT:
			EC20_parse_at();
			break;
		case EC20_ATCSQ:
			EC20_parsecsq();
			break;
		case EC20_ATE0:
		case EC20_ATZU:
		case EC20_ATI:
		case EC20_ATCOPS:
		case EC20_ATQICLOSE:
		case EC20_DISCONNECT:
			EC20_parsedefault();
			break;
		case EC20_ATCPIN:
			EC20_parsecpin();
			break;
		case EC20_ATCREG:
			EC20_parsecreg();
			break;
		case EC20_ATCGREG:
			EC20_parsecgreg();
			break;
		case EC20_ATQICSGP:
			EC20_parseatqicsgp();
			break;
		case EC20_ATQIDEACT:
			EC20_parseatqideact();
			break;
		case EC20_ATQIACT:
			EC20_parseatqiact();
			break;
		case EC20_QIOPEN:
		case EC20_CONNECT:
			EC20_parse_qiopen();
			break;
		case EC20_ATCCLK:
			EC20_parsecclk();
			break;
		case EC20_QISEND:
		case EC20_SEND_DEVICE_REAL:
		case EC20_SEND_PLATFORM:
		case EC20_SEND_LOCAL:
		case EC20_SEND_SUCCESS:
		case EC20_SEND_FAILED:
			EC20_parse_send();
			break;
		case EC20_QIWAITREPLY:
			EC20_parse_waitreply();
			break;
		case EC20_WAIT_SERVER:
			EC20_Parse_wait_server();
			break;
		case EC20_QUIT_TRANS:
			EC20_parse_quit_trans();
			break;
		case EC20_CONNECT_XIAXIE:
			EC20ParseConnectXiaXie();
			break;
		case EC20_WAIT_XIAXIE:
			EC20ParseWaitXiaXie();
			break;
		default:
			break;			
	}
}

void EC20_Init(void)
{

	EC20_msg._ec20_event._type = EC20_EVENT_INIT;
	EC20_msg._ec20_event._state = EC20_STATE_WAIT_START;
	EC20_msg._ec20_event._failed_num = 0;
	EC20_msg._ec20_event._res = EC20_RES_SUCCESS;
	EC20_msg._ec20_event._time_num = 0;
	
	EC20_msg._ec20_stage._type = EC20_AT;
	EC20_msg._ec20_stage._state = EC20_STATE_WAIT_START;
	EC20_msg._ec20_stage._failed_num = 0;
	EC20_msg._ec20_stage._max_failed_num = 50;
	EC20_msg._ec20_stage._res = EC20_RES_SUCCESS;
	EC20_msg._ec20_stage._time_num = 0;
	
	EC20_msg._csq1 = 0;
	EC20_msg._csq2 = 0;
	EC20_msg._ec20_haveinit = 0;
	EC20_msg._restart_num = 0;
	
	EC20_check();

	/*
	printf("AT\r\n"); 
    delay_ms(500);
	
    strx=strstr((const char*)RxBuffer,(const char*)"OK");//返回OK
  while(strx==NULL)
    {
        Clear_Buffer();	
        printf("AT\r\n"); 
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"OK");//返回OK
    }

    printf("ATE0\r\n"); //关闭回显
    delay_ms(500);
	
    Clear_Buffer();	
    printf("AT+CSQ\r\n"); //检查CSQ
    delay_ms(500);
    printf("ATI\r\n"); //检查模块的版本号
    delay_ms(500);
	Clear_Buffer();	
	*/
}


/*
void  EC20_Init(void)
{
    printf("AT\r\n"); 
    delay_ms(500);
	
    strx=strstr((const char*)RxBuffer,(const char*)"OK");//返回OK
  while(strx==NULL)
    {
        Clear_Buffer();	
        printf("AT\r\n"); 
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"OK");//返回OK
    }

    printf("ATE0\r\n"); //关闭回显
    delay_ms(500);
	
    Clear_Buffer();	
    printf("AT+CSQ\r\n"); //检查CSQ
    delay_ms(500);
    printf("ATI\r\n"); //检查模块的版本号
    delay_ms(500);
	
    /////////////////////////////////
    printf("AT+CPIN?\r\n");//检查SIM卡是否在位,卡的缺口朝外放置 
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"+CPIN: READY");//查看是否返回ready
  while(strx==NULL)
    {
        Clear_Buffer();
        printf("AT+CPIN?\r\n");
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"+CPIN: READY");//检查SIM卡是否在位，等待卡在位，如果卡识别不到，剩余的工作就没法做了
    }
    Clear_Buffer();	
    ///////////////////////////////////
    printf("AT+CREG?\r\n");//查看是否注册GSM网络
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"+CREG: 0,1");//返回正常
    extstrx=strstr((const char*)RxBuffer,(const char*)"+CREG: 0,5");//返回正常，漫游
  while(strx==NULL&&extstrx==NULL)
    {
        Clear_Buffer();
        printf("AT+CREG?\r\n");//查看是否注册GSM网络
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"+CREG: 0,1");//返回正常
        extstrx=strstr((const char*)RxBuffer,(const char*)"+CREG: 0,5");//返回正常，漫游
    }
    Clear_Buffer();
    /////////////////////////////////////
    printf("AT+CGREG?\r\n");//查看是否注册GPRS网络
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,1");//，这里重要，只有注册成功，才可以进行GPRS数据传输。
    extstrx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,5");//返回正常，漫游
  while(strx==NULL&&extstrx==NULL)
    {
        Clear_Buffer();
        printf("AT+CGREG?\r\n");//查看是否注册GPRS网络
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,1");//，这里重要，只有注册成功，才可以进行GPRS数据传输。
        extstrx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,5");//返回正常，漫游
    }
    Clear_Buffer();
    printf("AT+COPS?\r\n");//查看注册到哪个运营商，支持移动 联通 电信 
    delay_ms(500);
    Clear_Buffer();
    printf("AT+QICLOSE=0\r\n");//关闭socket连接
    delay_ms(500);
    Clear_Buffer();
    printf("AT+QICSGP=1,1,\042CMNET\042,\042\042,\042\042,0\r\n");//接入APN，无用户名和密码
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"OK");//开启成功
  while(strx==NULL)
    {
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"OK");////开启成功
    }
    Clear_Buffer();
    printf("AT+QIDEACT=1\r\n");//去激活
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"OK");//开启成功
  while(strx==NULL)
    {
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"OK");//开启成功
    }
    Clear_Buffer();
    printf("AT+QIACT=1\r\n");//激活
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"OK");//开启成功
  while(strx==NULL)
    {
        delay_ms(500);
        strx=strstr((const char*)RxBuffer,(const char*)"OK");//开启成功
    }
    Clear_Buffer();
    printf("AT+QIACT?\r\n");//获取当前卡的IP地址
    delay_ms(500);
    Clear_Buffer();
    printf("AT+QIOPEN=1,0,\"TCP\",\"115.236.153.170\",48500,0,1\r\n");//这里是需要登陆的IP号码，采用直接吐出模式
    delay_ms(500);
    strx=strstr((const char*)RxBuffer,(const char*)"+QIOPEN: 0,0");//检查是否登陆成功
  while(strx==NULL)
    {
        strx=strstr((const char*)RxBuffer,(const char*)"+QIOPEN: 0,0");//检查是否登陆成功
        delay_ms(100);
    }

    delay_ms(500);
    Clear_Buffer();
}		
*/
///发送字符型数据
void EC20Send_StrData(char *bufferdata)
{
	uint8_t untildata=0xff;
	printf("AT+QISEND=0\r\n");
	delay_ms(100);
	printf("%s",bufferdata);
    delay_ms(100);	
    USART_SendData(USART2, (uint8_t) 0x1a);//发送完成函数
  while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
    {
    }
	delay_ms(100);
    strx=strstr((char*)RxBuffer,(char*)"SEND OK");//是否正确发送
  while(strx==NULL)
    {
        strx=strstr((char*)RxBuffer,(char*)"SEND OK");//是否正确发送
        delay_ms(10);
    }
    delay_ms(100);
    Clear_Buffer();
    printf("AT+QISEND=0,0\r\n");
    delay_ms(200);
    strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//发送剩余字节数据
  while(untildata)//这个主要是确认服务器是否全部接收完数据，这个判断可以选择不要
    {
          printf("AT+QISEND=0,0\r\n");
          delay_ms(200);
          strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//发送剩余字节数据
          strx=strstr((char*)strx,(char*)",");//获取第一个,
          strx=strstr((char*)(strx+1),(char*)",");//获取第二个,
          untildata=*(strx+1)-0x30;
    }
   Clear_Buffer();
}


///发送十六进制
void EC20Send_HexData(char *bufferdata)
{
	u8 untildata=0xff;
	EC20_msg._csq2 = 29;
	if(EC20_msg._ec20_haveinit != 1)
		return;
	printf("AT+QIOPEN=1,0,\"TCP\",\"115.236.153.170\",48500,0,1\r\n");//这里是需要登陆的IP号码，采用直接吐出模式
    delay_ms(500);
	printf("AT+QISENDEX=0,\042%s\042\r\n",bufferdata);//选择十六进制方式发送数据
	delay_ms(10);
    USART_SendData(USART2, (u8) 0x1a);//发送完成函数
	delay_ms(100);
	printf("AT+QICLOSE=0\r\n");
	delay_ms(10);
	Clear_Buffer();
}

void EC20Recv_Data()
{
	char HEX_CHAR[3] = {0};
	unsigned char sendData[256] = {0};

	int i = 0,len = 0;
	/*
	Clear_Buffer(); 
	printf("AT+QIRD=0,128\r\n");
	delay_ms(500);
	*/


	for(;i < 50;i++)
	{
		sprintf(HEX_CHAR,"%02x",RxBuffer[i]);
		sendData[len++] = HEX_CHAR[0];
		sendData[len++] = HEX_CHAR[1];
	}
	sendData[len] = 0;
	EC20Send_HexData((char *)sendData);	
	Clear_Buffer();

}

void EC20Send_HexData_JxJS(uint8_t *data,uint32_t dataLen)
{
	char EC20_QISEND_CMD[512] = {0};
	char HEX_CHAR[3] = {0};
	uint32_t i = 0,len = 0;
	uint8_t untildata=0xff;
	
	for(i = 0;i < dataLen;i++)
	{
		sprintf(HEX_CHAR,"%02x",data[i]);
		EC20_QISEND_CMD[len++] = HEX_CHAR[0];
		EC20_QISEND_CMD[len++] = HEX_CHAR[1];
	}
	EC20_QISEND_CMD[len] = 0;
	

	printf("AT+QISENDEX=0,\042%s\042\r\n",EC20_QISEND_CMD);//选择十六进制方式发送数据
	delay_ms(10);
    USART_SendData(USART2, (uint8_t) 0x1a);//发送完成函数
	delay_ms(10);
	Clear_Buffer();
}


