#include "motor.h"

volatile uint8_t g_motor_running = 0;

void Motor_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);

    //PB12~PB15

    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;

    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;

    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;

    GPIO_Init(GPIOB,&GPIO_InitStructure);

    GPIO_ResetBits(GPIOB,
                   GPIO_Pin_12|
                   GPIO_Pin_13|
                   GPIO_Pin_14|
                   GPIO_Pin_15);
}

//PWM初始化
//PB6 --> 左轮转向
//PB7 --> 右轮转向
static void Motor_PWM_Init(void)
{
    GPIO_InitTypeDef 			GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef 	TIM_Base;
    TIM_OCInitTypeDef 			TIM_OC;// 定时器通道比较初始化结构体

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|
                           RCC_APB2Periph_AFIO,
                           ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,
                           ENABLE);

    GPIO_InitStructure.GPIO_Pin= GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);

	// 设置定时器TIM4时钟频率
    // 设置预分频系数为72-1，使得 72MHz / 72 = 1MHz
    TIM_Base.TIM_Prescaler=72-1;
    TIM_Base.TIM_Period=1000-1;
    TIM_Base.TIM_CounterMode=TIM_CounterMode_Up;
    TIM_Base.TIM_ClockDivision=TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM4,&TIM_Base);


	// 设置PWM输出模式

    // 初始化输出比较结构体为默认值
    TIM_OC.TIM_OCMode=TIM_OCMode_PWM1;				// 设置输出比较模式为PWM1模式
    TIM_OC.TIM_OutputState=TIM_OutputState_Enable;	// 使能输出通道
    TIM_OC.TIM_OCPolarity=TIM_OCPolarity_High;		// 设置输出极性为高电平有效
    TIM_OC.TIM_Pulse=0;								// 设置比较值为0，对应0%占空比
    
	TIM_OC1Init(TIM4,&TIM_OC);						// 初始化TIM4通道1的OC1输出
    TIM_OC2Init(TIM4,&TIM_OC);

    TIM_OC1PreloadConfig(TIM4,TIM_OCPreload_Enable);// 使能TIM4通道1预装载功能
    TIM_OC2PreloadConfig(TIM4,TIM_OCPreload_Enable);// 使能TIM4通道2预装载功能
    TIM_ARRPreloadConfig(TIM4,ENABLE);				// 使能自动重装载预装载功能

    TIM_Cmd(TIM4,ENABLE);							// 启动TIM4定时器
}

//初始化
void Motor_Init(void)
{
    Motor_GPIO_Init();

    Motor_PWM_Init();
}


//设置PWM
static void SetLeftPWM(uint16_t pwm)
{
    if(pwm>PWM_MAX)
        pwm=PWM_MAX;

    TIM_SetCompare1(TIM4,pwm); // 设置TIM4通道1的比较值，控制左轮PWM占空比
}

static void SetRightPWM(uint16_t pwm)
{
    if(pwm>PWM_MAX)
        pwm=PWM_MAX;

    TIM_SetCompare2(TIM4,pwm);// 设置TIM4通道2的比较值，控制右轮PWM占空比
}

//设置速度函数
void Motor_SetSpeed(int16_t left,int16_t right)
{
    if(left>=0)
    {
        GPIO_SetBits(GPIOB,GPIO_Pin_12);
        GPIO_ResetBits(GPIOB,GPIO_Pin_13);

        SetLeftPWM(left);
    }
    else
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_12);
        GPIO_SetBits(GPIOB,GPIO_Pin_13);

        SetLeftPWM(-left);
    }

    if(right>=0)
    {
        GPIO_SetBits(GPIOB,GPIO_Pin_14);
        GPIO_ResetBits(GPIOB,GPIO_Pin_15);

        SetRightPWM(right);
    }
    else
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_14);
        GPIO_SetBits(GPIOB,GPIO_Pin_15);

        SetRightPWM(-right);
    }
}

//运动停止
void Motor_Stop(void)
{
    SetLeftPWM(0);
    SetRightPWM(0);
    g_motor_running = 0;
}

void Motor_Forward(uint16_t speed)
{
    Motor_SetSpeed(speed,speed);
    g_motor_running = 1;
}

void Motor_Backward(uint16_t speed)
{
    Motor_SetSpeed(-speed,-speed);
    g_motor_running = 1;
}

void Motor_Left(uint16_t speed)
{
    Motor_SetSpeed(-speed,speed);
    g_motor_running = 1;
}

void Motor_Right(uint16_t speed)
{
    Motor_SetSpeed(speed,-speed);
    g_motor_running = 1;
}

// 单轮转向（避障专用）
// 只有一个轮子转，另一个轮子使车辆原地旋转
// 转向时"差速"原理：转向内侧轮停，外侧轮动

void Motor_LeftPivot(uint16_t speed)
{
    Motor_SetSpeed(0, speed);
    g_motor_running = 1;
}

void Motor_RightPivot(uint16_t speed)
{
    Motor_SetSpeed(speed, 0);
    g_motor_running = 1;
}

void Motor_LeftPivotBack(uint16_t speed)
{
    Motor_SetSpeed(0, -speed);
    g_motor_running = 1;
}

void Motor_RightPivotBack(uint16_t speed)
{
    Motor_SetSpeed(-speed, 0);
    g_motor_running = 1;
}
