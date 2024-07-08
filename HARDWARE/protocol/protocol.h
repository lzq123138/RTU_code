#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "stdint.h"
/**********************************水文协议********************************************/
/*
控制域方向DIR
*/

#define	JXJS_CONTROL_DIR_DOWN  0
#define	JXJS_CONTROL_DIR_UP 1


/*
控制域拆分标志位
*/
#define	JXJS_CONTROL_NO_SPILT  0
#define	JXJS_CONTROL_HAS_SPILT 1


/*
控制域功能码
*/
#define	JXJS_CONTROL_FUNC_CMD  0				//down-->cmd up-->confirm
#define	JXJS_CONTROL_FUNC_RAIN 1				//雨量
#define	JXJS_CONTROL_FUNC_WATERLEVEL 2			//水位
#define	JXJS_CONTROL_FUNC_FLOW 3				//流量
#define	JXJS_CONTROL_FUNC_FLOWSPEED	4			//流速
#define	JXJS_CONTROL_FUNC_LOCKSITE 5			//闸位
#define	JXJS_CONTROL_FUNC_CAPACITY 6			//功率
#define	JXJS_CONTROL_FUNC_GASPRESSURE 7			//气压
#define	JXJS_CONTROL_FUNC_WINDSPEED 8			//风速
#define	JXJS_CONTROL_FUNC_WATERTEMPREATURE 9	//水温
#define	JXJS_CONTROL_FUNC_WATERQUALITY 10		//水质
#define	JXJS_CONTROL_FUNC_SOILWATERCONTENT 11	//土壤含水率
#define	JXJS_CONTROL_FUNC_EVAPORATIONCAPACITY 12 	//蒸发量
#define	JXJS_CONTROL_FUNC_WARNING 13			//报警或状态参数
#define	JXJS_CONTROL_FUNC_COMPREHENSIVE 14		//下行-->综合参数 上行-->统计雨量
#define	JXJS_CONTROL_FUNC_WATERPRESSURE 15		//水压
//关于位的分布
#define	JXJS_CONTROL_FUNC_CMD_BIT  				0x0001			//down-->cmd up-->confirm
#define	JXJS_CONTROL_FUNC_RAIN_BIT				0x0002			//雨量
#define	JXJS_CONTROL_FUNC_WATERLEVEL_BIT 		0x0004			//水位
#define	JXJS_CONTROL_FUNC_FLOW_BIT 				0x0008			//流量
#define	JXJS_CONTROL_FUNC_FLOWSPEED_BIT			0x0010			//流速
#define	JXJS_CONTROL_FUNC_LOCKSITE_BIT 			0x0020			//闸位
#define	JXJS_CONTROL_FUNC_CAPACITY_BIT 			0x0040			//功率
#define	JXJS_CONTROL_FUNC_GASPRESSURE_BIT 		0x0080			//气压
#define	JXJS_CONTROL_FUNC_WINDSPEED_BIT 		0x0100			//风速
#define	JXJS_CONTROL_FUNC_WATERTEMPREATURE_BI 	0x0400			//水质
#define	JXJS_CONTROL_FUNC_SOILWATERCONTENT_BIT 	0x0800			//土壤含水率
#define	JXJS_CONTROL_FUNC_EVAPORATIONCAPACITY_BIT 0x1000 		//蒸发量
#define	JXJS_CONTROL_FUNC_WARNING_BIT 			0x2000			//报警或状态参数
#define	JXJS_CONTROL_FUNC_COMPREHENSIVE_BIT 	0x4000			//下行-->综合参数 上行-->统计雨量
#define	JXJS_CONTROL_FUNC_WATERPRESSURE_BIT 	0x8000			//水压

