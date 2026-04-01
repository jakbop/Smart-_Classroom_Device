
#include "tim.h"
#include "delay.h"






//微秒级延时函数
void Delay_US(uint16_t t)  // 参数 t 的范围：[0, 65535]
{
	__HAL_TIM_SetCounter(&htim2, 0);
	
	HAL_TIM_Base_Start(&htim2);
	
	while(__HAL_TIM_GetCounter(&htim2) < t);
	
	HAL_TIM_Base_Stop(&htim2);
}












