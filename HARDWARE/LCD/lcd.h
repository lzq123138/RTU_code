#ifndef __LCD_H
#define __LCD_H

#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_conf.h"
#include "protocol.h"

typedef enum
{
	LCD_DISPLAY_FORWARD = 0,
	LCD_DISPLAY_REVRESE
}LCD_DISPLAY_MODE;

#define LCD_AHB1Periph_GPIO RCC_AHB1Periph_GPIOE
#define LCD_GPIO GPIOE
#define LCD_RS GPIO_Pin_11
#define LCD_SCLK GPIO_Pin_7
#define LCD_RESET GPIO_Pin_10
#define LCD_CS GPIO_Pin_9
#define LCD_SID GPIO_Pin_8

#define ROM_OUT GPIO_Pin_14
#define ROM_IN GPIO_Pin_15
#define ROM_SCK GPIO_Pin_13
#define ROM_CK GPIO_Pin_12

#define LCD_BL_AHB_GPIO	RCC_AHB1Periph_GPIOB
#define LCD_BL_GPIOX GPIOB
#define LCD_BL_GPIO_PIN GPIO_Pin_14

#define LCD_BL_POWER	PBout(14)


void init_lcd_gpio(void);
void init_lcd_bl(void);
void clear_screen(void);
void turn_on_back(void);
void turn_off_back(void);
void display_128x64(unsigned char *dp);
void display_graphic_16x16(uint32_t page,uint32_t column,unsigned char *dp,LCD_DISPLAY_MODE mode);
void display_graphic_8x16(uint32_t page,uint32_t column,unsigned char *dp,LCD_DISPLAY_MODE mode);
void display_graphic_5x7(uint32_t page,uint32_t column,unsigned char *dp,LCD_DISPLAY_MODE mode);
void display_GB2312_string(uint8_t y,uint8_t x,unsigned char *text,LCD_DISPLAY_MODE mode);
void display_string_5x7(uint8_t y,uint8_t x,unsigned char  *text,LCD_DISPLAY_MODE mode);

#endif