/*
应用层功能码AFN
*/
#define	JXJS_APP_FUNC_LINK_DETECTION  0x02								//链路检测
#define	JXJS_APP_FUNC_SET_TERMINAL_ADDRESS  0x10						//设置遥测终端、中继站地址
#define	JXJS_APP_FUNC_SET_TERMINAL_CLOCK  0x11							//设置遥测终端、中继站时钟
#define	JXJS_APP_FUNC_SET_TERMINAL_WORKMODE  0x12						//设置遥测终端工作模式 
#define	JXJS_APP_FUNC_SET_TERMINAL_RECHARGE_AMOUNT  0x15				//设置遥测终端本次充值量
#define	JXJS_APP_FUNC_SET_TERMINAL_WATER_WARNING  0x16					//设置遥测终端剩余水量报警值
#define	JXJS_APP_FUNC_SET_TERMINAL_WATER_LEVEL_LIMIT  0x17				//设置遥测终端的水位基值、水位上下限
#define	JXJS_APP_FUNC_SET_TERMINAL_WATER_PRESSURE_LIMIT  0x18			//设置遥测终端水压上、下限
#define	JXJS_APP_FUNC_SET_TERMINAL_WATER_QUALITY_UPLIMIT  0x19			//设置遥测终端水质参数种类、上限值
#define	JXJS_APP_FUNC_SET_TERMINAL_WATER_QUALITY_DOWNLIMIT  0x1A		//设置遥测终端水质参数种类、下限值
#define	JXJS_APP_FUNC_SET_TERMINAL_WATER_INIT  0x1B						//设置终端站水量的表底（初始）值
#define	JXJS_APP_FUNC_SET_TERMINAL_FORWARDING_CODE_LENGTH  0x1C			//设置遥测终端转发中继引导码长
#define	JXJS_APP_FUNC_SET_FORWARDING_TERMINAL_ADDRESS  0x1D				//设置中继站转发终端地址
#define	JXJS_APP_FUNC_SET_STATION_SELF_REPORT  0x1E						//设置中继站工作机自动切换，自报状态
#define	JXJS_APP_FUNC_SET_TERMINAL_FLOW_UP  0x1F						//设置遥测终端流量参数上限值
#define	JXJS_APP_FUNC_SET_TREMINAL_TIMEOUT  0x20						//设置遥测终端检测参数启报阈值及固态存储时间段间隔
#define	JXJS_APP_FUNC_SET_TERMINAL_IC_VALID  0x30						//置遥测终端 IC 卡功能有效
#define	JXJS_APP_FUNC_CANCEL_TERMINAL_IC  0x31							//取消遥测终端 IC 卡功能
#define	JXJS_APP_FUNC_FIXEDVALUE_CONTROL_INPUT  0x32					//定值控制投入
#define	JXJS_APP_FUNC_FIXEDVALIUE_CONTROL_EXIT  0x33					//定值控制退出
#define	JXJS_APP_FUNC_SET_FIXEDVALIUE  0x34								//定值量设定
#define	JXJS_APP_FUNC_QUERY_TERMINAL_ADDRESS  0x50						//查询遥测终端、中继站地址
#define	JXJS_APP_FUNC_QUERY_TERMINAL_CLOCK  0x51						//查询遥测终端、中继站时钟
#define	JXJS_APP_FUNC_QUERY_TERMINAL_WORKMODE  0x52						//查询遥测终端工作模式
#define	JXJS_APP_FUNC_QUERY_TERMINAL_TYPE_TIMEOYT  0x53					//查询遥测终端的数据自报种类及时间间隔
#define	JXJS_APP_FUNC_QUERY_TERMINAL_NEED_TYPE  0x54					//查询遥测终端需查询的实时数据种类
#define	JXJS_APP_FUNC_QUERY_TERMINAL_RECHARG  0x55						//查询遥测终端最近成功充值量和现有剩余水量
#define	JXJS_APP_FUNC_QUERY_TERMINAL_WATER_WARNING  0x56				//查询遥测终端剩余水量和报警值
#define	JXJS_APP_FUNC_QUERY_TERMINAL_WATER_LEVEL_LIMIT  0x57			//查询遥测终端水位基值、水位上下限
#define	JXJS_APP_FUNC_QUERY_TERMINAL_WATER_PRESSURE_LIMIT  0x58			//查询遥测终端水压上、下限
#define	JXJS_APP_FUNC_QUERY_TERMINAL_WATER_QUALITY_UPLIMIT  0x59		//查询遥测终端水质参数种类、上限值
#define	JXJS_APP_FUNC_QUERY_TERMINAL_WATER_QUALITY_DOWNLIMIT  0x5A		//查询遥测终端水质参数种类、下限值
#define	JXJS_APP_FUNC_QUERY_TERMINAL_EVENT_RECORD  0x5D					//查询遥测终端的事件记录
#define	JXJS_APP_FUNC_QYERY_TERMINAL_STATU  0x5E						//查询遥测终端状态和报警状态
#define	JXJS_APP_FUNC_QUERY_PUMPMOTOR_REALTIME_WORKDATA  0x5F			//查询水泵电机实时工作数据
#define	JXJS_APP_FUNC_QUERY_TERMINAL_FORWARDING_CODE_LENGTH  0x60		//查询遥测终端转发中继引导码长
#define	JXJS_APP_FUNC_QUERY_TERMINAL_PHOTO_RECORD  0x61					//查询遥测终端图像记录
#define	JXJS_APP_FUNC_QUERY_STATION_ADDRESS  0x62						//查询中继站转发终端地址
#define	JXJS_APP_FUNC_QUERY_STATION_WORKSTATU  0x63						//查询中继站工作机状态和切换记录
#define	JXJS_APP_FUNC_QUERY_TERMINAL_FLOW_UP  0x64						//查询遥测终端流量参数上限值
#define	JXJS_APP_FUNC_RANDOM_DATA  0x81									//随机自报报警数据
#define	JXJS_APP_FUNC_MANUAL_SET  0x82									//人工置数
#define	JXJS_APP_FUNC_RESET_TERMINAL  0x90								//复位遥测终端参数和状态
#define	JXJS_APP_FUNC_EMPTY_TERMINAL_DATA  0x91							//清空遥测终端历史数据单元
#define	JXJS_APP_FUNC_REMOTE_START_PUMP  0x92							//遥控启动水泵或阀门/闸门
#define	JXJS_APP_FUNC_REMOTE_STOP_PUMP  0x93							//遥控关闭水泵或阀门/闸门
#define	JXJS_APP_FUNC_REMOTE_TERMINAL_SWITCH_COMM  0x94					//遥控终端或中继站通信机切换
#define	JXJS_APP_FUNC_REMOTE_STATION_SWITCH_WORK  0x95					//遥控中继站工作机切换
#define	JXJS_APP_FUNC_REMOTE_TERMINAL_CHANGE_PW  0x96					//修改遥测终端密码
#define	JXJS_APP_FUNC_SET_TERMINAL_NEED_QUERY_TYPE  0xA0				//设置遥测站需查询的实时数据种类
#define	JXJS_APP_FUNC_SET_TERMINAL_REPORT_TYPE_TIMEOUT  0xA1			//设置遥测终端的数据自报种类及时间间隔
#define	JXJS_APP_FUNC_QUERY_TERMINAL_REALTIME_VALUE  0XB0				//查询遥测终端实时值
#define	JXJS_APP_FUNC_QUERY_TERMINAL_SOILD_DATA  0xB1					//查询遥测终端固态存储数据
#define	JXJS_APP_FUNC_QUERY_TERMINAL_SELF_REPORT_DATA  0xB2				//查询遥测终端内存自报数据
#define	JXJS_APP_FUNC_TERMINAL_SELF_REPORT_DATA  0xC0					//遥测终端自报实时数据

