#include "MAX_ADC.h"
#include "stm32l432xx.h"
volatile uint8_t knock=0; //*Global counter for knock count*//
void AGPIO_init(){
    RCC->AHB2ENR|=1UL; //*Enabling Clock of GPIO Port-A via RCC*//
    GPIOA->MODER&= ~(3UL<<10); 
    GPIOA->MODER|=3UL<<10; //*Analog Mode set for PA5 which is mapped to ADC1 Channel 10*//
}
void TRGO_init(){
    RCC->APB1ENR1|=1UL<<4; //*Enabling Clock of Timer 6 on APB1 Bus-2*//
    TIM6->CR1=0; //*Clearing CR1 of TIM6 at once*//
    TIM6->CR1|=1UL<<2; //*Enabling an update event on each overflow*//
    TIM6->CR2&= ~((0x7U)<<4); 
    TIM6->CR2|=1UL<<5; //*Update mode selected*//
    TIM6->PSC=39; //*Prescaler set to 39 resulting in a 100kHz frequency*//
    TIM6->ARR=49; //*Auto-reload value set to 49 as the max limit*//
    TIM6->CR1|=1UL; //*Enabling timer to trigger the sampling of ADC1*//
}
void ADC_init(uint16_t *buff, uint16_t size){
    RCC->CCIPR|=3UL<<28; //*Setting System Clock i.e 4MHz selected as ADC's Clock*//
    RCC->AHB2ENR|=1UL<<13; //*Enabling Clock for ADC (AHB Bus-2)*//
	RCC->AHB2RSTR|=1UL<<13;
	RCC->AHB2RSTR&= ~(1UL<<13); //*Completing Reset of ADC to discard Stale Bits*//
    RCC->AHB1ENR|=1UL; //*Enabling Clock for DMA1 (AHB Bus-1)*//
    RCC->AHB2SMENR|=1UL<<13; //*Ensures ADC survives Sleep Mode*//
    RCC->AHB1SMENR|=1UL; //*Ensures DMA1 survives Sleep mode*//

    ADC1_COMMON->CCR=0; //*Resetting ADC's Common register for setting psc to 0 & Asynchronous Clock Mode*//
    ADC1->CR=0; //*Atomic clearing of CR & Waking ADC from Deep-Power-down mode*//
    ADC1->CR|=1UL<<28; //*Enabling ADC Voltage Regulator*//
    for(volatile int i=0;i<10000;i++);  //*Waiting to successfully enable ADC VREG to prevent race conditions for it, as suggested by RM*//
    ADC1->CR|=1UL<<31; //*Calibrating ADC*//
    while(ADC1->CR & (1UL<<31)); //*Waiting until ADC Calibration bit resets automatically after calibration *//

    ADC1->CFGR=0; //*Atomic clearing Configuration Register of ADC1*//
    ADC1->CFGR|=1UL<<1; //*Enabling DMA circular mode*//
    ADC1->CFGR|=1UL<<10; //*Setting Hardware trigger detection for rising edge*//
    ADC1->CFGR|=(13UL)<<6; //*Mapping TIM-6 as TRGO for ADC1*//
	ADC1->CFGR|=1UL<<23; //*AWD1 enabled for detecting the 75% threshold*//
    ADC1->TR1=0;
    ADC1->TR1|=(4085U)<<16; //*Setting the High-Threshold Value to 4085 for less sensitivity to background noise*// 
	ADC1->CFGR|=1UL<<22; //*AWD1 enabled for single channel*//
	ADC1->CFGR|=(10UL<<26); //*Mapping Channel 10 for ADC1*//
    ADC1->CFGR|=1UL; //*Enabling DMA data stream*//		
    ADC1->SMPR2|=3UL<<1; //*Setting Sampling Rate to 247.5 ADC Clock Cycles*//
    ADC1->SQR1|=(10UL)<<6; //*Mapping Channel 10 for the 1st & only sequence*//
	NVIC->IPR[18]=0x00; //*Setting Priority of ADC1_2 interrupt as the highest*//
	NVIC->ISER[0]|=1UL<<18; //*Enabling the ADC1_2 interrupt*//
	ADC1->IER|=1UL<<7; //*Enabling the AWD-1 interrupt for threshold detection*//

    DMA1_Channel1->CCR=0; //*Clearing DMA1 Channel 1's Control Register*//
	DMA1_Channel1->CCR|=(0x5U)<<5; //*Enabling Circular mode & memory increment mode for DMA1 CH_1*//
    DMA1_Channel1->CCR|=3UL<<12; //*Highest Priority for DMA1 CH_1*//
    DMA1_Channel1->CCR|=(0x5U)<<8; //*Setting Peripheral & Memory size to 16 bit*//
    DMA1_Channel1->CPAR=(uint32_t)&ADC1->DR;  //*Allocating the address of the ADC1's regular channel Data register as the source of transfer*//
    DMA1_Channel1->CMAR=(uint32_t)buff; //*Allocating the adrress of BUFF as the destination of the transfer*//
    DMA1_Channel1->CNDTR=size; //*Assigning the size of buffer via function argument*//
    DMA1_CSELR->CSELR&= ~(0xFU); //*Mapping ADC1 to DMA1 Channel 1 via Select Register*//
    DMA1_Channel1->CCR|=1UL; //*Arming DMA1 Channel 1 before enabling ADC1 *//

    ADC1->CR|=1UL; //*Enabling ADC1*//
    while(!(ADC1->ISR & (1UL))); //*Polling until ADC ready flag is set in the ISR as suggested by the RM*//
    ADC1->CR|=1UL<<2; //*Setting ADC start bit to start the conversion after ADC1 is ready*//
}
void ADC1_IRQHandler(){
	 if(ADC1->ISR & (1UL<<7)){ //*Check if the AWD1 interrupt flag is set in the ISR*//
	    ADC1->ISR|=1UL<<7; //*Clearing the flag to re-arm the interrupt*//
	    if(knock>0 && LPTIM1->CNT<8400) //*Temporal Masking or Blocking Window to discards the other triggers in a 262.5ms window*//
        {return;}  //*Otherwise the wood ringing & air vibration results in false triggers*// 
	    knock++; //*Incrementing knock variable after each knock detected by the microphone*//
		if(knock==1){ 
            start_window(); //*Starts the 2sec window to detect another one otherwise knock resets to 0 preventing false triggers*//
        }
        else if(knock==2){
	    	start_window(); //*Re-starting window after 2nd knock preventing false triggers*//
        }
	    else if(knock==3){ 
			ADC1->IER&= ~(1UL<<7); //*Disabling the AWD1 interrupt to prevent registering other knock or triggers*//
			LPTIM1->CR&= ~(1UL<<1); //*Disabling the Low-power timer as not needed now*//
			knock=0; //*Knock is reset to 0*//
			GPIOA->ODR|=1UL<<11; //*LED for visual feedback of proper working*//				
		}
	}
}
void Clock_LSI(){
	RCC->CSR&= ~(1UL); 
	RCC->CSR|=1UL; //*Enabling LSI clock for 32kHz clock by setting LSI_ON bit*//
	while(!(RCC->CSR & (1UL<<1))); //*Polling until LSI clock is ready*//
	RCC->CCIPR&= ~(3UL<<18); 
	RCC->CCIPR|=1UL<18; //*Mapping the Clock source of Low-power Timer-1 to LSI clock*//
}
void LPTIM1_init(){ 
	RCC->APB1ENR1|=1UL<<31; //*Enabling Clock of LPTIM1 on APB1 Bus-1 via RCC*//
	RCC->APB1SMENR1|=1UL<<31; //*Ensures LPTIM1 survives the Sleep mode*//
	
	LPTIM1->CFGR=0; //*Clearing CFGR at once*//
	LPTIM1->CR=0; //*Clearing CR at once*//
	LPTIM1->CNT=0; //*Counter set to 0 for clean start*// 	
	LPTIM1->CFGR|=(0x7U)<<9; //*LPTIM1 prescaler bit fields set*//
	LPTIM1->IER|=1UL<<1; //*Enabling ARR match event interrupt*//
	NVIC->IPR[65]=0x10;
    NVIC->ISER[2]|=1UL<<1; //*Enabling LPTIM1's interrupt*//
}
void start_window(){
	LPTIM1->CR&= ~(1UL); //*Clearing enable bit for re-using after each function call*//
	LPTIM1->CR|=1UL; //*Enabling LPTIM1 as suggested by RM before assigning ARR & setting single start*//
	LPTIM1->ARR=60000; //*Auto-reload value set as 60000*//
	LPTIM1->CR|=1UL<<1; //*Single Start bit is set for counting once*//
}
void LPTIM1_IRQHandler(){
  if(LPTIM1->ISR & (1UL<<1)){ //*Checking if ARR match event has occured indicating that the window has expired*// 
		LPTIM1->ICR|=1UL<<1; //*Clearing ARRM Flag*//
		knock=0; //*Knock resets to 0 for nullifying any false triggers*//
	}
}