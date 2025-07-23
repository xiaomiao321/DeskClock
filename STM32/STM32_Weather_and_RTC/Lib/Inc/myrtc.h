#ifndef INC_KK_RTC_H_
#define INC_KK_RTC_H_
#include "rtc.h"
#include "stm32f1xx_hal.h"
#include "time.h"

HAL_StatusTypeDef KK_RTC_SetTime(struct tm *time);
struct tm *KK_RTC_GetTime(void);
void KK_RTC_Init(void);

/* 时间结构体, 包括年月日周时分秒等信息 */
typedef struct {
  uint16_t year;      /* 年 */
  uint8_t month;      /* 月 */
  uint8_t date;       /* 日 */
  uint8_t week;       /* 周 */
  uint8_t hour;       /* 时 */
  uint8_t min;        /* 分 */
  uint8_t sec;        /* 秒 */
  uint32_t unix_time; /* 时间戳 */
} RTC_DateTimeTypeDef;
void RTC_GetTime(RTC_DateTimeTypeDef *calendar);
void RTC_SetTime(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour,
                 uint8_t min, uint8_t sec);
#endif /* INC_KK_RTC_H_ */
