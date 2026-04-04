#include "stm32l432xx.h"
#include <stdbool.h>
volatile bool tx=false; //*Flag for detecting Transmission completed in DMA1 CH_4's ISR*//
volatile bool rx=false; //*FLag for detecting Reception completed in DMA1 CH_5's ISR*//
void UGPIOinit(){
    RCC->AHB2ENR|=1UL<<1; //*Enabling Clock of GPIO port B on AHB2 Bus via RCC*//
    GPIOB->MODER&= ~(((0xFU)<<12));
    GPIOB->MODER|=(((0xAU)<<12)); //*Setting PB6 & PB7 as Alterate function(10) as mapped to USART1 Tx & Rx*//
    GPIOB->OTYPER&= ~((3UL<<6)); //*Default output type i.e push-pull*//
    GPIOB->PUPDR&= ~(((0xFU)<<12)); //*no pull-up/pull-down required for Tx i.e PB6*//
    GPIOB->PUPDR|=1UL<<14; //*Pull-up for Rx i.e PB7 to prevent noise & jitters in reception*//
    GPIOB->AFR[0]&= ~((0xFFU)<<24);
    GPIOB->AFR[0]|=(0x77U)<<24; //*PB6 & PB7 mapped to Tx & Rx (AF7) i via AFRL*//
	GPIOB->MODER&= ~((3UL));
    GPIOB->MODER|=(1UL); //*PB0 set as output mode as used for Power Management of the HC-05 Bluetooth Module*//
    GPIOB->OTYPER&= ~(1UL); //*Output type as push-pull*//
    GPIOB->PUPDR&= ~(3UL); //*No pull-up/pull-down*//
	GPIOB->ODR|=1UL; //*Turns ON the HC-05 module*// 
}
void UART_init(USART_TypeDef* USARTx, uint32_t baud){
	RCC->CCIPR|=1UL; //*Selecting Processor Clock as the clock source of USART1 i.e 4MHz*//
    RCC->APB2ENR|=1UL<<14; //*Enabling Clock of USART1 on APB2 Bus via RCC*//
    RCC->APB2RSTR|=1UL<<14; //*Enabling reset of USART1 peripheral to remove any stale bits or data*//
    RCC->APB2RSTR&= ~(1UL<<14); //*Completing the reset*//
	USARTx->ICR|=(0xFU); //*Clearing all the Status flags for safety to ensure proper initiation*//

    USARTx->CR1=0; //*Resetting the Control register by assigning it 0*//
    USARTx->CR1|=((3UL<<2)|(1UL<<4)); //*Enabling Transmitter & receiver for USART along with IDLE interrupt*//
    USARTx->BRR = baud; //*Assinging Baud Rate to the BRR via function argument for USART due to asynchronous data transfer*//

    USARTx->CR2=0; //*Cleared CR2 as not needed for this configuration*//
    USARTx->CR3|=3UL<<6; //*Setting DMAT & DMAR bits for generating DMA request for transfers*//
    USARTx->CR1|=1UL; //*Enabling USART as last after proper configuration*//
	NVIC->IPR[37]=0x10; //*Assiging priority for the USART1 interrupt*//
	NVIC->ISER[1]|=1UL<<5; //*Enabling USART1's interrupt via ISER-1*//
}
void DMA_init(){
	RCC->AHB1ENR|=1UL; //*Enabling the clock of DMA1 on AHB1 Bus via RCC*//

	DMA1_Channel5->CCR=0; //*Resetting CCR of CH_5 to prevent stale bits*// 
    DMA1_Channel5->CCR|=3UL<<12; //*Assigning priority of DMA1_CH5 as highest*//
    DMA1_Channel5->CCR|=1UL<<7; //*Enabling Memory Increment mode*//
    DMA1_Channel5->CCR|=(0xAU); //*Enabling Transfer Complete & Transfer Error interrupts*// 
	DMA1_Channel4->CCR=0; //*Resetting CCR of CH_4 to prevent stale bits*// 
    DMA1_Channel4->CCR|=3UL<<12; //*Assigning priority of DMA1_CH4 as highest*//
    DMA1_Channel4->CCR|=((1UL<<7)|(1UL<<4)); //*Enabling Memory Increment mode for Tx along with Read from memory*//
    DMA1_Channel4->CCR|=(0xAU); //*Enabling Transfer Complete & Transfer Error interrupts*// 
	DMA1_CSELR->CSELR&= ~((0xFU)<<16);
    DMA1_CSELR->CSELR|=2UL<<16; //*Mapping CH_5 to USART1 Rx (0010)*//
    NVIC->IPR[15]=0x11; //*Alloting high priority to DMA1's CH_5*//
    NVIC->ISER[0]|=1UL<<15; //*Enabling DMA1 CH_5's interrupt via ISER-0*//
	DMA1_CSELR->CSELR&= ~((0xFU)<<12);
    DMA1_CSELR->CSELR|=2UL<<12; //*Mapping CH_4 to USART1 Tx (0010)*//
    NVIC->IPR[14]=0x20;  //*Alloting lower priority to DMA1's CH_4 as compared to CH_5*//
    NVIC->ISER[0]|=1UL<<14; //*Enabling DMA1 CH_4's interrupt via ISER-0*//
}
void UDMA_Rx(uint8_t *Rbuff, uint32_t size){
	DMA1->IFCR|=(0xFU)<<16; //*Clearing all the Flags*//
	DMA1_Channel5->CCR&=~1UL; //*Disabling CH_5 to re-arm DMA after each function call*//
    DMA1_Channel5->CPAR=(uint32_t)(&USART1->RDR); //*Assigning the address of USART1's Read data register as the source of transfer*//
    DMA1_Channel5->CMAR=(uint32_t)Rbuff; //*Assigning the buffer address as the destination where the received data will be stored as the destination*//
    DMA1_Channel5->CNDTR=size; //*Assigning the size of buffer as a counter determining the end of transfer*//
    DMA1_Channel5->CCR|=1UL; //*Enabling DMA1 CH_5 data stream*//
}
void UDMA_Tx(uint8_t *Tbuff, uint32_t size){
	DMA1->IFCR|=(0xFU)<<12; //*Clearing all the Flags*//
	DMA1_Channel4->CCR&=~1UL; //*Disabling CH_4 to re-arm DMA after each function call*//
    DMA1_Channel4->CMAR=(uint32_t)Tbuff;//*Assigning the memory address of the buffer as the source of transfer*//
    DMA1_Channel4->CPAR=(uint32_t)(&USART1->TDR); //*Assigning the address of USART1's Transfer data register as the destination*//
    DMA1_Channel4->CNDTR=size; //*Assigning the size of buffer as a counter determining the end of transfer*//
    DMA1_Channel4->CCR|=1UL; //*Enabling DMA1 CH_5 data stream*//
}
void USART1_IRQHandler(){
	if(USART1->ISR & (1UL<<4)){
		USART1->ICR|=1UL<<4; //*Clearing the Idle flag set in the ISR of USART1*//
    }
}
void DMA1_Channel5_IRQHandler(){
    if(DMA1->ISR & (1UL<<17)){ //*Checking if data reception is complete*//
        DMA1->IFCR|=1UL<<17; //*Clearing the TC flag*//	
	    rx=true; //*rx flag is set to true*//
	}
	if(DMA1->ISR & (1UL<<19)){
	    DMA1->IFCR|=1UL<<19; //*Clearing the TE flag*//
    }		
}
void DMA1_Channel4_IRQHandler(){ //*Checking if data transmission is complete*//
    if(DMA1->ISR & (1UL<<13)){ 
        DMA1->IFCR|=1UL<<13; //*Clearing the TC flag*//	
		tx=true; //*tx flag is set to true*//
	
    }
    if(DMA1->ISR & (1UL<<15)){
	    DMA1->IFCR|=1UL<<15; //*Clearing the TE flag*//
    }
}