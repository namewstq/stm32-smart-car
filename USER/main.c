#include "stm32f10x.h"
#include <led.h>
#include <delay.h>
#include <key.h>
#include <sys.h>
#include <beep.h>
#include <usart.h>
#include <stdio.h>
#include <motor.h>
#include <dht11.h>
#include <servo.h>
#include <hcsr04.h>
#include <oled.h>
#include <esp8266_mqtt.h>

uint8_t buffer[5];

float distance;
extern volatile u16 speed;
extern volatile uint8_t g_log;
extern volatile uint8_t g_manual_timer;
extern volatile uint8_t g_remote_active;
volatile uint8_t g_oled_page = 0;

#define HYBRID_MANUAL_TIMEOUT  30

static uint8_t ir_debounce(GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        if (!GPIO_ReadInputDataBit(port, pin)) cnt++;
        delay_us(500);
    }
    return (cnt >= 2);
}
#define IR_L()    ir_debounce(GPIOC, GPIO_Pin_1)
#define IR_R()    ir_debounce(GPIOC, GPIO_Pin_2)
#define BTM_L()   ir_debounce(GPIOG, GPIO_Pin_3)
#define BTM_R()   ir_debounce(GPIOG, GPIO_Pin_2)

static void IR_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOG, &GPIO_InitStruct);
}

#define WARN_DIST 35.0f



static void boot_anim(void)
{
    OLED_Clear();
    OLED_ShowString(16, 8, "STM32 Car", 24, 1);
    OLED_ShowString(48, 36, "v2.0", 16, 1);
    for (u8 x = 10; x < 118; x += 3) {
        OLED_DrawPoint(x, 56, 1);
        OLED_DrawPoint(x+1, 56, 1);
    }
    OLED_Refresh(); delay_ms(1200);

    OLED_Clear();
    OLED_ShowString(8, 8, "Initializing", 16, 1);
    OLED_ShowString(28, 30, "Please wait", 12, 1);
    OLED_Refresh(); delay_ms(500);

    USART2_SendString("Self-test\n");
    beep_on(80);
    PBout(5)=0; delay_ms(150); PBout(5)=1;
    Servo_SetAngle(0); delay_ms(300);
    Servo_SetAngle(180); delay_ms(300);
    Servo_SetAngle(90); delay_ms(200);
}

static float scan_one_side(float angle)
{
    Servo_SetAngle(angle);
    delay_ms(400);
    float d = HC_SR04_GetDistance();
    if (d < 0) d = 999;
    Servo_SetAngle(90);
    delay_ms(200);
    return d;
}

static float scan_follow(float angle)
{
    Servo_SetAngle(angle);
    delay_ms(200);
    float d = HC_SR04_GetDistance();
    if (d < 0) d = 999;
    Servo_SetAngle(90);
    delay_ms(100);
    return d;
}

