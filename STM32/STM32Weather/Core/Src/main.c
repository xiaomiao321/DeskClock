#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "usart.h"

#include "oled.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SystemClock_Config(void);

int main(void) {
  uint8_t weather_data[128] = {
      0};                        // 增加缓冲区大小以接收完整JSON，128字节应足够
  uint8_t temp_buffer[10] = {0}; // 临时缓冲区存储温度和湿度值
  uint16_t data_len = 0;
  uint8_t OLED_temperature_buffer[10];
  uint8_t OLED_humidity_buffer[10];

      HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  HAL_Delay(20);
  OLED_Init();

  while (1) {
    // 接收数据，直到遇到换行符或缓冲区满
    HAL_UART_Receive(&huart1, &weather_data[data_len], 1, HAL_MAX_DELAY);
    if (weather_data[data_len] == '\n' ||
        data_len >= sizeof(weather_data) - 1) {
      weather_data[data_len] = '\0'; // 结束字符串

      // 解析 temperature
      char *temp_start = strstr((char *)weather_data, "\"temperature\":\"");
      if (temp_start) {
        temp_start += 13; // 跳过 "\"temperature\":\""
        char *temp_end = strchr(temp_start, '\"');
        if (temp_end) {
          uint8_t temp_len = temp_end - temp_start;
          if (temp_len < sizeof(temp_buffer)) {
            strncpy((char *)temp_buffer, temp_start, temp_len);
            temp_buffer[temp_len] = '\0';
            sprintf(OLED_temperature_buffer, "Temp: %sC", temp_buffer);

            OLED_PrintASCIIString(0, 0, OLED_temperature_buffer, &afont12x6,
                                  OLED_COLOR_NORMAL);
          }
        }
      }

      // 解析 humidity
      char *hum_start = strstr((char *)weather_data, "\"humidity\":\"");
      if (hum_start) {
        hum_start += 11; // 跳过 "\"humidity\":\""
        char *hum_end = strchr(hum_start, '\"');
        if (hum_end) {
          uint8_t hum_len = hum_end - hum_start;
          if (hum_len < sizeof(temp_buffer)) {
            strncpy((char *)temp_buffer, hum_start, hum_len);
            temp_buffer[hum_len] = '\0';
            sprintf(OLED_humidity_buffer, "Humid:%sC", temp_buffer);
            OLED_PrintASCIIString(0, 20, OLED_humidity_buffer, &afont12x6,
                                  OLED_COLOR_NORMAL);
          }
        }
      }

      // 清空缓冲区，准备下次接收
      data_len = 0;
      memset(weather_data, 0, sizeof(weather_data));
    } else {
      data_len++;
    }
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
