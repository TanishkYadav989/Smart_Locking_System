#include "BUZZER.h"
#include "stm32l432xx.h"
void BGPIO_init(){
    RCC->AHB2ENR|=1UL; //*Enabling the Clock of GPIO Port-A on AHB-2 Bus via RCC*//
    GPIOA->MODER&= ~(3UL<<12); 
    GPIOA->MODER|=(2UL<<12); //*Setting PA3 to Alternate Function Mode (10)*//
    GPIOA->OTYPER&= ~(3UL<<6); //*Output type default (Push-Pull)*//
    GPIOA->PUPDR&= ~(3UL<<12); //*No pull-up/pull-down*// 
    GPIOA->AFR[0]&= ~(0xFU)<<24; 
    GPIOA->AFR[0]|=(0xEU)<<24; //*Setting Alternate Function of PA3 for Timer 15 (AF14) in AFRL*//
}
void BUZZER_init(){
    RCC->APB2ENR|=1UL<<17; //*Enabling Clock of Timer 15 on APB-2 Bus via RCC*//
    
    TIM16->CR1&= ~1UL; //*Ensuring Timer 15 is disabled before configuration*//
    TIM16->PSC= 0; //*Prescaler to zero, runs on the same clock freq.*//
    TIM16->ARR= 1999; //*Assigned 1999 to ARR as Auto-Reload value*//
    TIM16->CCR1= 1000; //*Assigning 1000 to CCR2 as the duty cycle of PWM (half of AR value)*//
    TIM16->CCMR1=0;
    TIM16->CCMR1|=(0xDU)<<3; //*Output Compare 2 preload enabled along with PWM mode*//
    TIM16->BDTR&= ~(1UL<<15); 
    TIM16->BDTR|=1UL<<15; //*Enabling Outputs if the enable bits are set*//
    TIM16->CCER|=1UL; //*Capture/Compare 1 output enable*//
    TIM16->CR1&= ~1UL;
}
void ALERT(){
    TIM16->CR1|=1UL; //*Enabling Timer 15 to activate the passive buzzer for Security alert*//
}
void DISABLE(){
	TIM16->CCR1=0; //*Setting Duty cycle to 0 to mute the buzzer*//
   	TIM16->CR1&= ~(1UL); //*Disabling the Buzzer by disabling Timer 15*//
}