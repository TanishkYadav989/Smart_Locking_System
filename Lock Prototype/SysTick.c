#include "SysTick.h"
#include "stm32l432xx.h"
volatile uint16_t t; //*Variable denoting the delay time*//
void systick_init(uint32_t ticks){
    SysTick->CTRL=0; //*Clearing the SysTick Control register before configuring*//
    SysTick->LOAD= ticks-1; //*Alloting the Clock frequency (no. of ticks) to LOAD register*//
    SysTick->VAL=0; //*Assiging Initial value of the timer, start from 0*//
    SCB->SHPR[11]=0xF0; //*Priority set as lowest*//
    SysTick->CTRL=7UL; //*Assigning Clock Souce same as AHB (processor clock) & setting the Tickint & systick Enable bits as well*//
}
void SysTick_Handler(){
    if(t>0){ //*Sanity check, is the input is greater than 0 or not*// 
       t--; //*Decrements t in its ISR as a down-counter*//
    }
}
void delay(uint16_t time){
   t=time; //*Assigning t to time variable for the delay function*//
   while(t!=0); //*Delays until t hits 0*//
}