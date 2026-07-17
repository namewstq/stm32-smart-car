#include "music.h"
#include "sys.h"
#include "delay.h"

/**
 * 初始化 PF0 为推挽输出
 */
void music_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOF, &GPIO_InitStructure);
    PFout(0) = 0;
}

/**
 * 播放一个音符（阻塞）
 * 用 12.5% 低占空比短脉冲，避免蜂鸣器过于尖锐
 */
void music_play_note(uint16_t freq, uint16_t duration_ms)
{
    if (freq == 0) {
        delay_ms(duration_ms);
        return;
    }

    /* 半周期 → 总周期 */
    uint32_t half_us  = 500000UL / freq;
    uint32_t period_us = half_us * 2;

    /* 低占空比：脉冲 = 1/8 周期，其余低电平 */
    uint32_t pulse_us = period_us / 8;
    uint32_t low_us   = period_us - pulse_us;

    uint32_t cycles = ((uint32_t)duration_ms * 1000UL) / period_us;

    for (uint32_t i = 0; i < cycles; i++) {
        PFout(0) = 1;
        delay_us(pulse_us);
        PFout(0) = 0;
        delay_us(low_us);
    }
}

/**
 * 播放乐谱
 */
void music_play(const Note_t *score, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        music_play_note(score[i].freq, score[i].duration);
        if (score[i].freq != 0) {
            delay_ms(2);
        }
    }
}

/**
 * 测试音：低音提示，不刺耳
 */
void music_test(void)
{
    music_play_note(523, 200);   // C5
    delay_ms(100);
    music_play_note(523, 200);
    delay_ms(100);
    music_play_note(784, 400);   // G5
}
