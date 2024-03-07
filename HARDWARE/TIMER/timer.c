#include "timer.h"
#include "led.h"
#include "stm32f4xx_conf.h"
#include "protocol.h"
#include "liuqueue.h"
#include "sdhandle.h"
#include "handlewater.h"

unsigned char Timeout;
unsigned char recflag;
uint8_t TIM4_timeout;
uint8_t TIM2_timeout;
extern jxjs_time_t sys_time;
extern EC20_msg_t	EC20_msg;
extern platform_manager_t	platform_manager;
extern device_manager_t		device_manager;
extern hydrology_packet_t	hydrology_packet;
extern	uint8_t 	SDCardInitSuccessFlag;
extern EC20_send_data_t	EC20_Send_Data[6];
extern battery_data_t			battery_data;
extern uint8_t RS485Mode;
extern uint8_t SDFailedNums;
extern rs485_state_t rs485State;

//ͨ��RTC����
void RTC_Config_Jxjs(void)
{
	RTC_InitTypeDef RTC_InitStructure;
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
	PWR_BackupAccessCmd(ENABLE);//ʹ�ܱ��ݼĴ�������
#if 0
	RCC_LSICmd(ENABLE);//RCC_LSE_ON
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WaitForSynchro();
#endif
	
	if(RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x9999)   //һ������������RTC��ʼ��û
	{		
		RCC_LSEConfig(RCC_LSE_ON);//LSE ����    
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);		//����RTCʱ��(RTCCLK),ѡ��LSE��ΪRTCʱ��    
		RCC_RTCCLKCmd(ENABLE);	//ʹ��RTCʱ��		
		RTC_WaitForSynchro();
   
		RTC_WriteProtectionCmd(DISABLE);
 
		RTC_EnterInitMode();
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
		RTC_InitStructure.RTC_AsynchPrediv = 0x7F;//0x7F
		RTC_InitStructure.RTC_SynchPrediv = 0xFF;
		RTC_Init(&RTC_InitStructure);		
		
		RTC_TimeStructure.RTC_Seconds = 01;
		RTC_TimeStructure.RTC_Minutes = 27;
		RTC_TimeStructure.RTC_Hours = 15;
		RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
		RTC_SetTime(RTC_Format_BIN,&RTC_TimeStructure);
		
		RTC_DateStructure.RTC_Date = 15;
		RTC_DateStructure.RTC_Month = 11;
		RTC_DateStructure.RTC_WeekDay= RTC_Weekday_Wednesday;
		RTC_DateStructure.RTC_Year = 23;
		RTC_SetDate(RTC_Format_BIN,&RTC_DateStructure);
 
		RTC_ExitInitMode();
		RTC_WriteBackupRegister(RTC_BKP_DR0,0x9999);
		RTC_WriteProtectionCmd(ENABLE);
		RTC_WriteBackupRegister(RTC_BKP_DR0,0x9999);  //��ʼ����ɣ����ñ�־
	}
	PWR_BackupAccessCmd(DISABLE);
}


//ͨ�ö�ʱ��3�жϳ�ʼ��
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz
//����ʹ�õ��Ƕ�ʱ��3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///ʹ��TIM3ʱ��
	
	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//��ʼ��TIM3
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM3,DISABLE); //��ʹ�ܶ�ʱ��3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

void TIM4_Int_Init(u16 arr,u16 psc)
{		
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);  ///ʹ��TIM4ʱ��
	
	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);//��ʼ��TIM4
	
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM4,DISABLE); //��ʹ�ܶ�ʱ��3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM4_IRQn; //��ʱ��4�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM4_timeout = 0;
	
}

void TIM2_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  ///ʹ��TIM3ʱ��
	
	TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//��ʼ��TIM2
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //����ʱ��2�����ж�
	TIM_Cmd(TIM2,DISABLE); //��ʹ�ܶ�ʱ��2
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn; //��ʱ��2�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}	

uint8_t	is_leap_year(uint32_t year)
{
	if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
		return 1;
	return 0;
}
void sys_time_init(void)
{
	sys_time._year = 0;
	sys_time._month = 0;
	sys_time._day = 0;
	sys_time._hour = 0;
	sys_time._min = 0;
	sys_time._sec = 0;
	sys_time._diff = 0;
	sys_time._last_getwater_diff = 0;
	sys_time._last_send_diff = 0;
	sys_time._last_camera_diff = 0;
	sys_time._timing_state = TIMING_NO;
}

