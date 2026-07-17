#ifndef __MUSIC_H
#define __MUSIC_H

#include "stm32f10x.h"

/* 音符频率（Hz） */
#define NOTE_REST  0
#define NOTE_C4    262
#define NOTE_D4    294
#define NOTE_E4    330
#define NOTE_F4    349
#define NOTE_G4    392
#define NOTE_A4    440
#define NOTE_B4    494
#define NOTE_C5    523
#define NOTE_D5    587
#define NOTE_E5    659
#define NOTE_F5    698
#define NOTE_G5    784
#define NOTE_A5    880
#define NOTE_B5    988
#define NOTE_C6    1047

/* 音符结构：频率 + 时长(ms) */
typedef struct {
    uint16_t freq;
    uint16_t duration;
} Note_t;

/* 函数声明 */
void music_init(void);                                    // 初始化PF0
void music_play_note(uint16_t freq, uint16_t duration_ms); // 播放一个音符
void music_play(const Note_t *score, uint16_t len);        // 播放乐谱
void music_test(void);                                     // 测试音

#endif
