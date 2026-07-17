#include "beep.h"
#include "delay.h"
#include "sys.h"
#define _C3 131
#define _D3 147
#define _E3 165
#define _F3 175
#define _G3 196
#define _A3 220
#define _B3 247
#define _C4 262
#define _D4 294
#define _E4 330
#define _F4 349
#define _G4 392
#define _A4 440
#define _B4 494
#define _C5 523
#define _D5 587
#define _E5 659
#define _F5 698
#define _G5 784
#define _A5 880
#define _B5 988
#define _C6 1047
#define _R   0

typedef struct {
    uint16_t freq;
    uint16_t duration_ms;
} Note;

static volatile uint8_t  s_music_playing = 0;
static volatile uint8_t  s_music_stop = 0;
static volatile int      s_music_idx = -1;
static volatile uint32_t s_half_period_ticks = 0;
static volatile uint32_t s_flip_cnt = 0;
static volatile uint32_t s_note_remaining = 0;

static const Note *s_music_ptr = 0;
static int s_music_len = 0;

#define TICK_US  250

void beep_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOF, &GPIO_InitStructure);
    PFout(0) = 0;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseInitTypeDef TIM_Base;
    TIM_Base.TIM_Prescaler = 72 - 1;
    TIM_Base.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_Base.TIM_Period = TICK_US - 1;
    TIM_Base.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_Base.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_Base);

    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM1, DISABLE);

    NVIC_EnableIRQ(TIM1_UP_IRQn);
    NVIC_SetPriority(TIM1_UP_IRQn, 3);
}

void beep_on(uint16_t ms)
{
    uint32_t cycles = ms * 4;
    for (uint32_t i = 0; i < cycles; i++)
    {
        PFout(0) = !PFout(0);
        delay_us(250);
    }
    PFout(0) = 0;
}

void beep_stop_music(void) { s_music_stop = 1; }
uint8_t beep_is_playing(void) { return s_music_playing; }

static void load_note(int idx)
{
    if (idx >= s_music_len || s_music_stop)
    {
        s_music_playing = 0;
        PFout(0) = 0;
        TIM_Cmd(TIM1, DISABLE);
        s_music_idx = -1;
        return;
    }

    const Note *n = &s_music_ptr[idx];
    uint32_t duration_ticks = ((uint32_t)n->duration_ms * 1000) / TICK_US;
    if (duration_ticks < 1) duration_ticks = 1;

    if (n->freq > 0)
    {
        s_half_period_ticks = (500000 / n->freq) / TICK_US;
        if (s_half_period_ticks < 1) s_half_period_ticks = 1;
    }
    else
    {
        s_half_period_ticks = 0;
    }

    s_flip_cnt = 0;
    s_note_remaining = duration_ticks;
    PFout(0) = (n->freq > 0) ? 1 : 0;
}

static void music_start(const Note *melody, int len)
{
    TIM_Cmd(TIM1, DISABLE);
    s_music_stop = 0;
    s_music_ptr = melody;
    s_music_len = len;
    s_music_idx = 0;
    s_music_playing = 1;

    load_note(0);
    TIM_SetCounter(TIM1, 0);
    TIM_Cmd(TIM1, ENABLE);
}

void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == RESET) return;
    TIM_ClearITPendingBit(TIM1, TIM_IT_Update);

    if (s_music_stop || s_music_idx < 0 || !s_music_playing)
    {
        PFout(0) = 0;
        TIM_Cmd(TIM1, DISABLE);
        s_music_playing = 0;
        s_music_idx = -1;
        return;
    }

    if (s_half_period_ticks > 0)
    {
        s_flip_cnt++;
        if (s_flip_cnt >= s_half_period_ticks)
        {
            s_flip_cnt = 0;
            PFout(0) = !PFout(0);
        }
    }

    s_note_remaining--;
    if (s_note_remaining == 0)
    {
        PFout(0) = 0;
        s_music_idx++;
        load_note(s_music_idx);
    }
}

static const Note melody_xiongchumo[] = {
    {392,250},{392,250},{440,250},{392,250},{330,250},{294,250},{262,500},
    {330,250},{330,250},{349,250},{330,250},{294,250},{262,250},{294,500},
    {392,250},{392,250},{440,250},{392,250},{330,250},{294,250},{262,500},
    {330,250},{330,250},{349,250},{330,250},{294,250},{262,250},{262,500},
    {392,200},{392,200},{392,200},{330,600}
};

