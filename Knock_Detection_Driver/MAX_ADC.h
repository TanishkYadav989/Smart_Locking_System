#ifndef MAX_H
#define MAX_H
#include "stm32l432xx.h"
void AGPIO_init();
void TRGO_init();
void ADC_init(uint16_t *buff, uint16_t size);
void Clock_LSI();
void LPTIM1_init();
void start_window();
void LPTIM1_IRQHandler();
#endif