/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file wdt.h
 * @author Danh Pham
 * @date 12 Nov 2020
 * @version 2.0.0
 * @brief Header file of watchdog timer module.
 */
 #include <stdbool.h>
 
 #ifndef _WDT_H_
 #define _WDT_H_

/*!
* @define WDT_MAX_TASK_NAME		20
* Maximum length of task name
*/
#define WDT_MAX_TASK_NAME	(20)
/*!
*	@def WDT_RELOAD_TIME_S	(30)
*	This value define how long the wdt wait to next reload time.
*/
#define WDT_RELOAD_TIME_S								(30)
/*!
*	@def WATCHDOG_TASK_MAX_COUNTING				(90)
*	The value load to timeout counting of each task when the running flag of 
* task is set. After this time, if the task don't run, the MCU will be reset.
*/
#define WDT_TASK_MAX_COUNTING			(90)
/*!
* @struct STRU_WDT 
* Struct contains information of a task
*/
typedef struct
{
	uint8_t u8_task_id; /**<id of task after registered successfulfy */
	uint32_t u32_timeout_counter; /**<when a task was registered, 
			this value started to count down from WATCHDOG_TASK_MAX_COUNTING */
	char c_task_name[WDT_MAX_TASK_NAME]; /**<name of registered task, 
			name must be unique and less than WDT_MAX_TASK_NAME characters */
}STRU_WDT;
/*!
*	Extern functions
*/
extern bool b_wdt_reg_new_task(char* pc_task_name, uint8_t* u8_task_id);
extern bool b_wdt_unreg_task(uint8_t u8_task_id);
extern bool b_wdt_task_reload_counter(uint8_t u8_task_id);
extern void v_wdt_task_init(void);
void v_watchdog_reload_counter(void);
#endif /* _WDT_H_ */

