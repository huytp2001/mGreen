/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file wdt.c
 * @author Danh Pham
 * @date 12 Nov 2020
 * @version 2.0.0
 * @brief This file contains functions to work with watchdog timer.
 * 
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <string.h>
 #include "wdt.h"
 #include "driverlib.h"
 #include "act_log.h"
 #include "config.h"
 #include "HAL_BSP.h"
 
 #include "FreeRTOS.h"
 #include "task.h"

/*!
* @def WDT_MAX_TASK_MANAGE		(50)
* Maximum task to manage by watchdog timer
*/
#define WDT_MAX_TASK_MANAGE		(50)

/*!
* @def WDT_FEDING_PERIOD		(SYSTEM_CLOCK_GET() * WDT_RELOAD_TIME_S)
* Period value to feed to watchdog timer
*/
#define  WDT_FEDING_PERIOD                  (SYSTEM_CLOCK_GET() * WDT_RELOAD_TIME_S)

/*!
* @def WATCHDOG_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 1)
*/
#define WATCHDOG_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 1)
/*!
* @def WATCHDOG_TASK_PRIORITY          (tskIDLE_PRIORITY + 1)
*/
#define WATCHDOG_TASK_PRIORITY          (tskIDLE_PRIORITY + 1)
/*!
* @def WATCHDOG_TASK_DELAY             (portTickType)(1000 / portTICK_RATE_MS)
*/
#define WATCHDOG_TASK_DELAY             (portTickType)(1000 / portTICK_RATE_MS)

/*!
*	Variables declare
*/
static StaticTask_t xWdg_TaskBuffer;	/**< Buffer of wdt task */
static StackType_t  xWdg_Stack[WATCHDOG_TASK_STACK_SIZE];	/**< Stack of wdt task */

static STRU_WDT stru_wdt_table[WDT_MAX_TASK_MANAGE]; /**<table contains managed task */
/*!
*	Private functions prototype
*/
static void v_wdt_table_clear(void);
static void v_wdt_task (void *pvParameters);
static void v_wdt_update_status(bool b_is_ok);
/*!
*	Public functions
*/
/*!
*	@fn b_wdt_reg_new_task(char* pc_task_name, uint8_t* u8_task_id)
* @brief Register new task to be managed by watchdog timer
* @param[in] pc_task_name Name of task, name must be less than 20 characters and unique
* @param[out] u8_task_id task id which watchdog allocated
* @return True: registered successfully
*					False: can not register: duplicate name or table was full
*/
bool b_wdt_reg_new_task(char* pc_task_name, uint8_t* u8_task_id)
{
	bool b_result = true;
	/* Step1: scan over available events to find the duplicate name */
	for(uint8_t i = 0; i < WDT_MAX_TASK_MANAGE; i++)
	{
		if(0 != stru_wdt_table[i].u8_task_id)	
		{
			if(0 == strcmp(pc_task_name, stru_wdt_table[i].c_task_name))	//both strings are equal
			{
				b_result = false;
				*u8_task_id = stru_wdt_table[i].u8_task_id;
				break;
			}
		}
	}
	/* Step2: get new id for task */
	if(true == b_result)
	{
		b_result = false;
		for (uint8_t i = 0; i < WDT_MAX_TASK_MANAGE; i++)
		{
			if(0 == stru_wdt_table[i].u8_task_id)
			{
				stru_wdt_table[i].u8_task_id = (i + 1);
				strcpy(stru_wdt_table[i].c_task_name, pc_task_name);
				*u8_task_id = (i + 1);
				b_result = true;
				break;
			}
		}
	}
	return b_result;
}


/*!
*	@fn b_wdt_unreg_task(uint8_t u8_task_id)
* @brief Unregister a task from task list.
* @param[in] u8_task_id id of task
* @return True: unregistered successfully
*					False: the task is not exist
*/
bool b_wdt_unreg_task(uint8_t u8_task_id)
{
	bool b_result = false;
	for(uint8_t i = 0; i < WDT_MAX_TASK_MANAGE; i++)
	{
		if(stru_wdt_table[i].u8_task_id == u8_task_id)
		{
			stru_wdt_table[i].u8_task_id = 0;
			memset(stru_wdt_table[i].c_task_name, 0, WDT_MAX_TASK_NAME);
			b_result = true;
			break;
		}
	}
	return b_result;
}

