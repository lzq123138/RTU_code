#include "lcd.h"
#include "delay.h"

//囧
unsigned char code_jiong1[]={
0x00,0xFE,0x82,0x42,0xA2,0x9E,0x8A,0x82,0x86,0x8A,0xB2,0x62,0x02,0xFE,0x00,0x00,
0x00,0x7F,0x40,0x40,0x7F,0x40,0x40,0x40,0x40,0x40,0x7F,0x40,0x40,0x7F,0x00,0x00};

//畾
unsigned char code_lei1[]={
0x80,0x80,0x80,0xBF,0xA5,0xA5,0xA5,0x3F,0xA5,0xA5,0xA5,0xBF,0x80,0x80,0x80,0x00,
0x7F,0x24,0x24,0x3F,0x24,0x24,0x7F,0x00,0x7F,0x24,0x24,0x3F,0x24,0x24,0x7F,0x00};


/*写指令到LCD模块*/
void transfer_command(uint8_t data)
{
	uint8_t i;
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,LCD_RS,Bit_RESET);
	for(i = 0;i < 8;i++)
	{
		GPIO_WriteBit(LCD_GPIO,LCD_SCLK,Bit_RESET);
		if(data & 0x80)
		{
			GPIO_WriteBit(LCD_GPIO,LCD_SID,Bit_SET);
		}
		else
		{
			GPIO_WriteBit(LCD_GPIO,LCD_SID,Bit_RESET);
		}
		GPIO_WriteBit(LCD_GPIO,LCD_SCLK,Bit_SET);
		delay_us(1);
		data <<= 1;
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/*写数据到LCD模块*/
void transfer_data(uint8_t data)
{
	uint8_t i;
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,LCD_RS,Bit_SET);
	for(i = 0;i < 8;i++)
	{
		GPIO_WriteBit(LCD_GPIO,LCD_SCLK,Bit_RESET);
		if(data & 0x80)
		{
			GPIO_WriteBit(LCD_GPIO,LCD_SID,Bit_SET);
		}
		else
		{
			GPIO_WriteBit(LCD_GPIO,LCD_SID,Bit_RESET);
		}
		GPIO_WriteBit(LCD_GPIO,LCD_SCLK,Bit_SET);
		delay_us(1);
		data <<= 1;
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/*写数据到LCD模块 取反 反显*/
void transfer_data_revrese(uint8_t data)
{
	uint8_t i;
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,LCD_RS,Bit_SET);
	for(i = 0;i < 8;i++)
	{
		GPIO_WriteBit(LCD_GPIO,LCD_SCLK,Bit_RESET);
		if(data & 0x80)
		{
			GPIO_WriteBit(LCD_GPIO,LCD_SID,Bit_RESET);
		}
		else
		{
			GPIO_WriteBit(LCD_GPIO,LCD_SID,Bit_SET);
		}
		GPIO_WriteBit(LCD_GPIO,LCD_SCLK,Bit_SET);
		delay_us(1);
		data <<= 1;
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/*LCD模块初始化*/
void initial_lcd()
{
	delay_ms(100);
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,ROM_CK,Bit_SET);
	GPIO_WriteBit(LCD_GPIO,LCD_RESET,Bit_RESET);/*低电平复位*/
    delay_ms(20);
    GPIO_WriteBit(LCD_GPIO,LCD_RESET,Bit_SET); /*复位完毕*/
    delay_ms(20);        
	transfer_command(0xe2);	  /*软复位*/
	delay_ms(5);
	transfer_command(0x2c);  /*升压步聚1*/
	delay_ms(5);	
	transfer_command(0x2e);  /*升压步聚2*/
	delay_ms(5);
	transfer_command(0x2f);  /*升压步聚3*/
	delay_ms(5);
	transfer_command(0x24);   /*粗调对比度，可设置范围0x20～0x27*/
	transfer_command(0x81);  /*微调对比度*/
	transfer_command(0x2a);  /*0x1a,微调对比度的值，可设置范围0x00～0x3f*/
	transfer_command(0xa2);  /*1/9偏压比（bias）*/
	transfer_command(0xc8);  /*行扫描顺序：从上到下*/
	transfer_command(0xa0);  /*列扫描顺序：从左到右*/
	transfer_command(0x60);  /*起始行：第一行开始*/
	transfer_command(0xaf);  /*开显示*/
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

void init_lcd_bl(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(LCD_BL_AHB_GPIO,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = LCD_BL_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(LCD_BL_GPIOX,&GPIO_InitStructure);//初始化
	
	LCD_BL_POWER = 1;
}

//初始化各个引脚
void init_lcd_gpio(void)
{
	GPIO_InitTypeDef GPIOInitStructure;
	
	RCC_AHB1PeriphClockCmd(LCD_AHB1Periph_GPIO,ENABLE);
	
	GPIOInitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIOInitStructure.GPIO_OType = GPIO_OType_PP;
	GPIOInitStructure.GPIO_Pin = LCD_CS | LCD_RESET | LCD_RS | LCD_SCLK | LCD_SID | ROM_CK | ROM_IN | ROM_SCK;
	GPIOInitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIOInitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(LCD_GPIO,&GPIOInitStructure);

	GPIOInitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIOInitStructure.GPIO_OType = GPIO_OType_PP;
	GPIOInitStructure.GPIO_Pin = ROM_OUT;
	GPIOInitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIOInitStructure.GPIO_Speed = GPIO_Fast_Speed;
	GPIO_Init(LCD_GPIO,&GPIOInitStructure);
	
	//设置初始值
	initial_lcd();
	
	init_lcd_bl();
}

void lcd_address(uint32_t page,uint32_t column)
{
	column=column;
	transfer_command(0xb0+page-1);   /*设置页地址*/
	transfer_command(0x10+(column>>4&0x0f));	/*设置列地址的高4位*/
	transfer_command(column&0x0f);	/*设置列地址的低4位*/
}

/*全屏清屏*/
void clear_screen(void)
{
	uint8_t i,j;
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,ROM_CK,Bit_SET);
	for(i=0;i<9;i++)
	{
		transfer_command(0xb0+i);
		transfer_command(0x10);
		transfer_command(0x00);
		for(j=0;j<132;j++)
		{
		  	transfer_data(0x00);
			//delay(1);
		}
	}
 	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

void turn_on_back(void)
{
	LCD_BL_POWER = 1;
}

void turn_off_back(void)
{
	LCD_BL_POWER = 0;
}

void set_background_16x16(uint32_t page,uint32_t column,LCD_DISPLAY_MODE mode)
{
	uint8_t i = 0,j = 0;
	for(j = 0;j < 2;j++)
	{
		lcd_address(page + j,column);
		for(i = 0;i < 128;i++)
		{
			if(mode == LCD_DISPLAY_FORWARD)
			{
				transfer_data(0x00);
			}
			else if(mode == LCD_DISPLAY_REVRESE)
			{
				transfer_data_revrese(0x00);
			}
		}
	}
}

/*显示128x64点阵图像*/
void display_128x64(unsigned char *dp)
{
	uint8_t i,j;
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	for(j=0;j<8;j++)
	{
		lcd_address(j+1,1);
		for (i=0;i<128;i++)
		{	
			transfer_data(*dp);	/*写数据到LCD,每写完一个8位的数据后列地址自动加1*/
			dp++;
		}
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/*显示16x16点阵图像、汉字、生僻字或16x16点阵的其他图标*/
void display_graphic_16x16(uint32_t page,uint32_t column,unsigned char *dp,LCD_DISPLAY_MODE mode)
{
	uint8_t i,j;
 	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,ROM_CK,Bit_SET);	
	for(j=0;j<2;j++)
	{
		lcd_address(page+j,column);
		for (i=0;i<16;i++)
		{	if(mode == LCD_DISPLAY_FORWARD)
			{
				transfer_data(*dp);	/*写数据到LCD,每写完一个8位的数据后列地址自动加1*/
			}
			else if(mode == LCD_DISPLAY_REVRESE)
			{
				transfer_data_revrese(*dp);
			}			
			dp++;
		}
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/*显示8x16点阵图像、ASCII, 或8x16点阵的自造字符、其他图标*/
void display_graphic_8x16(uint32_t page,uint32_t column,unsigned char *dp,LCD_DISPLAY_MODE mode)
{
	uint8_t i,j;
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);	
	for(j=0;j<2;j++)
	{
		lcd_address(page+j,column);
		for (i=0;i<8;i++)
		{	
			if(mode == LCD_DISPLAY_FORWARD)
			{
				transfer_data(*dp);	/*写数据到LCD,每写完一个8位的数据后列地址自动加1*/
			}
			else if(mode == LCD_DISPLAY_REVRESE)
			{
				transfer_data_revrese(*dp);
			}
			dp++;
		}
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/*显示5*7点阵图像、ASCII, 或5x7点阵的自造字符、其他图标*/
void display_graphic_5x7(uint32_t page,uint32_t column,unsigned char *dp,LCD_DISPLAY_MODE mode)
{
	uint32_t col_cnt;
	uint8_t page_address;
	uint8_t column_address_L,column_address_H;
	page_address = 0xb0+page-1;
	
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_RESET);
	
	column_address_L =(column&0x0f)-1;
	column_address_H =((column>>4)&0x0f)+0x10;
	
	transfer_command(page_address); 		/*Set Page Address*/
	transfer_command(column_address_H);	/*Set MSB of column Address*/
	transfer_command(column_address_L);	/*Set LSB of column Address*/
	
	for (col_cnt=0;col_cnt<6;col_cnt++)
	{	
		if(mode == LCD_DISPLAY_FORWARD)
		{
			transfer_data(*dp);	/*写数据到LCD,每写完一个8位的数据后列地址自动加1*/
		}
		else if(mode == LCD_DISPLAY_REVRESE)
		{
			transfer_data_revrese(*dp);
		}
		dp++;
	}
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
}

/****送指令到字库IC***/
void send_command_to_ROM(uint8_t data)
{
	uint8_t i;
	for(i=0;i<8;i++ )
	{
		if(data&0x80)
			GPIO_WriteBit(LCD_GPIO,ROM_IN,Bit_SET);
		else
			GPIO_WriteBit(LCD_GPIO,ROM_IN,Bit_RESET);
		data <<= 1;
		GPIO_WriteBit(LCD_GPIO,ROM_SCK,Bit_RESET);
		GPIO_WriteBit(LCD_GPIO,ROM_SCK,Bit_SET);
	}
}

/****从字库IC中取汉字或字符数据（1个字节）***/
static uint8_t get_data_from_ROM( )
{
	
	uint8_t i;
	uint8_t ret_data=0;
	GPIO_WriteBit(LCD_GPIO,ROM_SCK,Bit_SET);
	for(i=0;i<8;i++)
	{
		//Rom_OUT=1;
		GPIO_WriteBit(LCD_GPIO,ROM_SCK,Bit_RESET);
		ret_data=ret_data<<1;
		if( GPIO_ReadInputDataBit(LCD_GPIO,ROM_OUT))
			ret_data=ret_data+1;
		else
			ret_data=ret_data+0;
		GPIO_WriteBit(LCD_GPIO,ROM_SCK,Bit_SET);
	}
	return(ret_data);
}

/*从相关地址（addrHigh：地址高字节,addrMid：地址中字节,addrLow：地址低字节）中连续读出DataLen个字节的数据到 pBuff的地址*/
/*连续读取*/
void get_n_bytes_data_from_ROM(uint8_t addrHigh,uint8_t addrMid,uint8_t addrLow,uint8_t *pBuff,uint8_t DataLen )
{
	uint8_t i;
	GPIO_WriteBit(LCD_GPIO,ROM_CK,Bit_RESET);
	GPIO_WriteBit(LCD_GPIO,LCD_CS,Bit_SET);
	GPIO_WriteBit(LCD_GPIO,ROM_SCK,Bit_RESET);
	send_command_to_ROM(0x03);
	send_command_to_ROM(addrHigh);
	send_command_to_ROM(addrMid);
	send_command_to_ROM(addrLow);
	for(i = 0; i < DataLen; i++ )
	     *(pBuff+i) =get_data_from_ROM();
	GPIO_WriteBit(LCD_GPIO,ROM_CK,Bit_SET);
}

/******************************************************************/

unsigned long  fontaddr=0;
void display_GB2312_string(uint8_t y,uint8_t x,unsigned char *text,LCD_DISPLAY_MODE mode)
{
	uint8_t i= 0;
	uint8_t addrHigh,addrMid,addrLow ;
	uint8_t fontbuf[32];

	set_background_16x16(y,x,mode);//设置背景为正显还是反显
	while((text[i]>0x00))
	{
		if(((text[i]>=0xb0) &&(text[i]<=0xf7))&&(text[i+1]>=0xa1))
		{						
			/*国标简体（GB2312）汉字在晶联讯字库IC中的地址由以下公式来计算：*/
			/*Address = ((MSB - 0xB0) * 94 + (LSB - 0xA1)+ 846)*32+ BaseAdd;BaseAdd=0*/
			/*由于担心8位单片机有乘法溢出问题，所以分三部取地址*/
			fontaddr = (text[i]- 0xb0)*94; 
			fontaddr += (text[i+1]-0xa1)+846;
			fontaddr = (unsigned long)(fontaddr*32);
			
			addrHigh = (fontaddr&0xff0000)>>16;  /*地址的高8位,共24位*/
			addrMid = (fontaddr&0xff00)>>8;      /*地址的中8位,共24位*/
			addrLow = fontaddr&0xff;	     /*地址的低8位,共24位*/
			get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,32 );/*取32个字节的数据，存到"fontbuf[32]"*/
			display_graphic_16x16(y,x,fontbuf,mode);/*显示汉字到LCD上，y为页地址，x为列地址，fontbuf[]为数据*/
			i+=2;
			x+=16;
		}
		else if(((text[i]>=0xa1) &&(text[i]<=0xa3))&&(text[i+1]>=0xa1))
		{						
			/*国标简体（GB2312）15x16点的字符在晶联讯字库IC中的地址由以下公式来计算：*/
			/*Address = ((MSB - 0xa1) * 94 + (LSB - 0xA1))*32+ BaseAdd;BaseAdd=0*/
			/*由于担心8位单片机有乘法溢出问题，所以分三部取地址*/
			fontaddr = (text[i]- 0xa1)*94; 
			fontaddr += (text[i+1]-0xa1);
			fontaddr = (unsigned long)(fontaddr*32);
			
			addrHigh = (fontaddr&0xff0000)>>16;  /*地址的高8位,共24位*/
			addrMid = (fontaddr&0xff00)>>8;      /*地址的中8位,共24位*/
			addrLow = fontaddr&0xff;	     /*地址的低8位,共24位*/
			get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,32 );/*取32个字节的数据，存到"fontbuf[32]"*/
			display_graphic_16x16(y,x,fontbuf,mode);/*显示汉字到LCD上，y为页地址，x为列地址，fontbuf[]为数据*/
			i+=2;
			x+=16;
		}
		else if((text[i]>=0x20) &&(text[i]<=0x7e))	
		{						
			unsigned char fontbuf[16];			
			fontaddr = (text[i]- 0x20);
			fontaddr = (unsigned long)(fontaddr*16);
			fontaddr = (unsigned long)(fontaddr+0x3cf80);			
			addrHigh = (fontaddr&0xff0000)>>16;
			addrMid = (fontaddr&0xff00)>>8;
			addrLow = fontaddr&0xff;

			get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,16 );/*取16个字节的数据，存到"fontbuf[32]"*/
			
			display_graphic_8x16(y,x,fontbuf,mode);/*显示8x16的ASCII字到LCD上，y为页地址，x为列地址，fontbuf[]为数据*/
			i+=1;
			x+=8;
		}
		else
			i++;	
	}
	
}


void display_string_5x7(uint8_t y,uint8_t x,unsigned char *text,LCD_DISPLAY_MODE mode)
{
	unsigned char i= 0;
	unsigned char addrHigh,addrMid,addrLow ;
	while((text[i]>0x00))
	{
		
		if((text[i]>=0x20) &&(text[i]<=0x7e))	
		{						
			unsigned char fontbuf[8];			
			fontaddr = (text[i]- 0x20);
			fontaddr = (unsigned long)(fontaddr*8);
			fontaddr = (unsigned long)(fontaddr+0x3bfc0);			
			addrHigh = (fontaddr&0xff0000)>>16;
			addrMid = (fontaddr&0xff00)>>8;
			addrLow = fontaddr&0xff;

			get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,8);/*取8个字节的数据，存到"fontbuf[32]"*/
			
			display_graphic_5x7(y,x,fontbuf,mode);/*显示5x7的ASCII字到LCD上，y为页地址，x为列地址，fontbuf[]为数据*/
			i+=1;
			x+=6;
		}
		else
		i++;	
	}
	
}

