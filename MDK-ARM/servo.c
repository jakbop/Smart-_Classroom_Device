
#include "tim.h"
#include "servo.h"

void Servo_Init(void){

	__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,500);
  HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);	

}

void Servo_Set_Angle(uint16_t angle){
	
	//改变占空比
	__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,500.0 / 45 * angle + 500);
	
	
}

