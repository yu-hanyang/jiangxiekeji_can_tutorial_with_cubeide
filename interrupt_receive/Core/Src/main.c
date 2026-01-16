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
#include "can.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "Key.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
//uint32_t TxID = 0x222;
//uint8_t TxLength = 4;
uint8_t TxDataArray[4][8] = {
		{ 0x11, 0x22, 0x33, 0x44 },
		{ 0xaa, 0xbb, 0xcc, 0xdd },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 }

};

CAN_TxHeaderTypeDef TxMsgArray[5] = {
		{ .StdId = 0x555, .ExtId = 0x00000000, .IDE =CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
		{ .StdId = 0x000, .ExtId = 0x12345678, .IDE =CAN_ID_EXT, .RTR = CAN_RTR_DATA, .DLC = 4 },
		{ .StdId = 0x666, .ExtId = 0x00000000, .IDE =CAN_ID_STD, .RTR = CAN_RTR_REMOTE, .DLC = 0 },
		{ .StdId = 0x000, .ExtId = 0x12345678, .IDE =CAN_ID_EXT, .RTR = CAN_RTR_REMOTE, .DLC = 0 },
		{0}
};

uint8_t pTxMsgArray = 0;

//uint32_t RxID;
//uint8_t RxLength;


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
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();

	OLED_ShowString(1, 1, "Rx :");
	OLED_ShowString(2, 1, "RxID:");
	OLED_ShowString(3, 1, "Leng:");
	OLED_ShowString(4, 1, "Data:");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		KeyNum = Key_GetNum();

		if (KeyNum == 1) {
			MyCAN_Transmit(TxMsgArray[pTxMsgArray], TxDataArray[pTxMsgArray]);
			pTxMsgArray ++;
			pTxMsgArray %= 4;
		}

		if (MyCAN_RxFlag == 1) {
			MyCAN_RxFlag = 0;

			if (RxHeader.IDE == CAN_ID_STD) {
				OLED_ShowString(1, 6, "Std");
				OLED_ShowHexNum(2, 6, RxHeader.StdId, 8);

			} else if (RxHeader.IDE == CAN_ID_EXT) {
				OLED_ShowString(1, 6, "Ext");
				OLED_ShowHexNum(2, 6, RxHeader.ExtId, 8);
			}

			if (RxHeader.RTR == CAN_RTR_DATA) {
				OLED_ShowString(1, 10, "Data  ");
				OLED_ShowHexNum(3, 6, RxHeader.DLC, 1);
				OLED_ShowHexNum(4, 6, RxData[0], 2);
				OLED_ShowHexNum(4, 9, RxData[1], 2);
				OLED_ShowHexNum(4, 12, RxData[2], 2);
				OLED_ShowHexNum(4, 15, RxData[3], 2);
			} else if (RxHeader.RTR == CAN_RTR_REMOTE) {
				OLED_ShowString(1, 10, "Remote");
				OLED_ShowHexNum(3, 6, RxHeader.DLC, 1);
				OLED_ShowHexNum(4, 6, 0x00, 2);
				OLED_ShowHexNum(4, 9, 0x00, 2);
				OLED_ShowHexNum(4, 12, 0x00, 2);
				OLED_ShowHexNum(4, 15, 0x00, 2);
			}

		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
	while (1) {
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
