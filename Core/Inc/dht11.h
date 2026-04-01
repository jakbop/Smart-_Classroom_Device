#ifndef _DHT11_H_
#define _DHT11_H_

#include <stdint.h>

//在不同stm32板子上移植只需要修改以下两个宏
#define DHT11_GPIO_PORT GPIOC
#define DHT11_PIN GPIO_PIN_10

void DHT11_Init(void);


uint8_t DHT11_Get(float *temp1,uint8_t *temp2,uint8_t *humi);




#endif
