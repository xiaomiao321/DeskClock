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
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "rtc.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// #include "OLED.h"
#include "font.h"
#include "myrtc.h"
#include "oled.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
uint8_t serial2_rxbuf[100];    // 增大缓冲区，容纳 JSON
volatile uint8_t rx2_len = 0;  // UART2 接收数据长度
volatile uint8_t rx2_flag = 0; // UART2 接收完成标志
int temperature = -1;          // 温度，-1 表示无效
int humidity = -1;             // 湿度，-1 表示无效
char tempstr[12] = {0};        // Temp:xxC
char humidstr[12] = {0};       // Hum:xx%
char reporttime[20] = "N/A";   // 天气 API 的 reporttime
char short_rpt[9] = "N/A";     // 截断 reporttime 为 hh:mm:ss
/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern RTC_HandleTypeDef hrtc;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/**
 * @brief  解析接收到的 JSON 数据，提取 temperature 和 humidity
 * @param  buffer: 接收到的 UART 数据缓冲区
 * @param  temp: 存储解析后的温度值
 * @param  hum: 存储解析后的湿度值
 */
/**
 * @brief  解析天气 JSON 数据，提取 temperature、humidity 和 reporttime
 * @param  buffer: 接收到的 UART 数据缓冲区
 * @param  temp: 存储解析后的温度值
 * @param  hum: 存储解析后的湿度值
 * @param  rpt: 存储解析后的 reporttime（yyyy-mm-dd hh:mm:ss）
 */
void parseWeatherData(uint8_t *buffer, int *temp, int *hum, char *rpt) {
  *temp = -1;
  *hum = -1;
  strcpy(rpt, "N/A");

  char *data = (char *)buffer;

  char *temp_start = strstr(data, "\"temperature\"");
  if (temp_start != NULL) {
    temp_start += strlen("\"temperature\"");
    while (*temp_start == ' ' || *temp_start == ':' || *temp_start == '\n') {
      temp_start++;
    }
    if (*temp_start == '"') {
      temp_start++;
      char *temp_end = strchr(temp_start, '"');
      if (temp_end != NULL && (temp_end - temp_start) < 10) {
        char temp_str[10];
        strncpy(temp_str, temp_start, temp_end - temp_start);
        temp_str[temp_end - temp_start] = '\0';
        char *p = temp_str;
        while (*p) {
          if (!isdigit(*p))
            break;
          p++;
        }
        if (*p == '\0' && temp_str[0] != '\0') {
          *temp = atoi(temp_str);
        }
      }
    }
  }

  char *hum_start = strstr(data, "\"humidity\"");
  if (hum_start != NULL) {
    hum_start += strlen("\"humidity\"");
    while (*hum_start == ' ' || *hum_start == ':' || *hum_start == '\n') {
      hum_start++;
    }
    if (*hum_start == '"') {
      hum_start++;
      char *hum_end = strchr(hum_start, '"');
      if (hum_end != NULL && (hum_end - hum_start) < 10) {
        char hum_str[10];
        strncpy(hum_str, hum_start, hum_end - hum_start);
        hum_str[hum_end - hum_start] = '\0';
        char *p = hum_str;
        while (*p) {
          if (!isdigit(*p))
            break;
          p++;
        }
        if (*p == '\0' && hum_str[0] != '\0') {
          *hum = atoi(hum_str);
        }
      }
    }
  }

  char *rpt_start = strstr(data, "\"reporttime\"");
  if (rpt_start != NULL) {
    rpt_start += strlen("\"reporttime\"");
    while (*rpt_start == ' ' || *rpt_start == ':' || *rpt_start == '\n') {
      rpt_start++;
    }
    if (*rpt_start == '"') {
      rpt_start++;
      char *rpt_end = strchr(rpt_start, '"');
      if (rpt_end != NULL && (rpt_end - rpt_start) < 20) {
        strncpy(rpt, rpt_start, rpt_end - rpt_start);
        rpt[rpt_end - rpt_start] = '\0';
      }
    }
  }
}
/**
 * @brief  解析 NTP JSON 数据，提取 ntptime
 * @param  buffer: 接收到的 UART 数据缓冲区
 * @param  ntptime: 存储解析后的 ntptime（yyyy-mm-dd hh:mm:ss）
 */
