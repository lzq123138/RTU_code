#include "ec20.h"
#include "stdlib.h"
#include "string.h"
#include "usart.h"	
#include "led.h"	
char *strx,*extstrx,*Readystrx;
extern 	char RxBuffer[200],Rxcouter;
	void Clear_Buffer(void)//清空缓存
{
		u8 i;
		Uart1_SendStr(RxBuffer);
		for(i=0;i<Rxcouter;i++)
		RxBuffer[i]=0;//缓存
		Rxcouter=0;
	//	IWDG_Feed();//喂狗
}
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
		printf("AT+CPIN?\r\n");//检查SIM卡是否在位
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
		printf("AT+QIOPEN=1,0,\042TCP\042,\042140.246.105.238\042,9004,0,1\r\n");//设置为透传模式
		delay_ms(500);
		strx=strstr((const char*)RxBuffer,(const char*)"CONNECT");//检查是否登陆成功
		while(strx==NULL)
		{
				strx=strstr((const char*)RxBuffer,(const char*)"CONNECT");//检查是否登陆成功
				delay_ms(100);
		}

		delay_ms(500);
		Clear_Buffer();
}		

///发送字符型数据
void EC20Send_StrData(char *bufferdata)
{
	u8 untildata=0xff;
	printf("AT+QISEND=0\r\n");
	delay_ms(100);
	printf(bufferdata);
  delay_ms(100);	
  USART_SendData(USART2, (u8) 0x1a);//发送完成函数
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
while(untildata)
{
	  printf("AT+QISEND=0,0\r\n");
	  delay_ms(200);
		strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//发送剩余字节数据
		strx=strstr((char*)strx,(char*)",");//获取第一个,
		strx=strstr((char*)(strx+1),(char*)",");//获取第二个,
	  untildata=*(strx+1)-0x30;
    Clear_Buffer();
	// IWDG_Feed();//喂狗
}

}


///发送十六进制
void EC20Send_HexData(char *bufferdata)
{
	u8 untildata=0xff;
	printf("AT+QISENDEX=0,\042%s\042\r\n",bufferdata);
	delay_ms(100);

  USART_SendData(USART2, (u8) 0x1a);//发送完成函数
		while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)
	{
	}
	delay_ms(100);
strx=strstr((char*)RxBuffer,(char*)"OK");//是否正确发送
while(strx==NULL)
{
		strx=strstr((char*)RxBuffer,(char*)"OK");//是否正确发送
		delay_ms(10);
}
delay_ms(100);
Clear_Buffer();
printf("AT+QISEND=0,0\r\n");
delay_ms(200);
strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//发送剩余字节数据
while(untildata)
{
	  printf("AT+QISEND=0,0\r\n");
	  delay_ms(200);
		strx=strstr((char*)RxBuffer,(char*)"+QISEND:");//发送剩余字节数据
		strx=strstr((char*)strx,(char*)",");//获取第一个,
		strx=strstr((char*)(strx+1),(char*)",");//获取第二个,
	  untildata=*(strx+1)-0x30;
    Clear_Buffer();
	// IWDG_Feed();//喂狗
}


}


///透传模式下接受数据
void EC20Send_RecAccessMode(void)
{
	
		strx=strstr((const char*)RxBuffer,(const char*)"A1");//对LED0开灯
			if(strx)
			{		
				LED0=0;
				Clear_Buffer();
      }
		strx=strstr((const char*)RxBuffer,(const char*)"A0");//对LED0关灯
			if(strx)
			{		
				LED0=1;
				Clear_Buffer();
      }
			strx=strstr((const char*)RxBuffer,(const char*)"B1");//对LED1开灯
			if(strx)
			{		
				LED1=0;
				Clear_Buffer();
      }
		strx=strstr((const char*)RxBuffer,(const char*)"B0");//对LED1关灯
			if(strx)
			{		
				LED1=1;
				Clear_Buffer();
      }
	
			strx=strstr((const char*)RxBuffer,(const char*)"NO CARRIER");//服务器主动关闭
			 if(strx)
			 {
				 while(1)
				 {
					 Uart1_SendStr("Server Is Closed!\r\n");//告知用户服务器被关闭
         }
       }


}

