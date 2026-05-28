#include "gpio.h"
#include "delay.h"
#include "dht11.h"


//DHT11��ʼ��
void DHT11_Init(void){
	__HAL_RCC_GPIOA_CLK_ENABLE();
}


// ע�⣺����Ҫ�Լ�ʵ�� Delay_US ΢����ʱ����
// ���磺SysTick ��ʱ 1us

uint8_t DHT11_Get(float *temp1,uint8_t *temp2, uint8_t *humi)
{
    GPIO_InitTypeDef conf = {0};
    uint8_t data[5] = {0};
    uint8_t i, j;

    // 1. ����Ϊ���������������ʼ�źţ�
    conf.Pin = DHT11_PIN;
    conf.Mode = GPIO_MODE_OUTPUT_PP;   // ��ȷ���������
    conf.Speed = GPIO_SPEED_FREQ_LOW;
    conf.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &conf);

    // 2. ������ʼ�źţ����� ��18ms
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    // 3. �����ͷ�����
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_PIN, GPIO_PIN_SET);
    Delay_US(30);  // �ȴ� 20~40us ������

    // 4. �л�Ϊ����ģʽ
    conf.Mode = GPIO_MODE_INPUT;
    conf.Pull = GPIO_PULLUP;  // �����������ȶ�
    HAL_GPIO_Init(DHT11_GPIO_PORT, &conf);

    // 5. ��� DHT11 ��Ӧ������ 80us��
    if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN))
    {
        return 1; // ����Ӧ
    }

    // �ȴ���Ӧ�͵�ƽ����
    while (!HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));

    // �ȴ���Ӧ�ߵ�ƽ����
    while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));

    // 6. ��ʼ��ȡ 40bit ����
    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 8; j++)
        {
            // �ȴ� 50us �͵�ƽ��ʼ�ź�
            while (!HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));

            // ��ʱ 35~40us ���ȡ��ƽ�ж� 0/1
            Delay_US(40);

            data[i] <<= 1;
            if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN))
            {
                data[i] |= 1;
            }

            // �ȴ���һλ����
            while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));
        }
    }

    // 7. У����ж�
    if (data[0] + data[1] + data[2] + data[3] != data[4])
    {
        return 2; // У�����
    }

    // 8. �������
    if (humi != NULL)
        *humi = data[0];  // ʪ������

    if (temp1 != NULL)
        *temp1 = data[2] + data[3] / 10.0f;  // �¶�С��

    

    return 0; // �ɹ�
}