/*!
*	@fn b_wdt_task_reload_counter(uint8_t u8_task_id)
* @brief Reload timeout counter of task in wdt table
* @param[in] u8_task_id id of task
* @return True: reload successfully
*					False: the task is not exist
*/
bool b_wdt_task_reload_counter(uint8_t u8_task_id)
{
	bool b_result = false;
	for(uint8_t i = 0; i < WDT_MAX_TASK_MANAGE; i++)
	{
		if(stru_wdt_table[i].u8_task_id == u8_task_id)
		{
			stru_wdt_table[i].u32_timeout_counter = WDT_TASK_MAX_COUNTING;
			b_result = true;
		}
	}
	return b_result;
}

void v_watchdog_reload_counter(void)
{
    taskENTER_CRITICAL();
    ROM_WatchdogReloadSet(WATCHDOG0_BASE, WDT_FEDING_PERIOD);
    taskEXIT_CRITICAL();
}
/*!
*	@fn v_wdt_task_init(void)
* @brief Init watchdog timer task.
* @return None
*/
void v_wdt_task_init(void)
{
	//Hardware init
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
	ROM_WatchdogReloadSet(WATCHDOG0_BASE, (uint32_t)SYSTEM_CLOCK_GET() * WDT_RELOAD_TIME_S);
	ROM_WatchdogResetEnable(WATCHDOG0_BASE);
	ROM_WatchdogEnable(WATCHDOG0_BASE);
	
	//Task init
	xTaskCreateStatic(v_wdt_task, "WATCHDOG TASK", 
				WATCHDOG_TASK_STACK_SIZE, NULL, WATCHDOG_TASK_PRIORITY,
				xWdg_Stack, &xWdg_TaskBuffer);
}
/*!
*	Private functions
*/
/*!
*	@fn v_wdt_table_clear(void)
* @brief Clear entire task table, call in the starting of watchdog task
* @return None
*/
static void v_wdt_table_clear(void)
{
	memset(stru_wdt_table, 0, WDT_MAX_TASK_MANAGE * sizeof(STRU_WDT));
}

/*!
*	@fn v_wdt_task (void *pvParameters)
* @brief main watchdog timer task
* @param None
* @return None
*/
static void v_wdt_task (void *pvParameters)
{
	static bool b_is_dbled_on = false;
	v_wdt_table_clear();
	for( ; ; )
	{
		static bool b_is_ok = true;
		for(uint8_t i = 0; i < WDT_MAX_TASK_MANAGE; i++)
		{
			if(stru_wdt_table[i].u8_task_id != 0)
			{
				if(0 == stru_wdt_table[i].u32_timeout_counter)		//timeout
				{
					b_is_ok = false;
					#if WRITE_ACTION_LOG			
            /* Write task id to sdcard */
            b_write_action_log("Task error watchdog: %s\r\n", stru_wdt_table[i].c_task_name);
					#endif /* WRITE_ACTION_LOG */
					break;
				}
				else
				{
					stru_wdt_table[i].u32_timeout_counter--;
				}
			}
		}
		/* Led blinky */
	#if LED_BLINKY_ENABLE
		if(b_is_dbled_on)
		{
			b_is_dbled_on = false;
			LED_STT_OFF();
		}
		else
		{
			b_is_dbled_on = true;
			LED_STT_ON();
		}
	#endif
		/* Update to watchdog */
		v_wdt_update_status(b_is_ok);    
		vTaskDelay(WATCHDOG_TASK_DELAY);
	}
}

/*!
*	@fn v_wdt_update_status(bool b_is_ok)
* @brief check status of task, if all tasks run properly, reload watchdog timer
* @param b_is_ok status of tasks
* @return None
*/
static void v_wdt_update_status(bool b_is_ok)
{
	if(b_is_ok)
	{
		/* Reload IWDG counter */
    ROM_WatchdogReloadSet(WATCHDOG0_BASE, WDT_FEDING_PERIOD);
	}
}
