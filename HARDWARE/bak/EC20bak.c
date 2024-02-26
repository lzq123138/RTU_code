#include "ec20.h"
#include "stdlib.h"
#include "string.h"
#include "usart.h"	
#include "led.h"	
char *strx,*extstrx,*Readystrx;
extern 	char RxBuffer[200],Rxcouter;
	void Clear_Buffer(void)//��ջ���
{
		u8 i;
		Uart1_SendStr(RxBuffer);
		for(i=0;i<Rxcouter;i++)
		RxBuffer[i]=0;//����
		Rxcouter=0;
	//	IWDG_Feed();//ι��
}
	void  EC20_Init(void)
{
		printf("AT\r\n"); 
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"OK");//����OK
		while(strx==NULL)
		{
            
            
            
				Clear_Buffer();	
				printf("AT\r\n"); 
				delay_ms(500);
				strx=strstr((const char*)RxBuffer,(const char*)"OK");//����OK
		}
		printf("ATE0\r\n"); //�رջ���
		delay_ms(500);
		Clear_Buffer();	
		printf("AT+CSQ\r\n"); //���CSQ
		delay_ms(500);
		printf("ATI\r\n"); //���ģ��İ汾��
		delay_ms(500);
		/////////////////////////////////
		printf("AT+CPIN?\r\n");//���SIM���Ƿ���λ
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"+CPIN: READY");//�鿴�Ƿ񷵻�ready
		while(strx==NULL)
		{
				Clear_Buffer();
				printf("AT+CPIN?\r\n");
				delay_ms(500);
				strx=strstr((const char*)RxBuffer,(const char*)"+CPIN: READY");//���SIM���Ƿ���λ���ȴ�����λ�������ʶ�𲻵���ʣ��Ĺ�����û������
		}
		Clear_Buffer();	
	
		/////////////////////////////////////
		printf("AT+CGREG?\r\n");//�鿴�Ƿ�ע��GPRS����
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,1");//��������Ҫ��ֻ��ע��ɹ����ſ��Խ���GPRS���ݴ��䡣
		extstrx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,5");//��������������
		while(strx==NULL&&extstrx==NULL)
		{
				Clear_Buffer();
				printf("AT+CGREG?\r\n");//�鿴�Ƿ�ע��GPRS����
				delay_ms(500);
				strx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,1");//��������Ҫ��ֻ��ע��ɹ����ſ��Խ���GPRS���ݴ��䡣
				extstrx=strstr((const char*)RxBuffer,(const char*)"+CGREG: 0,5");//��������������
		}
		Clear_Buffer();
		printf("AT+COPS?\r\n");//�鿴ע�ᵽ�ĸ���Ӫ�̣�֧���ƶ� ��ͨ ���� 
		delay_ms(500);
		Clear_Buffer();
		printf("AT+QICLOSE=0\r\n");//�ر�socket����
	  delay_ms(500);
		Clear_Buffer();
		printf("AT+QICSGP=1,1,\042CMNET\042,\042\042,\042\042,0\r\n");//����APN�����û���������
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"OK");//�����ɹ�
		while(strx==NULL)
		{
				delay_ms(500);
				strx=strstr((const char*)RxBuffer,(const char*)"OK");////�����ɹ�
		}
		Clear_Buffer();
		printf("AT+QIDEACT=1\r\n");//ȥ����
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"OK");//�����ɹ�
		while(strx==NULL)
		{
				delay_ms(500);
				strx=strstr((const char*)RxBuffer,(const char*)"OK");//�����ɹ�
		}
		Clear_Buffer();
		printf("AT+QIACT=1\r\n");//����
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"OK");//�����ɹ�
		while(strx==NULL)
		{
				delay_ms(500);
				strx=strstr((const char*)RxBuffer,(const char*)"OK");//�����ɹ�
		}
		Clear_Buffer();
		printf("AT+QIACT?\r\n");//��ȡ��ǰ����IP��ַ
			delay_ms(500);
				Clear_Buffer();
		printf("AT+QIOPEN=1,0,\042TCP\042,\042140.246.105.238\042,9004,0,1\r\n");//����Ϊ͸��ģʽ
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"CONNECT");//����Ƿ��½�ɹ�
		while(strx==NULL)
		{
				strx=strstr((const char*)RxBuffer,(const char*)"CONNECT");//����Ƿ��½�ɹ�
				delay_ms(100);
		}

		delay_ms(500);
		Clear_Buffer();
}		

