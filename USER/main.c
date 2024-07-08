#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h"
#include "key.h"
#include "rs485.h"
#include "modbus.h"
#include "timer.h"
#include "ec20.h"
#include "iwdg.h"
#include "protocol.h"
#include "flash.h"
#include "liuqueue.h"
#include "time.h"
#include "sdio_sdcard.h"
#include "hc08.h"
#include "lcdhandle.h"
#include "adc.h"
#include "handlewater.h"
#include "sdhandle.h"
#include "core_cm4.h"

uint8_t 				getFlowSpeedFlag;
EC20_msg_t 				EC20_msg;
jxjs_time_t 			sys_time;
platform_manager_t		platform_manager;
device_manager_t		device_manager;
LCD_msg_t				LCD_msg;
hydrology_packet_t		hydrology_packet;
battery_data_t			battery_data;
uint8_t 				SDCardInitSuccessFlag;
uint8_t					ChangeFlag = 1;
uint8_t 				ProUpdateFlag = 0;
uint8_t					RS485Mode = 0;//RS485ģʽ 0-->��й��̬���� 1-->��Ӵ���������
uint8_t					SDFailedNums = 0;
rs485_state_t			rs485State;
uint64_t 				connectServerDiff = 0;
uint8_t 				debugFlag = 0;
device_uart_config_t	currentConfig;
	
extern uint8_t RS485_RX_BUF[128];
extern uint8_t RS485_RX_CNT;

extern SD_CardInfo SDCardInfo;
uint8_t SD_BUFFER[512];

extern uint8_t send_xiaxie_stage;

void sys_reset(void)
{
	//��������
	if(SDCardInitSuccessFlag)
	{
		SDWriteConfig(&device_manager,&platform_manager,&battery_data);
	}
	delay_ms(100);
	//�� EC20 �ػ�
	PWRKEY = 1;
	//�ȴ�500ms
	delay_ms(1000);

	NVIC_SystemReset();
}

void test_data()
{
	device_manager._device_count = 1;
	device_manager._devices[0]._device_number = 0x01;
	device_manager._devices[0]._device_type = DEVICE_WATERLEVEL;
	device_manager._devices[0]._device_protocol = WATERLEVEL_PROTOCOL1;
	device_manager._devices[0]._device_get_time = 60;
	device_manager._devices[0]._device_last_get_time = 0;
	
	platform_manager._platform_count = 2;
	platform_manager._platforms[0]._interval_time = 3600;
	platform_manager._platforms[0]._ip1 = 218;
	platform_manager._platforms[0]._ip2 = 95;
	platform_manager._platforms[0]._ip3 = 67;
	platform_manager._platforms[0]._ip4 = 107;
	platform_manager._platforms[0]._port = 24567;
	platform_manager._platforms[0]._mode = NO_RESPONSE_MODE;
	platform_manager._platforms[0]._type = JXJS_PLATFORM_SELF;
	platform_manager._platforms[0]._ip_type = 0x01;
	
	platform_manager._platforms[1]._interval_time = 3600;
	platform_manager._platforms[1]._ip1 = 218;
	platform_manager._platforms[1]._ip2 = 95;
	platform_manager._platforms[1]._ip3 = 67;
	platform_manager._platforms[1]._ip4 = 107;
	platform_manager._platforms[1]._port = 9004;
	platform_manager._platforms[1]._mode = NO_RESPONSE_MODE;
	platform_manager._platforms[1]._type = JXJS_PLATFORM_SELF;
	platform_manager._platforms[1]._ip_type = 0x01;
	
	platform_manager._platform_number_a1 = 0x1234;
	platform_manager._platform_number_a2 = 0x5678;
	
	platform_manager._ipv4[0] = 218;
	platform_manager._ipv4[1] = 95;
	platform_manager._ipv4[2] = 67;
	platform_manager._ipv4[3] = 107;
	
	platform_manager._port = 2347;
}

//�����ú���
void writeSD(void)
{
	uint16_t i;
	for(i = 0; i < 512;i++)
	{
		SD_BUFFER[i] = 0;
	}
	
	SD_WriteDisk(SD_BUFFER,0,1000);
}

//�����ú���
void readSD(void)
{
	uint8_t i = 0;
	SD_ReadDisk(SD_BUFFER,200 * 1024,1);
	
	for(i = 0;i < 8;i++)
	{
		battery_data._extend_data[i] = SD_BUFFER[i];
	}
	
}

