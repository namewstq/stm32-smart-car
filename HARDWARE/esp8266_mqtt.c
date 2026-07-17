#include "stm32f10x.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "motor.h"
#include "beep.h"
#include "dht11.h"
#include "esp8266.h"
#include "esp8266_mqtt.h"
#include "servo.h"
#include <oled.h>

const uint8_t g_packet_connect_ack[4] = {0x20, 0x02, 0x00, 0x00};
const uint8_t g_packet_disconnect[2]  = {0xe0, 0x00};
const uint8_t g_packet_heart[2]       = {0xc0, 0x00};
const uint8_t g_packet_sub_ack[2]     = {0x90, 0x03};

char  g_mqtt_msg[526];
uint16_t g_mqtt_tx_len;

void mqtt_send_bytes(uint8_t *buf, uint32_t len)
{
    esp8266_send_bytes(buf, len);
}

/* ─── 发送 PUBACK (QoS 1 确认) ─── */
static void mqtt_send_puback(uint16_t packet_id)
{
    uint8_t puback[4] = {0x40, 0x02, (uint8_t)(packet_id >> 8), (uint8_t)(packet_id & 0xFF)};
    mqtt_send_bytes(puback, 4);
}

void mqtt_send_heart(void)
{
    mqtt_send_bytes((uint8_t *)g_packet_heart, sizeof(g_packet_heart));
}

void mqtt_disconnect(void)
{
    mqtt_send_bytes((uint8_t *)g_packet_disconnect, sizeof(g_packet_disconnect));
}