typedef struct
{
	uint8_t			_index;//平台编号
	//控制域
	uint8_t			_control_dir;//控制域方向dir
	uint8_t 		_control_spilt;//控制域拆分标志位
	uint8_t 		_control_fcb;//控制域帧计数位FCB
	uint8_t			_control_func;//控制域功能码
	//地址域
	uint32_t 		_address_a1;//地址域行政区划码A1
	uint16_t 		_address_a2;//地址域测站地址A2
	//应用层
	uint8_t 		_application_func;//应用层功能码AFN
	uint8_t			_user_data[128];//应用层数据域
	uint8_t 		_user_data_length;//应用层数据域长度
	//附加信息域
	uint16_t 		_app_pw;//应用层附加信息域(aux) 密钥
	uint8_t 		_app_day;
	uint8_t 		_app_hour;
	uint8_t 		_app_min;
	uint8_t			_app_sec;
	uint8_t 		_app_delay;//允许延误时长
} hydrology_packet_t;

uint8_t crcCalc(unsigned char *data,unsigned int dataLen);
uint16_t modBusCRC16(uint8_t *data,uint16_t dataSize);
uint32_t crc32_in_c(uint8_t *p, int32_t len);

uint8_t CheckDataLegality(uint8_t *data,uint32_t dataSize,uint8_t mode);

