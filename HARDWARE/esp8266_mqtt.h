#ifndef __ESP8266_MQTT_H
#define __ESP8266_MQTT_H

#include "stm32f10x.h"

/* === 请替换为你的华为云IoT平台配置 === */
#define MQTT_BROKER_ADDRESS    "YOUR_BROKER_ADDRESS"               /* 平台接入地址 */
#define MQTT_PORT              1883                                 /* MQTT端口 */
#define MQTT_CLIENT_ID         "YOUR_CLIENT_ID"                     /* 客户端ID */
#define MQTT_USERNAME          "YOUR_DEVICE_ID"                     /* 设备ID（用户名） */
#define MQTT_PASSWORD          "YOUR_PASSWORD"                      /* 设备密钥 */
#define MQTT_DEVICE_ID         "YOUR_DEVICE_ID"                     /* 设备ID */

#define MQTT_TOPIC_PROP_REPORT "$oc/devices/" MQTT_DEVICE_ID "/sys/properties/report"
#define MQTT_TOPIC_CMD_SUB     "$oc/devices/" MQTT_DEVICE_ID "/sys/commands/#"
#define MQTT_TOPIC_CMD_RSP     "$oc/devices/" MQTT_DEVICE_ID "/sys/commands/response/"
#define MQTT_TOPIC_EVENT_UP    "$oc/devices/" MQTT_DEVICE_ID "/sys/events/up"
#define MQTT_TOPIC_MSG_UP      "$oc/devices/" MQTT_DEVICE_ID "/sys/messages/up"

#define BYTE0(x)  ((uint8_t)((x) & 0xFF))
#define BYTE1(x)  ((uint8_t)(((x) >> 8) & 0xFF))
#define BYTE2(x)  ((uint8_t)(((x) >> 16) & 0xFF))
#define BYTE3(x)  ((uint8_t)(((x) >> 24) & 0xFF))

extern int32_t mqtt_connect(char *client_id, char *user_name, char *password);
extern int32_t mqtt_subscribe_topic(char *topic, uint8_t qos, uint8_t whether);
extern uint16_t mqtt_publish_data(char *topic, char *message, uint8_t qos);
extern void mqtt_send_heart(void);
extern int32_t esp8266_mqtt_init(void);
extern void mqtt_report_devices_status(void);
extern void mqtt_report_event(const char *event_type, const char *paras_json);
extern void mqtt_report_message(const char *msg);
extern void mqtt_handle_incoming(void);

#endif