static void mode_auto(void)
{
    float front, left, right;
    static uint8_t stuck_cnt = 0, stuck_dir = 0;
    static uint8_t ir_both_cnt = 0, ir_left_cnt = 0, ir_right_cnt = 0;

    front = HC_SR04_GetDistance();
    if (front < 0 || front < 3.0f) front = 999;
    if(g_log){char b[30];sprintf(b,"D:%.0f L:%d R:%d\r\n",front,IR_L(),IR_R());USART2_SendString(b);}

    uint16_t auto_speed = speed * 4 / 5;

    if (front > WARN_DIST)
    {
        stuck_cnt = 0;
        if (IR_L() && IR_R()) {
            if(++ir_both_cnt>=4){ir_both_cnt=0;if(g_log)USART2_SendString(">ESC\n");Motor_Stop();Motor_Backward(650);delay_ms(300);Motor_Stop();delay_ms(50);Motor_Left(650);delay_ms(350);}
            else{if(g_log)USART2_SendString(">TR\n");Motor_Stop();Motor_RightPivot(650);delay_ms(350);}
            Motor_Stop();delay_ms(30);
        }
        else if (IR_L()) {
            ir_left_cnt++;
            if(ir_left_cnt>=4){ir_left_cnt=0;if(g_log)USART2_SendString(">ESC\n");Motor_Stop();Motor_Backward(650);delay_ms(300);Motor_Stop();delay_ms(50);Motor_Right(650);delay_ms(350);}
            else{if(g_log)USART2_SendString(">TR\n");Motor_Stop();Motor_RightPivot(650);delay_ms(350);}
            Motor_Stop();delay_ms(30);
        }
        else if (IR_R()) {
            ir_right_cnt++;
            if(ir_right_cnt>=4){ir_right_cnt=0;if(g_log)USART2_SendString(">ESC\n");Motor_Stop();Motor_Backward(650);delay_ms(300);Motor_Stop();delay_ms(50);Motor_Left(650);delay_ms(350);}
            else{if(g_log)USART2_SendString(">TL\n");Motor_Stop();Motor_LeftPivot(650);delay_ms(350);}
            Motor_Stop();delay_ms(30);
        }
        else { ir_both_cnt=0; ir_left_cnt=0; ir_right_cnt=0; if(g_log) USART2_SendString(">FW\n"); Motor_Forward(auto_speed); }
        return;
    }

    if(g_log) USART2_SendString(">OBS\n");
    Motor_Stop();
    stuck_cnt++;

    if (front < 5.0f) {
        Motor_Backward(650); delay_ms(200);
        Motor_Stop(); delay_ms(50);
    }

    if(g_log) USART2_SendString(">SL\n");
    left = scan_one_side(0);
    if(g_log){char b[20];sprintf(b,"L=%.0f\n",left);USART2_SendString(b);}

    if(g_log) USART2_SendString(">SR\n");
    right = scan_one_side(180);
    if(g_log){char b[20];sprintf(b,"R=%.0f\n",right);USART2_SendString(b);}

    if (stuck_cnt >= 3) {
        if(g_log) USART2_SendString(">STUCK\n");
        stuck_cnt = 0;
        beep_on(80); delay_ms(100); beep_on(80);
        Motor_Backward(650); delay_ms(500); Motor_Stop(); delay_ms(100);
        if(stuck_dir==0){Motor_Left(650);delay_ms(350);stuck_dir=1;}
        else{Motor_Right(650);delay_ms(350);stuck_dir=0;}
        Motor_Stop();delay_ms(100); return;
    }

    if(left>=right && left>WARN_DIST){if(g_log)USART2_SendString(">TR\n");Motor_RightPivot(650);}
    else if(right>WARN_DIST){if(g_log)USART2_SendString(">TL\n");Motor_LeftPivot(650);}
    else{if(g_log)USART2_SendString(">TD\n");Motor_Left(650);}
    delay_ms(350); Motor_Stop(); delay_ms(80);
}

static void mode_remote(void)
{
    if (g_manual_timer > 0) { g_manual_timer--; return; }

    float d = HC_SR04_GetDistance();
    if (d < 0 || d < 3.0f) d = 999;
    if (d < 30.0f) { static uint8_t w=0; if(!w){beep_on(100);w=1;} else w=0; }

    if (IR_L()&&IR_R()) {
        static uint8_t c=0;
        if(++c>=4){c=0;Motor_Stop();Motor_Backward(650);delay_ms(600);Motor_Stop();delay_ms(50);Motor_Left(650);delay_ms(300);}
        else{Motor_Stop();Motor_Backward(650);delay_ms(300);}
        Motor_Stop();delay_ms(30);
    }
    else if (IR_L()) {
        static uint8_t c=0; c++;
        if(c>=4){c=0;Motor_Stop();Motor_Backward(650);delay_ms(500);Motor_Stop();delay_ms(50);Motor_Right(650);delay_ms(250);}
        else{Motor_Stop();Motor_RightPivot(650);delay_ms(250);}
        Motor_Stop();delay_ms(30);
    }
    else if (IR_R()) {
        static uint8_t c=0; c++;
        if(c>=4){c=0;Motor_Stop();Motor_Backward(650);delay_ms(500);Motor_Stop();delay_ms(50);Motor_Left(650);delay_ms(250);}
        else{Motor_Stop();Motor_LeftPivot(650);delay_ms(250);}
        Motor_Stop();delay_ms(30);
    }
}

