#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "act_log.h"
#include "sd_card.h"

#include "FreeRTOS.h"
#include "task.h"

static char *convert(uint32_t u32_num, int base)
{
	static char Rep[] = "0123456789ABCDEF";
	static char buffer[11];
	char *ptr;
	ptr = &buffer[10];
	*ptr = '\0';
	do
	{
		*--ptr = Rep[u32_num%base];
		u32_num /= base;
	}while(u32_num != 0);
	return (ptr);
}
bool b_write_action_log(char * c_log_message, ...)
{
	T_DATETIME t_time;
  v_rtc_read_time(&t_time);
	
	char pcfilename[15];
	sprintf(pcfilename, "AL%d%d%d.txt", t_time.u8_date, t_time.u8_month, t_time.u16_year%100);
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(pcfilename, WRITE_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		return false;
	}
	sprintf(pcfilename, "%d:%d:%d  ", t_time.u8_hour, t_time.u8_minute, t_time.u8_second);
	if(i16_sd_write_bytes(pcfilename, strlen(pcfilename)) != SD_OK)
	{
		i16_sd_close_file();
		i16_sd_give_semaphore();
		return false;
	}
	
	//convert data
	char *traverse;
	int32_t i;
	char *s;
	//module 1: Initalizing argument
	va_list arg;
	va_start(arg, c_log_message);
	for(traverse = c_log_message; *traverse != '\0'; traverse++)
	{
		while(*traverse != '%')
		{
			if(*traverse == '\0')
				break;
			if(i16_sd_write_bytes(traverse, 1) != SD_OK)
			{
				i16_sd_close_file();
				i16_sd_give_semaphore();
				return false;
			}
			traverse++;
		}
		if(*traverse == '\0')
				break;
		traverse++;
		//module 2: Fetching and executing argunemts
		switch(*traverse)
		{
			case 'c': break;
			case 'd':
			{
				i = va_arg(arg, int);
				if(i < 0)
				{
					i = -i;
					//write '-' to sd card
				}
				char *ptr = convert(i, 10);
				if(i16_sd_write_bytes(ptr, strlen(ptr)) != SD_OK)
				{
					i16_sd_close_file();
					i16_sd_give_semaphore();
					return false;
				}
			}
			break;
			case 's':
			{
				s = va_arg(arg, char *);
				if(i16_sd_write_bytes(s, strlen(s)) != SD_OK)
				{
					i16_sd_close_file();
					i16_sd_give_semaphore();
					return false;
				}
			}
			default: break;
		}
	}
	//module 3: closing argument list to necessary clean-up
	va_end(arg);
//	if(i16_sd_write_bytes(c_log_message, strlen(c_log_message)) != SD_OK)
//	{
//		i16_sd_close_file();
//		i16_sd_give_semaphore();
//		return false;
//	}
	i16_sd_close_file();
	i16_sd_give_semaphore();
	return true;
}
bool b_delete_action_file(T_DATETIME t_date)
{
	char pcfilename[15];
	sprintf(pcfilename, "AL%d%d%d.txt", t_date.u8_date, t_date.u8_month, t_date.u16_year%100);
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(pcfilename, READ_SD) != SD_OK)				//file is not exist
	{
		i16_sd_give_semaphore();
		return false;
	}
	i16_sd_close_file();
	sd_remove_storage_file(pcfilename);
	i16_sd_give_semaphore();
	return true;
}
