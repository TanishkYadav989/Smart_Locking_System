#ifndef UPASS_H
#define UPASS_H
#include "stm32l432xx.h"
typedef enum{
	SLEEP,
	IDLE,
	RECEIVE,
	CHECK,
	WAIT,
	GRANTED,
	DENIED,
	BREACH,
}state_t;
void HSI_enable();
void Enter_Stop2();
void LPUGPIO_init();
void LPUART_init(uint32_t baud);
void DMA_init();
void UDMA_Rx(uint8_t *Rbuff, uint32_t size);
void UDMA_Tx(uint8_t *Tbuff, uint32_t size);
#endif