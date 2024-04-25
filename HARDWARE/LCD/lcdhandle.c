#include "stm32f4xx_conf.h"
#include "lcdhandle.h"
#include "string.h"
#include "key.h"

extern EC20_msg_t 			EC20_msg;
extern jxjs_time_t 			sys_time;
extern platform_manager_t	platform_manager;
extern device_manager_t		device_manager;
extern LCD_msg_t			LCD_msg;
extern uint8_t 				ProUpdateFlag;

extern uint8_t pro_update_packet_num;
extern uint8_t current_packet;
extern uint8_t send_xiaxie_stage;
extern EC20_send_data_t	EC20_Send_Data[6];

static void make_data_main(char *title,char *data1,char* data2,char* data3)
{
	sprintf(title,"京水RTU");
	sprintf(data1,"传感器数据");
	sprintf(data2,"平台数据");
	sprintf(data3,"本机数据");
}

static void make_data_device(char *title,char *data1,char* data2,char* data3)
{
	uint8_t i;
	
	char *dataTmp;
	
	//写标题
	sprintf(title,"传感器数据");
	
	for(i = LCD_msg._lineTop;i < LCD_msg._lineTop + 3;i++)
	{
		if(i >= device_manager._device_count + 1)
			break;
		
		switch(i - LCD_msg._lineTop)
		{
			case 0:
				dataTmp = data1;
				break;
			case 1:
				dataTmp = data2;
				break;
			case 2:
				dataTmp = data3;
				break;
			default:
				dataTmp = data1;
				break;
		}
		
		if(i == 0)
		{
			sprintf(dataTmp,"传感器数量%6d",device_manager._device_count);
		}
		else
		{
			sprintf(dataTmp,"传感器%d数据",i);
		}
	}
}

static void make_data_device_data(char *title,char *data1,char* data2,char* data3,uint8_t index)
{
	uint8_t i,type,maxItemNum;
	char *tmp;
	double flowRate_h;//瞬时流量 每小时
	
	if(index >= device_manager._device_count)
		index = 0;
	
	if(device_manager._devices[index]._device_type == DEVICE_WATERLEVEL)
	{
		if(device_manager._devices[index]._device_formula == FORMULA_NONE)
			maxItemNum = 2;
		else 
			maxItemNum = 6;
		type = 0;
	}
	else if(device_manager._devices[index]._device_type == DEVICE_FLOWMETER)
	{
		maxItemNum = 3;
		type = 3;
	}
	else if(device_manager._devices[index]._device_type == DEVICE_FLOWSPEED)
	{
		maxItemNum = 1;
		type = 2;
	}
	
	//标题数据
	if(device_manager._devices[index]._device_type == DEVICE_WATERLEVEL)
	{
		sprintf(title,"传感器%d   水位计",index + 1);
	}
	else if(device_manager._devices[index]._device_type == DEVICE_FLOWMETER)
	{
		sprintf(title,"传感器%d   流量计",index + 1);
	}
	else if(device_manager._devices[index]._device_type == DEVICE_FLOWSPEED)
	{
		sprintf(title,"传感器%d   流速仪",index + 1);
	}
	else
	{
		sprintf(title,"传感器%d     未知",index + 1);
	}
	
	
	for(i = LCD_msg._lineTop;i < LCD_msg._lineTop + 3;i++)
	{
		if(i > maxItemNum)
			break;
		
		switch(i - LCD_msg._lineTop)
		{
			case 0:
				tmp = data1;
				break;
			case 1:
				tmp = data2;
				break;
			case 2:
				tmp = data3;
				break;
			default:
				tmp = data1;
				break;
		}
		
		if(i == 0)
		{
			if(device_manager._devices[index]._device_com_state == 0x01)
			{
				sprintf(tmp,"通信状态    正常");
			}
			else if(device_manager._devices[index]._device_com_state == 0x00)
			{
				sprintf(tmp,"通信状态    等待");
			}
			else
			{
				sprintf(tmp,"通信状态    异常");
			}
		}
		else
		{
			switch(i + type)
			{
				case 1:
					sprintf(tmp,"空高:%10.3fm",device_manager._devices[index]._water_data._airHeight);
					break;
				case 2:
					sprintf(tmp,"水位:%10.3fm",device_manager._devices[index]._water_data._waterlevel);
					break;
				case 3:
					sprintf(tmp,"流速:%8.3fm/s",device_manager._devices[index]._water_data._flowspeed);
					break;
				case 4:
					{
						if(device_manager._devices[index]._water_data._flowRate >= 100000.0)
							sprintf(tmp,"流量:%7.0fm3/s",device_manager._devices[index]._water_data._flowRate);
						else if(device_manager._devices[index]._water_data._flowRate >= 10000.0)
							sprintf(tmp,"流量:%7.1fm3/s",device_manager._devices[index]._water_data._flowRate);
						else if(device_manager._devices[index]._water_data._flowRate >= 1000.0)
							sprintf(tmp,"流量:%7.2fm3/s",device_manager._devices[index]._water_data._flowRate);
						else
							sprintf(tmp,"流量:%7.3fm3/s",device_manager._devices[index]._water_data._flowRate);
					}
					break;
				case 5:
					{
						flowRate_h = device_manager._devices[index]._water_data._flowRate * 3600;
						if(flowRate_h >= 100000.0)
							sprintf(tmp,"流量:%7.0fm3/h",flowRate_h);
						else if(flowRate_h >= 10000.0)
							sprintf(tmp,"流量:%7.1fm3/h",flowRate_h);
						else if(flowRate_h >= 1000.0)
							sprintf(tmp,"流量:%7.2fm3/h",flowRate_h);
						else
							sprintf(tmp,"流量:%7.3fm3/h",flowRate_h);
					}
					break;
				case 6:
					{
						if(device_manager._devices[index]._water_data._flow >= 10000000.0)
							sprintf(tmp,"水量:%9.0fm3",device_manager._devices[index]._water_data._flow);
						else if(device_manager._devices[index]._water_data._flow >= 1000000.0)
							sprintf(tmp,"水量:%9.1fm3",device_manager._devices[index]._water_data._flow);
						else if(device_manager._devices[index]._water_data._flow >= 100000.0)
							sprintf(tmp,"水量:%9.2fm3",device_manager._devices[index]._water_data._flow);
						else
							sprintf(tmp,"水量:%9.3fm3",device_manager._devices[index]._water_data._flow);
					}
					break;
				default:
					break;
			}
		}
		
	}
}

