#ifndef REED_H
#define REED_H
#include "stm32l432xx.h"
void RGPIO_init();
void REED_init();
void EXTI0_IRQHandler();
#endif
