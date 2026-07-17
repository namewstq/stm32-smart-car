#include "stm32f10x.h" 
#include "delay.h"
#include "dht11.h"

#define DHT_TIMEOUT  10000

void DHT_Init_InPut(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);	
}

void DHT_Init_OutPut(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);	
}


void DHT_STart(void)
{
	DHT_Init_OutPut();
    GPIO_ResetBits(GPIOA,GPIO_Pin_1);
	delay_ms(20);
	GPIO_SetBits(GPIOA,GPIO_Pin_1);
	delay_us(30);
	DHT_Init_InPut();
}

uint16_t DHT_Scan(void)
{
	return GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1);
}

uint16_t DHT_ReadBit(void)
{	
	uint32_t timeout = DHT_TIMEOUT;
	while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == RESET)
	{
		if(--timeout == 0) return 0;
	}
	delay_us(40);
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == SET)
	{
		timeout = DHT_TIMEOUT;
		while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1) == SET)
		{
			if(--timeout == 0) return 0;
		}
		return 1;
	}else
	{
		return 0;
	}	
}

uint16_t DHT_ReadByte(void)
{
	uint16_t i,data = 0;
	for(i = 0;i < 8;i++)
	{
		data <<= 1;
		data |= DHT_ReadBit();
	}
	return data;
}

uint16_t DHT_ReadData(uint8_t buffer[5])
{
	uint8_t i;
	uint32_t timeout;
	DHT_STart();
	if(DHT_Scan() == RESET)
	{
		timeout = DHT_TIMEOUT;
		while(DHT_Scan() == RESET)
		{
			if(--timeout == 0) return 1;
		}
		timeout = DHT_TIMEOUT;
		while(DHT_Scan() == SET)
		{
			if(--timeout == 0) return 1;
		}
		
		for(i = 0;i < 5;i++)
		{
			buffer[i] = DHT_ReadByte();
		}
		timeout = DHT_TIMEOUT;
		while(DHT_Scan() == RESET)
		{
			if(--timeout == 0) return 1;
		}
		DHT_Init_OutPut();
		GPIO_SetBits(GPIOA, GPIO_Pin_1);
		
		uint8_t check = buffer[0] + buffer[1] + buffer[2] + buffer[3];
		if(check != buffer[4])
			{
				return 1;
			}
	}
	
	return 0;
	
}

////读取dht11温湿度数据
////返回0表示成功，非0失败
////temp和humi为输出参数，分别存储温度和湿度
//u8 DHT11_Readdata(u8 *temp,u8 *humi)
//{
//	u8 i,buf[5];
//	
//	//起始信号
//	if(DHT_ReadBit()==0){
//		//读取40bits(5bytes)数据
//		for(i=0;i<5;i++){
//			buf[i] = DHT_ReadByte();
//		}
//		//验证校验和
//		if(((buf[0]+buf[1]+buf[2]+buf[3])&0xff)==buf[4]){
//			*temp = buf[2];
//			*humi = buf[0];
//			
//			return 0;
//		}
//	}
//	
//	return 1;
//}
