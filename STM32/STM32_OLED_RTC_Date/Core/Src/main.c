/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "gpio.h"
#include "i2c.h"
#include "rtc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "myrtc.h"
#include "oled.h"
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DAY_SECOND 86400
#define BAR_DAY_START_X 13
#define BAR_DAY_START_Y 20
#define BAR_HOUR_START_X 13
#define BAR_HOUR_START_Y 28
#define BAR_MINUTE_START_X 13
#define BAR_MINUTE_START_Y 36
#define BAR_LENGTH 51
#define BAR_HEIGHT 6
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
int main(void) {
  uint8_t cnt = 0;

  char weeks[7][10] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                       "Thursday", "Friday", "Saturday"};
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(20);
  OLED_Init();
  OLED_NewFrame();
  KK_RTC_Init();
  uint32_t total_second_day = 0;
  uint16_t total_second_minute = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    RTC_DateTimeTypeDef calendar;
    RTC_GetTime(&calendar);
    uint32_t total_second_day =
        calendar.hour * 3600 + calendar.min * 60 + calendar.sec;
    uint16_t total_second_minute = calendar.min * 60 + calendar.sec;
    uint8_t day_percentage =
        (uint8_t)((float)total_second_day / DAY_SECOND * 100); // 天进度条百分比
    uint8_t hour_percentage =
        (uint8_t)((float)total_second_minute / 3600 * 100); // 小时进度条百分比
    uint8_t minute_percentage =
        (uint8_t)((float)calendar.sec / 60 * 100); // 分钟进度条百分比
    char str[30];
    char datestr[30];
    char percentStr[10]; // 用于存储百分比字符串

    sprintf(str, "%02d:%02d:%02d", calendar.hour, calendar.min, calendar.sec);
    sprintf(datestr, "%d-%02d-%02d", calendar.year, calendar.month,
            calendar.date);
    OLED_NewFrame();
    OLED_PrintASCIIString(20, 50, datestr, &afont8x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(32, 0, str, &afont16x8, OLED_COLOR_NORMAL);

    // === 天进度条 + 百分比 ===
    OLED_DrawRectangle(BAR_DAY_START_X, BAR_DAY_START_Y, BAR_LENGTH, BAR_HEIGHT,
                       OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(BAR_DAY_START_X, BAR_DAY_START_Y + 1,
                             (float)total_second_day * BAR_LENGTH / DAY_SECOND,
                             BAR_HEIGHT - 1, OLED_COLOR_NORMAL);

    sprintf(percentStr, "Day:%u%%", day_percentage); // 格式化为 "xx%"
    OLED_PrintASCIIString(BAR_DAY_START_X + BAR_LENGTH + 10, BAR_DAY_START_Y,
                          percentStr, &afont8x6, OLED_COLOR_NORMAL);

    // === 小时进度条 + 百分比 ===
    OLED_DrawRectangle(BAR_HOUR_START_X, BAR_HOUR_START_Y, BAR_LENGTH,
                       BAR_HEIGHT, OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(BAR_HOUR_START_X, BAR_HOUR_START_Y + 1,
                             (float)total_second_minute * BAR_LENGTH / 3600,
                             BAR_HEIGHT - 1, OLED_COLOR_NORMAL);

    sprintf(percentStr, "Hour:%u%%", hour_percentage); // 整数百分比
    OLED_PrintASCIIString(BAR_HOUR_START_X + BAR_LENGTH + 10, BAR_HOUR_START_Y,
                          percentStr, &afont8x6, OLED_COLOR_NORMAL);

    // === 分钟进度条 + 百分比 ===
    OLED_DrawRectangle(BAR_MINUTE_START_X, BAR_MINUTE_START_Y, BAR_LENGTH,
                       BAR_HEIGHT, OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(BAR_MINUTE_START_X, BAR_MINUTE_START_Y + 1,
                             (float)calendar.sec * BAR_LENGTH / 60,
                             BAR_HEIGHT - 1, OLED_COLOR_NORMAL);

    sprintf(percentStr, "Min:%u%%", minute_percentage); // 整数百分比
    OLED_PrintASCIIString(BAR_MINUTE_START_X + BAR_LENGTH + 10,
                          BAR_MINUTE_START_Y, percentStr, &afont8x6,
                          OLED_COLOR_NORMAL);

    OLED_ShowFrame();
    HAL_Delay(1000);
    /* USER CODE END WHILE */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