static void make_data_platform(char *title,char *data1,char* data2,char* data3)
{
	uint8_t i;
	
	char *dataTmp;
	uint8_t num[5] = {0};
	
	//写标题
	sprintf(title,"平台数据");
	
	for(i = LCD_msg._lineTop;i < LCD_msg._lineTop + 3;i++)
	{
		if(i >= platform_manager._platform_count + 3)
			break;
		
		switch(i - LCD_msg._lineTop)
		{
			case 0:
				dataTmp = data1;
				break;
			case 1:
				dataTmp = data2;
				break;
			case 2:
				dataTmp = data3;
				break;
			default:
				dataTmp = data1;
				break;
		}
		
		switch(i)
		{
			case 0:
				sprintf(dataTmp,"站点编号",device_manager._device_count);
				break;
			case 1:
				get_address_bytes(num,5,platform_manager._platform_number_a1,platform_manager._platform_number_a2,0);
				sprintf(dataTmp,"%02X%02X%02X%02X%02X",num[0],num[1],num[2],num[3],num[4]);
				break;
			case 2:
				sprintf(dataTmp,"平台数量%8d",platform_manager._platform_count);
				break;
			default:
				sprintf(dataTmp,"平台%d数据",i - 2);
				break;
		}
	}
}

static void make_data_platform_data(char *title,char *data1,char* data2,char* data3,uint8_t index)
{
	uint8_t i;
	
	char *tmp;
	
	if(index >= MAX_PLAT_NUM)
		index = 0;
	
	//写标题
	sprintf(title,"平台%d",index + 1);
	
	for(i = LCD_msg._lineTop;i < LCD_msg._lineTop + 3;i++)
	{
		if(i >= 5)
			break;
		
		switch(i - LCD_msg._lineTop)
		{
			case 0:
				tmp = data1;
				break;
			case 1:
				tmp = data2;
				break;
			case 2:
				tmp = data3;
				break;
			default:
				tmp = data1;
				break;
		}
		
		switch(i)
		{
			case 0:
				sprintf(tmp,"通信状态");
				break;
			case 1:
				{
					if(platform_manager._platforms[index]._connect_state == 0x00)
						sprintf(tmp,"等待通信");
					else if(platform_manager._platforms[index]._connect_state == 0x01)
						sprintf(tmp,"通信正常");
					else
						sprintf(tmp,"通信异常");
				}
				
				break;
			case 2:
				sprintf(tmp,"IP");
				break;
			case 3:
				sprintf(tmp,"%d.%d.%d.%d",platform_manager._platforms[index]._ip1,platform_manager._platforms[index]._ip2,
						platform_manager._platforms[index]._ip3,platform_manager._platforms[index]._ip4);
				break;
			case 4:
				sprintf(tmp,"端口%12d",platform_manager._platforms[index]._port);
				break;
			default:
				break;
		}
	}
		
}


