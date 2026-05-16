#include "stm32l432xx.h"
#include "UART_pass.h"
#include "LATCH.h"
#include "REED_EXTI.h"
#include "BUZZER.h"
#include "OLED_I2C.h"
#include "BITMAPS.h"
#include <stdbool.h>
extern volatile state_t state; 
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
volatile bool wrong=false;
volatile bool config=false;
volatile bool sleep=false;
volatile bool stop=false;
extern volatile bool access;
extern volatile bool counting;
extern volatile bool breach;
extern volatile bool reload;
extern volatile bool rx;
extern volatile bool tx;
extern volatile bool busy;
extern volatile bool grant;
extern volatile bool alarmed;
extern volatile bool done;
extern volatile bool wake;
int main(){
	HSI_enable();
	LOCK_init();
	RGPIO_init();
	BGPIO_init();
	LPUGPIO_init();
	LPUART_init(0x682AB);
	DMA_init();
	OGPIO_init();
	I2C_init(I2C3);
	IDMA_init();
	OTIM_init();
	REED_init();
	BUZZER_init();
    OLED_init();
	for(volatile int i=0;i<1000;i++);
	while(1){
		switch(state){
//*After Boot & Initialisation, Enters Stop-2 mode to save power*// 
			case SLEEP:			
                if(!sleep){
			    	LOCK_CLOSE();
					DISPLAY_OFF();
					sleep=true;
				}
			    if(!stop && !busy){
					stop=true;
					Enter_Stop2();
				}
			break;
//*Resets security flags, reset the lock & places the system in listening mode to trigger the bluetooth for Password reception*// 		
    	  case IDLE:  
			if(!armed && !busy){
			    busy=false;	
			    alerted=false;
				access=false;
				granted=false;
				reload=false;
				received=false;
				armed=true;
				DISPLAY_LOCK();
				armed=false;
    		    state = RECEIVE;
    		}
    		break;
//*Acknowledges the start of transmission & triggers a DMA-based read to capture the full password string*//	
    	  case RECEIVE:
    		if(!received && rx){
				rx=false;
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
    		    }
                else{
					DISPLAY_CROSS(); //*Displaying a cross on the OLED indicating a wrong attempt*//
					count++;	
    		        UDMA_Tx((uint8_t *)REJ,21);
					state = WAIT;
                }
			}
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
			   busy=false;
               grant=true; //*Setting 'grant' triggers the animation of Lock opening with 25-FPS*//				 
    		   OPEN_LOCK();
    		   UDMA_Tx((uint8_t *)ACC,15);
    		   received = false;
    		   count = 0;
			   breach=false;
    		   access = true;
			   granted =true;
				delay_hw(10000);
			}
			if(done){
			   done=false;
			   LOCK_CLOSE();
			}
			if(reload){
				counting=false;
				config=false;
				grant=false;				//*Resetting to stop the animation & prvent looping back*//
				state = SLEEP;
			}
    		break;
//*Executes a 10-second penalty lockout with a continuos buzzer alert after four consecutive failed password attempts*// 			
    	   case DENIED:
			 if(!alerted){ 
			    UDMA_Tx((uint8_t *)AL,7);
				ALERT();
				alarmed=true; //*Triggering the Alarm bell inversion animation for visual feedback for an Alert*//
				delay_hw(15000);
				alerted=true;
				count=0;
				received=false;
			}
			if(done){
			   alarmed=false; //*Reseting to Stop the Inversion animation*//	 
			   DISABLE();
			   state = SLEEP;
			}
    		break;
//*Triggered by the Reed Switch EXTI, this states activates a high-priority emergency alert if the door is opened without authorization*//			
    	   case BREACH:
			alarmed=true; //*Triggering the Alarm bell inversion animation for visual feedback for an Alert*//
			ALERT();
       	    break;
        }
//*Constantly checking if there is unauthorized access of the door, every loop iteration*//
	    if(!access && breach) 
        {state = BREACH;}
    }
}       