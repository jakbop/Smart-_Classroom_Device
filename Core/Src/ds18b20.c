#include "ds18b20.h"

/* 微秒延时 — 需根据主频校准 */
static void delay_us(uint32_t us)
{
    uint32_t i;
    for (i = 0; i < us * 8; i++)  // 72MHz下，i < us * 8 约等于 us 微秒
    {
        __NOP();
    }
}

/* 函数说明 */
static void DS18B20_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;      // 开漏输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;              // 内部上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

static void DS18B20_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

/**
 * @brief DS18B20 初始化
 */
void DS18B20_Init(void)
{
    DS18B20_SetOutput();
    DS18B20_SET_HIGH();
    HAL_Delay(10);
    DS18B20_Reset();
}

/**
 * @brief 复位并检测从机存在脉冲
 * @retval 0: 存在, 1: 不存在
 */
uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    
    /* 1. 主机拉低总线480~960us */
    DS18B20_SetOutput();
    DS18B20_SET_LOW();
    delay_us(500);
    
    /* 2. 释放总线（拉高） */
    DS18B20_SET_HIGH();
    delay_us(70);            // 等待15~60us后从机响应
    
    /* 3. 读取从机响应（存在脉冲：低电平） */
    DS18B20_SetInput();
    presence = DS18B20_READ();
    
    /* 4. 等待存在脉冲结束（60~240us） */
    delay_us(500);
    
    return presence;  // 0=存在, 1=不存在
}

/**
 * @brief 向DS18B20写入一个字节
 */
void DS18B20_WriteByte(uint8_t data)
{
    uint8_t i;
    
    DS18B20_SetOutput();
    for (i = 0; i < 8; i++)
    {
        DS18B20_SET_LOW();
        delay_us(2);                     // 拉低至少1us
        
        if (data & 0x01)
            DS18B20_SET_HIGH();          // 写“1”
        else
            DS18B20_SET_LOW();           // 写“0”
        
        delay_us(60);                    // 保持至少60us
        
        DS18B20_SET_HIGH();              // 释放总线
        delay_us(2);
        data >>= 1;
    }
}

/**
 * @brief 从DS18B20读取一个字节
 */
uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, data = 0;
    
    for (i = 0; i < 8; i++)
    {
        DS18B20_SetOutput();
        DS18B20_SET_LOW();
        delay_us(2);                     // 拉低至少1us
        
        DS18B20_SET_HIGH();              // 释放总线
        DS18B20_SetInput();
        delay_us(12);                    // 等待数据稳定（<15us）
        
        data >>= 1;
        if (DS18B20_READ())
            data |= 0x80;
        
        delay_us(50);                    // 等待读周期结束（约60us）
    }
    return data;
}

/**
 * @brief 启动温度转换
 */
void DS18B20_StartConversion(void)
{
    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);
}

/**
 * @brief 读取温度值（摄氏度）
 * @retval 温度值（支持负温度和小数）
 */
float DS18B20_GetTemperature(void)
{
    uint8_t temp_l, temp_h;
    int16_t raw_temp;
    float temperature;
    
    DS18B20_Reset();
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCH);
    
    temp_l = DS18B20_ReadByte();   // 低字节
    temp_h = DS18B20_ReadByte();   // 高字节
    
    raw_temp = (int16_t)((temp_h << 8) | temp_l);
    
    /* 温度换算（12位分辨率，0.0625°C/LSB） */
    temperature = raw_temp * 0.0625f;
    
    return temperature;
}
