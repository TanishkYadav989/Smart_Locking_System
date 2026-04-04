#ifndef DELAY_H
#define DELAY_H
#include "stm32l432xx.h"
void systick_init(uint32_t ticks);
void delay(uint16_t time);
void SysTick_Handler();
#endif