void init_SD_Card(void)
{
	uint8_t i = 0;
	
	SDCardInitSuccessFlag = 0;//��ʼ��֮ǰ����־λ����
	
	while(i < 3)
	{
		if(SD_Init() == SD_OK)
		{
			SDCardInitSuccessFlag = 1;
			break;
		}
		
		delay_ms(200);
		i++;
	}
	
	if(SDCardInitSuccessFlag == 0)
		SDFailedNums++;
}

void save_all_data(void)
{
	if(SDCardInitSuccessFlag)
	{
		SDWriteConfig(&device_manager,&platform_manager,&battery_data);
	}
}

void GetBatteryVoltage(void)
{
	uint16_t value;
	float voltage;
	value = Get_Adc_Average(1,5);
	
	voltage = (float)value * 3.3 * 133 / (4096 * 33);
	
	battery_data._voltage = voltage * 10;
}

void sendRS485Xaxie(uint8_t *data,uint16_t dataSize)
{
	uint8_t RSdata[16];
	uint16_t RsdataSize = 0;
	uint16_t byte16;
	uint32_t byte32;
	
	
	if(dataSize < 5)
		return;
	
	if(CheckDataLegality(RS485_RX_BUF,RS485_RX_CNT,0xFF) != 1)
		return;//���ݺϷ���(modbus crc16)У��
	
	if(data[0] == 0x08)
	{
		//ˮλ����
		RSdata[RsdataSize++] = 0x08;
		RSdata[RsdataSize++] = 0x03;
		RSdata[RsdataSize++] = 0x02;
		byte16 = device_manager._devices[0]._water_data._waterlevel * 1000;
		appendUint16ToData(RSdata,&RsdataSize,byte16);			
		byte16 = modBusCRC16(RSdata,RsdataSize);
		RSdata[RsdataSize++] = byte16 & 0xFF;
		RSdata[RsdataSize++] = byte16 >> 8 & 0xFF;
		RS485_Send_Data(RSdata,RsdataSize & 0xFF);
	}
	else if(data[0] == 0x09)
	{
		//��������
		RSdata[RsdataSize++] = 0x09;
		RSdata[RsdataSize++] = 0x03;
		RSdata[RsdataSize++] = 0x04;
		byte32 = device_manager._devices[0]._water_data._flowRate * 1000;
		appendUint32ToData(RSdata,&RsdataSize,byte32);	
		byte16 = modBusCRC16(RSdata,RsdataSize);
		RSdata[RsdataSize++] = byte16 & 0xFF;
		RSdata[RsdataSize++] = byte16 >> 8 & 0xFF;
		RS485_Send_Data(RSdata,RsdataSize & 0xFF);
	}
	sys_time._last_camera_diff = sys_time._diff;
}

void read_uart1(void)
{
	uint8_t data[128];
	uint16_t dataSize = 0;
	uint16_t startIndex = 0,i = 0;
		
	if(RS485_RX_CNT < 5)
	{
		RS485_RX_CNT = 0;
		return;
	}
	
	while(1)
	{
		if(RS485_RX_BUF[startIndex] == 0x08 || RS485_RX_BUF[startIndex] == 0x09)
		{
			dataSize = 0;
			for(i = startIndex;i < startIndex + 8;i++)
			{
				data[dataSize++] = RS485_RX_BUF[startIndex + i];
			}
			sendRS485Xaxie(data,dataSize);
			startIndex += 8;
		}
		else
		{
			if(RS485_RX_BUF[startIndex + 1] == 0x03 || RS485_RX_BUF[startIndex + 1] == 0x04)
			{
				if(RS485_RX_BUF[startIndex + 2] > 100)
					return;
				
				dataSize = 0;
				for(i = startIndex;i < startIndex + RS485_RX_BUF[startIndex + 2] + 5;i++)
				{
					data[dataSize++] = RS485_RX_BUF[startIndex + i];
				}
				readWaterData(data,dataSize,rs485State._index);
				startIndex += RS485_RX_BUF[startIndex + 2] + 5;
			}
			else
			{
				break;
			}
		}
		
		if(startIndex >= RS485_RX_CNT)
			break;
	}
	
	RS485_RX_CNT = 0;
	
}