void mqtt_init(uint8_t *prx, uint16_t rxlen, uint8_t *ptx, uint16_t txlen)
{
    memset(g_esp8266_tx_buf, 0, sizeof(g_esp8266_tx_buf));
    memset((void *)g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
    mqtt_disconnect();
    delay_ms(100);
    mqtt_disconnect();
    delay_ms(100);
}

int32_t mqtt_connect(char *client_id, char *user_name, char *password)
{
    int32_t client_id_len = strlen(client_id);
    int32_t user_name_len = strlen(user_name);
    int32_t password_len = strlen(password);
    int32_t data_len;
    uint32_t cnt = 2;
    uint32_t wait;
    g_mqtt_tx_len = 0;

    data_len = 10 + (client_id_len + 2) + (user_name_len + 2) + (password_len + 2);

    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0x10;
    do
    {
        uint8_t encodedByte = data_len % 128;
        data_len = data_len / 128;
        if (data_len > 0)
            encodedByte = encodedByte | 128;
        g_esp8266_tx_buf[g_mqtt_tx_len++] = encodedByte;
    } while (data_len > 0);

    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0;
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 4;
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 'M';
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 'Q';
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 'T';
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 'T';
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 4;
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0xc2;
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0;
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 60;

    g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE1(client_id_len);
    g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE0(client_id_len);
    memcpy(&g_esp8266_tx_buf[g_mqtt_tx_len], client_id, client_id_len);
    g_mqtt_tx_len += client_id_len;

    if (user_name_len > 0)
    {
        g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE1(user_name_len);
        g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE0(user_name_len);
        memcpy(&g_esp8266_tx_buf[g_mqtt_tx_len], user_name, user_name_len);
        g_mqtt_tx_len += user_name_len;
    }

    if (password_len > 0)
    {
        g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE1(password_len);
        g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE0(password_len);
        memcpy(&g_esp8266_tx_buf[g_mqtt_tx_len], password, password_len);
        g_mqtt_tx_len += password_len;
    }

    while (cnt--)
    {
        memset((void *)g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
        g_esp8266_rx_cnt = 0;

        mqtt_send_bytes(g_esp8266_tx_buf, g_mqtt_tx_len);

        wait = 30;
        while (wait--)
        {
            if ((g_esp8266_rx_buf[0] == g_packet_connect_ack[0]) &&
                (g_esp8266_rx_buf[1] == g_packet_connect_ack[1]))
            {
                return 0;
            }
            delay_ms(100);
        }
    }
    return -1;
}

int32_t mqtt_subscribe_topic(char *topic, uint8_t qos, uint8_t whether)
{
    uint32_t cnt = 2;
    uint32_t wait;
    int32_t topiclen = strlen(topic);
    int32_t data_len = 2 + (topiclen + 2) + (whether ? 1 : 0);

    g_mqtt_tx_len = 0;

    if (whether)
        g_esp8266_tx_buf[g_mqtt_tx_len++] = 0x82;
    else
        g_esp8266_tx_buf[g_mqtt_tx_len++] = 0xA2;

    do
    {
        uint8_t encodedByte = data_len % 128;
        data_len = data_len / 128;
        if (data_len > 0)
            encodedByte = encodedByte | 128;
        g_esp8266_tx_buf[g_mqtt_tx_len++] = encodedByte;
    } while (data_len > 0);

    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0;
    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0x01;

    g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE1(topiclen);
    g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE0(topiclen);
    memcpy(&g_esp8266_tx_buf[g_mqtt_tx_len], topic, topiclen);
    g_mqtt_tx_len += topiclen;

    if (whether)
    {
        g_esp8266_tx_buf[g_mqtt_tx_len++] = qos;
    }

    while (cnt--)
    {
        g_esp8266_rx_cnt = 0;
        memset((void *)g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
        mqtt_send_bytes(g_esp8266_tx_buf, g_mqtt_tx_len);

        wait = 30;
        while (wait--)
        {
            if (g_esp8266_rx_buf[0] == g_packet_sub_ack[0] &&
                g_esp8266_rx_buf[1] == g_packet_sub_ack[1])
            {
                return 0;
            }
            delay_ms(100);
        }
    }

    return -1;
}

uint16_t mqtt_publish_data(char *topic, char *message, uint8_t qos)
{
    static uint16_t id = 1;
    int32_t topicLength = strlen(topic);
    int32_t messageLength = strlen(message);
    int32_t data_len;
    uint8_t encodedByte;
    g_mqtt_tx_len = 0;

    if (qos) data_len = (2 + topicLength) + 2 + messageLength;
    else     data_len = (2 + topicLength) + messageLength;

    g_esp8266_tx_buf[g_mqtt_tx_len++] = 0x30 | ((qos & 3) << 1);

    do
    {
        encodedByte = data_len % 128;
        data_len = data_len / 128;
        if (data_len > 0)
            encodedByte = encodedByte | 128;
        g_esp8266_tx_buf[g_mqtt_tx_len++] = encodedByte;
    } while (data_len > 0);

    g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE1(topicLength);
    g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE0(topicLength);
    memcpy(&g_esp8266_tx_buf[g_mqtt_tx_len], topic, topicLength);
    g_mqtt_tx_len += topicLength;

    if (qos)
    {
        g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE1(id);
        g_esp8266_tx_buf[g_mqtt_tx_len++] = BYTE0(id);
        id++;
    }

    memcpy(&g_esp8266_tx_buf[g_mqtt_tx_len], message, messageLength);
    g_mqtt_tx_len += messageLength;

    mqtt_send_bytes(g_esp8266_tx_buf, g_mqtt_tx_len);
    return g_mqtt_tx_len;
}

/* ─── 从 MQTT 包中解析出 topic 和 payload ─── */
static int mqtt_parse_packet(volatile const uint8_t *raw, uint16_t raw_len,
                             const char **topic_out, uint16_t *topic_len_out,
                             const char **payload_out, uint16_t *payload_len_out,
                             uint16_t *packet_id_out)
{
    uint16_t idx = 0;
    if (idx >= raw_len) return -1;

    uint8_t fixed_header = raw[idx++];
    uint8_t msg_type = (fixed_header >> 4) & 0x0F;
    if (msg_type != 3) return -1;

    uint8_t qos = (fixed_header >> 1) & 0x03;

    uint32_t multiplier = 1;
    uint32_t remaining_length = 0;
    uint8_t encodedByte;
    do {
        if (idx >= raw_len) return -1;
        encodedByte = raw[idx++];
        remaining_length += (encodedByte & 0x7F) * multiplier;
        multiplier *= 128;
    } while (encodedByte & 0x80);

    if (idx + 2 > raw_len) return -1;
    uint16_t topic_len = (raw[idx] << 8) | raw[idx + 1];
    idx += 2;

    if (idx + topic_len > raw_len) return -1;
    *topic_out = (const char *)&raw[idx];
    *topic_len_out = topic_len;
    idx += topic_len;

    if (qos > 0)
    {
        if (idx + 2 > raw_len) return -1;
        *packet_id_out = (raw[idx] << 8) | raw[idx + 1];
        idx += 2;
    }
    else
    {
        *packet_id_out = 0;
    }

    *payload_out = (const char *)&raw[idx];
    *payload_len_out = raw_len - idx;
    return 0;
}

/* ─── 从 topic 中提取 request_id ─── */
static void extract_request_id_from_topic(const char *topic, uint16_t topic_len, char *out, uint8_t max_len)
{
    out[0] = '\0';
    const char *tag = "request_id=";
    const char *p = NULL;
    for (uint16_t i = 0; i + 11 <= topic_len; i++)
    {
        if (strncmp(&topic[i], tag, 10) == 0)
        {
            p = &topic[i + 10];
            break;
        }
    }
    if (!p) return;
    uint8_t i = 0;
    while (*p && *p != '/' && (uint16_t)(p - topic) < topic_len && i < max_len - 1)
    {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

/* ─── 从 JSON payload 中提取 command_name ─── */
static void extract_command_name(const char *json, char *out, uint8_t max_len)
{
    const char *p = strstr(json, "\"command_name\"");
    out[0] = '\0';
    if (!p) return;
    p = strchr(p + 14, '"');
    if (!p) return;
    p++;
    uint8_t i = 0;
    while (*p && *p != '"' && i < max_len - 1)
    {
        out[i++] = *p++;
    }
    out[i] = '\0';
}

/* ─── 从 JSON payload 中提取整数参数值 ─── */
static int32_t extract_json_int(const char *json, const char *key)
{
    char search[32];
    sprintf(search, "\"%s\"", key);
    const char *p = strstr(json, search);
    if (!p) return 0;
    p = strchr(p + strlen(search), ':');
    if (!p) return 0;
    p++;
    while (*p == ' ') p++;
    if (*p == '"') p++;
    int negative = 0;
    if (*p == '-') { negative = 1; p++; }
    int32_t val = 0;
    while (*p >= '0' && *p <= '9')
    {
        val = val * 10 + (*p - '0');
        p++;
    }
    return negative ? -val : val;
}

/* ─── 发送命令响应 ─── */
static void mqtt_cmd_response(const char *request_id, int32_t result_code, const char *result_msg)
{
    char topic[180];
    sprintf(topic, "%srequest_id=%s", MQTT_TOPIC_CMD_RSP, request_id);
    sprintf(g_mqtt_msg,
        "{\"request_id\":\"%s\",\"result_code\":%ld,\"response_name\":\"execute\",\"paras\":{\"result\":\"%s\"}}",
        request_id, result_code, result_msg);
    
    printf("RSP: %s\r\n", g_mqtt_msg);
    
    mqtt_publish_data(topic, g_mqtt_msg, 0);
}

/* ─── 上报属性数据 ─── */
void mqtt_report_devices_status(void)
{
    extern float distance;
    float temperature = 0.0;
    int humidity = 0;
    uint8_t mqtt_buf[5] = {0};

    if (DHT_ReadData(mqtt_buf) == 0)
    {
        temperature = (float)mqtt_buf[2];
        humidity = mqtt_buf[0];
    }

    uint8_t irL = 0, irR = 0, btmL = 0, btmR = 0;
    for (uint8_t i = 0; i < 3; i++)
    {
        if (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1)) irL++;
        if (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2)) irR++;
        if (!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_3)) btmL++;
        if (!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_2)) btmR++;
        delay_us(500);
    }
    irL = (irL >= 2) ? 1 : 0;
    irR = (irR >= 2) ? 1 : 0;
    btmL = (btmL >= 2) ? 1 : 0;
    btmR = (btmR >= 2) ? 1 : 0;
    extern volatile u16 speed;
    extern volatile uint8_t g_oled_page;

    float d = distance > 999 ? 0 : distance;
    uint8_t mode = (uint8_t)g_current_mode;

    if (g_oled_page == 0) {
        sprintf(g_mqtt_msg,
            "{\"services\":[{\
                \"service_id\":\"smokeDetector\",\
                \"properties\":{\
                \"temperature\":%.1f,\
                \"humidity\":%d,\
                \"distance\":%.1f,\
                \"mode\":%d,\
                \"speed\":%d\
                }}]}",
            temperature, humidity, d, mode, (int)speed);
    } else if (g_oled_page == 1) {
        sprintf(g_mqtt_msg,
            "{\"services\":[{\
                \"service_id\":\"smokeDetector\",\
                \"properties\":{\
                \"distance\":%.1f,\
                \"mode\":%d,\
                \"speed\":%d,\
                \"irLeft\":%d,\
                \"irRight\":%d,\
                \"bottomLeft\":%d,\
                \"bottomRight\":%d\
                }}]}",
            d, mode, (int)speed, irL, irR, btmL, btmR);
    } else {
        sprintf(g_mqtt_msg,
            "{\"services\":[{\
                \"service_id\":\"smokeDetector\",\
                \"properties\":{\
                \"temperature\":%.1f,\
                \"humidity\":%d,\
                \"distance\":%.1f\
                }}]}",
            temperature, humidity, d);
    }

    printf("REPORT: %s\r\n", g_mqtt_msg);
    mqtt_publish_data(MQTT_TOPIC_PROP_REPORT, g_mqtt_msg, 1);
}

/* ─── 带蓝牙处理的阻塞延时 ─── */
static void delay_parse(uint32_t ms)
{
    extern void parse_cmd(void);
    while (ms >= 10) {
        delay_ms(10);
        parse_cmd();
        ms -= 10;
    }
    if (ms) delay_ms(ms);
}

/* ─── 阻塞式 WiFi + MQTT 初始化（原始工作代码） ─── */
int32_t esp8266_mqtt_init(void)
{
    int32_t rt;

    printf("[IOT] start init\r\n");
    esp8266_init();
    printf("[IOT] USART3 ready\r\n");

    rt = esp8266_exit_transparent_transmission();
    if (rt) { printf("exit trans fail\r\n"); return -1; }
    printf("exit trans ok\r\n");
    delay_parse(500);

    esp8266_send_at("AT+RST\r\n");
    esp8266_find_str_in_rx_packet("ready", 5000);
    delay_parse(500);

    rt = esp8266_enable_echo(0);
    if (rt) { printf("echo off fail\r\n"); return -2; }
    printf("echo off ok\r\n");
    delay_parse(500);

    esp8266_send_at("AT+CWRFPOWER=52\r\n");
    esp8266_find_str_in_rx_packet("OK", 1000);
    delay_parse(100);

    rt = esp8266_connect_ap(WIFI_SSID, WIFI_PASSWORD);
    if (rt) { printf("AP connect fail\r\n"); return -3; }
    printf("AP connect ok\r\n");
    delay_parse(500);

    rt = esp8266_connect_server("TCP", MQTT_BROKER_ADDRESS, MQTT_PORT);
    if (rt) { printf("TCP connect fail\r\n"); return -4; }
    printf("TCP connect ok\r\n");
    delay_parse(500);

    rt = esp8266_entry_transparent_transmission();
    if (rt) { printf("trans mode fail\r\n"); return -5; }
    printf("trans mode ok\r\n");
    delay_parse(500);

    if (mqtt_connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
        printf("mqtt_connect fail\r\n"); return -6; }
    printf("mqtt_connect ok\r\n");
    delay_parse(500);

    if (mqtt_subscribe_topic(MQTT_TOPIC_CMD_SUB, 1, 1)) {
        printf("mqtt_subscribe fail\r\n"); return -7; }
    printf("mqtt_subscribe ok\r\n");

    mqtt_report_devices_status();
    printf("first report sent\r\n");
    return 0;
}

/* ─── 上报事件 ─── */
void mqtt_report_event(const char *event_type, const char *paras_json)
{
    sprintf(g_mqtt_msg,
        "{\"service_id\":\"smokeDetector\",\
         \"event_type\":\"%s\",\
         \"paras\":{%s}}",
        event_type, paras_json);
    mqtt_publish_data(MQTT_TOPIC_EVENT_UP, g_mqtt_msg, 0);
}

/* ─── 上报消息（日志等） ─── */
void mqtt_report_message(const char *msg)
{
    sprintf(g_mqtt_msg,
        "{\"message_id\":\"msg_%lu\",\"msg\":\"%s\"}",
        (unsigned long)SysTick->VAL, msg);
    mqtt_publish_data(MQTT_TOPIC_MSG_UP, g_mqtt_msg, 0);
}

/* ─── 云平台命令解析与执行 ─── */
static void mqtt_parse_incoming(void)
{
    if (g_esp8266_rx_cnt == 0) return;

    const char *topic = NULL;
    uint16_t topic_len = 0;
    const char *payload = NULL;
    uint16_t payload_len = 0;
    uint16_t packet_id = 0;

    if (mqtt_parse_packet(g_esp8266_rx_buf, g_esp8266_rx_cnt,
                          &topic, &topic_len, &payload, &payload_len, &packet_id) != 0)
    {
        printf("MQTT: not a PUBLISH packet\r\n");
        return;
    }

    if (packet_id > 0)
    {
        mqtt_send_puback(packet_id);
    }

    if (topic_len == 0 || payload_len == 0) return;

    printf("TOPIC: %.*s\r\n", topic_len, &topic[0]);
    printf("PAYLOAD: %.*s\r\n", payload_len > 128 ? 128 : payload_len, &payload[0]);

    char request_id[40] = {0};
    extract_request_id_from_topic(topic, topic_len, request_id, sizeof(request_id));
    
    char cmd_name[40] = {0};
    extract_command_name(payload, cmd_name, sizeof(cmd_name));

    if (cmd_name[0] == '\0') return;

    printf("CMD: rid=%s cmd=%s\r\n", request_id, cmd_name);

    extern volatile uint8_t g_remote_active;
    extern volatile uint8_t g_manual_timer;
    extern volatile uint16_t speed;

    if (strstr(cmd_name, "forward") || strstr(payload, "\"goA\""))
    {
        g_remote_active = 1;
        g_manual_timer = 50;
        Motor_Forward(speed);
        mqtt_cmd_response(request_id, 0, "forward ok");
    }
    else if (strstr(cmd_name, "backward") || strstr(payload, "\"goB\""))
    {
        g_remote_active = 1;
        g_manual_timer = 50;
        Motor_Backward(speed);
        mqtt_cmd_response(request_id, 0, "backward ok");
    }
    else if (strstr(cmd_name, "left") || strstr(payload, "\"goL\""))
    {
        g_remote_active = 1;
        g_manual_timer = 50;
        Motor_Left(speed);
        mqtt_cmd_response(request_id, 0, "left ok");
    }
    else if (strstr(cmd_name, "right") || strstr(payload, "\"goR\""))
    {
        g_remote_active = 1;
        g_manual_timer = 50;
        Motor_Right(speed);
        mqtt_cmd_response(request_id, 0, "right ok");
    }
    else if (strstr(cmd_name, "stop"))
    {
        g_remote_active = 0;
        Motor_Stop();
        mqtt_cmd_response(request_id, 0, "stop ok");
    }
    else if (strstr(cmd_name, "mode0"))
    {
        Cmd_SwitchMode(0);
        mqtt_cmd_response(request_id, 0, "mode: remote");
    }
    else if (strstr(cmd_name, "mode1"))
    {
        Cmd_SwitchMode(1);
        mqtt_cmd_response(request_id, 0, "mode: auto");
    }
    else if (strstr(cmd_name, "mode2"))
    {
        Cmd_SwitchMode(2);
        mqtt_cmd_response(request_id, 0, "mode: hybrid");
    }
    else if (strstr(cmd_name, "mode3"))
    {
        Cmd_SwitchMode(3);
        mqtt_cmd_response(request_id, 0, "mode: follow");
    }
    else if (strstr(cmd_name, "mode4"))
    {
        Cmd_SwitchMode(4);
        mqtt_cmd_response(request_id, 0, "mode: line");
    }
    else if (strstr(cmd_name, "mode?"))
    {
        char rsp[30];
        sprintf(rsp, "mode:%d", (int)g_current_mode);
        mqtt_cmd_response(request_id, 0, rsp);
    }
    else if (strstr(cmd_name, "ledon"))
    {
        PBout(5) = 0;
        mqtt_cmd_response(request_id, 0, "led on ok");
    }
    else if (strstr(cmd_name, "ledoff"))
    {
        PBout(5) = 1;
        mqtt_cmd_response(request_id, 0, "led off ok");
    }
    else if (strstr(cmd_name, "beep"))
    {
        beep_on(500);
        mqtt_cmd_response(request_id, 0, "beep ok");
    }
    else if (strstr(cmd_name, "music"))
    {
        beep_music_xiongchumo();
        mqtt_cmd_response(request_id, 0, "music playing");
    }
    else if (strstr(cmd_name, "stopmusic"))
    {
        beep_stop_music();
        mqtt_cmd_response(request_id, 0, "music stopped");
    }
    else if (strstr(cmd_name, "contra"))
    {
        beep_music_contra();
        mqtt_cmd_response(request_id, 0, "contra playing");
    }
    else if (strstr(cmd_name, "speedup"))
    {
        if (speed < 1000) speed += 100;
        char rsp[20];
        sprintf(rsp, "speed:%d", (int)speed);
        mqtt_cmd_response(request_id, 0, rsp);
    }
    else if (strstr(cmd_name, "speeddown"))
    {
        if (speed > 0) speed -= 100;
        char rsp[20];
        sprintf(rsp, "speed:%d", (int)speed);
        mqtt_cmd_response(request_id, 0, rsp);
    }
    else if (strstr(cmd_name, "log"))
    {
        extern volatile uint8_t g_log;
        g_log = !g_log;
        mqtt_cmd_response(request_id, 0, g_log ? "log on" : "log off");
    }
    else if (strstr(cmd_name, "getdata"))
    {
        mqtt_report_devices_status();
        mqtt_cmd_response(request_id, 0, "data reported");
    }
    else if (strstr(cmd_name, "servo") || strstr(cmd_name, "setServo"))
    {
        int32_t angle = extract_json_int(payload, "servo");
        if (angle == 0) angle = extract_json_int(payload, "angle");

        if (angle == 0)
        {
            uint32_t rlen = g_esp8266_rx_cnt;
            const uint8_t raw_keys[2][8] = {{"\"servo\""}, {"\"angle\""}};
            for (int ki = 0; ki < 2 && angle == 0; ki++)
            {
                const uint8_t *key = raw_keys[ki];
                uint32_t klen = strlen((const char*)key);
                for (uint32_t i = 0; i + klen <= rlen; i++)
                {
                    if (memcmp((const void*)&g_esp8266_rx_buf[i], key, klen) == 0)
                    {
                        printf("SERVO: raw found at %lu\r\n", (unsigned long)i);
                        char ang_str[16] = {0};
                        char *p = strchr((char*)&g_esp8266_rx_buf[i + klen], ':');
                        if (p)
                        {
                            p++;
                            while (*p == ' ') p++;
                            if (*p == '"') p++;
                            uint8_t ai = 0;
                            while (*p >= '0' && *p <= '9' && ai < 15)
                                ang_str[ai++] = *p++;
                            ang_str[ai] = '\0';
                            angle = atoi(ang_str);
                            printf("SERVO: raw angle=%ld\r\n", (long)angle);
                        }
                        break;
                    }
                }
            }
        }

        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;
        Servo_SetAngle((float)angle);
        char rsp[30];
        sprintf(rsp, "servo:%lddeg", (long)angle);
        mqtt_cmd_response(request_id, 0, rsp);
    }
    else if (strstr(cmd_name, "setspeed"))
    {
        int32_t val = extract_json_int(payload, "speed");
        if (val < 0) val = 0;
        if (val > 1000) val = 1000;
        speed = (uint16_t)val;
        char rsp[20];
        sprintf(rsp, "speed:%d", (int)speed);
        mqtt_cmd_response(request_id, 0, rsp);
    }
    else if (strstr(cmd_name, "screen"))
    {
        int32_t page = extract_json_int(payload, "page");
        if (page >= 0 && page <= 2)
        {
            extern volatile uint8_t g_oled_page;
            g_oled_page = (uint8_t)page;
            OLED_Clear();
            mqtt_report_devices_status();
            char rsp[20];
            sprintf(rsp, "screen %ld", page);
            mqtt_cmd_response(request_id, 0, rsp);
        }
        else
        {
            mqtt_cmd_response(request_id, -1, "invalid page 0-2");
        }
    }
    else if (strstr(cmd_name, "test"))
    {
        mqtt_cmd_response(request_id, 0, "test ok");
    }
    else if (strstr(cmd_name, "help"))
    {
        mqtt_cmd_response(request_id, 0,
            "goA/goB/goL/goR/stop|speedup/speeddown|"
            "mode0-3/mode?|ledon/ledoff|beep/music/stopmusic/contra|"
            "servo{angle}|setspeed{speed}|screen{page 0-2}|log/getdata/test");
    }
    else
    {
        mqtt_cmd_response(request_id, -1, "unknown cmd");
    }

    printf("CMD:%s rsp=%s\r\n", cmd_name, request_id);
}

void mqtt_handle_incoming(void)
{
    if (g_esp8266_rx_overflow)
    {
        g_esp8266_rx_overflow = 0;
        g_esp8266_rx_cnt = 0;
        memset((void *)g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
        printf("[IOT] RX overflow, buffer reset\r\n");
        return;
    }
    if (g_esp8266_rx_cnt > 0)
    {
        mqtt_parse_incoming();
        memset((void *)g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
        g_esp8266_rx_cnt = 0;
    }
}