static void make_data_RTU(char *title,char *data1,char* data2,char* data3)
{

	uint8_t i,index,type;
	uint8_t csq;
	char *tmp;
	
	if(EC20_msg._csq1 <0 || EC20_msg._csq1 > 31)
		csq = 0;
	else
		csq = EC20_msg._csq1;
	
	//标题数据
	sprintf(title,"本地数据");
	
	//其余数据
	for(i = LCD_msg._lineTop;i < LCD_msg._lineTop + 3;i++)
	{
		switch(i - LCD_msg._lineTop)
		{
			case 0:
				tmp = data1;
				break;
			case 1:
				tmp = data2;
				break;
			case 2:
				tmp = data3;
				break;
			default:
				tmp = data1;
				break;
		}
		
		switch(i)
		{
			case 0:
				sprintf(tmp,"设备编号");
				break;
			case 1:
				sprintf(tmp,"JS%02X%02X%02X%02X%02X%02X%02X",platform_manager._device_number[0],platform_manager._device_number[1],platform_manager._device_number[2]
			,platform_manager._device_number[3],platform_manager._device_number[4],platform_manager._device_number[5],platform_manager._device_number[6]);
				break;
			case 2:
				sprintf(tmp,"日期");
				break;
			case 3:
				sprintf(tmp,"%04d-%02d-%02d",sys_time._year,sys_time._month,sys_time._day);
				break;
			case 4:
				sprintf(tmp,"时间");
				break;
			case 5:
				//,EC20_msg._ec20_stage._type,send_xiaxie_stage,EC20_Send_Data[0]._current_sector,EC20_Send_Data[0]._sector_num
				sprintf(tmp,"%02d:%02d:%02d",sys_time._hour,sys_time._min,sys_time._sec);
				break;
			case 6:
				sprintf(tmp,"网络状态");
				break;
			case 7:
				{
					if(EC20_msg._ec20_haveinit)
						sprintf(tmp,"正常");
					else
					{
						if(EC20_msg._ec20_stage._type == EC20_ATCPIN && EC20_msg._ec20_stage._failed_num >= 20)
						{
							sprintf(tmp,"未识别到sim卡");
						}
						else if((EC20_msg._ec20_stage._type == EC20_ATCREG || EC20_msg._ec20_stage._type == EC20_ATCGREG)&& EC20_msg._ec20_stage._failed_num >= 20)
						{
							sprintf(tmp,"组网失败");
						}
						else
						{
							sprintf(tmp,"组网中...");
						}
					}
				}
				break;
			case 8:
				sprintf(tmp,"信号质量:%7d",csq);
				break;
			case 9:
				sprintf(tmp,"任务状态");
				break;
			case 10:
				sprintf(tmp,"%02X %02X %02X %02X %02X",EC20_msg._ec20_event._type,EC20_msg._ec20_event._state,
			EC20_msg._ec20_stage._type,EC20_msg._ec20_stage._state,send_xiaxie_stage);
		}
	}
}

