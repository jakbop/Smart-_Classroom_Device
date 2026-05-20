#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f1xx_hal.h"

/* 引脚定义 — PA7 */
#define DS18B20_PORT        GPIOA
#define DS18B20_PIN         GPIO_PIN_7

/* 单总线操作宏（开漏输出模式） */
#define DS18B20_SET_LOW()   HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET)
#define DS18B20_SET_HIGH()  HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET)
#define DS18B20_READ()      HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)

/* DS18B20 常用命令 */
#define DS18B20_CMD_SKIP_ROM       0xCC
#define DS18B20_CMD_CONVERT_T      0x44
#define DS18B20_CMD_READ_SCRATCH   0xBE

/* 函数声明 */
void     DS18B20_Init(void);
uint8_t  DS18B20_Reset(void);
void     DS18B20_WriteByte(uint8_t data);
uint8_t  DS18B20_ReadByte(void);
void     DS18B20_StartConversion(void);
float    DS18B20_GetTemperature(void);

#endif
