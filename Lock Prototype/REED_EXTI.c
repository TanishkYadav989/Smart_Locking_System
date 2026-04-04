#include "REED_EXTI.h"
#include "stm32l432xx.h"
#include <stdbool.h>
volatile bool access = false; //*Flag used to detect if the door is accessed securely*//
volatile bool breach= false; //*Flag used to detect if the door is accessed forcefully*//
volatile bool reload = false; //*Flag used to detect if the door is locked after being accessed securely*//
void RGPIO_init(){
    RCC->AHB2ENR|=1UL; //*Enabling Clock of GPIO Port-A on AHB-2 via RCC*//
    GPIOA->MODER&= ~(3UL); //*Setting PA0 as Input Mode (00)*//
    GPIOA->PUPDR&= ~(3UL); 
    GPIOA->PUPDR|=1UL; //*Pull-up activated to ensure pin connected to Reed Switch is not floating & set as active high*//
}
void REED_init(){
    RCC->APB2ENR|=1UL; //*Enabling Clock of SYSCFG on APB-2 Bus via RCC*//
    SYSCFG->EXTICR[0]&= ~(0x7U); //*Mapping PA0 to EXTI (EXTICR is 0 as pin 0 is used & cleared bits as 000-Port A)*//

    EXTI->FTSR1|= (1UL); //*Enabling Falling Trigger to detect door is closed*//
    EXTI->RTSR1|=1UL; //*Enabling Rising Trigger to detect door is being opened*//
    EXTI->IMR1|=1UL; //*Setting bit 0 in Interrupt-Mask register to make it visible to the CPU*//
    NVIC->IPR[6]=0x00; //*Priority is set to highest for secruity purpose*//
    NVIC->ISER[0]|=1UL<<6; //*Enabling EXTI-0 Interupt via ISER*// 
}    
void EXTI0_IRQHandler(){
    if(EXTI->PR1 & (1UL)){ //*Check if the Interrupt request is pending*//
		EXTI->PR1|=1UL; //*Clearing pending flag of EXTI-0*//
		for(volatile int i=0;i<10000;i++); //*Software debounce fo the Reed Switch to prevent false triggers*//
		if((GPIOA->IDR & (1UL))){ //*Checking if input is high (circuit broken)*//
			if(!access){breach = true;} //*If door was not acces via correct password, breach flag for Alert*//  
		}
		if(!(GPIOA->IDR & (1UL))){ //*Checking if input is low (circuit complete)*//
			reload = true; //*Reload flag is set indicating the door is closed after being accessed securely*//
        }    
    }
}