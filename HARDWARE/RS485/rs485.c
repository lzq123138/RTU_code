#include "sys.h"		    
#include "rs485.h"	 
#include "delay.h"
#include "stm32f4xx_conf.h"
extern unsigned char Timeout;
extern uint8_t RS485Mode;
//���ջ����� 	
uint8_t RS485_RX_BUF[128];  	//���ջ���,���64���ֽ�.
//���յ������ݳ���
uint8_t RS485_RX_CNT=0;   									 
//��ʼ��IO ����3
//bound:������	

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
	
	RCC_AHB1PeriphClockCmd(RS485_AHB_GPIO,ENABLE); //ʹ��GPIOAʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//ʹ��USART1ʱ��
	
  //����1���Ÿ���ӳ��
	GPIO_PinAFConfig(RS485_GPIOX,RS485_TX_AF,GPIO_AF_USART1); //GPIOB10����ΪUSART1
	GPIO_PinAFConfig(RS485_GPIOX,RS485_RX_AF,GPIO_AF_USART1); //GPIOB11����ΪUSART1
	//USART1    
    GPIO_InitStructure.GPIO_Pin = RS485_TX | RS485_RX; //GPIOA9��GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(RS485_GPIOX,&GPIO_InitStructure); //��ʼ��PA2��PA3
	//PA8���������485ģʽ����  
    GPIO_InitStructure.GPIO_Pin = RS485_RE; //GPIOB9
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //�������
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(RS485_GPIOX,&GPIO_InitStructure); //��ʼ��PA8
	//PA15 RS485ģʽ����
	GPIO_InitStructure.GPIO_Pin = RS485_MODE; //GPIOA15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//�ٶ�100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //�������
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(RS485_GPIOX,&GPIO_InitStructure); //��ʼ��PA15
   //USART3 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
    USART_Init(RS485_USART, &USART_InitStructure); //��ʼ������3
    USART_Cmd(RS485_USART, ENABLE);  //ʹ�ܴ��� 3
	USART_ClearFlag(RS485_USART, USART_FLAG_TC);
	USART_ITConfig(RS485_USART, USART_IT_RXNE, ENABLE);//���������ж�
	//Usart3 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����
	RS485_TX_EN=0;				//Ĭ��Ϊ����ģʽ
	RS485_MODE_EN=0;			//Ĭ��Ϊ��й��̬����
	RS485Mode = 0;
}

//RS485����len���ֽ�.
//buf:�������׵�ַ
//len:���͵��ֽ���(Ϊ�˺ͱ�����Ľ���ƥ��,���ｨ�鲻Ҫ����64���ֽ�)
void RS485_Send_Data(uint8_t *buf,uint8_t len)
{
	uint8_t t;
	RS485_TX_EN=1;			//����Ϊ����ģʽ
  	for(t=0;t<len;t++)		//ѭ����������
	{
		while(USART_GetFlagStatus(RS485_USART,USART_FLAG_TC)==RESET); //�ȴ����ͽ���		
		USART_SendData(RS485_USART,buf[t]); //��������
	}	 
	while(USART_GetFlagStatus(RS485_USART,USART_FLAG_TC)==RESET); //�ȴ����ͽ���		
	RS485_RX_CNT=0;	  
	RS485_TX_EN=0;				//����Ϊ����ģʽ	
}
//RS485��ѯ���յ�������
//buf:���ջ����׵�ַ
//len:���������ݳ���
void RS485_Receive_Data(uint8_t *buf,uint8_t *len)
{
	uint8_t rxlen=RS485_RX_CNT;
	uint8_t i=0;
	*len=0;				//Ĭ��Ϊ0
	delay_ms(10);		//�ȴ�10ms,��������10msû�н��յ�һ������,����Ϊ���ս���
	if(rxlen==RS485_RX_CNT&&rxlen)//���յ�������,�ҽ��������
	{
		for(i=0;i<rxlen;i++)
		{
			buf[i]=RS485_RX_BUF[i];	
		}		
		*len=RS485_RX_CNT;	//��¼�������ݳ���
		RS485_RX_CNT=0;		//����
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
	if(USART_GetITStatus(RS485_USART, USART_IT_RXNE) != RESET)//���յ�����
	{	 	
         TIM_Cmd(TIM3, ENABLE);
         Timeout=0;
         res =USART_ReceiveData(RS485_USART);//;��ȡ���յ�������USART3->DR
		if(RS485_RX_CNT<64)
		{
			RS485_RX_BUF[RS485_RX_CNT]=res;		//��¼���յ���ֵ
			RS485_RX_CNT++;						//������������1 
		} 
	}  											 
} 


