#ifndef BUZZER_H
#define BUZZER_H
#include "stm32l432xx.h"
void BGPIO_init();
void BUZZER_init();
void ALERT();
void DISABLE();
#endif