#include "stm32l432xx.h"
#include "LPUART_pass.h"
#include "OLED_I2C.h"
#include <stdbool.h>
volatile bool tx=false; //*Flag for detecting Transmission completed in DMA2 CH_6's ISR*//
volatile bool rx=false; //*FLag for detecting Reception completed in DMA2 CH_7's ISR*//
extern uint8_t c[3]; //*Buffer for the sacrificial character along with CR+LF*//
extern volatile bool sleep; //*Flags for entering STOP-2 Mode from the FSM*//
extern volatile bool stop;
volatile state_t state = SLEEP; //*Default State set to SLEEP, where system enters STOP-2 after boot*//
void LPUGPIO_init(){
    RCC->AHB2ENR|=1UL; //*Enabling Clock of GPIO Port-A*//
    GPIOA->MODER&= ~(((0xFU)<<4)); 
    GPIOA->MODER|=(((0xAU)<<4)); //*Setting PA2 & PA3 as Alterate function(10) as mapped to LPUART1 Tx & Rx*//
    GPIOA->OTYPER&= ~(3UL<<2); //*Default output type i.e push-pull*//
    GPIOA->PUPDR&= ~(((0xFU)<<4));
    GPIOA->PUPDR|=(0x5U)<<4; //*Pull-up for Tx & Rx i.e PA2 & PA3 to prevent noise & jitters, preventing floating pin*//
    GPIOA->AFR[0]&= ~((0xFFU)<<8);
    GPIOA->AFR[0]|=(0x88U)<<8; ; //*PA2 & PA3 mapped to Tx & Rx (AF8) via AFRL*//
}
void LPUART_init(uint32_t baud){
    RCC->APB1ENR2|=1UL; ; //*Enabling Clock of LPUART1 on APB-1 Bus via RCC*//
    RCC->APB1RSTR2|=1UL; ; //*Enabling reset of LPUART1 peripheral to remove any stale bits or data*//
    RCC->APB1RSTR2&= ~(1UL); //*Completing the reset*//
	LPUART1->ICR|=(0xFU); //*Clearing Status Flags of LPUART1*//

    LPUART1->CR1=0; //*Clearing CR1 at once*//
    LPUART1->CR1|=(3UL<<2); //*Enabling Transmitter and Receiver*//
    LPUART1->BRR = baud; ; //*Assinging Baud Rate to the BRR via function argument for LPUART for asynchronous data transfer*//

    LPUART1->CR2=0; //*Cleared CR2 as not needed for this configuration*//
	LPUART1->CR3=0; //*Clearing CR3 at once*//
	LPUART1->CR3|=(0xE)<<20; //*Enabling LPUART Clock enable in STOP-Mode, Wakeup from Stop mode interrupt & Waking up when start bit is detected*// 
    LPUART1->CR3|=3UL<<6; //*Setting DMAT & DMAR bits for generating DMA request for transfers*//
    LPUART1->CR1|=3UL; //*Setting LPUART enable & enable in stop mode bit*//
	EXTI->IMR1|=1UL<<31; //*Un-masking LPUART1 wakeup Interrupt*//
	NVIC->IPR[70]=0x10; //*Assiging priority for the LPUART1 interrupt*//
	NVIC->ISER[2]|=1UL<<6; //*Enabling LPUART1's interrupt via ISER-2*// 
}
void DMA_init(){
	RCC->AHB1ENR|=1UL<<1; //*Enabling the clock of DMA2 on AHB1 Bus via RCC*//

	DMA2_Channel7->CCR=0; //*Resetting CCR of CH_7 to prevent stale bits*//
    DMA2_Channel7->CCR|=3UL<<12; //*Assigning priority of DMA2_CH7 as highest*//
    DMA2_Channel7->CCR|=1UL<<7; //*Enabling Memory Increment mode*/
    DMA2_Channel7->CCR|=(0xAU); //*Enabling Transfer Complete & Transfer Error interrupts*/
	DMA2_Channel6->CCR=0; //*Resetting CCR of CH_6 to prevent stale bits*//
    DMA2_Channel6->CCR|=3UL<<12; //*Assigning priority of DMA2_CH6 as highest*//
    DMA2_Channel6->CCR|=((1UL<<7)|(1UL<<4)); //*Enabling Memory Increment mode for Tx along with Read from memory*//
    DMA2_Channel6->CCR|=(0xAU);  //*Enabling Transfer Complete & Transfer Error interrupts*// 
	DMA2_CSELR->CSELR&= ~((0xFU)<<24); 
    DMA2_CSELR->CSELR|=4UL<<24; //*Mapping CH_7 to LPUART1 Rx (0100)*//
    NVIC->IPR[69]=0x11; //*Alloting high priority to DMA2's CH_7*
    NVIC->ISER[2]|=1UL<<5; //*Enabling DMA2 CH_7's interrupt via ISER-2*//
	DMA2_CSELR->CSELR&= ~((0xFU)<<20);
    DMA2_CSELR->CSELR|=4UL<<20; //*Mapping CH_6 to LPUART1 Tx (0100)*//
    NVIC->IPR[68]=0x20; /*Alloting high priority to DMA2's CH_6*//
    NVIC->ISER[2]|=1UL<<4; //*Enabling DMA2 CH_6's interrupt via ISER-2*//
}
void UDMA_Rx(uint8_t *Rbuff, uint32_t size){
	DMA2->IFCR|=(0xFU)<<24; //*Clearing all the Flags*//
	DMA2_Channel7->CCR&=~1UL; //*Disabling CH_7 to re-arm DMA after each function call*//
    DMA2_Channel7->CPAR=(uint32_t)(&LPUART1->RDR); //*Assigning the address of LPUART1's Read data register as the source of transfer*//
    DMA2_Channel7->CMAR=(uint32_t)Rbuff; //*Assigning the buffer address as the destination where the received data will be stored as the destination*//
    DMA2_Channel7->CNDTR=size; //*Assigning the size of buffer as a counter determining the end of transfer*//
    DMA2_Channel7->CCR|=1UL; //*Enabling DMA2 CH_7 data stream*//
}
void UDMA_Tx(uint8_t *Tbuff, uint32_t size){
	DMA2->IFCR|=(0xFU)<<20; //*Clearing all the Flags*//
	DMA2_Channel6->CCR&=~1UL; //*Disabling CH_6 to re-arm DMA after each function call*//
    DMA2_Channel6->CMAR=(uint32_t)Tbuff; //*Assigning the buffer address as the source*//
    DMA2_Channel6->CPAR=(uint32_t)(&LPUART1->TDR); //*Assigning the address of LPUART1's Transmit data register as the destination of transfer*//
    DMA2_Channel6->CNDTR=size; //*Assigning the size of buffer as a counter determining the end of transfer*//
    DMA2_Channel6->CCR|=1UL; //*Enabling DMA2 CH_6 data stream*//
}
void LPUART1_IRQHandler(){
	if(LPUART1->ISR & (1UL<<20)){ //*Check if WUF flag is detected*//
	    LPUART1->ICR|=((1UL<<1)|(1UL<<20)); //*Clearing Frame error & WUF flag*//  
		(void)LPUART1->RDR; //*Clearing RDR*//
	    sleep=false; //*FSM flags reset*//
		stop=false;
		state=IDLE; //*FSM transitions to IDLE state*//
		UDMA_Rx(c,3); //*Buffer store the sacrificial character*//
	}
}
void DMA2_Channel7_IRQHandler(){ 
    if(DMA2->ISR & (1UL<<25)){ //*Checking if data reception is complete*//
        DMA2->IFCR|=1UL<<25; //*Clearing the TC flag*//
	    rx=true; //*rx flag is set to true*/
	}
	if(DMA2->ISR & (1UL<<27)){
	    DMA2->IFCR|=1UL<<27;  //*Clearing the TE flag*//
    }		
}
void DMA2_Channel6_IRQHandler(){
    if(DMA2->ISR & (1UL<<21)){ //*Checking if data transmission is complete*//
        DMA2->IFCR|=1UL<<21; //*Clearing the TC flag*//	
	    tx=true; //*tx flag is set to true*//
    }
    if(DMA2->ISR & (1UL<<23)){
	    DMA2->IFCR|=1UL<<23; //*Clearing the TE flag*//
    }
}
void HSI_enable(){
	RCC->CR|=1UL<<8; //*Enabling HSI16 Clock*//
	RCC->CR|=(0x5U)<<9; //*Enabling HSI16 kernel clock & wakeup from stop mode*//
	while(!(RCC->CR & (1UL<<10))); //*Waiting until Clock is stable i.e HSIRDY flag is set*//
	RCC->CCIPR&=~(3UL<<10); 
	RCC->CCIPR|=1UL<<11; //*Mapping HSI16 clock source for LPUART1*// 
}
void Enter_Stop2(){
	TIM7->CR1&=~(1UL); //*Disabling Timer-7 before entering STOP-Mode*//
	RCC->APB1ENR1|=1UL<<28; //*Enabling Clock of Power Controller (PWR)*// 
	I2C3->CR1&=~(1UL); //*Disabling I2C3 peripheral*//
	GPIOA->MODER&=~(3UL<<14); 
	GPIOA->MODER|=3UL<<14; //*Setting I2C SCL pin PA7 to Analog mode to prevent leaking current*//
	GPIOB->MODER&=~(3UL<<8);
	GPIOB->MODER|=3UL<<8; //*Setting I2C SDA pin PB4 to Analog mode to prevent leaking current*//
    GPIOA->MODER&= ~(3UL<<12);
	GPIOA->MODER|=3UL<<12; //*Setting PWM (passive buzzer) pin PA6 to Analog mode to prevent leaking current*//
	LPUART1->ICR=0xFFFFFFFF; //*Clearing All LPUART Flags*//
	I2C3->ICR=0xFFFFFFFF; //*Clearing All I2C3 Flags*//
	EXTI->PR1=0xFFFFFFFF; //*Clearing All Pending Interrupts*//
	PWR->SCR|=1UL; //*Clearing Wakeup Flag-1*//
	SCB->SCR|=1UL<<2; //*Setting Deepsleep bit in Arm Cortex-M4 System Control Block*//
	PWR->CR1&=~7UL;
	PWR->CR1|=2UL; //*Setting Stop-2 Mode*//
	__DSB(); //*Finishing any ongoing data or tasks*//
	__WFI(); //*Calling Wait for Interrupt to enter Stop-2 Mode & waiting for LPUART wakeup*//
	SCB->SCR&=~(1UL<<2); //*Clearing deepsleep bit*//
	OTIM_set(); //*Enabling Timer-7 after wakeup*//
	GPIOA->MODER&= ~(((0x3U)<<14));
    GPIOA->MODER|=(((0x2U)<<14)); //*Initialising PA7 back to Alternate Function for SCL*//
	GPIOB->MODER&= ~((0x3U)<<8); 
    GPIOB->MODER|=((0x2U)<<8); //*Initialising PB4 back to Alternate Function for SDA*//
	GPIOA->MODER&= ~(3UL<<12);
	GPIOA->MODER|=2UL<<12; //*Initialising PA6 back to Alternate Function for PWM output*//
	I2C_init(I2C3); //*Re-initialising I2C3 after wakeup as suggested in Errata-sheet ES0318*//
	PWR->SCR|=1UL; //*Clearing Wakeup Flag-1*//
	OLED_init(); //*Re-initialising OLED by sending the 28-byte initialising buffer due to volatile RAM*//
}