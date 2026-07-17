#ifndef _BEEP_H_
#define _BEEP_H_

#include "stm32f10x.h"

void beep_init(void);
void beep_on(uint16_t ms);

void beep_music_xiongchumo(void);
void beep_music_contra(void);
void beep_music_ji(void);
void beep_music_twinkle(void);
void beep_music_tigers(void);
void beep_music_birthday(void);
void beep_music_mama(void);
void beep_music_jasmine(void);
void beep_music_elise(void);
void beep_music_castle(void);
void beep_music_always(void);
void beep_music_canon(void);
void beep_music_fairytale(void);
void beep_music_next(void);
void beep_stop_music(void);
uint8_t beep_is_playing(void);

#endif
