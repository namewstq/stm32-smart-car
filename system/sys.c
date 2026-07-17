#include "sys.h"

void NVIC_Configuration(void)
{

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//配置NVIC中断分组2:2位抢占优先级，2位响应优先级

}