static const Note melody_contra[] = {
    {_G4,200},{_B4,200},{_D5,200},{_G5,200},
    {_E5,200},{_D5,200},{_B4,200},{_G4,400},
    {_G4,200},{_B4,200},{_D5,200},{_G5,200},
    {_A5,200},{_G5,200},{_E5,200},{_D5,400},
    {_G4,200},{_A4,200},{_B4,200},{_D5,200},
    {_C5,200},{_B4,200},{_A4,200},{_G4,400},
    {_G4,200},{_A4,200},{_B4,200},{_E5,200},
    {_D5,200},{_C5,200},{_B4,200},{_A4,400},
    {_A4,200},{_B4,200},{_C5,200},{_E5,200},
    {_D5,200},{_C5,200},{_B4,200},{_A4,400},
    {_G4,200},{_B4,200},{_D5,200},{_G5,200},
    {_E5,200},{_D5,200},{_B4,200},{_G4,400},
    {_G4,200},{_A4,200},{_B4,200},{_D5,200},
    {_C5,200},{_B4,200},{_A4,200},{_G4,400},
    {_G4,200},{_A4,200},{_B4,200},{_E5,200},
    {_E5,200},{_D5,200},{_C5,200},{_B4,400},
    {_C5,200},{_D5,200},{_E5,200},{_G5,200},
    {_E5,200},{_D5,200},{_C5,200},{_A4,400},
    {_B4,200},{_D5,200},{_G5,200},{_G5,200},
    {_E5,200},{_D5,200},{_B4,200},{_G4,400},
    {_B4,200},{_D5,200},{_G5,200},{_E5,200},
    {_D5,200},{_C5,200},{_B4,200},{_A4,400},
    {_C5,200},{_E5,200},{_G5,200},{_E5,200},
    {_D5,200},{_C5,200},{_B4,200},{_G4,400},
    {_B4,200},{_D5,200},{_G5,200},{_E5,200},
    {_G5,200},{_E5,200},{_D5,200},{_C5,400},
    {_B4,200},{_D5,200},{_G5,200},{_E5,200},
    {_D5,200},{_C5,200},{_B4,200},{_G4,400},
    {_G4,200},{_A4,200},{_B4,200},{_D5,200},
    {_C5,200},{_B4,200},{_A4,200},{_G4,400},
    {_G4,200},{_A4,200},{_B4,200},{_E5,200},
    {_D5,400},{_C5,400},{_B4,400},{_G4,800},
};

static const Note melody_ji[] = {
    {_G5,300},{_G5,150},{_A5,300},{_G5,300},{_E5,300},{_D5,300},{_C5,150},{_D5,300},
    {_E5,300},{_G5,300},{_A5,300},{_G5,300},{_E5,300},{_D5,300},{_C5,150},{_D5,300},
    {_E5,300},{_E5,150},{_G5,300},{_E5,300},{_D5,300},{_C5,300},{_D5,150},{_E5,300},
    {_C5,300},{_D5,300},{_E5,300},{_G5,300},{_A5,300},{_G5,300},{_E5,300},{_D5,600},
    {_G5,300},{_G5,150},{_A5,300},{_G5,300},{_E5,300},{_D5,300},{_C5,150},{_D5,300},
    {_E5,300},{_G5,300},{_A5,300},{_G5,300},{_E5,300},{_D5,300},{_C5,300},{_D5,600},
};

static const Note melody_twinkle[] = {
    {_C4,300},{_C4,300},{_G4,300},{_G4,300},{_A4,300},{_A4,300},{_G4,600},
    {_F4,300},{_F4,300},{_E4,300},{_E4,300},{_D4,300},{_D4,300},{_C4,600},
    {_G4,300},{_G4,300},{_F4,300},{_F4,300},{_E4,300},{_E4,300},{_D4,600},
    {_G4,300},{_G4,300},{_F4,300},{_F4,300},{_E4,300},{_E4,300},{_D4,600},
    {_C4,300},{_C4,300},{_G4,300},{_G4,300},{_A4,300},{_A4,300},{_G4,600},
    {_F4,300},{_F4,300},{_E4,300},{_E4,300},{_D4,300},{_D4,300},{_C4,600},
};

static const Note melody_tigers[] = {
    {_C4,300},{_D4,300},{_E4,300},{_C4,300},
    {_C4,300},{_D4,300},{_E4,300},{_C4,300},
    {_E4,300},{_F4,300},{_G4,600},
    {_E4,300},{_F4,300},{_G4,600},
    {_G4,150},{_A4,150},{_G4,150},{_F4,150},{_E4,300},{_C4,300},
    {_G4,150},{_A4,150},{_G4,150},{_F4,150},{_E4,300},{_C4,300},
    {_C4,300},{_G3,300},{_C4,600},
    {_C4,300},{_G3,300},{_C4,600},
};

