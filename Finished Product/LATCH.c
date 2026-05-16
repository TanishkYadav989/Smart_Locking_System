#include "LATCH.h"
#include "stm32l432xx.h"
void LOCK_init(){
    RCC->AHB2ENR|=1UL; //*Enabling Clock for GPIO Port-A via RCC*//  
    GPIOA->MODER&= ~(3UL<<16);
    GPIOA->MODER|=3UL<<16; //*Setting PA8 as analog*//
    GPIOA->OTYPER&= ~(1UL<<8); 
    GPIOA->PUPDR&= ~(3UL<<16);
	GPIOA->OSPEEDR|=3UL<<16; //*Setting Output speed very high*//
}
void OPEN_LOCK(){
	GPIOA->MODER&=~(3UL<<16); //*Clearing PA8 bits*// 
	GPIOA->MODER|=1UL<<16; //*Setting PA8 as General-Purpose Output mode*//
    GPIOA->ODR|=1UL<<8; //*Opening the Lock by Setting GPIO pin high*//
}
void LOCK_CLOSE(){
    GPIOA->ODR&= ~(1UL<<8); //*Closing the Lock by setting GPIO pin Low*//
	GPIOA->MODER&= ~(3UL<<16); //*Clearing PA8 bits*// 
    GPIOA->MODER|=3UL<<16; //*Pin set back to Analog mode to prevent leakage current*// 
}