void parseNTPTime(uint8_t *buffer, char *ntptime) {
  strcpy(ntptime, "N/A");

  char *data = (char *)buffer;

  // 查找 "ntptime": " 字段
  char *ntp_start = strstr(data, "\"ntptime\"");
  if (ntp_start != NULL) {
    ntp_start += strlen("\"ntptime\"");
    while (*ntp_start == ' ' || *ntp_start == ':' || *ntp_start == '\n') {
      ntp_start++;
    }
    if (*ntp_start == '"') {
      ntp_start++;
      char *ntp_end = strchr(ntp_start, '"');
      if (ntp_end != NULL && (ntp_end - ntp_start) < 20) {
        strncpy(ntptime, ntp_start, ntp_end - ntp_start);
        ntptime[ntp_end - ntp_start] = '\0';
      }
    }
  }
}
/**
 * @brief  使用 ntptime 更新 RTC
 * @param  ntptime: 格式为 yyyy-mm-dd hh:mm:ss 的字符串
 */
void updateRTC(const char *ntptime) {
  if (strcmp(ntptime, "N/A") == 0) {
    return;
  }

  int year, month, day, hour, minute, second;
  if (sscanf(ntptime, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute,
             &second) == 6) {
    // second += 5;
    RTC_SetTime(year, month, day, hour, minute, second);
  } else {
    printf("Invalid ntptime format: %s\r\n", ntptime);
  }
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance == USART2) {
    rx2_len = Size; // 记录接收到的数据长度
    rx2_flag = 1;   // 设置接收完成标志
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial2_rxbuf,
                                 sizeof(serial2_rxbuf)); // 重新启动 DMA 接收
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);    // 禁用半传输中断
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  // 错误处理，重新启动 DMA 接收
  if (huart->Instance == USART2) {
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial2_rxbuf, sizeof(serial2_rxbuf));
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
  }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f) // printf
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY); // printf
  // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&ch, 1);
  return (ch);
}

int fgetc(FILE *f) {
  uint8_t ch;
  HAL_UART_Receive(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY); // scanf
  return ch;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(20);
  OLED_Init();
  OLED_NewFrame();
  KK_RTC_Init();
  uint32_t total_second_day = 0;
  uint16_t total_second_minute = 0;

  // 启动 DMA 接收
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, serial2_rxbuf, sizeof(serial2_rxbuf));
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {

    // RTC部分
    RTC_DateTimeTypeDef calendar;
    RTC_GetTime(&calendar);
    uint32_t total_second_day =
        calendar.hour * 3600 + calendar.min * 60 + calendar.sec;
    uint16_t total_second_minute = calendar.min * 60 + calendar.sec;
    uint8_t day_percentage =
        (uint8_t)((float)total_second_day / 86400 * 100); // 天进度条百分比
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

    //  检查 UART2 接收到的天气数据
    if (rx2_flag) {
      serial2_rxbuf[rx2_len] = '1';
      printf("Received: %s\r\n", serial2_rxbuf);

      // 判断 JSON 类型
      if (strstr((char *)serial2_rxbuf, "\"ntptime\"") != NULL) {
        char ntptime[20] = "N/A";
        parseNTPTime(serial2_rxbuf, ntptime);
        if (strcmp(ntptime, "N/A") != 0) {
          updateRTC(ntptime);
        } else {
          printf("Parse failed: 'ntptime' invalid\r\n");
        }
      } else if (strstr((char *)serial2_rxbuf, "\"temperature\"") != NULL) {
        parseWeatherData(serial2_rxbuf, &temperature, &humidity, reporttime);
        if (temperature == -1 || humidity == -1 ||
            strcmp(reporttime, "N/A") == 0) {
          printf("Parse failed: ");
          if (strstr((char *)serial2_rxbuf, "\"temperature\"") == NULL) {
            printf("'temperature' not found; ");
          }
          if (strstr((char *)serial2_rxbuf, "\"humidity\"") == NULL) {
            printf("'humidity' not found; ");
          }
          if (strstr((char *)serial2_rxbuf, "\"reporttime\"") == NULL) {
            printf("'reporttime' not found");
          }
          printf("\r\n");
        }
      } else {
        printf("Unknown JSON format\r\n");
      }
      rx2_flag = 0;
    }

    // 截断 reporttime 为 hh:mm:ss
    if (strcmp(reporttime, "N/A") == 0 || strlen(reporttime) < 19) {
      strcpy(short_rpt, "N/A");
    } else {
      strncpy(short_rpt, reporttime + 11, 8);
      short_rpt[8] = '\0';
    }

    sprintf(tempstr, "Temp:%d °C", temperature);
    sprintf(humidstr, "Hum:%d %%", humidity);
    OLED_NewFrame();
    OLED_PrintASCIIString(20, 50, datestr, &afont8x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(32, 30, str, &afont16x8, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(0, 0, tempstr, &afont8x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(0, 10, short_rpt, &afont8x6, OLED_COLOR_NORMAL);
    OLED_PrintASCIIString(0, 20, humidstr, &afont8x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(1000); // 每秒检查一次
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
      RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
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
  /* User can add his own implementation to report the HAL error return state
   */
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
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