float Hex2Float(uint8_t *data);
uint8_t jxjs_double2data(double f1,uint8_t *data,uint8_t datasize);
double jxjs_data2double(uint8_t *data);
uint16_t append_byte4_to_data(uint8_t *data,uint32_t byte,uint16_t size);
uint8_t create_waterLevel_data(uint8_t *data,uint8_t *dataLen,uint32_t ullage_mm);
uint8_t make_hydrology_pack(unsigned char *data,uint32_t *dataLen,hydrology_packet_t *packet,uint16_t port);

uint8_t byteToBcd(uint8_t byte);
uint16_t byte16ToBcd(uint16_t byte16);
uint32_t bcdToByte(uint32_t bcd);

uint8_t get_address_bytes(uint8_t *addressBytes,uint8_t bytesLength,uint32_t a1,uint16_t a2,uint16_t port);


/********************************************EC20状态****************************************************************/
//stage
#define EC20_NULL		0x00
#define EC20_AT 		0x01
#define EC20_ATE0		0x02
#define EC20_ATCSQ		0x03
#define EC20_ATI		0x04
#define EC20_ATZU		0xff
#define EC20_ATCPIN 	0x05
#define EC20_ATCREG		0x06
#define EC20_ATCGREG	0x07
#define EC20_ATCOPS		0x08
#define EC20_ATQICLOSE	0x09
#define	EC20_ATQICSGP	0x0a
#define EC20_ATQIDEACT	0x0b
#define EC20_ATQIACT	0x0c
#define	EC20_QIOPEN		0x0d
#define EC20_ATCCLK		0x0e
#define EC20_GPSLOC		0x0f

#define EC20_QISEND			0x20
#define EC20_QIWAITREPLY	0x21

#define EC20_CONNECT	0x30
#define EC20_SEND_LOCAL				0x31
#define EC20_SEND_PLATFORM			0x32
#define EC20_SEND_DEVICE_REAL		0x33
#define EC20_SEND_SUCCESS			0x34
#define EC20_SEND_FAILED			0x35
#define EC20_SEND_UP_SUCCESS		0x36
#define EC20_SEND_UP_FAILED			0x37
#define EC20_SEND_REBOOT_SUCCESS	0x38
#define EC20_SEND_REBOOT_FAILED		0x39
#define EC20_WAIT_SERVER			0x40
#define EC20_QUIT_TRANS				0x41
#define EC20_DISCONNECT				0x42

#define EC20_CONNECT_XIAXIE			0x50
#define EC20_WAIT_XIAXIE			0x51
//state
#define EC20_STATE_WAIT_START	0x00
#define EC20_STATE_IN_PROGRESS	0x01
#define EC20_STATE_FINISHED		0x02

//res
#define EC20_RES_FAILED		0x00
#define EC20_RES_SUCCESS	0x01

//event type
#define EC20_EVENT_NULL		0x00
#define EC20_EVENT_INIT		0x01
#define	EC20_EVENT_CSQ		0x02
#define	EC20_EVENT_CCLK		0x03
#define	EC20_EVENT_SENDMSG	0x04

#define EC20_EVENT_DATA_UPDATE	0x05
#define EC20_EVENT_SEND_XIAXIE	0x06
#define EC20_EVENT_GPS		0x07
typedef struct
{
	uint8_t		_type;
	uint8_t		_state;
	uint8_t		_res;
	uint8_t		_time_num;
	uint8_t		_failed_num;
	uint8_t		_max_failed_num;
	uint8_t		_index;//部分功能使用
	uint64_t	_cmd_time;//命令发送的时间
}EC20_handle_t;

