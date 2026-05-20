

#include "adc.h"
#include "light.h"


uint32_t Light_Get(void){
	
	
	uint32_t value ;
	
	//ADC —— Analog-to-Digital Converter(数模转换器)
	HAL_ADC_Start(&hadc1);  //启动ADC——Analog-to-Digital Converter
	
	HAL_ADC_PollForConversion(&hadc1,5);   //等待adc转换完成，设置超时时间为5毫秒
	
	value = HAL_ADC_GetValue(&hadc1);   //读取adc转换结果的数据
	
	HAL_ADC_Stop(&hadc1);    //停止adc
	
	return 4095-value;
   
}	
	
















