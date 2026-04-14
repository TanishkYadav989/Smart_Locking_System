#include "OLED_I2C.h"
#include "stm32l432xx.h"
#include "BITMAPS.h"
#include <stdbool.h>
volatile uint8_t counter=0; //*Used for Incrementing in TIM-7's ISR, to display the next frame on OLED*// 
volatile uint32_t bytes=0;
volatile bool busy=true; //*Flag used to determine if I2C is busy*//
volatile bool alarmed=false; //*Flag to initiate the alarm inversion animation*//
volatile bool grant=false; //*Flag to trigger the unlock animation*//
volatile bool done=false; //*Flag used to indicate that the delay is over*//
volatile bool counting=false; //*Flag to trigger timer delay*//
volatile int tic=0; 
volatile int target=0;
static int a =0;
static int slow=0;
void OGPIO_init(){
    RCC->AHB2ENR|=3UL; //*Enabling Clock of GPIO port A & B*//
    GPIOA->MODER&= ~((0x3U)<<14); 
    GPIOA->MODER|=((0x2U)<<14); //*Enabling Alternate Function on PA7*//
    GPIOA->PUPDR&= ~((0x3U)<<14);
    GPIOA->PUPDR|=(0x1U)<<14; //*Setting Internal Pull-up on PA7*//
    GPIOA->OTYPER&= ~(1UL<<7);
    GPIOA->OTYPER|=1UL<<7; //*Enabling Open-Drain Output on PA7 as required by I2C*//
	GPIOA->AFR[0]&= ~((0xFU)<<28);
    GPIOA->AFR[0]|=(0x4U)<<28; //*AF4 for Mapping PA7 to I2C3 SDA line*//
	GPIOB->MODER&= ~((0x3U)<<8);
    GPIOB->MODER|=((0x2U)<<8); //*Enabling Alternate Function on PB4*//
    GPIOB->PUPDR&= ~((0x3U)<<8);
    GPIOB->PUPDR|=(0x1U)<<8; //*Setting Internal Pull-up on PB4*//
    GPIOB->OTYPER&= ~(1UL<<4);
    GPIOB->OTYPER|=1UL<<4; //*Enabling Open-Drain Output on PB4 as required by I2C*//
    GPIOB->AFR[0]&= ~((0xFU)<<16);
    GPIOB->AFR[0]|=(0x4U)<<16; //*AF4 for Mapping PB4 to I2C3 SDA line*//
}
void I2C_init(I2C_TypeDef* I2Cx){
    RCC->CCIPR|=1UL<<16; //*Mapping I2C3's clock source to the system clock i.e 4MHz*//
    RCC->APB1ENR1|=1UL<<23; //*Enabling Clock of I2C3 on APB-1 Bus-1 via RCC*// 
    RCC->APB1RSTR1|=1UL<<23; //*Hard resetting I2C3 before configuring to remove any stale bits*//
    RCC->APB1RSTR1&= ~(1UL<<23); //*Completing the Reset*//
	  
	I2Cx->CR1=0; //*Clearing CR1 at once*//
    I2Cx->CR1|=3UL<<5; //*Enabling STOP interrupt &  Transfer Complete interrupt*//

    I2Cx->TIMINGR=0; //*Clearing Timing Register at once*//
    I2Cx->TIMINGR|=((2U<<16)|(3U<<8)|(5U)); //*SCLL = 5, SCLH = 8, SDADEL = 2, SCLDEL = 0 & Pre-scaler = 0 for 400 kHz fast mode*//

    I2Cx->CR1|=1UL;	//*Enabling I2C3 peripheral*//
    NVIC->IPR[72]=0x30; //*Setting priority of I2C3's Interrupt*//
    NVIC->ISER[2]|=1UL<<8; //*Enabling Interrupt of I2C3*//
}
void IDMA_init(){
    RCC->AHB1ENR|=1UL; //*Enabling Clock of DMA1 on AHB-1 Bus via RCC*//
    DMA1_Channel2->CCR=0; //*Clearing CCR at once*//

    DMA1_Channel2->CCR|=((0xAU)|(1UL<<4)); //*Enabling Transfer error & Transfer complete interrupt, Enabling read from memory*//
    DMA1_Channel2->CCR|=1UL<<7; //*Enabling Memory increment mode to read the full buffer*//
    DMA1_Channel2->CCR|=2UL<<12; //*DMA1 priority set to very high*//
    DMA1_CSELR->CSELR&= ~((0xFU)<<4); 
    DMA1_CSELR->CSELR|=(0x3U)<<4; //*Mappping I2C3 Tx to DMA1 Channel 2*//
    NVIC->IPR[12]|=0x30; //*Setting priority of DMA1 CH_2's interrupt*//
    NVIC->ISER[0]|=1UL<<12; //*Enabling DMA1 CH_2's interrupt*//
}
void I2C_Start(uint8_t add, uint32_t size, uint8_t *Tbuffer){
	busy=true; //*Busy set to true for Bus busy detection*//
	I2C3->CR2=0; //*Clearing CR2 at once*//
    uint32_t REG = (I2C3->CR2); //*Using Temporary register for CR2's Atomic configuration*//
    DMA1_Channel2->CCR&= ~1UL; //*Re-arming DMA1 CH_2 after every function call*//
    DMA1->IFCR|=(0xFU)<<4; //*Clearing Status Flags of DMA1 CH_2*// 
    bytes=size; //*Copied the value of the size function argument*//
    DMA1_Channel2->CMAR=(uint32_t)Tbuffer; //*Assigning the address of the buffer to CMAR as the source of transfer for Large chunks of Pixel data*// 
    DMA1_Channel2->CPAR=(uint32_t)&I2C3->TXDR; //*Assigning the address of I2C3's Transmit Data register as the destination of the transfer*//
    DMA1_Channel2->CNDTR=size; //*The size of the buffer determines the end of stream as a decrementing counter value*//
    DMA1_Channel2->CCR|=1UL; //*Enabling DMA1 CH_2 data stream & arming it before I2C3's initiation*//
	
    REG |= ((uint32_t)(add<<1)); //*Assigning the Address of the Slave device*//
    if(bytes>255){  //*Checks if bytes are greater than 255 as the NBYTES is only 8-bits & can store atmost 255 bytes*// 
		REG|=(255UL<<16); //*Alloting Maximum value of NBYTES to it*//
		REG|=1UL<<24; //*Enabling Re-load bit to continuously set the NBYTES counter*//
		bytes-=255; //*Subtracting the value from NBYTES to get the remaining bytes*//
	}
	else{ //*If the remaining bytes is less than 255 then the left value is set in NBYTES*// 
		REG|=(bytes<<16); 
		REG|=1UL<<25; //*Enables Autoend to end the transmission automatically*//
		bytes=0; //*Bytes set to 0*//
	}
    REG |= 1UL<<13; //*Setting the START bit*//
    I2C3->CR1|=1UL<<14; //*Enabling DMA data request for I2C3 by setting TXDMAEN bit in CR1*//
    I2C3->CR2=REG; //*Assigning the value of Temporary register back to CR2 to complete the atomic write*//
}
void I2C3_EV_IRQHandler(){
	if(I2C3->ISR & (1UL<<7)){ //*Checking if Transfer Complete flag is set*//
		uint32_t REG = (I2C3->CR2); //*Atomic write again & reload of the remaining data for continuous transmission*//
		REG&= ~(((0xFFU)<<16)|(1UL<<24)|(1UL<<25));
		if(bytes>255){
	     	REG|=(255UL<<16);
			REG|=1UL<<24;
			bytes-=255;
		}
		else{
			REG|=(bytes<<16);
			REG|=1UL<<25;
			bytes=0;
		}
        I2C3->CR2=REG; //*Reading CR2 automatically clears the TC flag in the ISR*// 
	}
    if(I2C3->ISR & (1UL<<5)){ //*Check if the STOP bit was sent successfully*//
        I2C3->ICR|=1UL<<5; //*Clears the STOPF flag*//
	    busy=false; //*Busy set to false indicating I2C's transfer is complete*//
	}	
}
void DMA1_Channel2_IRQHandler(){
    if(DMA1->ISR & (1UL<<5)){
        DMA1->IFCR|=1UL<<5; //*Clearing the TC flag*//
    }
    if(DMA1->ISR & (1UL<<7)){ 
        DMA1->IFCR|=1UL<<7; //*Clearing the Transfer error flag*//
    }
}
void OLED_init(){ //*This function transmits the Intitiation commands for the OLED to set it up for Bitmap reception*//
    uint8_t inst[]={0x00,0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x00,0xA1,0xC8,0xDA,0x12,0x81,0xCF,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF};
    I2C_Start(0x3C,sizeof(inst),inst);
}
void OTIM_init(){
	RCC->APB1ENR1|=1UL<<5; //*Enabling the Clock of Timer 7 on APB-1 Bus-1 via RCC*//
	TIM7->CR1=0; //*Clearing CR1 at once*//
	TIM7->CR1|=1UL<<2; //*Enabling Update on Counter overflow*//
	TIM7->CR2|=1UL<<5; 
	TIM7->PSC=399; //*Setting Pre-scaler as 399 which divides the system clock to 1KHz*// 
	TIM7->ARR=399; //*ARR set to 399 make the intervals as 40 ms*//
	TIM7->CNT=0; //*Counting starts from 0*//
	TIM7->DIER|=1UL; //*Timer 7 interrupt enable*//
	TIM7->EGR&= ~(1UL);
	TIM7->EGR|=1UL; //*Event generation enabled for the ISR*//
	TIM7->CR1|=1UL; //*Enabling TIM 7*//
    NVIC->IPR[55]=0x31; //*Setting priority of TIM-7's interrupt*//
	NVIC->ISER[1]|=1UL<<23; //*Enabling TIM-7's interrupt*// 
}
void TIM7_IRQHandler(){
	if(TIM7->SR & (1UL)){ //*Checking if Update event flag in Status register*//
	    TIM7->SR&= ~(1UL);	//*Clearing Update event flag*//
		if(grant && !busy){	//*If grant is set in the FSM then the Lock opening animation is triggered*// 
			if(counter==0){ //*Each frame is sent after every 40 ms using a counter to achieve 25 FPS of smooth animation*//
				DISPLAY_FRAME1();
				counter++;
			}
			else if(counter==1){
	    	    DISPLAY_FRAME2();
				counter++;
			}
			else if(counter==2){
			    DISPLAY_FRAME3();
				counter++;
			}
			else if(counter==3){
				DISPLAY_FRAME4();
				counter++;
			}
			else if(counter==4){
			    DISPLAY_FRAME5();
				counter++;
			}
			else if(counter==5){
				DISPLAY_FRAME6();
				counter++;
			}
			else if(counter==6){
				DISPLAY_FULL();
				counter=0;
		        grant=false; //*When last frame is transmitted, grant is set to false to prevent looping & re-arm it, counter is also reset*//
			}
		}
		if(alarmed && !busy){ //*If alarmed flag is set in the FSM then the Alarm bell with inverting graphics appears as an alert*//
			slow++; //*Used to scale down the Timer frequency*//
			if(slow>=12){ //*When slow reaches 12, it indicates a delay of ~480 ms to slowly transition between inverted frames*// 
				slow=0; //*Slow resets to 0 after every 480 ms to provide latency in transition*//
			    if(a==0){a=1;DISPLAY_ALARM();}
		        else if(a==1){a=0;DISPLAY_INVALARM();}
			}	
	    }  
		if(counting){ //*If counting is set in the FSM a non-blocking delay occurs to transition to other states without blocking any background tasks*//
			tic++; //*Used to scale down the Timer frequency*//
			if(tic>=target){
				tic=0;
				counting=false; //*Set to false to end when overflow occurs the delay*//
				done=true; //*Done set to true to inidicate that the delay has ended*//
			}
		}
	}	
}
void delay_hw(uint16_t ms){ //*Function used to generate a non-blocking delay using the running TIM-7*//
	done=false; //*Done set to false after each call*//
	counting =true; //*Counting is set to true to access the if block for a non-blocking delay in the ISR of TIM-7*//
	tic=0;
	target=ms/40; //*Function argument in Milli-seconds is divided by 40 as the timer update event occurs every 40 ms*//
}