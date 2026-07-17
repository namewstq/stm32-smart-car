#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

#define PWM_MAX     999

extern volatile uint8_t g_motor_running;

void Motor_Init(void);

void Motor_GPIO_Init(void);

void Motor_SetSpeed(int16_t left,int16_t right);

void Motor_Stop(void);

void Motor_Forward(uint16_t speed);

void Motor_Backward(uint16_t speed);

void Motor_Left(uint16_t speed);

void Motor_Right(uint16_t speed);

// ─── 单轮转向（避障专用） ───
void Motor_LeftPivot(uint16_t speed);        // 左轮停、右轮进 → 绕左轮向左转
void Motor_RightPivot(uint16_t speed);       // 右轮停、左轮进 → 绕右轮向右转
void Motor_LeftPivotBack(uint16_t speed);    // 左轮停、右轮退 → 绕左轮向后甩尾
void Motor_RightPivotBack(uint16_t speed);   // 右轮停、左轮退 → 绕右轮向后甩尾

#endif
