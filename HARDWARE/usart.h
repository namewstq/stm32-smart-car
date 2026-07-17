#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <string.h>
#include <stdio.h>

typedef enum {
    MODE_REMOTE = 0,
    MODE_OBSTACLE_AVOID,
    MODE_HYBRID,
    MODE_FOLLOW,
    MODE_LINE_FOLLOW
} CarMode;

extern CarMode g_current_mode;
extern volatile uint8_t g_log;
extern volatile uint8_t g_manual_timer;
extern volatile uint8_t g_remote_active;
extern volatile uint8_t g_oled_page;

void USART1_Init(uint32_t baud);
void USART1_SendByte(uint8_t ch);
void USART1_SendString(char *str);

void USART2_Init(uint32_t baudrate);
void USART2_SendChar(uint8_t ch);
void USART2_SendString(char *str);

void USART3_Init(uint32_t baud);
void USART3_SendString(char *str);
void usart3_send_bytes(uint8_t *buf, uint32_t len);
void usart3_send_str(char *buf);

void Cmd_SwitchMode(uint8_t mode);
void parse_cmd(void);

#endif