typedef struct
{
	EC20_handle_t	_ec20_stage;
	EC20_handle_t	_ec20_event;
	uint8_t			_ec20_haveinit; 	//是否初始化完成 0-->no 1-->yes
	uint8_t			_restart_num;		//EC20模块重启次数
	uint8_t			_csq1;
	uint8_t			_csq2;
}EC20_msg_t;


typedef struct
{
	uint32_t 	_size;
	uint8_t		_sector_num;
	uint8_t 	_current_sector;
	uint8_t		_sector_size[8];//最多存8段数据
	uint8_t		_data[400];
}EC20_send_data_t;


uint8_t data_fcb_sub(uint8_t *data,uint32_t dataSize,uint32_t fcbIndex,uint8_t bitIndex);
uint8_t EC20DataFcb(EC20_send_data_t *pack,uint32_t fcbIndex,uint8_t bitIndex);
/********************************************系统时间************************************************************/
#define TIMING_NO 			0x01
#define TIMING_IN_PROGRESS	0x02
#define	TIMING_SUCCESS		0x10
#define TIMING_FAILED		0x11

typedef struct
{
	uint32_t	_year;
	uint8_t		_month;
	uint8_t		_day;
	uint8_t		_weekday;
	uint8_t		_hour;
	uint8_t		_min;
	uint8_t		_sec;
	uint64_t	_diff;
	uint64_t	_last_getwater_diff;
	uint64_t	_last_send_diff;
	
	uint64_t	_last_camera_diff;
	//是否校时
	uint8_t		_timing_state;//校时状态
	uint8_t		_timing_num;//允许连续校时失败次数
}jxjs_time_t;

uint8_t getWeekdayByYearDay(uint32_t year,uint8_t month,uint8_t day);
void UTCToBeijing(jxjs_time_t* time);


/**********************************水位计算***************************************************************************/
#define RECT_CANAL 		0x01
#define CIR_CANAL		0x02
#define U_CANAL			0x03
#define DOUBLE_NUMS		10

#define FORMULA_NONE	0x02
#define FORMULA_XIECAI	0x01

typedef struct
{
	uint8_t _type;		//明渠类型
	double	_deep_water;//积水深
	double	_n;			//粗糙度
	double	_J;			//水力坡度 -->比降
	double	_topwidth;	//顶宽
	double	_bottomwidth;//底宽
	double 	_insheight;	//安装高度
	uint16_t _insheight_mm;
	double	_canheight; //渠高
	double	_r;			//圆形渠半径
	double	_ur;		//U形渠半径
	double	_a;			//U形渠外倾角
	double	_H;			//U形渠深度
	uint8_t	_range;		//量程
}water_param_t;

void init_waterParam(water_param_t *param);
uint8_t Chezy_formula(water_param_t param,double airHeight,double *flowSpeed,double *flowRate);

typedef struct
{
	double 		_flow;		//累计流量
	double		_flowspeed;	//流速 m/s
	double		_flowRate;	//瞬时流量 m3/s
	double		_waterlevel;//水位高度 m
	double		_flow_backup;
	double		_flow_change_backup;
	double		_airHeight;
}jxjs_water_data_t;

void init_waterData(jxjs_water_data_t *data);


typedef struct
{
	uint16_t	_wordlength;
	uint16_t	_stopbit;
	uint16_t	_parity;
	uint32_t	_bound;
}device_uart_config_t;
void initDeviceUartConfig(device_uart_config_t *config);

uint8_t checkDeviceUartConfigIsEqual(device_uart_config_t *config1,device_uart_config_t *config2);

/********************************************设备****************************************************************/
#define DEVICE_WATERLEVEL		0x01
#define	DEVICE_FLOWMETER 		0x02
#define DEVICE_FLOWSPEED		0x03

#define WATERLEVEL_PROTOCOL1	0x01//古大雷达
#define WATERLEVEL_PROTOCOL2	0x02//4-20 ma

