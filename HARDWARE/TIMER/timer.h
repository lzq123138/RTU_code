#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"

extern unsigned char recflag;
void RTC_Config_Jxjs(void);
void sys_time_init(void);

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM4_Int_Init(u16 arr,u16 psc);
void TIM2_Int_Init(u16 arr,u16 psc);
#endif