void update_lcd(void)
{
	uint8_t i = 0;
	char title[18] = {0};
	char data1[18] = {0};
	char data2[18] = {0};
	char data3[18] = {0};
	LCD_DISPLAY_MODE mode;
	
	if(ProUpdateFlag == 1)
	{
		if(LCD_msg._backState == LCD_BACK_OFF)
		{
			turn_on_back();
			LCD_msg._backState = LCD_BACK_ON;
		}
		display_GB2312_string(1,1,"下载升级文件中 ",LCD_DISPLAY_FORWARD);
		display_GB2312_string(3,1,"请勿操作!!!",LCD_DISPLAY_FORWARD);
		i = current_packet * 100 / pro_update_packet_num;
		sprintf(data3,"下载进度:%3d%%",i);
		display_GB2312_string(5,1,(unsigned char*)data3,LCD_DISPLAY_FORWARD);
		if(i == 100)
			display_GB2312_string(7,1,"校验文件中...",LCD_DISPLAY_FORWARD);
		else
		{
			display_GB2312_string(7,1," ",LCD_DISPLAY_FORWARD);
		}
		return;
	}
	
	//背光
	if(LCD_msg._backState == LCD_BACK_ON && (sys_time._diff - LCD_msg._lastBackOnTime >= 20))
	{
		turn_off_back();
		LCD_msg._backState = LCD_BACK_OFF;
	}
	
	//拿取屏幕要显示的数据
	switch(LCD_msg._menu)
	{
		case MENU_MAIN:
			make_data_main(title,data1,data2,data3);
			break;
		case MENU_DEVICE:
			make_data_device(title,data1,data2,data3);
			break;
		case MENU_DEVICE_DATA:
			make_data_device_data(title,data1,data2,data3,LCD_msg._menuIndex);
			break;
		case MENU_PLATFORM:
			make_data_platform(title,data1,data2,data3);
			break;
		case MENU_PLATFORM_DATA:
			make_data_platform_data(title,data1,data2,data3,LCD_msg._menuIndex);
			break;
		case MENU_RTU:
			make_data_RTU(title,data1,data2,data3);
			break;
		default:
			i = 0xff;
			break;
	}
	
	if(i == 0xff)
		return;
	
	//刷新屏幕
	display_GB2312_string(1,1,title,LCD_DISPLAY_FORWARD);
	
	for(i = LCD_msg._lineTop;i <= LCD_msg._lineTop + 3;i++)
	{
		if(i >= LCD_msg._currentLine && i <= LCD_msg._currentLine + LCD_msg._currentLineHeight - 1)
			mode = LCD_DISPLAY_REVRESE;
		else
			mode = LCD_DISPLAY_FORWARD;
		
		switch(i - LCD_msg._lineTop)
		{
			case 0:
				display_GB2312_string(3,1,data1,mode);
				break;
			case 1:
				display_GB2312_string(5,1,data2,mode);
				break;
			case 2:
				display_GB2312_string(7,1,data3,mode);
				break;
			default:
				break;
		}
	}
}

void setNormalLine(uint8_t lineTop,uint8_t currentLine,uint8_t lineHeight,uint8_t index)
{
	LCD_msg._lineTop = lineTop;
	LCD_msg._currentLine = currentLine;
	LCD_msg._currentLineHeight = lineHeight;
	LCD_msg._menuIndex = index;
}


void doKeyUp(void)
{
	uint8_t item_nums;
	if(LCD_msg._menu == MENU_MAIN)
	{
		if(LCD_msg._currentLine == 0)
			LCD_msg._currentLine = 2;
		else 
			LCD_msg._currentLine -= 1;
	}
	else if(LCD_msg._menu == MENU_DEVICE)
	{
		item_nums = device_manager._device_count;
		
		if(LCD_msg._currentLine == 0)
			LCD_msg._currentLine = item_nums;
		else 
			LCD_msg._currentLine -= 1;		
	}
	else if(LCD_msg._menu == MENU_PLATFORM)
	{
		item_nums = platform_manager._platform_count + 2;
		
		if(LCD_msg._currentLine == 0)
		{
			LCD_msg._currentLine = item_nums;
			LCD_msg._currentLineHeight = 1;
		}
		else if(LCD_msg._currentLine == 2)
		{
			LCD_msg._currentLine = 0;
			LCD_msg._currentLineHeight = 2;
		}
		else
		{
			LCD_msg._currentLine -= 1;
			LCD_msg._currentLineHeight = 1;
		}
		
	}
	else if(LCD_msg._menu == MENU_RTU)
	{
		item_nums = 10;
		if(LCD_msg._currentLine == 0)
		{
			LCD_msg._currentLine = 9;
			LCD_msg._currentLineHeight = 2;
		}
		else if(LCD_msg._currentLine == 9)
		{
			LCD_msg._currentLine -= 1;
			LCD_msg._currentLineHeight = 1;
		}
		else
		{
			LCD_msg._currentLine -= 2;
			LCD_msg._currentLineHeight = 2;
		}
	}
	else if(LCD_msg._menu == MENU_DEVICE_DATA)
	{
		if(device_manager._devices[LCD_msg._menuIndex]._device_type == DEVICE_WATERLEVEL)
		{
			if(device_manager._devices[LCD_msg._menuIndex]._device_formula == FORMULA_NONE)
				item_nums = 2;
			else 
				item_nums = 6;
		}
		else if(device_manager._devices[LCD_msg._menuIndex]._device_type == DEVICE_FLOWMETER)
		{
			item_nums = 3;
		}
		else if(device_manager._devices[LCD_msg._menuIndex]._device_type == DEVICE_FLOWSPEED)
		{
			item_nums = 1;
		}
		else
		{
			item_nums = 1;
		}
		
		if(LCD_msg._currentLine == 0)
			LCD_msg._currentLine = item_nums;
		else
			LCD_msg._currentLine -= 1;
	}
	else if(LCD_msg._menu == MENU_PLATFORM_DATA)
	{
		item_nums = 4;
		if(LCD_msg._currentLine == 0)
		{
			LCD_msg._currentLine = 4;
			LCD_msg._currentLineHeight = 1;
		}
		else
		{
			LCD_msg._currentLine -= 2;
			LCD_msg._currentLineHeight = 2;
		}
	}
	
	if(item_nums < 2)
	{
		LCD_msg._lineTop = 0;
	}
	else
	{
		if((LCD_msg._currentLine + LCD_msg._currentLineHeight - 1) == item_nums)
			LCD_msg._lineTop = item_nums - 2;
		else if(LCD_msg._currentLine < LCD_msg._lineTop)
			LCD_msg._lineTop = LCD_msg._currentLine;
	}
}

