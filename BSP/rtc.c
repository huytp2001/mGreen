#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driverlib.h"
#include "rtc.h"

static const uint8_t MONTH_STR[12][4] = 
{
	{"Jan\0"},
	{"Feb\0"},
	{"Mar\0"},
	{"Apr\0"},
	{"May\0"},
	{"Jun\0"},
	{"Jul\0"},
	{"Aug\0"},
	{"Sep\0"},
	{"Oct\0"},
	{"Nov\0"},
	{"Dec\0"}
};
static uint16_t u16_time_zone = (7*60 + 0); /**<Timezone in minute */

bool b_set_time_zone(uint8_t u8_hour, uint8_t u8_minute)
{
	if(u8_hour < 24 && u8_minute < 60)
	{
		u16_time_zone = u8_hour * 60 + u8_minute;
	}
	return true;
}
uint16_t u16_get_time_zone(void)
{
	return u16_time_zone;
}


void v_rtc_init(void)
{
	uint32_t ui32Status, ui32HibernateCount;
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);

	if(HibernateIsActive())
	{
		//
		// Read the status bits to see what caused the wake.  Clear the wake
		// source so that the device can be put into hibernation again.
		//
		ui32Status = HibernateIntStatus(0);
		HibernateIntClear(ui32Status);
	}

	HibernateDataGet(&ui32HibernateCount, 1);
		
	HibernateEnableExpClk(SYSTEM_CLOCK_GET());

#ifdef RTC_USE_INTERNAL_XTAL
	HibernateClockConfig(HIBERNATE_OSC_LFIOSC);
#else
	HibernateClockConfig(HIBERNATE_OSC_LOWDRIVE);
#endif

	HibernateRTCEnable();
}

void v_rtc_read_time(T_DATETIME* pdt_time)
{
	uint32_t u32_time = HibernateRTCGet();
  v_long_2_datetime (pdt_time, u32_time);
}

bool b_rtc_set_time(T_DATETIME* t_time)
{
	uint32_t u32_time;
	
	u32_time = u32_datetime_2_long(t_time);
	HibernateRTCSet(u32_time);
	
	return true;
}

bool b_rtc_set_unixtime(uint32_t u32_unixtime)
{
	HibernateRTCSet(u32_unixtime);
	
	return true;
}

void v_long_2_timestamp_string(uint32_t DateTime, char *string)
{
  static const uint8_t MONTH[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  static const uint8_t MONTH_EX[12]={31,29,31,30,31,30,31,31,30,31,30,31};
	
  Time_TypeDef Time; 
  uint32_t temp1=0,temp2=0;
  uint32_t sum=0;
  uint8_t i=0;

  Time.Sec=DateTime%60;
  Time.Min=(DateTime/60)%60;
  Time.Hour=(DateTime/3600)%24;
  
  temp1=(DateTime/((3600*24*(365*3+366))));
  temp1*=4;
  temp1+=1970;
  temp2=(DateTime%((3600*24*(365*3+366))))/(24*3600);
  if(temp2<365)
  {
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH[i];
      if(temp2<sum)
      {
        sum-=MONTH[i];
        break;
      }
    }
		Time.Date=temp2-sum+1;
    Time.Month=i+1;
  }
  else if((temp2>=365)&&(temp2<(365*2)))
  {
    temp2-=365;
    temp1++;
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH[i];
      if(temp2<sum)
      {
        sum-=MONTH[i];
        break;
      }
    }
		Time.Date=temp2-sum+1;
    Time.Month=i+1;
  }
  else if(temp2>=(365*2)&&(temp2<(365*2+366)))
  {
    temp2-=(365*2);
    temp1+=2;
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH_EX[i];
      if(temp2<sum)
      {
        sum-=MONTH_EX[i];
        break;
      }
    }
		Time.Date=temp2-sum+1;
    Time.Month=i+1;
  }
  else
  {
    temp2-=(365*2+366);
    temp1+=3;
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH[i];
      if(temp2<sum)
      {
        sum-=MONTH[i];
        break;
      }
    }
		Time.Date=temp2-sum+1;
    Time.Month=i+1;
  };
  
  Time.Year=temp1;
	
	sprintf(string, "%02d %s %d %02d:%02d:%02d", Time.Date, MONTH_STR[Time.Month-1], Time.Year, Time.Hour, Time.Min, Time.Sec);
}