void parseEvent(msg_event_t event)
{
	switch (event._msg_type)
	{
		case MSG_CHECK_EC20:
			EC20_check();
			break;
		case MSG_IWDF_FEED:
			IWDG_Feed();
			break;
		case MSG_READ_USART2:
			EC20_parse();
			break;
		case MSG_READ_USART3:
			read_uart3();
			break;
		case MSG_READ_USART1:
			read_uart1();
			break;
		case MSG_FLUSH_LCD:
			update_lcd();
			break;
		case MSG_SEND_EC20:
			//EC20Send_HexData(str);
			break;
		case MSG_GET_WATER:
			handle_water_data(event._index);
			break;
		case MSG_SET_BLUETOOTH_NAME:
			setBluetoothName();
			break;
		case MSG_INIT_SD:
			init_SD_Card();
			break;
		case MSG_SAVE_DATA:
			save_all_data();
			break;
		case MSG_REBOOT:
			sys_reset();
			break;
		case MSG_GET_VOLTAGE:
			GetBatteryVoltage();
			break;
		default:
			break;
	}
}

void LED_on(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;//��©���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOE,&GPIO_InitStructure);//��ʼ��
	
	GPIO_ResetBits(GPIOE,GPIO_Pin_6);
}

int main(void)
{ 	
	msg_event_t event;
	uint8_t key_num;
	uint8_t i = 0;
	
	SCB->VTOR = FLASH_BASE | 0x08008000;//������ʵ������ַ
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);   //��ʼ����ʱ����
	
	//���മ�ڳ�ʼ��
	RS485_Init(9600);		//��ʼ��RS485����1
    uart2_init(115200);
	hc08_init(9600);
	//��ʼ��lcd����
	init_lcd_msg(&LCD_msg);
	LED_on();
	//���ඨʱ����ʼ��
	TIM2_Int_Init(1000-1,8400-1);//��ʱ�� 100ms
    TIM3_Int_Init(100-1,8400-1);	//��ʱ��ʱ��84M����Ƶϵ��8400������84M/8400=10Khz�ļ���Ƶ�ʣ�����100��Ϊ10ms
	TIM4_Int_Init(10000 - 1,8400 - 1); //��ʱ������10000�� 1s	
	TIM7_Int_Init(100 -1,8400 - 1);// ��ʱ��10ms
	//��Ļ
	init_lcd_gpio();//��ʼ����Ļ
	clear_screen();
	//��ʼ��SD��
	init_SD_Card();
	//���Ź�
	IWDG_Init(IWDG_Prescaler_256,625 * 2);//10s���ʱ��	
	//RTCʱ��
	RTC_Config_Jxjs();
	sys_time_init();
	//������ʼ��
	init_device_manager(&device_manager);
	init_platform_manager(&platform_manager);
	init_battery_data(&battery_data);
	initRs485State(&rs485State);
	initDeviceUartConfig(&currentConfig);
	//��ȡ�豸ID
	getDeviceNumber();//ÿ���豸����Ψһ���豸ID
	
	TIM_Cmd(TIM4, ENABLE); //������ʱ��4 ��ʱ����
	//EC20
	PWER_Init();
    PWRKEY=0;//��ģ�鿪��
	EC20_Init();
	//��ʼ������
	KEY_Init();
	//��ʼ��ADC
	Adc_Init();
	
	//���³���汾
	battery_data._version = 6;
	
	//��ȡ�ڴ����� ���û�������Ĭ��ֵ
	if(SDCardInitSuccessFlag)
	{
		if(SDReadConfig(&device_manager,&platform_manager,&battery_data) == 0)
		{
			test_data();
			SDWriteConfig(&device_manager,&platform_manager,&battery_data);
		}
		
	}
	else
	{
		test_data();
	}

	while(1)
	{	
		//������ȡ ����
		key_num	= KEY_Scan(0);
		if(key_num != 0)
			handle_key(key_num);
		
		//�Ӷ���pop�¼�����
		if(pop_event(&event))
		{
			parseEvent(event);
		}
		
		if((rs485State._flag == 0) && (sys_time._diff - rs485State._cmd_time >= 1))
		{
			if(pop_485_event(&event))
			{
				rs485State._index = event._index;
				rs485State._failed_num = 0;
				rs485State._stage = 0;
			
				handle_water_data(event._index);
			}
			
		}
		
		//��ʱ
		delay_ms(10);
	}
	
}