void get_sys_time(void)
{
	msg_event_t event;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	sys_time._diff++;
	
	if(sys_time._timing_state == TIMING_NO || sys_time._diff % 3600 == 0)
	{//δУʱʱ��Уʱһ�� ֮��ÿ�˸�Сʱ��Уʱһ��
		if(EC20_msg._ec20_haveinit == 1)
		{
			event._msg_type = EC20_EVENT_CCLK;
			push_ec20_event(event);
			sys_time._timing_state = TIMING_IN_PROGRESS;
		}				
	}
	
	RTC_GetTime(RTC_Format_BIN,&time);
	sys_time._hour = time.RTC_Hours;
	sys_time._min = time.RTC_Minutes;
	sys_time._sec = time.RTC_Seconds;
	
	RTC_GetDate(RTC_Format_BIN,&date);
	sys_time._year = 2000 + date.RTC_Year;
	sys_time._month = date.RTC_Month;
	sys_time._day = date.RTC_Date;
	sys_time._weekday = date.RTC_WeekDay;
}

static uint8_t check_save_time(uint8_t index)
{
	uint64_t timeStamp;
	//if(platform_manager._platforms[index]._enable == 0)
		//return 0;
	timeStamp = sys_time._hour * 3600 + sys_time._min * 60 + sys_time._sec;
	if((sys_time._diff - platform_manager._platforms[index]._last_save_time) >= 20 &&
		(timeStamp % platform_manager._platforms[index]._interval_time) < 5)
	{
		return 1;
	}
	return 0;
}

static void report_save_data_check()
{
	uint8_t i;
	uint16_t port;
	msg_event_t event;	
		
	for(i = 0;i < platform_manager._platform_count;i++)
	{
		//��ʱ�������ڵ��ڹ涨���ϱ�ʱ����ʱ ���
		if(check_save_time(i))
		{
			//���
			/*
			create_hydrology_packet(&hydrology_packet,&sys_time,&device_manager,&platform_manager,JXJS_CONTROL_FUNC_FLOW,i);
			if(platform_manager._platforms[EC20_msg._ec20_event._index]._address_type == 0x02)
			{
				port = 5055;
			}
			else if(platform_manager._platforms[EC20_msg._ec20_event._index]._address_type == 0x03)
			{
				port = 5069;
			}
			//��� ����֮����SD��
			make_hydrology_pack(EC20_Send_Data[i]._data,&(EC20_Send_Data[i]._size),&hydrology_packet,port);
			*/
			craeteHygPackPro1(&EC20_Send_Data[i],&platform_manager,&device_manager,&sys_time,i);
			//������
			if(SDCardInitSuccessFlag)
				SDPushHistoryData(&EC20_Send_Data[i],i);
			//���±���ʱ��
			platform_manager._platforms[i]._last_save_time = sys_time._diff;
			
			//push ��������
			if(EC20_msg._ec20_haveinit && EC20_msg._csq1 >= 12 && EC20_msg._csq1 <= 31)
			{	
				if(platform_manager._platforms[i]._protocol == 0x01)
				{//szy-2016
					event._msg_type = EC20_EVENT_SENDMSG;
				}
				else if(platform_manager._platforms[i]._protocol == 0x02)
				{//��й��̬����
					event._msg_type = EC20_EVENT_SEND_XIAXIE;
				}
				else
				{
					event._msg_type = EC20_EVENT_SENDMSG;
				}
				
				event._index = i;
				push_ec20_event(event);										
			}
	
			//�����ϱ�ʱ��
			platform_manager._platforms[i]._last_send_time = sys_time._diff;
		}
		else if(sys_time._diff > platform_manager._platforms[i]._last_send_time  
			&& (sys_time._diff - platform_manager._platforms[i]._last_send_time) >= 60 && SDCardInitSuccessFlag)
		{
			if(EC20_msg._ec20_haveinit && EC20_msg._csq1 >= 12 && EC20_msg._csq1 <= 31 && platform_manager._platforms[i]._cont_failed_num < 5)
			{
				if(getHistorySize(i) != 0)
				{
					if(platform_manager._platforms[i]._protocol == 0x01)
					{//szy-2016
						event._msg_type = EC20_EVENT_SENDMSG;
					}
					else if(platform_manager._platforms[i]._protocol == 0x02)
					{//��й��̬����
						event._msg_type = EC20_EVENT_SEND_XIAXIE;
					}
					else
					{	
						event._msg_type = EC20_EVENT_SENDMSG;
					}
					event._index = i;
					push_ec20_event(event);
				}				
			}
			
			//�����ϱ�ʱ��
			platform_manager._platforms[i]._last_send_time = sys_time._diff;
		}
	}

}

static void get_water_data_check(void)
{
	uint8_t i;
	msg_event_t event;
	uint8_t startTime;

	//�����ȡ����������
	for(i = 0;i < device_manager._device_count;i++)
	{
		if(device_manager._devices[i]._device_type == DEVICE_WATERLEVEL && device_manager._devices[i]._device_protocol == WATERLEVEL_PROTOCOL2)
		{
			startTime = 25;
		}
		else
		{
			startTime = 15;
		}
		
		if(sys_time._diff >= startTime)
		{
			if(sys_time._diff <= startTime + 5 || (sys_time._diff - device_manager._devices[i]._device_last_get_time) >= device_manager._devices[i]._device_get_time)
			{
				if((sys_time._diff - sys_time._last_camera_diff > 13) ||
					(sys_time._diff - sys_time._last_camera_diff >= 1 && sys_time._diff - sys_time._last_camera_diff <= 8))
				{
					//��ȡˮ���ݵ��¼����뵽������
					event._index = i;
					event._msg_type = MSG_GET_WATER;
					push_485_event(event);
					//ˢ�»�ȡʱ��
					device_manager._devices[i]._device_last_get_time = sys_time._diff;
				}
				
			}			
		}
	}
	
	//��⴫������ʱ
	if((rs485State._flag == 1) && (sys_time._diff - rs485State._cmd_time >= 3))
	{
		failedWaterData(rs485State._index);
	}
}

