#include "stm32f10x.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "esp8266.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void esp8266_init(void)
{
	USART3_Init(115200);
}

void esp8266_send_at(char *str)
{
	memset((void *)g_esp8266_rx_buf, 0, sizeof(g_esp8266_rx_buf));
	g_esp8266_rx_cnt = 0;
	USART3_SendString(str);
}

void esp8266_send_bytes(uint8_t *buf, uint32_t len)
{
	usart3_send_bytes(buf, len);
}

void esp8266_send_str(char *buf)
{
	usart3_send_str(buf);
}

int32_t esp8266_find_str_in_rx_packet(char *str, uint32_t timeout)
{
    while (timeout)
    {
        if (strstr((const char *)g_esp8266_rx_buf, str) != NULL)
            return 0;
        delay_ms(1);
        timeout--;
    }
    return -1;
}

int32_t esp8266_self_test(void)
{
	esp8266_send_at("AT\r\n");
	return esp8266_find_str_in_rx_packet("OK", 1000);
}

int32_t esp8266_connect_ap(char* ssid, char* pswd)
{
	esp8266_send_at("AT+CWMODE_CUR=1\r\n");
	if (esp8266_find_str_in_rx_packet("OK", 1000))
		return -1;

	esp8266_send_at("AT+CWJAP_CUR=");
	esp8266_send_at("\""); esp8266_send_at(ssid); esp8266_send_at("\"");
	esp8266_send_at(",");
	esp8266_send_at("\""); esp8266_send_at(pswd); esp8266_send_at("\"");
	esp8266_send_at("\r\n");

	if (esp8266_find_str_in_rx_packet("OK", 5000) == 0)
		return 0;

	return -2;
}

int32_t esp8266_exit_transparent_transmission(void)
{
	esp8266_send_at("+++");
	delay_ms(1500);
	esp8266_send_at("\r\n");
	delay_ms(500);
	g_esp8266_transparent_transmission_sta = 0;
	return 0;
}

int32_t esp8266_entry_transparent_transmission(void)
{
	esp8266_send_at("AT+CIPMODE=1\r\n");
	if (esp8266_find_str_in_rx_packet("OK", 5000))
		return -1;

	delay_ms(2000);
	esp8266_send_at("AT+CIPSEND\r\n");
	if (esp8266_find_str_in_rx_packet(">", 5000))
		return -2;

	g_esp8266_transparent_transmission_sta = 1;
	return 0;
}

int32_t esp8266_connect_server(char* mode, char* ip, uint16_t port)
{
	char buf[16] = {0};

	esp8266_send_at("AT+CIPSTART=");
	esp8266_send_at("\""); esp8266_send_at(mode); esp8266_send_at("\"");
	esp8266_send_at(",");
	esp8266_send_at("\""); esp8266_send_at(ip); esp8266_send_at("\"");
	esp8266_send_at(",");
	sprintf(buf, "%d", port);
	esp8266_send_at(buf);
	esp8266_send_at("\r\n");

	if (esp8266_find_str_in_rx_packet("CONNECT", 5000) == 0)
		return 0;
	if (esp8266_find_str_in_rx_packet("OK", 5000) == 0)
		return 0;

	return -1;
}

int32_t esp8266_disconnect_server(void)
{
	esp8266_send_at("AT+CIPCLOSE\r\n");
	if (esp8266_find_str_in_rx_packet("CLOSED", 5000))
		if (esp8266_find_str_in_rx_packet("OK", 5000))
			return -1;
	return 0;
}

int32_t esp8266_enable_multiple_id(uint32_t b)
{
	char buf[32] = {0};
	sprintf(buf, "AT+CIPMUX=%d\r\n", b);
	esp8266_send_at(buf);
	if (esp8266_find_str_in_rx_packet("OK", 5000))
		return -1;
	return 0;
}

int32_t esp8266_create_server(uint16_t port)
{
	char buf[32] = {0};
	sprintf(buf, "AT+CIPSERVER=1,%d\r\n", port);
	esp8266_send_at(buf);
	if (esp8266_find_str_in_rx_packet("OK", 5000))
		return -1;
	return 0;
}

int32_t esp8266_close_server(uint16_t port)
{
	char buf[32] = {0};
	sprintf(buf, "AT+CIPSERVER=0,%d\r\n", port);
	esp8266_send_at(buf);
	if (esp8266_find_str_in_rx_packet("OK", 5000))
		return -1;
	return 0;
}

int32_t esp8266_enable_echo(uint32_t b)
{
	if (b)
		esp8266_send_at("ATE1\r\n");
	else
		esp8266_send_at("ATE0\r\n");

	if (esp8266_find_str_in_rx_packet("OK", 5000))
		return -1;
	return 0;
}

int32_t esp8266_reset(void)
{
	esp8266_send_at("AT+RST\r\n");
	if (esp8266_find_str_in_rx_packet("OK", 10000))
		return -1;
	return 0;
}
