#ifndef OLED_H
#define OLED_H
#include "stm32l432xx.h"
void OGPIO_init();
void I2C_init(I2C_TypeDef* I2Cx);
void IDMA_init();
void OTIM_init();
void I2C_Start(uint8_t add, uint32_t size, uint8_t *Tbuffer);
void I2C3_EV_IRQHandler();
void DMA2_Channel7_Handler();
void TIM7_IRQHandler();
void OLED_init();
void delay_hw(uint16_t ms);
void DISPLAY_OFF();
#endif