static const Note melody_birthday[] = {
    {_G4,200},{_G4,200},{_A4,400},{_G4,400},{_C5,400},{_B4,800},
    {_G4,200},{_G4,200},{_A4,400},{_G4,400},{_D5,400},{_C5,800},
    {_G4,200},{_G4,200},{_G5,400},{_E5,400},{_C5,400},{_B4,400},{_A4,400},
    {_F5,200},{_F5,200},{_E5,400},{_C5,400},{_D5,400},{_C5,800},
};

static const Note melody_mama[] = {
    {_C5,250},{_C5,250},{_D5,250},{_E5,250},{_C5,500},{_A4,250},{_A4,250},
    {_G4,250},{_E4,250},{_F4,250},{_G4,250},{_C5,500},{_R,250},
    {_C5,250},{_C5,250},{_D5,250},{_E5,250},{_C5,500},{_A4,250},{_A4,250},
    {_G4,250},{_E4,250},{_F4,250},{_G4,250},{_C5,500},{_R,250},
    {_A4,250},{_A4,250},{_A4,250},{_F4,250},{_G4,500},{_C5,250},{_R,250},
    {_C5,250},{_D5,250},{_C5,250},{_A4,250},{_G4,500},{_E4,250},{_R,500},
};

static const Note melody_jasmine[] = {
    {_E5,250},{_G5,250},{_A5,375},{_A5,125},{_G5,250},{_E5,250},{_D5,250},{_E5,250},{_F5,250},{_E5,250},{_D5,250},{_C5,500},
    {_E5,250},{_G5,250},{_A5,375},{_A5,125},{_G5,250},{_E5,250},{_D5,250},{_E5,250},{_F5,250},{_E5,250},{_D5,250},{_C5,500},
    {_C5,250},{_D5,250},{_E5,250},{_F5,250},{_G5,250},{_A5,250},{_G5,250},{_F5,250},{_E5,250},{_D5,500},
    {_E5,250},{_G5,250},{_A5,375},{_A5,125},{_G5,250},{_E5,250},{_D5,250},{_E5,250},{_F5,250},{_E5,250},{_D5,250},{_C5,500},
};

static const Note melody_elise[] = {
    {_E5,150},{_D5,150},{_E5,150},{_D5,150},{_E5,150},{_B4,150},{_D5,150},{_C5,150},
    {_A4,300},{_R,150},{_C4,150},{_E4,150},{_A4,150},{_B4,300},{_R,150},
    {_E4,150},{_A4,150},{_B4,150},{_C5,300},{_R,150},{_E4,150},{_E5,150},{_D5,150},
    {_C5,150},{_B4,150},{_A4,150},{_B4,150},{_C5,150},{_D5,150},{_E5,300},{_R,150},
    {_E5,150},{_D5,150},{_E5,150},{_D5,150},{_E5,150},{_B4,150},{_D5,150},{_C5,150},
    {_A4,300},{_R,150},{_C4,150},{_E4,150},{_A4,150},{_B4,300},{_R,150},
    {_E4,150},{_C5,150},{_B4,150},{_A4,300},{_R,150},{_B4,150},{_C5,150},{_D5,300},
};

static const Note melody_castle[] = {
    {_C5,250},{_D5,250},{_E5,250},{_C5,250},{_D5,250},{_E5,125},{_C5,125},{_D5,375},{_R,125},
    {_E5,250},{_C5,250},{_D5,250},{_E5,125},{_F5,125},{_E5,250},{_D5,250},{_C5,500},{_R,250},
    {_G4,250},{_A4,250},{_C5,250},{_D5,250},{_E5,250},{_F5,250},{_E5,250},{_D5,250},
    {_C5,250},{_D5,250},{_E5,250},{_C5,250},{_D5,250},{_C5,250},{_A4,500},{_R,250},
    {_C5,250},{_D5,250},{_E5,250},{_C5,250},{_D5,250},{_E5,125},{_C5,125},{_D5,375},{_R,125},
    {_E5,250},{_C5,250},{_D5,250},{_E5,125},{_F5,125},{_E5,250},{_D5,250},{_C5,500},{_R,500},
};

static const Note melody_always[] = {
    {_C5,250},{_D5,125},{_E5,125},{_E5,125},{_F5,125},{_E5,250},{_D5,250},{_C5,250},{_D5,125},{_E5,125},
    {_C5,500},{_R,250},{_A4,250},{_C5,125},{_D5,125},{_E5,375},{_R,125},
    {_F5,125},{_E5,125},{_D5,250},{_F5,125},{_E5,125},{_D5,250},{_C5,125},{_A4,125},
    {_C5,500},{_R,250},{_A4,125},{_C5,125},{_D5,375},{_R,125},{_E5,125},{_C5,125},
    {_D5,250},{_R,125},{_A4,125},{_C5,125},{_D5,125},{_E5,250},{_F5,125},{_E5,125},
    {_D5,375},{_R,125},{_C5,125},{_D5,125},{_E5,125},{_C5,250},{_D5,125},{_E5,125},
    {_C5,500},{_R,500},
};

