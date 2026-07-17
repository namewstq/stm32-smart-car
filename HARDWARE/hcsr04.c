#include "hcsr04.h"
#include "delay.h"

/*
 * 超声波模块
 * Trig → PA7   (GPIO 输出)
 * Echo → PA6   (GPIO 输入，轮询方式)
 * TIM3 作为微秒计时器
 */

#define TRIG_PORT   GPIOA
#define TRIG_PIN    GPIO_Pin_7
#define ECHO_PORT   GPIOA
#define ECHO_PIN    GPIO_Pin_6

/* ─── TIM3 初始化（1 tick = 1 μs） ─── */
static void HC_SR04_TIM_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_Base;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_Base.TIM_Prescaler     = 72 - 1;
    TIM_Base.TIM_Period        = 65535;
    TIM_Base.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_Base.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM3, &TIM_Base);

    TIM_Cmd(TIM3, ENABLE);
}

/* ─── 初始化 ─── */
void HC_SR04_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA7 → Trig 推挽输出 */
    GPIO_InitStruct.GPIO_Pin   = TRIG_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TRIG_PORT, &GPIO_InitStruct);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    /* PA6 → Echo 浮空输入 */
    GPIO_InitStruct.GPIO_Pin   = ECHO_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(ECHO_PORT, &GPIO_InitStruct);

    HC_SR04_TIM_Init();
}

/* ─── 测距：返回 cm，-1 表示超时 ─── */
float HC_SR04_GetDistance(void)
{
    uint32_t pulse_us;

    /* 发送 12μs 触发脉冲 */
    GPIO_SetBits(TRIG_PORT, TRIG_PIN);
    delay_us(12);
    GPIO_ResetBits(TRIG_PORT, TRIG_PIN);

    /* 等待 Echo 升为 HIGH（超时 10ms） */
    TIM_SetCounter(TIM3, 0);
    while (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == 0)
    {
        if (TIM_GetCounter(TIM3) > 10000)
            return -1.0f;
    }

    /* 测量 Echo HIGH 脉宽（超时 30ms） */
    TIM_SetCounter(TIM3, 0);
    while (GPIO_ReadInputDataBit(ECHO_PORT, ECHO_PIN) == 1)
    {
        if (TIM_GetCounter(TIM3) > 30000)
            break;
    }

    pulse_us = TIM_GetCounter(TIM3);
    if (pulse_us == 0)
        return -1.0f;

    /* 距离(cm) = 脉宽(μs) / 58 */
    return (float)pulse_us / 58.0f;
}
