#include "myrtc.h"

// RTC已经被初始化的值 记录在RTC_BKP_DR1中
#define RTC_INIT_FLAG 0x2333

/**
 * @brief  进入RTC初始化模式
 * @param  hrtc  指向包含RTC配置信息的RTC_HandleTypeDef结构体的指针
 * @retval HAL status
 */
static HAL_StatusTypeDef RTC_EnterInitMode(RTC_HandleTypeDef *hrtc) {
  uint32_t tickstart = 0U;

  tickstart = HAL_GetTick();
  /* 等待RTC处于INIT状态，如果到达Time out 则退出 */
  while ((hrtc->Instance->CRL & RTC_CRL_RTOFF) == (uint32_t)RESET) {
    if ((HAL_GetTick() - tickstart) > RTC_TIMEOUT_VALUE) {
      return HAL_TIMEOUT;
    }
  }

  /* 禁用RTC寄存器的写保护 */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

  return HAL_OK;
}

/**
 * @brief  退出RTC初始化模式
 * @param  hrtc   指向包含RTC配置信息的RTC_HandleTypeDef结构体的指针
 * @retval HAL status
 */
static HAL_StatusTypeDef RTC_ExitInitMode(RTC_HandleTypeDef *hrtc) {
  uint32_t tickstart = 0U;

  /* 禁用RTC寄存器的写保护。 */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  tickstart = HAL_GetTick();
  /* 等到RTC处于INIT状态，如果到达Time out 则退出 */
  while ((hrtc->Instance->CRL & RTC_CRL_RTOFF) == (uint32_t)RESET) {
    if ((HAL_GetTick() - tickstart) > RTC_TIMEOUT_VALUE) {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/**
 * @brief  写入RTC_CNT寄存器中的时间计数器。
 * @param  hrtc   指向包含RTC配置信息的RTC_HandleTypeDef结构体的指针。
 * @param  TimeCounter: 写入RTC_CNT寄存器的计数器
 * @retval HAL status
 */
static HAL_StatusTypeDef RTC_WriteTimeCounter(RTC_HandleTypeDef *hrtc,
                                              uint32_t TimeCounter) {
  HAL_StatusTypeDef status = HAL_OK;

  /* 进入RTC初始化模式 */
  if (RTC_EnterInitMode(hrtc) != HAL_OK) {
    status = HAL_ERROR;
  } else {
    /* 设置RTC计数器高位寄存器 */
    WRITE_REG(hrtc->Instance->CNTH, (TimeCounter >> 16U));
    /* 设置RTC计数器低位寄存器 */
    WRITE_REG(hrtc->Instance->CNTL, (TimeCounter & RTC_CNTL_RTC_CNT));

    /* 退出RTC初始化模式 */
    if (RTC_ExitInitMode(hrtc) != HAL_OK) {
      status = HAL_ERROR;
    }
  }

  return status;
}

/**
 * @brief  读取RTC_CNT寄存器中的时间计数器。
 * @param  hrtc   指向包含RTC配置信息的RTC_HandleTypeDef结构体的指针。
 * @retval 时间计数器
 */
static uint32_t RTC_ReadTimeCounter(RTC_HandleTypeDef *hrtc) {
  uint16_t high1 = 0U, high2 = 0U, low = 0U;
  uint32_t timecounter = 0U;

  high1 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);
  low = READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT);
  high2 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);

  if (high1 != high2) {
    /* 当读取CNTL和CNTH寄存器期间计数器溢出时,
     * 重新读取CNTL寄存器然后返回计数器值 */
    timecounter = (((uint32_t)high2 << 16U) |
                   READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT));
  } else {
    /* 当读取CNTL和CNTH寄存器期间没有计数器溢出,
     * 计数器值等于第一次读取的CNTL和CNTH值 */
    timecounter = (((uint32_t)high1 << 16U) | low);
  }

  return timecounter;
}

/**
 * @brief 设置RTC时间
 * @param time 时间
 * @retval HAL status
 */
HAL_StatusTypeDef KK_RTC_SetTime(struct tm *time) {
  uint32_t unixTime = mktime(time);
  return RTC_WriteTimeCounter(&hrtc, unixTime);
}

/**
 * @brief 获取RTC时间
 * @retval 时间
 */