static void hybrid_mode(void)
{
    static uint8_t hybrid_manual_cnt = 0;

    if (g_remote_active)
    {
        hybrid_manual_cnt = HYBRID_MANUAL_TIMEOUT;
        g_remote_active = 0;
        return;
    }

    if (hybrid_manual_cnt > 0)
    {
        hybrid_manual_cnt--;
        return;
    }

    mode_auto();
}

static void mode_follow(void)
{
    uint8_t left = IR_L(), right = IR_R();
    if(g_log){char b[20];sprintf(b,"L:%d R:%d\r\n",left,right);USART2_SendString(b);}

    // 纯靠红外灯感应，去掉超声波和舵机扫描
    // 红外亮=感应到目标，不亮=没有目标
    if (left && right) {
        // 左右都有感应 → 目标在正前方 → 前进
        Motor_Forward(500);
        if(g_log) USART2_SendString(">FW\n");
    }
    else if (left && !right) {
        // 只有左边亮 → 目标偏左 → 左转追过去
        Motor_Left(400);
        if(g_log) USART2_SendString(">FL\n");
    }
    else if (!left && right) {
        // 只有右边亮 → 目标偏右 → 右转追过去
        Motor_Right(400);
        if(g_log) USART2_SendString(">FR\n");
    }
    else {
        // 两个都不亮 → 目标丢失 → 停车等
        Motor_Stop();
        if(g_log) USART2_SendString(">LS\n");
    }
}

static void mode_line_follow(void)
{
    uint8_t l = BTM_L(), r = BTM_R();
    if(g_log){char b[30];sprintf(b,"LINE L:%d R:%d\r\n",l,r);USART2_SendString(b);}

    if (!l && !r) {
        Motor_Forward(speed);
    } else if (l && !r) {
        Motor_Right(450);
    } else if (!l && r) {
        Motor_Left(450);
    } else {
        Motor_Stop();
    }
}