uint32_t u32_datetime_2_long(T_DATETIME* CurrentTime)
{
	static const uint8_t MONTH[12]={31,28,31,30,31,30,31,31,30,31,30,31};
	static const uint8_t MONTH_EX[12]={31,29,31,30,31,30,31,31,30,31,30,31};
	
	uint32_t ui32Time=0;

	uint32_t tmp1=0, tmp2=0, tmp3=0;
	
	if (CurrentTime->u16_year < 1970)
	{
		CurrentTime->u16_year = 1970;
	}
	
	if (!((CurrentTime->u8_month >= 1) && (CurrentTime->u8_month <= 12)))
	{
		CurrentTime->u8_month = 1; 
	}

	if (!((CurrentTime->u8_date >= 1) && (CurrentTime->u8_date <= 31)))
	{
		CurrentTime->u8_date = 1; 
	}

	if (!(CurrentTime->u8_hour <= 23))
	{
		CurrentTime->u8_hour = 0; 
	}

	if (!(CurrentTime->u8_minute <= 59))
	{
		CurrentTime->u8_minute = 0; 
	}

	if (!(CurrentTime->u8_second <= 59))
	{
		CurrentTime->u8_second = 0; 
	}
	
	tmp1 = CurrentTime->u16_year - 1970;
	tmp2 = (tmp1 / 4) * (365 * 3 + 366) * 24 * 3600;
	tmp1 %= 4;
	switch (tmp1)
	{	
		case 0:
			ui32Time = tmp2;
		break;
		case 1:
			ui32Time = tmp2 + 365 * 24 * 3600;
		break;
		case 2:
			ui32Time = tmp2 + 2 * 365 * 24 * 3600;
		break;
		case 3:
			ui32Time = tmp2 + (365 * 2 + 366) * 24 * 3600;
		break;
		default:
		break;
	}
	
	if (tmp1 == 2)
	{
		tmp3=0;
    for (tmp2 = 0; tmp2 < (CurrentTime->u8_month - 1); tmp2++)
		{
			tmp3 += MONTH_EX[tmp2];
		}
	}
	else
	{
		tmp3=0;
    for (tmp2 = 0; tmp2 < (CurrentTime->u8_month - 1); tmp2++)
		{
			tmp3 += MONTH[tmp2];
		}		
	}
	
	ui32Time += (((24 * (tmp3 + CurrentTime->u8_date - 1)) +  CurrentTime->u8_hour) * 3600) + (CurrentTime->u8_minute * 60) + CurrentTime->u8_second;
	
	return ui32Time;
}

/**
  * @brief : Convert long to date time
  * @param :   
  * @retval : 
  */