struct tm *KK_RTC_GetTime(void) {
  time_t unixTime = RTC_ReadTimeCounter(&hrtc);
  return gmtime(&unixTime);
}

void KK_RTC_Init(void) {
  uint32_t initFlag = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
  if (initFlag == RTC_INIT_FLAG)
    return;
  if (HAL_RTC_Init(&hrtc) != HAL_OK) {
    Error_Handler();
  }
  struct tm time = {
      .tm_year = 2025 - 1900,
      .tm_mon = 7 - 1,
      .tm_mday = 21,
      .tm_hour = 15,
      .tm_min = 45,
      .tm_sec = 30,
  };
  KK_RTC_SetTime(&time);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_INIT_FLAG);
}

/*下面是网友的*/

/*
获取RTC计数器CNT里面的值（既时间戳）
*/
uint32_t RTC_Get_Sec(void) {
  uint16_t seccount_high1 = 0;
  uint16_t seccount_high2 = 0;
  uint16_t seccount_low = 0;
  uint32_t seccount = 0;
  /*摘抄于STM32 rtc库，优化可能在读CNT的时候出现了溢出*/
  seccount_high1 = RTC->CNTH; /* 得到计数器中的值高16位(秒钟数) */
  seccount_low = RTC->CNTL;   /* 得到计数器中的值低16位(秒钟数) */
  seccount_high2 = RTC->CNTH; /* 得到计数器中的值高16位(秒钟数) */
  if (seccount_high1 !=
      seccount_high2) /*防止在获取秒钟低16位时，计数器刚好溢出导致获取的秒钟高16位就是错误的*/
  {
    seccount = (seccount_high2 << 16) | (RTC->CNTL & 0xFFFF);
  } else {
    seccount = (seccount_high1 << 16) | seccount_low;
  }
  return seccount;
}

/**
 * @brief       判断年份是否是闰年
 *   @note      月份天数表:
 *              月份   1  2  3  4  5  6  7  8  9  10 11 12
 *              闰年   31 29 31 30 31 30 31 31 30 31 30 31
 *              非闰年 31 28 31 30 31 30 31 31 30 31 30 31
 * @param       year : 年份
 * @retval      0, 非闰年; 1, 是闰年;
 */