int main(void)
{
    static uint16_t report_cnt = 0;
    static uint16_t heart_cnt = 0;
    static uint8_t iot_ready = 0;
    static int mqtt_retry = 0;

    led_init(); delay_init(); NVIC_Configuration(); beep_init();
    HC_SR04_Init(); OLED_Init();
    Motor_Init(); Servo_Init(); IR_Init();
    KEY_EXTI_Init();
    USART1_Init(9600); USART2_Init(9600);

    /* 检测复位原因 */
    if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
        printf("[RST] Pin reset\r\n");
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
        printf("[RST] POR/BOR reset (voltage drop?)\r\n");
    if (RCC_GetFlagStatus(RCC_FLAG_SFTRST) != RESET)
        printf("[RST] Software reset\r\n");
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)
        printf("[RST] IWDG reset\r\n");
    if (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) != RESET)
        printf("[RST] WWDG reset\r\n");
    RCC_ClearFlag();

    boot_anim();
    OLED_Clear();

    OLED_Refresh();

    USART2_SendString("OK\n");

    static uint8_t last_mode = 99;
    static uint8_t popup_timer = 0;
    static uint8_t oled_dirty = 1;
    const char *mode_names[] = {"Remote","Auto  ","Hybrid","Follow","Line  "};

    uint8_t dht_cnt = 0;
    uint8_t oled_tick = 0;

    while (1)
    {
        if (++dht_cnt >= 15) {
            DHT_ReadData(buffer);
            dht_cnt = 0;
            oled_dirty = 1;
        }
        delay_ms(20);

        parse_cmd();

        if (g_key1_pressed) {
            g_key1_pressed = 0;
            g_oled_page = (g_oled_page + 1) % 3;
            OLED_Clear();
            oled_dirty = 1;
            mqtt_report_devices_status();
        }

        if (g_current_mode != last_mode) {
            last_mode = g_current_mode;
            popup_timer = 25;
            oled_dirty = 1;
        }

        if (popup_timer > 0) {
            popup_timer--;
            oled_dirty = 1;
        }

        if (++oled_tick >= 15) {
            oled_tick = 0;
            oled_dirty = 1;
        }

        if (oled_dirty) {
            oled_dirty = 0;
            if (g_oled_page == 0) {
                char line[21];
                sprintf(line, "T:%3dC H:%3d%%", buffer[2], buffer[0]);
                OLED_ShowString(0, 0, (u8*)line, 16, 1);
                sprintf(line, "Dist:%3.0fcm", distance > 999 ? 0 : distance);
                OLED_ShowString(0, 20, (u8*)line, 16, 1);
                OLED_ShowString(0, 40, "Mode:", 12, 1);
                OLED_ShowString(48, 40, (u8*)mode_names[g_current_mode], 12, 1);
                sprintf(line, "Spd:%d", g_motor_running ? (int)speed : 0);
                OLED_ShowString(0, 52, (u8*)line, 12, 1);

            } else if (g_oled_page == 1) {

                OLED_ShowString(0, 0, "Mode:", 16, 1);
                  OLED_ShowString(48, 0, (u8*)mode_names[g_current_mode], 16, 1);
                char dist_str[20];
                sprintf(dist_str, "Dist:%3.0fcm", distance > 999 ? 0 : distance);
                OLED_ShowString(0, 18, (u8*)dist_str, 12, 1);
                uint8_t bar_w = (distance > 100) ? 120 : (uint8_t)(distance * 1.2f);
                for (u8 x = 4; x < 124; x++) {
                    u8 on = (x < bar_w) ? 1 : 0;
                    OLED_DrawPoint(x, 32, on);
                    OLED_DrawPoint(x, 33, on);
                }
                OLED_ShowString(0, 40, "L:", 12, 1);
                OLED_ShowString(16, 40, (u8*)(IR_L() ? "1 " : "0 "), 12, 1);
                OLED_ShowString(36, 40, "R:", 12, 1);
                OLED_ShowString(52, 40, (u8*)(IR_R() ? "1 " : "0 "), 12, 1);
                OLED_ShowString(72, 40, "Spd:", 12, 1);
                sprintf(dist_str, "%d", g_motor_running ? (int)speed : 0);
                OLED_ShowString(104, 40, (u8*)dist_str, 12, 1);
                OLED_ShowString(0, 52, "BL:", 12, 1);
                OLED_ShowString(24, 52, (u8*)(BTM_L() ? "1 " : "0 "), 12, 1);
                OLED_ShowString(48, 52, "BR:", 12, 1);
                OLED_ShowString(72, 52, (u8*)(BTM_R() ? "1 " : "0 "), 12, 1);

            } else {
                OLED_ShowString(0, 0, "Sys Info", 16, 1);
                char s[20];
                sprintf(s, "T:%dC H:%d%%", buffer[2], buffer[0]);
                OLED_ShowString(0, 20, (u8*)s, 12, 1);
                sprintf(s, "D:%.0fcm", distance > 999 ? 0 : distance);
                OLED_ShowString(0, 36, (u8*)s, 12, 1);
                OLED_ShowString(0, 52, "KEY1:Switch", 8, 1);
            }

            if (popup_timer > 0) {
                OLED_ShowString(20, 52, (u8*)mode_names[g_current_mode], 12, 1);
            }

            OLED_Refresh();
        }

        switch (g_current_mode)
        {
            case MODE_REMOTE:
                mode_remote();
                break;
            case MODE_OBSTACLE_AVOID:
                mode_auto();
                break;
            case MODE_HYBRID:
                hybrid_mode();
                break;
            case MODE_FOLLOW:
                mode_follow();
                break;
            case MODE_LINE_FOLLOW:
                mode_line_follow();
                break;
        }

        distance = HC_SR04_GetDistance();

        /* IoT initialization (blocking, retry every ~50 loops) */
        if (!iot_ready)
        {
            mqtt_retry++;
            if (mqtt_retry >= 50)
            {
                mqtt_retry = 0;
                printf("[IOT] trying init...\r\n");
                if (esp8266_mqtt_init() == 0)
                {
                    iot_ready = 1;
                    mqtt_report_devices_status();
                    printf("[IOT] READY\r\n");
                    beep_on(80); delay_ms(100); beep_on(80);
                }
                else
                {
                    printf("[IOT] init failed, retrying...\r\n");
                }
            }
        }
        else
        {
            report_cnt++;
            if (report_cnt >= 200)
            {
                report_cnt = 0;
                mqtt_report_devices_status();
            }
            heart_cnt++;
            if (heart_cnt >= 100)
            {
                heart_cnt = 0;
                mqtt_send_heart();
            }
            mqtt_handle_incoming();
        }

        delay_ms(40);
    }
}