void doKeyDown(void)
{
	uint8_t item_nums;
	if(LCD_msg._menu == MENU_MAIN)
	{
		if(LCD_msg._currentLine == 2)
			LCD_msg._currentLine = 0;
		else 
			LCD_msg._currentLine += 1;
	}
	else if(LCD_msg._menu == MENU_DEVICE)
	{
		item_nums = device_manager._device_count;
		
		if(LCD_msg._currentLine == item_nums)
			LCD_msg._currentLine = 0;
		else 
			LCD_msg._currentLine += 1;				
	}
	else if(LCD_msg._menu == MENU_PLATFORM)
	{
		item_nums = platform_manager._platform_count + 2;
		
		if(LCD_msg._currentLine == item_nums)
		{
			LCD_msg._currentLine = 0;
			LCD_msg._currentLineHeight = 2;
		}
		else if(LCD_msg._currentLine == 0)
		{
			LCD_msg._currentLine = 2;
			LCD_msg._currentLineHeight = 1;
		}
		else
		{
			LCD_msg._currentLine += 1;
			LCD_msg._currentLineHeight = 1;
		}
	}
	else if(LCD_msg._menu == MENU_RTU)
	{
		item_nums = 10;
		if(LCD_msg._currentLine == 9)
		{
			LCD_msg._currentLine = 0;
			LCD_msg._currentLineHeight = 2;
		}
		else if(LCD_msg._currentLine == 6)
		{
			LCD_msg._currentLine += 2;
			LCD_msg._currentLineHeight = 1;
		}
		else if(LCD_msg._currentLine == 8)
		{
			LCD_msg._currentLine += 1;
			LCD_msg._currentLineHeight = 2;
		}
		else
		{
			LCD_msg._currentLine += 2;
			LCD_msg._currentLineHeight = 2;
		}
		
	}
	else if(LCD_msg._menu == MENU_DEVICE_DATA)
	{
		if(device_manager._devices[LCD_msg._menuIndex]._device_type == DEVICE_WATERLEVEL)
		{
			if(device_manager._devices[LCD_msg._menuIndex]._device_formula == FORMULA_NONE)
				item_nums = 2;
			else 
				item_nums = 6;
		}
		else if(device_manager._devices[LCD_msg._menuIndex]._device_type == DEVICE_FLOWMETER)
		{
			item_nums = 3;
		}
		else if(device_manager._devices[LCD_msg._menuIndex]._device_type == DEVICE_FLOWSPEED)
		{
			item_nums = 1;
		}
		else
		{
			item_nums = 1;
		}
		
		if(LCD_msg._currentLine == item_nums)
			LCD_msg._currentLine = 0;
		else
			LCD_msg._currentLine += 1;
	}
	else if(LCD_msg._menu == MENU_PLATFORM_DATA)
	{
		item_nums = 4;
		if(LCD_msg._currentLine == 4)
		{
			LCD_msg._currentLine = 0;
			LCD_msg._currentLineHeight = 2;
		}
		else if(LCD_msg._currentLine == 2)
		{
			LCD_msg._currentLine += 2;
			LCD_msg._currentLineHeight = 1;
		}
		else
		{
			LCD_msg._currentLine += 2;
			LCD_msg._currentLineHeight = 2;
		}
	}
	
	if(item_nums < 2)
			LCD_msg._lineTop = 0;
	else
	{
		if(LCD_msg._currentLine == 0)
			LCD_msg._lineTop = 0;
		else if(LCD_msg._currentLine >= 2 && (LCD_msg._currentLine + LCD_msg._currentLineHeight - 1) > LCD_msg._lineTop + 2)
			LCD_msg._lineTop = LCD_msg._currentLine + LCD_msg._currentLineHeight - 1 - 2;
	}
}

