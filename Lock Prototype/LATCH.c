#include "LATCH.h"
#include "stm32l432xx.h"
void LOCK_init(){
    RCC->AHB2ENR|=1UL; //*Enabling Clock of GPIO Port-A*//
    GPIOA->MODER&= ~(3UL<<22);
    GPIOA->MODER|=1UL<<22; //*Setting PA11 to General Purpose Ouput Mode (01)*//
    GPIOA->OTYPER&= ~(1UL<<11); //*Default push-pull for PA11*//
    GPIOA->PUPDR&= ~(3UL<<22); //*No pull-up/pull-down required*//
}
void OPEN_LOCK(){  
    GPIOA->ODR|=1UL<<11; //*Opens the Solenoidal lock by Setting PA11 to high, retracts the latch*//
}
void LOCK_CLOSE(){
    GPIOA->ODR&= ~(1UL<<11); //*Closed the Solenoidal lock by Clearing PA11 as low, unleashes the latch*//
}