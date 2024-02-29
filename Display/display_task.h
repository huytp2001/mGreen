/*! @file display_task.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _DISPLAY_TASK_H_
#define _DISPLAY_TASK_H_
#ifdef __cplusplus
extern ?C? {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
/*!
* @data types, constants and macro defintions
*/	
/* TODO: do we need large stack size 128x16 = 2KB??*/
#define DISPLAY_TASK_STACK_SIZE												(configMINIMAL_STACK_SIZE * 16)
#define DISPLAY_TASK_PRIORITY													(tskIDLE_PRIORITY + 1)
#define DISPLAY_TASK_DELAY														3000	//3s

typedef struct
{
	char cname[20];
	uint8_t u8_state;
	uint16_t u16_pH_set;
	uint16_t u16_pH_current;
}DISPLAY_INFO_STRUCT;

/*!
* @public functions prototype
*/	

void v_display_task_init (void);
void v_display_pH_2_LCD(uint16_t u16_pH_set, uint16_t u16_pH_current);
void v_display_waiting_LCD(void);
void v_display_no_schedule(void);
void v_display_active(void);
void v_display_active_zone(char *cname);
#ifdef __cplusplus
}
#endif

#endif /* _DISPLAY_TASK_H_ */