void doKeyEsc(void)
{
	if(LCD_msg._menu == MENU_DEVICE_DATA)
	{
		LCD_msg._menu = MENU_DEVICE;
		
		LCD_msg._currentLine = LCD_msg._menuIndex + 1;
		if(LCD_msg._currentLine - 2 >= 0)
			LCD_msg._lineTop = LCD_msg._currentLine - 2;
		else
			LCD_msg._lineTop = 0;
		LCD_msg._currentLineHeight = 1;
		LCD_msg._menuIndex = 0;
	}
	else if(LCD_msg._menu == MENU_PLATFORM_DATA)
	{
		LCD_msg._menu = MENU_PLATFORM;
		LCD_msg._currentLine = LCD_msg._menuIndex + 3;
		if(LCD_msg._currentLine - 2 >= 0)
			LCD_msg._lineTop = LCD_msg._currentLine - 2;
		else
			LCD_msg._lineTop = 0;
		LCD_msg._currentLineHeight = 1;
		LCD_msg._menuIndex = 0;
	}
	else
	{
		if(LCD_msg._menu != MENU_MAIN)
		{
			setNormalLine(0,LCD_msg._menu - MENU_LEVEL_2,1,0);
			LCD_msg._menu = MENU_MAIN;
			
		}
	}
}

void doKeyEnter(void)
{
	if(LCD_msg._menu == MENU_MAIN)
	{
		LCD_msg._menu = MENU_LEVEL_2 + LCD_msg._currentLine;
		if(LCD_msg._currentLine == 0)
			setNormalLine(0,0,1,0);
		else
			setNormalLine(0,0,2,0);
	}
	else if(LCD_msg._menu == MENU_DEVICE)
	{
		if(LCD_msg._currentLine >= 1)
		{
			LCD_msg._menu = MENU_DEVICE_DATA;
			setNormalLine(0,0,1,LCD_msg._currentLine - 1);
		}		
	}
	else if(LCD_msg._menu == MENU_PLATFORM)
	{
		if(LCD_msg._currentLine >= 3)
		{
			LCD_msg._menu = MENU_PLATFORM_DATA;
			setNormalLine(0,0,2,LCD_msg._currentLine - 3);
		}
	}
}

/*谨慎修改*/
void handle_key(uint8_t key_num)
{

	uint8_t i = 0;
	
	if(key_num == 0)
		return;
	
	//开启背光
	turn_on_back();
	LCD_msg._backState = LCD_BACK_ON;
	LCD_msg._lastBackOnTime = sys_time._diff;
	
	switch(key_num)
	{
		case KEY_UP_PRES:
			doKeyUp();
			break;
		case KEY_DOWN_PRES:
			doKeyDown();
			break;
		case KEY_ESC_PRES:
			doKeyEsc();
			break;
		case KEY_ENTER_PRES:
			doKeyEnter();
			break;
		default:
			i = 1;
			break;
	}
	
	if(i == 1)
		return;
	
	update_lcd();
	return;
}


void goFirstPage(void)
{
	
	LCD_msg._menu = MENU_MAIN;
	LCD_msg._menuIndex = 0;
	LCD_msg._lineTop = 0;
	
	LCD_msg._currentLine = 0;
	LCD_msg._currentLineHeight = 1;
	
	update_lcd();
}
