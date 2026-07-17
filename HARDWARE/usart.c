#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys.h>
#include <beep.h>
#include <delay.h>
#include <motor.h>
#include <servo.h>
#include <oled.h>

volatile u16 speed=650;

CarMode g_current_mode = MODE_REMOTE;
volatile uint8_t g_log = 0;
volatile uint8_t g_manual_timer = 0;
volatile uint8_t g_remote_active = 0;

uint8_t USART2_RX_BUF[128];
uint8_t USART2_RX_CNT = 0;

u8 uart_buf[200];
u16 uart_cnt = 0;
u8 uart_flag = 0;

/* ESP8266 buffers */
uint8_t  g_esp8266_tx_buf[512];
volatile uint8_t  g_esp8266_rx_buf[1024];
volatile uint32_t g_esp8266_rx_cnt = 0;
volatile uint8_t  g_esp8266_rx_overflow = 0;
volatile uint32_t g_esp8266_rx_end = 0;
volatile uint32_t g_esp8266_transparent_transmission_sta = 0;

int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

/*
    USART1: TX-PA9, RX-PA10 (debug printf)
*/
void USART1_Init(uint32_t baud)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_USART1 |
                           RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1,&USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1,ENABLE);
}

/*
    USART2: TX-PA2, RX-PA3 (command 9600bps)
*/
void USART2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);
}

/*
    USART3: TX-PB10, RX-PB11 (ESP8266, 115200bps)
*/
void USART3_Init(uint32_t baud)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);
}

void USART1_SendByte(uint8_t ch)
{
    USART_SendData(USART1,ch);
    while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}

void USART2_SendChar(uint8_t ch)
{
    USART_SendData(USART2, ch);
    while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void USART1_SendString(char *str)
{
    while(*str)
    {
        USART1_SendByte(*str++);
    }
}

void USART2_SendString(char *str)
{
    while(*str)
    {
        USART2_SendChar(*str++);
    }
}

void USART3_SendString(char *str)
{
    while(*str)
    {
        USART_SendData(USART3, (uint8_t)*str++);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    }
}

void usart3_send_bytes(uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        USART_SendData(USART3, buf[i]);
        while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    }
}

void usart3_send_str(char *buf)
{
    USART3_SendString(buf);
}

void Cmd_SwitchMode(uint8_t mode)
{
    if (mode > MODE_LINE_FOLLOW) return;

    Motor_Stop();
    Servo_SetAngle(90);
    beep_on(80);

    g_current_mode = (CarMode)mode;
    g_manual_timer = 0;

    char buf[20];
    sprintf(buf, "MODE:%d\r\n", mode);
    USART2_SendString(buf);
}

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        USART_ReceiveData(USART1);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void USART2_IRQHandler(void)
{
    uint8_t res;

    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        res = USART_ReceiveData(USART2);

        if(uart_cnt >= sizeof(uart_buf) - 1)
        {
            uart_flag = 1;
        }
        else if(res == '\r' || res == '\n')
        {
            if (uart_cnt > 0)
                uart_flag = 1;
        }
        else if(res == '*')
        {
            uart_flag = 1;
        }
        else
        {
            uart_buf[uart_cnt++] = res;
        }

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}

