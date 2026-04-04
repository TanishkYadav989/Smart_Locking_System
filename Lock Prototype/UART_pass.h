#ifndef UPASS_H
#define UPASS_H
#include "stm32l432xx.h"
void UGPIOinit();
void UART_init(USART_TypeDef* USARTx, uint32_t baud);
void DMA_init();
void UDMA_Rx(uint8_t *Rbuff, uint32_t size);
void UDMA_Tx(uint8_t *Tbuff, uint32_t size);
void DMA1_Channel4_IRQHandler();
void DMA1_Channel5_IRQHandler();
#endif