static const Note melody_canon[] = {
    {_D5,200},{_A4,200},{_B4,200},{_F4,200},{_G4,200},{_D4,200},{_G4,200},{_A4,200},
    {_D5,200},{_A4,200},{_B4,200},{_F4,200},{_G4,200},{_D4,200},{_G4,200},{_A4,200},
    {_B4,200},{_B3,200},{_D4,200},{_G4,200},{_A4,200},{_D4,200},{_G4,200},{_F4,200},
    {_E4,200},{_C5,200},{_B4,200},{_A4,200},{_G4,200},{_B4,200},{_A4,200},{_G4,200},
    {_F4,200},{_A4,200},{_G4,200},{_F4,200},{_E4,200},{_G4,200},{_F4,200},{_E4,200},
    {_D4,200},{_A4,200},{_B4,200},{_F4,200},{_G4,200},{_D4,200},{_G4,200},{_A4,200},
    {_D5,200},{_A4,200},{_B4,200},{_F4,200},{_G4,200},{_D4,200},{_G4,200},{_A4,200},
    {_D4,400},{_R,400},
};

static const Note melody_fairytale[] = {
    {_G4,300},{_E4,150},{_G4,150},{_A4,300},{_G4,300},{_E4,600},
    {_D4,300},{_E4,300},{_G4,300},{_A4,300},{_C5,300},{_A4,300},{_G4,600},
    {_G4,300},{_E4,150},{_G4,150},{_A4,300},{_G4,300},{_E4,600},
    {_D4,300},{_E4,300},{_G4,300},{_A4,300},{_G4,300},{_E4,600},
    {_D4,300},{_G4,150},{_A4,150},{_C5,300},{_D5,300},{_C5,300},{_A4,300},{_G4,300},
    {_E4,300},{_D4,300},{_E4,300},{_G4,600},
};

void beep_music_xiongchumo(void) {
    music_start(melody_xiongchumo, sizeof(melody_xiongchumo)/sizeof(melody_xiongchumo[0]));
}
void beep_music_contra(void) {
    music_start(melody_contra, sizeof(melody_contra)/sizeof(melody_contra[0]));
}
void beep_music_ji(void) {
    music_start(melody_ji, sizeof(melody_ji)/sizeof(melody_ji[0]));
}
void beep_music_twinkle(void) {
    music_start(melody_twinkle, sizeof(melody_twinkle)/sizeof(melody_twinkle[0]));
}
void beep_music_tigers(void) {
    music_start(melody_tigers, sizeof(melody_tigers)/sizeof(melody_tigers[0]));
}
void beep_music_birthday(void) {
    music_start(melody_birthday, sizeof(melody_birthday)/sizeof(melody_birthday[0]));
}
void beep_music_mama(void) {
    music_start(melody_mama, sizeof(melody_mama)/sizeof(melody_mama[0]));
}
void beep_music_jasmine(void) {
    music_start(melody_jasmine, sizeof(melody_jasmine)/sizeof(melody_jasmine[0]));
}
void beep_music_elise(void) {
    music_start(melody_elise, sizeof(melody_elise)/sizeof(melody_elise[0]));
}
void beep_music_castle(void) {
    music_start(melody_castle, sizeof(melody_castle)/sizeof(melody_castle[0]));
}
void beep_music_always(void) {
    music_start(melody_always, sizeof(melody_always)/sizeof(melody_always[0]));
}
void beep_music_canon(void) {
    music_start(melody_canon, sizeof(melody_canon)/sizeof(melody_canon[0]));
}
void beep_music_fairytale(void) {
    music_start(melody_fairytale, sizeof(melody_fairytale)/sizeof(melody_fairytale[0]));
}

static void (*s_song_list[])(void) = {
    beep_music_twinkle,
    beep_music_tigers,
    beep_music_birthday,
    beep_music_mama,
    beep_music_jasmine,
    beep_music_elise,
    beep_music_castle,
    beep_music_always,
    beep_music_canon,
    beep_music_fairytale,
    beep_music_xiongchumo,
    beep_music_contra,
    beep_music_ji,
};
#define SONG_COUNT  (sizeof(s_song_list) / sizeof(s_song_list[0]))

void beep_music_next(void)
{
    static uint8_t current = 0;
    s_song_list[current]();
    current = (current + 1) % SONG_COUNT;
}
