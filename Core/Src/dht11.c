
#include "gpio.h"
#include "delay.h"
#include "dht11.h"


//DHT11初始化
void DHT11_Init(void){
	__HAL_RCC_GPIOC_CLK_ENABLE();
}

/*
@Brief  通过DHT11传感器采集一次温湿度数据

@Params
  temp 输出参数，接收温度数据，如果对温度数据不感兴趣，本参数可以传NULL
  humi 输出参数，接收湿度数据，如果对湿度数据不感兴趣，本参数可以传NULL

@Return
  如果采集成功返回0，失败返回非零错误码，具体意义如下：
    1 表示DHT11传感器损坏或连接失败；
    2 表示校验失败，通信可能受干扰；

@Author
  Ww

@Email
  @qq.com

@Remark
本函数依赖微秒级函数Delay_US，移植时需要自己实现

@Date
  2026-3-17
	
@Example
float t, h;
if(DHT11_Get(&t,&h)==0){
  //成功处理
}else{
  //失败处理
}

*/



/*
uint8_t DHT11_Get(float *temp1, uint8_t *temp2,uint8_t *humi ){
	
	
  GPIO_InitTypeDef conf = {0};
	uint8_t data[5];   //用于存放温湿度数据
	uint8_t i,j;

	conf.Pin = DHT11_PIN;
	conf.Speed = GPIO_SPEED_FREQ_LOW;
	
	
  
		
  //将pc10设置为开漏输出
	conf.Mode = GPIO_MODE_AF_OD;
	HAL_GPIO_Init(DHT11_GPIO_PORT,&conf);
		
	//MCU发出开始信号,开始为低电平,保持电平大于18ms
	HAL_GPIO_WritePin(DHT11_GPIO_PORT,DHT11_PIN,GPIO_PIN_RESET);
	HAL_Delay(20);
		
	//将MCU切换到输入模式,释放单总线，此时在上拉电阻的作用下，总线的电平状态为高电平
	conf.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(DHT11_GPIO_PORT,&conf);
		
	//总线释放13微秒,拉高等待
	Delay_US(13);
		
	//取中点检测电平
	Delay_US(40);
	
	//读取DHT11响应信号，如果是高电平说明传感器发生损坏或链接异常
	if (HAL_GPIO_ReadPin(DHT11_GPIO_PORT,DHT11_PIN)){
		return 1;   //返回1表示错误码
		//HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin,GPIO_PIN_SET);
		//HAL_Delay(100);
		//HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin,GPIO_PIN_RESET);
		}
		
	//如果传感器正常
	//等待传感器响应结束
	while(!HAL_GPIO_ReadPin(DHT11_GPIO_PORT,DHT11_PIN));
	Delay_US(120);
			
	//开始逐位接收温度湿度数据（共40位bit，高位先出）
	for(i = 0;i<5;i++){
		data[i] = 0;
		for(j = 0;j<8;j++){
			//接收 1bit  数据
			//等待前导低电平信号结束
			while(!HAL_GPIO_ReadPin(DHT11_GPIO_PORT,DHT11_PIN));
			Delay_US(50);
					
			data[i] <<= 1;  //左移一位
					
			//判断是1还是0
			if(HAL_GPIO_ReadPin(DHT11_GPIO_PORT,DHT11_PIN)){
				//判断为1
				data[i] += 1;
				Delay_US(50);
			}
		}
	}
			
	// 校验数据是否有效（通过校验和实现）
	//五个八位数据，第五个数据为校验和数据
	if(data[0] + data[1] + data[2] + data[3] != data[4]){
	// 校验失败，数据无效
		return 2;   //返回错误码2
//				HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin,GPIO_PIN_SET);
//			  HAL_Delay(50);
//			  HAL_GPIO_WritePin(BUZZER_GPIO_Port,BUZZER_Pin,GPIO_PIN_RESET);
		}
			
	//校验成功
			
	//返回湿度数据
	if(humi != NULL) *humi = data[0];   
			
	//返回温度数据 
		if(temp1 != NULL){
			*temp1 = data[2] + data[3] / 10.0;
			*temp2 = data[2];
		}
			
		return 0;  //返回0，采集数据成功
	}
	
*/

#include "dht11.h"

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




































