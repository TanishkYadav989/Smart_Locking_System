#include "stm32l432xx.h"
#include "MAX_ADC.h"
#include "LATCH.h"
int main(){
  Clock_LSI();
	LPTIM1_init();
	LOCK_init();
	uint16_t adc[1];
	AGPIO_init();
	ADC_init(adc,1);
	TRGO_init();
	while(1);
}