///�����ַ�������
void EC20Send_StrData(char *bufferdata)
{
	u8 untildata=0xff;
	printf("AT+QISEND=0\r\n");
	delay_ms(100);
	printf(bufferdata);
  delay_ms(100);	
  USART_SendData(USART2, (u8) 0x1a);//������ɺ���
		while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
	{
	}
	delay_ms(100);
strx=strstr((char*)RxBuffer,(char*)"SEND OK");//�Ƿ���ȷ����
while(strx==NULL)
{
		strx=strstr((char*)RxBuffer,(char*)"SEND OK");//�Ƿ���ȷ����
		delay_ms(10);
}
delay_ms(100);
Clear_Buffer();
printf("AT+QISEND=0,0\r\n");
delay_ms(200);
strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//����ʣ���ֽ�����
while(untildata)
{
	  printf("AT+QISEND=0,0\r\n");
	  delay_ms(200);
		strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//����ʣ���ֽ�����
		strx=strstr((char*)strx,(char*)",");//��ȡ��һ��,
		strx=strstr((char*)(strx+1),(char*)",");//��ȡ�ڶ���,
	  untildata=*(strx+1)-0x30;
    Clear_Buffer();
	// IWDG_Feed();//ι��
}

}


///����ʮ������
void EC20Send_HexData(char *bufferdata)
{
	u8 untildata=0xff;
	printf("AT+QISENDEX=0,\042%s\042\r\n",bufferdata);
	delay_ms(100);

  USART_SendData(USART2, (u8) 0x1a);//������ɺ���
		while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
	{
	}
	delay_ms(100);
strx=strstr((char*)RxBuffer,(char*)"OK");//�Ƿ���ȷ����
while(strx==NULL)
{
		strx=strstr((char*)RxBuffer,(char*)"OK");//�Ƿ���ȷ����
		delay_ms(10);
}
delay_ms(100);
Clear_Buffer();
printf("AT+QISEND=0,0\r\n");
delay_ms(200);
strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//����ʣ���ֽ�����
while(untildata)
{
	  printf("AT+QISEND=0,0\r\n");
	  delay_ms(200);
		strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//����ʣ���ֽ�����
		strx=strstr((char*)strx,(char*)",");//��ȡ��һ��,
		strx=strstr((char*)(strx+1),(char*)",");//��ȡ�ڶ���,
	  untildata=*(strx+1)-0x30;
    Clear_Buffer();
	// IWDG_Feed();//ι��
}


}


///͸��ģʽ�½�������
void EC20Send_RecAccessMode(void)
{
	
		strx=strstr((const char*)RxBuffer,(const char*)"A1");//��LED0����
			if(strx)
			{		
				LED0=0;
				Clear_Buffer();
      }
		strx=strstr((const char*)RxBuffer,(const char*)"A0");//��LED0�ص�
			if(strx)
			{		
				LED0=1;
				Clear_Buffer();
      }
			strx=strstr((const char*)RxBuffer,(const char*)"B1");//��LED1����
			if(strx)
			{		
				LED1=0;
				Clear_Buffer();
      }
		strx=strstr((const char*)RxBuffer,(const char*)"B0");//��LED1�ص�
			if(strx)
			{		
				LED1=1;
				Clear_Buffer();
      }
	
			strx=strstr((const char*)RxBuffer,(const char*)"NO CARRIER");//�����������ر�
			 if(strx)
			 {
				 while(1)
				 {
					 Uart1_SendStr("Server Is Closed!\r\n");//��֪�û����������ر�
         }
       }


}

