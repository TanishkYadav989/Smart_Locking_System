#include "BUZZER.h"
#include "stm32l432xx.h"
void BGPIO_init(){
    RCC->AHB2ENR|=1UL; //*Enabling the Clock of GPIO Port-A on AHB-2 Bus via RCC*//
    GPIOA->MODER&= ~(3UL<<6); 
    GPIOA->MODER|=(2UL<<6); //*Setting PA3 to Alternate Function Mode (10)*//
    GPIOA->OTYPER&= ~(3UL<<6); //*Output type default (Push-Pull)*//
    GPIOA->PUPDR&= ~(3UL<<6); //*No pull-up/pull-down*// 
    GPIOA->AFR[0]&= ~(0xFU)<<12; 
    GPIOA->AFR[0]|=(0xEU)<<12; //*Setting Alternate Function of PA3 for Timer 15 (AF14) in AFRL*//
}
void BUZZER_init(){
    RCC->APB2ENR|=1UL<<16; //*Enabling Clock of Timer 15 on APB-2 Bus via RCC*//
    
    TIM15->CR1&= ~1UL; //*Ensuring Timer 15 is disabled before configuration*//
    TIM15->PSC= 0; //*Prescaler to zero, runs on the same clock freq.*//
    TIM15->ARR= 1999; //*Assigned 1999 to ARR as Auto-Reload value*//
    TIM15->CCR2= 1000; //*Assigning 1000 to CCR2 as the duty cycle of PWM (half of AR value)*//
    TIM15->CCMR1&= ~(((0x7U)<<12)|(1UL<<24)|(1UL<<11));
    TIM15->CCMR1|=3UL<<13; //*Enabling PWM mode 1 (0110)*//
    TIM15->CCMR1|=1UL<<11; //*Output Compare 2 preload enabled*//
    TIM15->BDTR&= ~(1UL<<15); 
    TIM15->BDTR|=1UL<<15; //*Enabling Outputs if the enable bits are set*//
    TIM15->CCER|=1UL<<4; //*Capture/Compare 2 output enable*//
    TIM15->CR1&= ~1UL;
}
void ALERT(){
    TIM15->CR1|=1UL; //*Enabling Timer 15 to activate the passive buzzer for Security alert*//
}
void DISABLE(){
	TIM15->CCR2=0; //*Setting Duty cycle to 0 to mute the buzzer*//
   	TIM15->CR1&= ~(1UL); //*Disabling the Buzzer by disabling Timer 15*//
}