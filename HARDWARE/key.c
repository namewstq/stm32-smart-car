#include <key.h>

void key_init(void)
{	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE |
                           RCC_APB2Periph_AFIO,
                           ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin	 = GPIO_Pin_4|GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;

	GPIO_Init(GPIOE,&GPIO_InitStructure);
}

//中断源配置
static void KEY_EXTI_GPIO_Config(void)
{
    /*---------------------------------------------
        EXTI3 连接 PE3
    ----------------------------------------------*/
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,
                        GPIO_PinSource3);

    /*---------------------------------------------
        EXTI4 连接 PE4
    ----------------------------------------------*/
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,
                        GPIO_PinSource4);
}

/*
    配置EXTI中断触发
*/
static void KEY_EXTI_Config(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    /*-----------------------------------------
        EXTI3配置
    ------------------------------------------*/
    EXTI_InitStructure.EXTI_Line = EXTI_Line3;

    // 中断模式
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;

    // 按键按下时触发（下降沿触发）
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;

    EXTI_InitStructure.EXTI_LineCmd = ENABLE;

    EXTI_Init(&EXTI_InitStructure);

    /*-----------------------------------------
        EXTI4配置
    ------------------------------------------*/
    EXTI_InitStructure.EXTI_Line = EXTI_Line4;

    EXTI_Init(&EXTI_InitStructure);
}

/*
    配置NVIC
*/
static void KEY_NVIC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /*-------------------------------------
        EXTI3
    --------------------------------------*/
    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;

    // 设置抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;

    // 设置响应优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;

    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

    /*-------------------------------------
        EXTI4
    --------------------------------------*/
    NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;

    NVIC_Init(&NVIC_InitStructure);
}

/*
    外部中断初始化
*/
void KEY_EXTI_Init(void)
{
    //1.GPIO初始化
    key_init();

    //2.GPIO映射到EXTI
    KEY_EXTI_GPIO_Config();

    //3.配置EXTI
    KEY_EXTI_Config();

    //4.配置NVIC
    KEY_NVIC_Config();
}


volatile uint8_t g_key1_pressed = 0;  /* KEY1(PE3)按下标志 */
volatile uint8_t g_key0_pressed = 0;  /* KEY0(PE4)按下标志 */

/*
    PE3外部中断，用于KEY1切换OLED页面
*/
void EXTI3_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line3) != RESET)
    {
        g_key1_pressed = 1;
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}

/*
    PE4外部中断，用于KEY0功能
*/
void EXTI4_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        g_key0_pressed = 1;
        EXTI_ClearITPendingBit(EXTI_Line4);
    }
}

