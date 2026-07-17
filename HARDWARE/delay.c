#include "delay.h"

static uint8_t fac_us = 0;   // 微秒延时系数

/*
 * 函数：delay_init
 * 功能：初始化SysTick
 * 说明：系统时钟SYSCLK=72MHz
 */
void delay_init(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);//分频/8，使得计数频率为72M/8=9MHz

    // SysTick时钟 = 72MHz / 8 = 9MHz
    // 1us = 9个计数
    fac_us = SystemCoreClock / 8000000;//9

    // 1ms = 1000us
    //fac_ms = (uint16_t)fac_us * 1000;  //9000
}

/*
 * 函数：delay_us
 * 功能：微秒延时
 */
void delay_us(uint32_t nus)
{
    uint32_t temp;

    SysTick->LOAD = nus * fac_us;   // 装载值
    SysTick->VAL = 0x00;            // 清空计数器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 开始计数

    do
    {
        temp = SysTick->CTRL;
    }
    while((temp & 0x01) && !(temp & (1 << 16)));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭计数器
    SysTick->VAL = 0x00;
}

/*
 * 函数：delay_ms
 * 功能：毫秒延时
 */
void delay_ms(uint32_t nms)
{
    while(nms--)
    {
        delay_us(1000);
    }
}