void v_long_2_datetime(T_DATETIME* Time, uint32_t u32_data)
{
  static const uint8_t MONTH[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  static const uint8_t MONTH_EX[12]={31,29,31,30,31,30,31,31,30,31,30,31};
  uint32_t temp1=0,temp2=0;
  uint32_t sum=0;
  uint8_t i=0;

  Time->u8_second=u32_data%60;
  Time->u8_minute=(u32_data/60)%60;
  Time->u8_hour=(u32_data/3600)%24;
  
  temp1=(u32_data/((3600*24*(365*3+366))));
  temp1*=4;
  temp1+=1970;
  temp2=(u32_data%((3600*24*(365*3+366))))/(24*3600);
  if(temp2<365)
  {
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH[i];
      if(temp2<sum)
      {
        sum-=MONTH[i];
        Time->u8_date=temp2-sum+1;
        break;
      }
    }
    Time->u8_month=i+1;
  }
  else if((temp2>=365)&&(temp2<(365*2)))
  {
    temp2-=365;
    temp1++;
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH[i];
      if(temp2<sum)
      {
        sum-=MONTH[i];
        Time->u8_date=temp2-sum+1;
        break;
      }
    }
    Time->u8_month=i+1;
  }
  else if(temp2>=(365*2)&&(temp2<(365*2+366)))
  {
    temp2-=(365*2);
    temp1+=2;
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH_EX[i];
      if(temp2<sum)
      {
        sum-=MONTH_EX[i];
        Time->u8_date=temp2-sum+1;
        break;
      }
    }
    Time->u8_month=i+1;
  }
  else
  {
    temp2-=(365*2+366);
    temp1+=3;
    sum=0;
    for(i=0;i<12;i++)
    {
      sum+=MONTH[i];
      if(temp2<sum)
      {
        sum-=MONTH[i];
        Time->u8_date=temp2-sum+1;
        break;
      }
    }
    Time->u8_month=i+1;
  };
  
  Time->u16_year=temp1;
}

uint32_t u32_timestamp_string_2_long(char *string)
{
	int i, str_len, temp;
	T_DATETIME time;
	uint32_t u32_time;
	char *p_Substr;
	
	str_len = strlen(string);
	if (str_len > 25)
		str_len = 25;
	
	for (i = 0; i < str_len; i++)	//String's length can't larger than 21
	{
		if ((string[i] >= '0') && (string[i] <= '9'))		//Search for first digit of date
		{
			break;
		}
	}
	
	if (i == str_len)	//error
	{
		return 0;
	}
	
	temp = atoi(&string[i]);
	time.u8_date = temp;
	
	for (i = 0; i < 12; i++)
	{
		if ((p_Substr = strstr(string, (char *)MONTH_STR[i])) != NULL)
		{
			time.u8_month = i + 1;
			break;
		}
	}
	
	if (i == 12)
		return 0;
	
	str_len = strlen(p_Substr);
	if (str_len > 25)
		str_len = 25;
	
	for (i = 0; i < str_len; i++)	//String's length can't larger than 21
	{
		if ((p_Substr[i] >= '0') && (p_Substr[i] <= '9'))		//Search for first digit of date
		{
			break;
		}
	}
	
	if (i == str_len)	//error
	{
		return 0;
	}
	
	temp = atoi(&p_Substr[i]);
	time.u16_year = temp;
	
	p_Substr += i;
	
	if ((p_Substr = strstr(p_Substr, " ")) != NULL)
	{
		p_Substr++;
		temp = atoi(p_Substr);
		time.u8_hour = temp;
		
		if ((p_Substr = strstr(p_Substr, ":")) != NULL)
		{
			p_Substr++;
			temp = atoi(p_Substr);
			time.u8_minute = temp;
			
			if ((p_Substr = strstr(p_Substr, ":")) != NULL)
			{
				p_Substr++;
				temp = atoi(p_Substr);
				time.u8_second = temp;
			}
			else
				return 0;
		}
		else return 0;
	}
	else
		return 0;
	
	u32_time = u32_datetime_2_long(&time);
	
	return u32_time;
}

/*
% Return day of week from timestamp in unixtime
% 0: Sunday
% 1: Monday
% ....
& 6: Saturday
*/
uint8_t u8_week_day_get(T_DATETIME* t_time)
{
	uint32_t u32_timestamp = u32_datetime_2_long(t_time);
	return (uint32_t)(floor(u32_timestamp / 86400) + 4) % 7;
}

uint32_t u32_rtc_unix_time_get(void)
{
	T_DATETIME t_time;
	uint32_t u32_unix_time = 0;
	v_rtc_read_time(&t_time);
	u32_unix_time = u32_datetime_2_long(&t_time) - u16_time_zone * 60;
	return u32_unix_time;
	
}
