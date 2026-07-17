#ifndef __ESP8266_MQTT_H
#define __ESP8266_MQTT_H

#include "stm32f10x.h"

#define MQTT_BROKER_ADDRESS    "efff2cb551.st1.iotda-device.cn-south-1.myhuaweicloud.com"
#define MQTT_PORT              1883
#define MQTT_CLIENT_ID         "6a4baaccc9429d337f57cf79_myNodeId_0_0_2026070703"
#define MQTT_USERNAME          "6a4baaccc9429d337f57cf79_myNodeId"
#define MQTT_PASSWORD          "1f34abb3d40a1fcdce5ee8584c5f975da9c564b0964a8f8efcb385b2f17c5808"

#define MQTT_DEVICE_ID         "6a4baaccc9429d337f57cf79_myNodeId"

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