#define FLOWMETER_PROTOCOL1		0x01 //施耐德电磁流量计
#define FLOWMETER_PROTOCOL2		0x02 //大连海峰超声波流量计
#define FLOWMETER_PROTOCOL3		0x03 //安布雷拉超声波流量计
#define FLOWMETER_PROTOCOL4		0x04 //安布雷拉电磁流量计
#define FLOWMETER_PROTOCOL5		0x05 //上海肯特电磁流量计
#define FLOWMETER_PROTOCOL6		0x06 //建恒DCT1158 超声波流量计
#define FLOWMETER_PROTOCOL7		0x07 //百江通 超声波流量计
#define FLOWMETER_PROTOCOL8		0x08 //新乡华隆 电磁流量计
#define	FLOWMETER_JINYI			0x09 //京仪电磁流量计
#define	FLOWMETER_JDLY			0x0A //沈阳佳德联益超声波流量计
#define FLOWMETER_HZZH			0x0B //杭州振华

#define	FLOWSPEED_MSYX1			0x01 //迈时永信流速仪
#define FLOWSPEED_BYD			0x02 //博意达流速流量计

#define MAX_DEVICE_NUM	4
typedef struct
{
	uint8_t 			_device_type;
	uint8_t				_device_protocol;
	uint8_t 			_device_number;
	uint8_t				_device_com_state;
	uint8_t				_failed_num;
	uint16_t			_device_get_time;
	uint8_t				_device_formula;
	uint64_t			_device_last_get_time;
	water_param_t		_water_param;
	jxjs_water_data_t	_water_data;
	device_uart_config_t	_uart_config;
}jxjs_device_t;
void init_deviceStruct(jxjs_device_t *device);

typedef struct
{
	uint8_t			_device_count;
	jxjs_device_t	_devices[8];
}device_manager_t;
void init_device_manager(device_manager_t	*manager);
/*********************************TCP*********************************************************************************/
//定义站点类型
#define JXJS_PLATFORM_PROVINCE 0x01
#define JXJS_PLATFORM_DONG 0x02
#define JXJS_PLATFORM_SELF 0x03

#define RESPONSE_MODE		0x01 	//应答模式
#define	NO_RESPONSE_MODE 	0x00	//非应答模式

#define IPTYPE_IPV4			0x01
#define IPTYPE_IPV6			0x00

#define DATATYPE_FLOW_BEHIND 	0x01
#define DATATYPE_FLOW_IN_FRONT 	0x02

#define ADDRESSTYPE_NORMAL			0x01
#define ADDRESSTYPE_EXCHANGE		0x02
#define ADDRESSTYPE_CONVERT_FORMAT	0x03

#define PROTOCOL_WATER2016	0x01
#define PROTOCOL_ECOLOGICAL 0x02


#define MAX_PLAT_NUM		6
/*
 *use ipv4
 *ex: 192.168.100.4:48500 ip1:192 ip2:168 ip3:100 ip4:4 port:48500
*/
typedef struct
{
	uint8_t		_type;
	uint8_t		_mode;
	uint8_t		_enable;
	uint8_t		_ip_type;
	uint8_t 	_connect_state;//0x00 还未连接，0x01 连接成功，0x02连接失败 
	uint8_t		_sendFlag;
	uint8_t 	_ip1;
	uint8_t 	_ip2;
	uint8_t 	_ip3;
	uint8_t 	_ip4;
	uint16_t 	_port;
	uint8_t		_ipv6[16];
	uint8_t		_protocol;//水文规约
	uint8_t		_address_type;//地址类型
	uint8_t		_data_type;
	uint16_t 	_send_data_type; //发送数据类型
	uint32_t	_interval_time;
	uint32_t	_last_send_time;
	uint32_t	_last_save_time;
	uint16_t    _cont_failed_num;
}jxjs_platform_t;
void init_platform(jxjs_platform_t	*platform);