static void calculate_water_data(void)
{
	uint8_t i;
	for(i = 0;i < device_manager._device_count;i++)
	{
		if(device_manager._devices[i]._device_type == DEVICE_WATERLEVEL)
			device_manager._devices[i]._water_data._flow += device_manager._devices[i]._water_data._flowRate;
	}
}

static void CheckEC20StateToReboot(void)
{
	msg_event_t event;
	
	if(EC20_msg._ec20_haveinit == 0 || EC20_msg._csq1 < 12 || EC20_msg._csq1 > 31)
	{
		EC20_msg._ec20_stage._cmd_time = sys_time._diff;
		return;
	}

	if( (sys_time._diff - EC20_msg._ec20_stage._cmd_time) > 15 * 60 )
	{
		event._msg_type = MSG_REBOOT;
		push_event(event);
	}		
}

//��ʱ��2�жϷ�����
void TIM2_IRQHandler(void)
{
	msg_event_t event;
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET) //����ж�
	{	
		//500msû����������Ϊһ֡���ݽ���
		TIM2_timeout++;
		if(TIM2_timeout >= 5)
		{
			TIM2_timeout = 0;
			event._msg_type = MSG_READ_USART2;
			push_event(event);
			TIM_Cmd(TIM2, DISABLE);  //��ʹ��TIMx����	
		}
		
	}
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //����жϱ�־λ
}

//��ʱ��3�жϷ�����
void TIM3_IRQHandler(void)
{
	msg_event_t event;
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //����ж�
	{		
		Timeout++;     
		if(Timeout>=5)//50ms��û�����ݣ�����һ֡���ݽ���
		{
			Timeout=0;
			event._msg_type = MSG_READ_USART1;
			push_event(event);
		 
			TIM_Cmd(TIM3, DISABLE);  //��ʹ��TIMx����	
		}
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ
}

//��ʱ��4�жϷ�����
void TIM4_IRQHandler(void)
{
	msg_event_t event;
	uint8_t i;
	if(TIM_GetITStatus(TIM4,TIM_IT_Update)==SET) //����ж�
	{	
		//��ȡϵͳʱ��
		get_sys_time();
		
		//ˮλ���ݼ���
		calculate_water_data();
		
		//ȡˮλ�����߼�
		get_water_data_check();
		
		//ȡ�ź�ֵ
		if(EC20_msg._ec20_haveinit && sys_time._sec % 10 == 0 && EC20_msg._ec20_event._state != EC20_STATE_IN_PROGRESS)
		{
			event._msg_type = EC20_EVENT_CSQ;
			push_ec20_event(event);
		}
				
		//ÿһСʱ��һ������
		if(sys_time._diff > 0 && sys_time._diff % 3600 == 0)
		{
			event._msg_type = MSG_SAVE_DATA;
			push_event(event);
		}
		
		if(sys_time._diff > 0 && sys_time._diff % 10 == 0)
		{
			event._msg_type = MSG_GET_VOLTAGE;
			push_event(event);
		}
		
		//������������ ��һ��
		if(sys_time._diff == 3)
		{
			event._msg_type = MSG_SET_BLUETOOTH_NAME;
			push_event(event);
		}
		
		//��������
		report_save_data_check();
		
		//��Զ��ƽ̨��ͨ��
		if(sys_time._diff > 0 && sys_time._diff % 300 == 0 && EC20_msg._ec20_haveinit)
		{
			event._msg_type = EC20_EVENT_DATA_UPDATE;
			push_ec20_event(event);
		}
		
		//����Ƿ�EC20���̿�ס����ס������
		CheckEC20StateToReboot();
		
		//���û�г�ʼ���ɹ�SD�� �ͼ�����ʼSD��
		if(SDCardInitSuccessFlag == 0 && sys_time._diff % 60 == 0 && sys_time._diff != 0 && SDFailedNums < 3)
		{
			event._msg_type = MSG_INIT_SD;
			push_event(event);
		}
		
		//push ��Ҫ��ˢ�µ��¼�
		event._msg_type = MSG_FLUSH_LCD;
		push_event(event);
		event._msg_type = MSG_IWDF_FEED;
		push_event(event);
		event._msg_type = MSG_CHECK_EC20;
		push_event(event);
	}
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);  //����жϱ�־λ
}
