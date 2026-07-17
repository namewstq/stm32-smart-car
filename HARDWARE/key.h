#ifndef _KEY_H_
#define _KEY_H_
#include <stm32f10x.h>

extern volatile uint8_t g_key1_pressed;
extern volatile uint8_t g_key0_pressed;

void key_init(void);
void KEY_EXTI_Init(void);

#endif

