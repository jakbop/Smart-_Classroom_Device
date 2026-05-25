#include "gpio.h"
#include "delay.h"
#include "dht11.h"


//DHT11初始化
void DHT11_Init(void){
	__HAL_RCC_GPIOC_CLK_ENABLE();
}


// 注意：你需要自己实现 Delay_US 微秒延时函数
// 例如：SysTick 延时 1us

uint8_t DHT11_Get(float *temp1,uint8_t *temp2, uint8_t *humi)
{
    GPIO_InitTypeDef conf = {0};
    uint8_t data[5] = {0};
    uint8_t i, j;

    // 1. 设置为推挽输出（发送起始信号）
    conf.Pin = DHT11_PIN;
    conf.Mode = GPIO_MODE_OUTPUT_PP;   // 正确：推挽输出
    conf.Speed = GPIO_SPEED_FREQ_LOW;
    conf.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &conf);

    // 2. 主机起始信号：拉低 ≥18ms
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    // 3. 拉高释放总线
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_PIN, GPIO_PIN_SET);
    Delay_US(30);  // 等待 20~40us 都可以

    // 4. 切换为输入模式
    conf.Mode = GPIO_MODE_INPUT;
    conf.Pull = GPIO_PULLUP;  // 开启上拉更稳定
    HAL_GPIO_Init(DHT11_GPIO_PORT, &conf);

    // 5. 检测 DHT11 响应（拉低 80us）
    if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN))
    {
        return 1; // 无响应
    }

    // 等待响应低电平结束
    while (!HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));

    // 等待响应高电平结束
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));

    // 6. 开始读取 40bit 数据
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            // 等待 50us 低电平开始信号
            while (!HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));

            // 延时 35~40us 后读取电平判断 0/1
            Delay_US(40);

            data[i] <<= 1;
            if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN))
            {
                data[i] |= 1;
            }

            // 等待这一位结束
            while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));
        }
    }

    // 7. 校验和判断
    if (data[0] + data[1] + data[2] + data[3] != data[4])
    {
        return 2; // 校验错误
    }

    // 8. 输出数据
    if (humi != NULL)
        *humi = data[0];  // 湿度整数

    if (temp1 != NULL)
        *temp1 = data[2] + data[3] / 10.0f;  // 温度小数

    

    return 0; // 成功
}












