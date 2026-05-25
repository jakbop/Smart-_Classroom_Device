/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "light.h"
#include "ds18b20.h"
#include "oled.h"
#include "dht11.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEVICE_ID "wengyuanhang"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
	
	//温湿度初始化
	DHT11_Init();
	
	//ds18b20初始化
	//DS18B20_Init();
	
	//OLED初始化
	OLED_Init();
	OLED_Clear();
	
	//显示标题
	OLED_ShowString(0, 0, (u8*)"Smart Device", 16);
	OLED_ShowString(0, 2, (u8*)"----------------", 12);
	
	//开启灯光设备1和设备2，LED1高电平有效，高电平时亮灯。LED2低电平有效，低电平时亮灯
	HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,GPIO_PIN_SET);
	
	
	
	uint8_t switch_state = 0;
	
	char upload_data[100];
	char oled_buf[20];  //OLED显示缓冲区
	
	uint32_t lightValue;  //记录光照数据
	
	float temp1 = 0;   //记录温度
	uint8_t temp2 = 0;  //记录温度
	uint8_t humi = 0;   //记录湿度
	
	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		
		if(DHT11_Get(&temp1,&temp2,&humi) == 0){
			//如果采集成功，就上传云端
			sprintf(upload_data,"%s/sensor/Temp %.2f_%u\n",DEVICE_ID,temp1,humi);
			HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,strlen(upload_data),1000);	
		}
		
		
		//OLED显示温度
		OLED_ShowString(0, 3, (u8*)"Temp:   ", 12);
		sprintf(oled_buf, "%.2f", temp1);
		OLED_ShowString(48, 3, (u8*)oled_buf, 12);
		
		//OLED显示湿度
		OLED_ShowString(0, 4, (u8*)"Humi:   ",12);
		sprintf(oled_buf, "%u", humi);
		OLED_ShowString(48, 4, (u8*)oled_buf, 12);
		
		//获取光照值，并上传到云端
		lightValue = Light_Get();
		
		sprintf(upload_data,"%s/sensor/Light %u\n",DEVICE_ID,lightValue);
		
		HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,strlen(upload_data),1000);
		
		//OLED显示光照
		OLED_ShowString(0, 5, (u8*)"Light:     ", 12);
		sprintf(oled_buf, "%u", lightValue);
		OLED_ShowString(48, 5, (u8*)oled_buf, 12);
		
		
		
		//获取灯光设备1LED1的状态，并上传到云端，
		switch_state = HAL_GPIO_ReadPin(LED1_GPIO_Port,LED1_Pin);
		
		//sprintf(upload_data,"%s/state/Led1 %s\n",DEVICE_ID, switch_state ? "ON" : "OFF");
		sprintf(upload_data,"%s/state/Led1 %u\n",DEVICE_ID, switch_state);
		
		HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,strlen(upload_data),1000);
		
		//OLED显示LED1状态
		OLED_ShowString(0, 6, (u8*)"LED1:", 12);
		OLED_ShowString(48, 6, (u8*)(switch_state ? "ON " : "OFF"), 12);
		
		
		
		////获取灯光设备2LED2的状态，并上传到云端
		switch_state = HAL_GPIO_ReadPin(LED2_GPIO_Port,LED2_Pin);
		
		//sprintf(upload_data,"%s/state/Led2 %s\n",DEVICE_ID, switch_state ? "ON" : "OFF");
		sprintf(upload_data,"%s/state/Led2 %u\n",DEVICE_ID, switch_state);
		
		HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,strlen(upload_data),1000);
		
		//OLED显示LED2状态
		OLED_ShowString(0, 7, (u8*)"LED2:", 12);
		OLED_ShowString(48, 7, (u8*)(switch_state ? "OFF" : "ON "), 12);
		
		HAL_Delay(1000);
		
		
		
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
