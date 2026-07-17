#include "servo.h"

static uint16_t Servo_AngleToCCR(float angle);

void Servo_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBase;
    TIM_OCInitTypeDef TIM_OC;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // PA0 -> TIM2_CH1

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA,&GPIO_InitStruct);

    // 1MHz

    TIM_TimeBase.TIM_Prescaler = 72-1;

    //20ms

    TIM_TimeBase.TIM_Period = 20000-1;

    TIM_TimeBase.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBase.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBase.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM2,&TIM_TimeBase);

    TIM_OC.TIM_OCMode = TIM_OCMode_PWM1;

    TIM_OC.TIM_OutputState = TIM_OutputState_Enable;

    TIM_OC.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC.TIM_Pulse = 1500;

    TIM_OC1Init(TIM2,&TIM_OC);

    TIM_OC1PreloadConfig(TIM2,TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM2,ENABLE);

    TIM_Cmd(TIM2,ENABLE);
}

static uint16_t Servo_AngleToCCR(float angle)
{
    if(angle < 0)
        angle = 0;

    if(angle > 180)
        angle = 180;

    return (uint16_t)(500 + angle * 2000.0f / 180.0f);
}

void Servo_SetAngle(float angle)
{
    TIM_SetCompare1(TIM2, Servo_AngleToCCR(angle));
}