static uint8_t RTC_Is_Leap_Year(uint16_t year) {
  /* 闰年规则: 四年闰百年不闰，四百年又闰 */
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief       输入公历日期得到星期
 *   @note      起始时间为: 公元0年3月1日开始, 输入往后的任何日期,
 * 都可以获取正确的星期 使用 基姆拉尔森计算公式 计算, 原理说明见此贴:
 *              https://www.cnblogs.com/fengbohello/p/3264300.html
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @retval      0, 星期天; 1 ~ 6: 星期一 ~ 星期六
 */
uint8_t RTC_GetWeek(uint16_t syear, uint8_t smonth, uint8_t sday) {
  uint8_t week = 0;

  if (smonth < 3) {
    smonth += 12;
    --syear;
  }
  week = (sday + 1 + 2 * smonth + 3 * (smonth + 1) / 5 + syear + (syear >> 2) -
          syear / 100 + syear / 400) %
         7;
  return week;
}

/**
功能：得到当前的时间（年月日 时分秒）。该函数不直接返回时间,
时间数据保存在calendar结构体里面
 */
void RTC_GetTime(RTC_DateTimeTypeDef *calendar) {
  static uint16_t daycnt = 0;
  uint32_t seccount = 0;
  uint32_t temp = 0;
  uint16_t temp1 = 0;
  const uint8_t month_table[12] = {31, 28, 31, 30, 31, 30, 31,
                                   31, 30, 31, 30, 31}; /* 平年的月份日期表 */
  seccount = RTC_Get_Sec();
  calendar->unix_time = seccount;
  temp = seccount / 86400; /* 得到天数(秒钟数对应的) */
  if (daycnt != temp)      /* 超过一天了 */
  {
    daycnt = temp;
    temp1 = 1970; /* 从1970年开始 */
    while (temp >= 365) {
      if (RTC_Is_Leap_Year(temp1)) /* 是闰年 */
      {
        if (temp >= 366) {
          temp -= 366; /* 闰年的秒钟数 */
        } else {
          break;
        }
      } else {
        temp -= 365; /* 平年 */
      }

      temp1++;
    }
    calendar->year = temp1; /* 得到年份 */
    temp1 = 0;
    while (temp >= 28) /* 超过了一个月 */
    {
      if (RTC_Is_Leap_Year(calendar->year) &&
          temp1 == 1) /* 当年是不是闰年/2月份 */
      {
        if (temp >= 29) {
          temp -= 29; /* 闰年的秒钟数 */
        } else {
          break;
        }
      } else {
        if (temp >= month_table[temp1]) {
          temp -= month_table[temp1]; /* 平年 */
        } else {
          break;
        }
      }
      temp1++;
    }
    calendar->month = temp1 + 1; /* 得到月份 */
    calendar->date = temp + 1;   /* 得到日期 */
  }
  temp = seccount % 86400;            /* 得到秒钟数 */
  calendar->hour = temp / 3600;       /* 小时 */
  calendar->min = (temp % 3600) / 60; /* 分钟 */
  calendar->sec = (temp % 3600) % 60; /* 秒钟 */
  calendar->week = RTC_GetWeek(calendar->year, calendar->month,
                               calendar->date); /* 获取星期 */
}

/*下面是网友的设置时间函数，也可以在KK_Init设置*/

/**
 * @brief       将年月日时分秒转换成秒钟数
 *   @note      以1970年1月1日为基准, 1970年1月1日, 0时0分0秒, 表示第0秒钟
 *              最大表示到2105年, 因为uint32_t最大表示136年的秒钟数(不包括闰年)!
 *              本代码参考只linux mktime函数, 原理说明见此贴:
 *              http://www.openedv.com/thread-63389-1-1.html
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @param       hour  : 小时
 * @param       min   : 分钟
 * @param       sec   : 秒钟
 * @retval      转换后的秒钟数
 */
static long RTC_Data2Sec(uint16_t syear, uint8_t smon, uint8_t sday,
                         uint8_t hour, uint8_t min, uint8_t sec) {
  uint32_t Y, M, D, X, T;
  signed char monx = smon; /* 将月份转换成带符号的值, 方便后面运算 */

  if (0 >= (monx -= 2)) /* 1..12 -> 11,12,1..10 */
  {
    monx += 12; /* Puts Feb last since it has leap day */
    syear -= 1;
  }
  Y = (syear - 1) * 365 + syear / 4 - syear / 100 +
      syear / 400; /* 公元元年1到现在的闰年数 */
  M = 367 * monx / 12 - 30 + 59;
  D = sday - 1;
  X = Y + M + D - 719162;                      /* 减去公元元年到1970年的天数 */
  T = ((X * 24 + hour) * 60 + min) * 60 + sec; /* 总秒钟数 */
  return T;
}

/**
 * @brief       设置时间, 包括年月日时分秒
 *   @note      以1970年1月1日为基准, 往后累加时间
 *              合法年份范围为: 1970 ~ 2105年
                HAL默认为年份起点为2000年
 * @param       syear : 年份
 * @param       smon  : 月份
 * @param       sday  : 日期
 * @param       hour  : 小时
 * @param       min   : 分钟
 * @param       sec   : 秒钟
 * @retval      0, 成功; 1, 失败;
 */
void RTC_SetTime(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour,
                 uint8_t min, uint8_t sec) /* 设置时间 */
{
  uint32_t seccount = 0;
  seccount = RTC_Data2Sec(syear, smon, sday, hour, min,
                          sec); /* 将年月日时分秒转换成总秒钟数 */
  __HAL_RCC_PWR_CLK_ENABLE();   /* 使能电源时钟 */
  __HAL_RCC_BKP_CLK_ENABLE();   /* 使能备份域时钟 */
  HAL_PWR_EnableBkUpAccess();   /* 取消备份域写保护 */
  /* 上面三步是必须的! */
  SET_BIT(RTC->CRL, RTC_CRL_CNF); /* 进入配置模式 */
  //    RTC->CRL |= 1 << 4;         /* 进入配置模式 */
  RTC->CNTL = seccount & 0xFFFF;
  RTC->CNTH = seccount >> 16;
  CLEAR_BIT(RTC->CRL, RTC_CRL_CNF); /* 退出配置模式 */
  //    RTC->CRL &= ~(1 << 4);      /* 退出配置模式 */
  while (!__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_RTOFF))
    ; /* 等待RTC寄存器操作完成, 即等待RTOFF == 1 */
}
