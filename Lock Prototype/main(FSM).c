#include "stm32l432xx.h"
#include "UART_pass.h"
#include "LATCH.h"
#include "REED_EXTI.h"
#include "BUZZER.h"
#include "SysTick.h"
#include <stdbool.h>
typedef enum{
	IDLE,
	RECEIVE,
	CHECK,
	WAIT,
	GRANTED,
	DENIED,
	BREACH
}state_t;
volatile state_t state = IDLE; 
uint8_t c[3]; //*Reception buffer for the trigger data for One character & CR+LF*//
uint8_t ch[6]; //*Reception buffer for the passkey & CR+LF*//
const uint8_t IN[15]="ENTER PASSWORD:";
const uint8_t ACC[15]="ACCESS GRANTED!";
const uint8_t REJ[21]="INCORRECT! TRY AGAIN:";
const uint8_t AL[7]="ALERT!!";
volatile int count=0; //*Incorrect password & tries counter*//
volatile bool armed=false; 
volatile bool alerted=false; 
volatile bool granted=false;
volatile bool received=false;
extern volatile bool access;
extern volatile bool breach;
extern volatile bool reload;
extern volatile bool rx;
extern volatile bool tx;
int main(){
	LOCK_init();
	UGPIOinit();
	RGPIO_init();
	BGPIO_init();
	UART_init(USART1,0x1A0);
	BUZZER_init();
	DMA_init();
	systick_init(4000);
	REED_init();
    while(1){
    	switch(state){	
//Resets security flags, reset the lock & places the system in listening mode to trigger the bluetooth for Password reception*// 		
    	  case IDLE:  
			if(!armed){	
    		    LOCK_CLOSE();
			    alerted=false;
				access=false;
				granted=false;
				reload=false;
				received=false;
    		    UDMA_Rx(c,3);
				armed=true;}
    		if(rx){
				rx=false;
				armed=false;
    		    state = RECEIVE;
    		}
    		break;
//*Acknowledges the start of transmission & triggers a DMA-based read to capture the full password string*//	
    	  case RECEIVE:
    		if(!received){
    		    UDMA_Tx((uint8_t *)IN,15);
    			received=true;
    			UDMA_Rx(ch,6);
    		}
    		if(rx){
				rx=false;
    		    state = CHECK;
    		}
    		break;
//*Performs a bit by bit comparision of the received buffer against the hardcoded '1234' key to determine the next state*//		
    	   case CHECK:
    		 if(ch[0]=='1' && ch[1]=='2' && ch[2]=='3' && ch[3]=='4'){
    		    state = GRANTED;
    		  }
    		  else{
    		    if(count==3){
    		    state = DENIED;
    		    }else{
					count++;	
    		        UDMA_Tx((uint8_t *)REJ,21);
					state = WAIT;
                }}
    		break;
//*Acts as a non-blocking state that ensures the 'Incorrect Password' message is fully transmitted before allowing a retry*//			
		   case WAIT:
            if(tx){
				tx=false;
				UDMA_Rx(ch,6);
				state = RECEIVE;
			}
            break;
//*Energizes the solenoidal latch & sets the access flag, allowing the door to be opened without triggering an alarm*//									 
    	   case GRANTED:
			 if(!granted){ 
    		   OPEN_LOCK();
    		   UDMA_Tx((uint8_t *)ACC,15);
    		   received = false;
    		   count = 0;
    		   access = true;
			   granted =true;
			}
			if(reload){
    		    state = IDLE;}
    		break;
//*Executes a 10-second penalty lockout with a continuos buzzer alert after four consecutive failed password attempts*// 			
    	   case DENIED:
			 if(!alerted){ 
			    UDMA_Tx((uint8_t *)AL,7);
				ALERT();
				delay(10000);
				alerted=true;
				count=0;
				received=false;
				DISABLE();
				state = IDLE;}
    		break;
//*Triggered by the Reed Switch EXTI, this states activates a high-priority emergency alert if the door is opened without authorization*//			
    	   case BREACH:
			 ALERT();
       	   break;
        }
//*Constantly checking if there is unauthorized access of the door, every loop iteration*//
	    if(!access && breach) 
          {state = BREACH;}
    }
}    