typedef struct
{
	uint8_t				_platform_count;
	uint32_t			_platform_number_a1;
	uint16_t			_platform_number_a2;
	uint8_t				_device_number[7];
	uint8_t				_ipv4[4];//远程管理平台ip
	uint16_t 			_port;//远程管理平台端口
	jxjs_platform_t		_platforms[8];
}platform_manager_t;
void init_platform_manager(platform_manager_t	*manager);
/**************************************************消息队列***************************************************************************/
#define MSG_READ_USART2		0x01
#define MSG_READ_USART3		0x02
#define MSG_READ_USART1		0x03

#define MSG_GET_WATER		0x10
#define MSG_SEND_EC20		0x11
#define	MSG_CHECK_EC20		0x12
#define MSG_FLUSH_LCD		0x13
#define	MSG_INIT_SD			0x14
#define MSG_SAVE_DATA		0x15
#define MSG_REBOOT_EC20		0x16

#define MSG_IWDF_FEED		0x20

#define MSG_SET_BLUETOOTH_NAME 0x30

#define MSG_REBOOT			0x40

#define MSG_GET_VOLTAGE		0x50

typedef struct
{
	uint8_t _msg_type;
	uint8_t	_index;//部分事件扩展使用
}msg_event_t;


typedef struct
{
	uint8_t _flag;
	uint8_t _index;
	uint8_t _stage;
	uint8_t _failed_num;
	uint8_t _last_res;
	uint64_t _cmd_time;
}
rs485_state_t;
void initRs485State(rs485_state_t *state);

/*********************************************显示屏*******************************************************************************************/
#define MENU_MAIN 			0x01

#define MENU_LEVEL_2		0x10
#define MENU_DEVICE			0x10
#define MENU_PLATFORM		0x11
#define MENU_RTU			0x12


#define MENU_DEVICE_DATA	0x20
#define MENU_PLATFORM_DATA	0x21



#define	LCD_BACK_OFF	0x00
#define LCD_BACK_ON		0x01

typedef struct
{
	uint8_t		_menu;
	uint8_t		_menuIndex;
	uint8_t		_lineTop;
	uint8_t		_currentLine;
	uint8_t		_currentLineHeight;
	uint64_t	_lastBackOnTime;
	uint8_t		_backState;
}LCD_msg_t;
void init_lcd_msg(LCD_msg_t *msg);


typedef struct
{
	uint8_t		_index;
	char		_data[17];
}LCD_data_t;


/****************************************************电池数据************************************************************************************/
typedef struct
{
	uint8_t		_version;
	uint8_t		_power_mode;	//外部供电方式
	uint8_t		_have_power;	//是否有外部供电
	uint8_t		_voltage;		//电池电压
	
	uint8_t		_extend_data[8];//扩展数据
}battery_data_t;
void init_battery_data(battery_data_t *data);

/***************************************************最终函数**************************************************************************************/
uint8_t get_time_tag_pack(uint8_t *timeTags,uint8_t len,uint8_t dd,uint8_t hh,uint8_t mm,uint8_t ss,uint8_t delay);
uint16_t hexdata2hexstring(uint8_t *data,uint32_t dataSize,unsigned char *string,uint32_t stringSize);
uint8_t create_flow_data(uint8_t *data,device_manager_t *manager,uint8_t data_type);
uint8_t	create_user_data(uint8_t *data,uint8_t *size,uint8_t maxsize,device_manager_t manager,uint8_t func);
void create_hydrology_packet(hydrology_packet_t *packet,jxjs_time_t *time,device_manager_t *manager,platform_manager_t *platform_manager,uint8_t func,uint8_t index);


uint16_t stringToUint16(uint8_t* data,uint16_t *index);

uint32_t stringToUint32(uint8_t* data,uint16_t *index);

//读byte_count字节的数组 赋值给uint32 并做下标移动
uint32_t stringToUint32ByBytes(uint8_t* data,uint16_t *index,uint8_t byte_count);

void appendUint16ToData(uint8_t *data,uint16_t *index,uint16_t halfByte);

void appendUint32ToData(uint8_t *data,uint16_t *index,uint32_t byte);

uint8_t craeteHygPackPro1(EC20_send_data_t *pack,platform_manager_t *pmanager,device_manager_t *dmanager,jxjs_time_t *time,uint8_t index);
#endif