void USART3_IRQHandler(void)
{
    uint8_t res;
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        res = USART_ReceiveData(USART3);
        if (g_esp8266_rx_cnt < sizeof(g_esp8266_rx_buf))
        {
            g_esp8266_rx_buf[g_esp8266_rx_cnt++] = res;
        }
        else
        {
            g_esp8266_rx_overflow = 1;
        }
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

void parse_cmd(void)
{
    if(uart_flag)
    {
        if(strstr((char *)uart_buf,"mode0"))
        {
            Cmd_SwitchMode(0);
            USART2_SendString("Mode: Remote\r\n");
        }
        else if(strstr((char *)uart_buf,"mode1"))
        {
            Cmd_SwitchMode(1);
            USART2_SendString("Mode: Obstacle Avoid\r\n");
        }
        else if(strstr((char *)uart_buf,"mode2"))
        {
            Cmd_SwitchMode(2);
            USART2_SendString("Mode: Hybrid\r\n");
        }
        else if(strstr((char *)uart_buf,"mode3"))
        {
            Cmd_SwitchMode(3);
            USART2_SendString("Mode: Follow\r\n");
        }
        else if(strstr((char *)uart_buf,"mode4"))
        {
            Cmd_SwitchMode(4);
            USART2_SendString("Mode: Line Follow\r\n");
        }
        else if(strstr((char *)uart_buf,"mode?"))
        {
            char buf[20];
            sprintf(buf, "Current Mode: %d\r\n", g_current_mode);
            USART2_SendString(buf);
        }
        else if(strstr((char *)uart_buf,"test"))
        {
            USART2_SendString("test command ok\r\n");
        }
        else if(strstr((char *)uart_buf,"ledon"))
        {
           PBout(5)=0;
           USART2_SendString("LED ON OK\r\n");
        }
        else if(strstr((char *)uart_buf,"ledoff"))
        {
           PBout(5)=1;
           USART2_SendString("LED OFF OK\r\n");
        }
        else if(strstr((char *)uart_buf,"beepon"))
        {
            beep_on(80);
            USART2_SendString("BEEP on OK\r\n");
        }
        else if(strstr((char *)uart_buf, "music off"))
        {
            beep_stop_music();
            USART2_SendString("Music stopped!\r\n");
        }
        else if(strstr((char *)uart_buf, "music"))
        {
            beep_music_next();
            USART2_SendString("Next song!\r\n");
        }
        else if(strstr((char *)uart_buf, "contra"))
        {
            beep_music_contra();
            USART2_SendString("Contra playing!\r\n");
        }
        else if(strstr((char *)uart_buf,"goA"))
        {
            g_manual_timer = 50;
            g_remote_active = 1;
            Motor_Forward(speed);
            USART2_SendString("Go ahead OK\r\n");
        }
        else if(strstr((char *)uart_buf,"goB"))
        {
            g_manual_timer = 50;
            g_remote_active = 1;
            Motor_Backward(speed);
            USART2_SendString("Go back OK\r\n");
        }
        else if(strstr((char *)uart_buf,"goL"))
        {
            g_manual_timer = 50;
            g_remote_active = 1;
            Motor_Left(speed);
            USART2_SendString("Go Left OK\r\n");
        }
        else if(strstr((char *)uart_buf,"goR"))
        {
            g_manual_timer = 50;
            g_remote_active = 1;
            Motor_Right(speed);
            USART2_SendString("Go Right OK\r\n");
        }
        else if(strstr((char *)uart_buf,"stop"))
        {
            g_remote_active = 0;
            Motor_Stop();
            USART2_SendString("Stop OK\r\n");
        }
        else if(strstr((char *)uart_buf,"speedup"))
        {
            static char buf[20];
            if(speed<1000)
            {
                speed+=100;
            }
            sprintf(buf,"speed:%d",speed);
            USART2_SendString(buf);
        }
        else if(strstr((char *)uart_buf,"speeddown"))
        {
            static char buf[20];
            if(speed>0)
            {
                speed-=100;
            }
            sprintf(buf,"speed:%d",speed);
            USART2_SendString(buf);
        }
        else if(strstr((char *)uart_buf,"mode remote"))
        {
            g_current_mode = MODE_REMOTE;
            USART2_SendString("Mode: Remote\r\n");
        }
        else if(strstr((char *)uart_buf,"mode auto"))
        {
            g_current_mode = MODE_OBSTACLE_AVOID;
            USART2_SendString("Mode: Auto\r\n");
        }
        else if(strstr((char *)uart_buf,"mode hybrid"))
        {
            g_current_mode = MODE_HYBRID;
            USART2_SendString("Mode: Hybrid\r\n");
        }
        else if(strstr((char *)uart_buf,"mode follow"))
        {
            g_current_mode = MODE_FOLLOW;
            USART2_SendString("Mode: Follow\r\n");
        }
        else if(strstr((char *)uart_buf,"mode line"))
        {
            g_current_mode = MODE_LINE_FOLLOW;
            USART2_SendString("Mode: Line Follow\r\n");
        }
        else if(strstr((char *)uart_buf,"log"))
        {
            g_log = !g_log;
            USART2_SendString(g_log ? "Log ON\r\n" : "Log OFF\r\n");
        }
        else if(strstr((char *)uart_buf,"setspeed"))
        {
            char *p = strstr((char *)uart_buf, "setspeed") + 8;
            int val = atoi(p);
            if (val >= 0 && val <= PWM_MAX) {
                speed = (uint16_t)val;
                char buf[24];
                sprintf(buf, "Speed: %d\r\n", (int)speed);
                USART2_SendString(buf);
            } else {
                USART2_SendString("Invalid 0-999\r\n");
            }
        }
        else if(strstr((char *)uart_buf,"screen0"))
        {
            g_oled_page = 0;
            OLED_Clear();
            USART2_SendString("Screen 0\r\n");
        }
        else if(strstr((char *)uart_buf,"screen1"))
        {
            g_oled_page = 1;
            OLED_Clear();
            USART2_SendString("Screen 1\r\n");
        }
        else if(strstr((char *)uart_buf,"screen2"))
        {
            g_oled_page = 2;
            OLED_Clear();
            USART2_SendString("Screen 2\r\n");
        }
        else if(strstr((char *)uart_buf,"help"))
        {
            USART2_SendString("goA goB goL goR stop | speedup speeddown\r\n");
            USART2_SendString("music music off contra | beepon\r\n");
            USART2_SendString("mode0/1/2/3/4 mode? | mode remote/auto/hybrid/follow/line\r\n");
            USART2_SendString("screen0/1/2 | setspeed0-999 | log | help\r\n");
        }
        else
        {
            USART2_SendString("Unknown\r\n");
        }

        memset((char *)uart_buf,0,sizeof(uart_buf));
        uart_cnt = 0;
        uart_flag = 0;
    }
}
