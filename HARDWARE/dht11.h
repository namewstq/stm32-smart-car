#ifndef __DHT11_H__
#define __DHT11_H__

#include "stm32f10x.h"

void DHT_Init_InPut(void);
void DHT_Init_OutPut(void);
void DHT_STart(void);
uint16_t DHT_Scan(void);
uint16_t DHT_ReadBit(void);
uint16_t DHT_ReadByte(void);
uint16_t DHT_ReadData(uint8_t buffer[5]);

#endif
