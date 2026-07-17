#ifndef __HCSR04_H
#define __HCSR04_H

#include "stm32f10x.h"

void HC_SR04_Init(void);
float HC_SR04_GetDistance(void);   // 返回距离(cm)，-1表示超时/无效

#endif
