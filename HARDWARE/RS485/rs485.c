#include "sys.h"		    
#include "rs485.h"	 
#include "delay.h"
#include "stm32f4xx_conf.h"
extern unsigned char Timeout;
extern uint8_t RS485Mode;
//接收缓存区 	
uint8_t RS485_RX_BUF[128];  	//接收缓冲,最大64个字节.
//接收到的数据长度
uint8_t RS485_RX_CNT=0;   									 
//初始化IO 串口3
//bound:波特率	

#define RS485_USART		USART1
#define RS485_AHB_GPIO 	RCC_AHB1Periph_GPIOA
#define RS485_GPIOX		GPIOA
#define RS485_TX		GPIO_Pin_9
#define	RS485_RX		GPIO_Pin_10
#define RS485_TX_AF		GPIO_PinSource9
#define	RS485_RX_AF		GPIO_PinSource10
#define RS485_RE		GPIO_Pin_8
#define RS485_MODE	    GPIO_Pin_11

void RS485_Init(uint32_t bound)
{  	 
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RS485_AHB_GPIO,ENABLE); //使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟
	
  //串口1引脚复用映射
	GPIO_PinAFConfig(RS485_GPIOX,RS485_TX_AF,GPIO_AF_USART1); //GPIOB10复用为USART1
	GPIO_PinAFConfig(RS485_GPIOX,RS485_RX_AF,GPIO_AF_USART1); //GPIOB11复用为USART1
	//USART1    
    GPIO_InitStructure.GPIO_Pin = RS485_TX | RS485_RX; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(RS485_GPIOX,&GPIO_InitStructure); //初始化PA2，PA3
	//PA8推挽输出，485模式控制  
    GPIO_InitStructure.GPIO_Pin = RS485_RE; //GPIOB9
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(RS485_GPIOX,&GPIO_InitStructure); //初始化PA8
	//PA15 RS485模式控制
	GPIO_InitStructure.GPIO_Pin = RS485_MODE; //GPIOA15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(RS485_GPIOX,&GPIO_InitStructure); //初始化PA15
   //USART3 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(RS485_USART, &USART_InitStructure); //初始化串口3
    USART_Cmd(RS485_USART, ENABLE);  //使能串口 3
	USART_ClearFlag(RS485_USART, USART_FLAG_TC);
	USART_ITConfig(RS485_USART, USART_IT_RXNE, ENABLE);//开启接受中断
	//Usart3 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
	RS485_TX_EN=0;				//默认为接收模式
	RS485_MODE_EN=0;			//默认为下泄生态流量
	RS485Mode = 0;
}

//RS485发送len个字节.
//buf:发送区首地址
//len:发送的字节数(为了和本代码的接收匹配,这里建议不要超过64个字节)
void RS485_Send_Data(uint8_t *buf,uint8_t len)
{
	uint8_t t;
	RS485_TX_EN=1;			//设置为发送模式
  	for(t=0;t<len;t++)		//循环发送数据
	{
		while(USART_GetFlagStatus(RS485_USART,USART_FLAG_TC)==RESET); //等待发送结束		
		USART_SendData(RS485_USART,buf[t]); //发送数据
	}	 
	while(USART_GetFlagStatus(RS485_USART,USART_FLAG_TC)==RESET); //等待发送结束		
	RS485_RX_CNT=0;	  
	RS485_TX_EN=0;				//设置为接收模式	
}
//RS485查询接收到的数据
//buf:接收缓存首地址
//len:读到的数据长度
void RS485_Receive_Data(uint8_t *buf,uint8_t *len)
{
	uint8_t rxlen=RS485_RX_CNT;
	uint8_t i=0;
	*len=0;				//默认为0
	delay_ms(10);		//等待10ms,连续超过10ms没有接收到一个数据,则认为接收结束
	if(rxlen==RS485_RX_CNT&&rxlen)//接收到了数据,且接收完成了
	{
		for(i=0;i<rxlen;i++)
		{
			buf[i]=RS485_RX_BUF[i];	
		}		
		*len=RS485_RX_CNT;	//记录本次数据长度
		RS485_RX_CNT=0;		//清零
	}
}

void switchRS485Mode(uint8_t mode)
{
	if(mode != 0 && mode != 1)
		return;	
	
	RS485Mode = mode;
	if(mode == 0)
		RS485_MODE_EN=0;
	else
		RS485_MODE_EN=1;
	RS485_RX_CNT = 0;
}

 
void USART1_IRQHandler(void)
{
	uint8_t res;	    
	if(USART_GetITStatus(RS485_USART, USART_IT_RXNE) != RESET)//接收到数据
	{	 	
         TIM_Cmd(TIM3, ENABLE);
         Timeout=0;
         res =USART_ReceiveData(RS485_USART);//;读取接收到的数据USART3->DR
		if(RS485_RX_CNT<64)
		{
			RS485_RX_BUF[RS485_RX_CNT]=res;		//记录接收到的值
			RS485_RX_CNT++;						//接收数据增加1 
		} 
	}  											 
} 


