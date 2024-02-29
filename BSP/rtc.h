
#ifndef RTC_H_
#define RTC_H_

#include <stdint.h>
#include <stdbool.h> 

typedef struct
{
	uint8_t u8_second;
	uint8_t u8_minute;
	uint8_t u8_hour;
	uint8_t u8_date;
	uint8_t u8_month;
	uint16_t u16_year;
} T_DATETIME;

typedef struct
{
  uint8_t Month;
  uint8_t Date;
  uint16_t Year;
  uint8_t Hour;
  uint8_t Min;
  uint8_t Sec;
  uint8_t TimeZone;
  uint8_t DayLight;
}Time_TypeDef;

extern bool b_set_time_zone(uint8_t u8_hour, uint8_t u8_minute);
extern uint16_t u16_get_time_zone(void);
extern void v_rtc_init(void);
extern void v_rtc_read_time(T_DATETIME* pdt_time);
extern bool b_rtc_set_time(T_DATETIME* t_time);
extern bool b_rtc_set_unixtime(uint32_t u32_unixtime);
extern void v_long_2_timestamp_string(uint32_t DateTime, char *string);
extern uint32_t u32_datetime_2_long(T_DATETIME* CurrentTime);
extern void v_long_2_datetime(T_DATETIME* Time, uint32_t u32_data);
extern uint32_t u32_timestamp_string_2_long(char *string);
extern uint8_t u8_week_day_get(T_DATETIME* t_time);
extern uint32_t u32_rtc_unix_time_get(void);
#endif
