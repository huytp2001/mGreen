/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file error_manage.c
 * @author Danh Pham
 * @date 16 Nov 2020
 * @version: 1.0.0
 * @brief This file contain functions and task to handle errors of machine.
 * Each process tha has the potential to generate errors needs to be registered
 * to this handler.
 */ 
 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <stdbool.h>
 #include <stdio.h>
 #include <string.h>
 
 #include "error_handler.h"
 #include "wdt.h"
 #include "HardFault_debug.h"
 #include "act_log.h"
 #include "config.h"
 
 #include "FreeRTOS.h"
 #include "task.h"

/*!
* @def ERROR_MAX_HANDLER
* Maximum error task can handle
*/
#define ERROR_MAX_HANDLER			10
/*!
* @def ERROR_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 1)
*/
#define ERROR_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 1)
/*!
* @def ERROR_TASK_PRIORITY          (tskIDLE_PRIORITY + 1)
*/
#define ERROR_TASK_PRIORITY          (tskIDLE_PRIORITY + 1)
/*!
* @def ERROR_TASK_DELAY             (portTickType)(1000 / portTICK_RATE_MS)
*/
#define ERROR_TASK_DELAY             (portTickType)(1000 / portTICK_RATE_MS)

/*!
*	Variables declare
*/
static StaticTask_t xError_TaskBuffer;	/**< Buffer of error handler task */
static StackType_t  xError_Stack[ERROR_TASK_STACK_SIZE];	/**< Stack of error handler task */

static STRU_ERROR astru_error_table[ERROR_MAX_HANDLER]; /**< desciption for u8_demo_var */
static bool b_is_reset = true;

/*!
*	Private functions prototype
*/
static void v_error_table_clear(void);
static void v_error_handler_task (void *pvParameters);

/*!
*	Public functions
*/

/*!
*	@fn b_error_handler_reg(char* pc_error_name , uint32_t u32_timeout,  void (*pc_callback_func), 
													uint8_t *pu8_id)
* @brief Register a new handler in handler list. This function will allocate an id for registered handler.
* @param[in] pc_error_name Name of error handler, name must unique
* @param[in] u32_timeout Maximum counter for handler, when timeout, the callback function will be called
* @param[in] (*pc_callback_func) Callback function
* @param[out] pu8_id Handler id which is allocated
* @return True: registered successfully
*					False: can not register: duplicate name or table was full
*/
bool b_error_handler_reg(char* pc_error_name , uint32_t u32_timeout,  void (*pc_callback_func)(void), 
													uint8_t *pu8_id)
{
	bool b_result = true;
	//Step1: scan over available handler to find the duplicate name
	for(uint8_t i = 0; i < ERROR_MAX_HANDLER; i++)
	{
		if(0 != astru_error_table[i].u8_error_id)	
		{
			if(0 == strcmp(pc_error_name, astru_error_table[i].c_error_name))	//both strings are equal
			{
				b_result = false;
				*pu8_id = astru_error_table[i].u8_error_id;
				break;
			}
		}
	}
	//Step2: get new id for handler, register callback function
	if(true == b_result)
	{
		b_result = false;
		for (uint8_t i = 0; i < ERROR_MAX_HANDLER; i++)
		{
			if(0 == astru_error_table[i].u8_error_id)
			{
				astru_error_table[i].u8_error_id = (i + 1);
				astru_error_table[i].u32_error_timeout = u32_timeout;
				astru_error_table[i].u32_error_counter = 0;
				strcpy(astru_error_table[i].c_error_name, pc_error_name);
				astru_error_table[i].pv_error_func = pc_callback_func;
				*pu8_id = (i + 1);
				b_result = true;
				break;
			}
		}
	}
	return b_result;
}


/*!
*	@fn b_error_unreg(uint8_t u8_error_id)
* @brief Unregister an error handler from handler list.
* This function must be called in callback function to prevent infinity error call.
* @param[in] u8_error_id Id of error handler
* @return True: unregistered successfully
*					False: the handler is not exist
*/
bool b_error_unreg(uint8_t u8_error_id)
{
	bool b_result = false;
	for(uint8_t i = 0; i < ERROR_MAX_HANDLER; i++)
	{
		if(astru_error_table[i].u8_error_id == u8_error_id)
		{
			astru_error_table[i].u8_error_id = 0;
			astru_error_table[i].u32_error_timeout = 0;
			astru_error_table[i].u32_error_counter = 0;
			memset(astru_error_table[i].c_error_name, 0, ERROR_MAX_NAME);
			b_result = true;
			break;
		}
	}
	return b_result;
}

/*!
*	@fn b_error_reset_counter(uint8_t u8_error_id)
* @brief Reset counter of error hanlder in handler table
* @param[in] u8_error_id id of error handler
* @return True: reset successfully
*					False: the handler is not exist
*/
bool b_error_reset_counter(uint8_t u8_error_id)
{
	bool b_result = false;
	for(uint8_t i = 0; i < ERROR_MAX_HANDLER; i++)
	{
		if(astru_error_table[i].u8_error_id == u8_error_id)
		{
			astru_error_table[i].u32_error_counter = 0;
			b_result = true;
		}
	}
	return b_result;
}

/*!
*	@fn b_error_change_timeout(uint8_t u8_error_id, uint32_t u32_timeout)
* @brief Change the maximum value of handler counter.
* The counter will be reset when function is called
* @param[in] u8_error_id id of error handler
* @param[in] u32_timeout New maximum value
* @return True: change successfully
*					False: the handler is not exist
*/
bool b_error_change_timeout(uint8_t u8_error_id, uint32_t u32_timeout)
{
	bool b_result = false;
	for(uint8_t i = 0; i < ERROR_MAX_HANDLER; i++)
	{
		if(astru_error_table[i].u8_error_id == u8_error_id)
		{
			astru_error_table[i].u32_error_timeout = u32_timeout;
			astru_error_table[i].u32_error_counter = 0;
			b_result = true;
		}
	}
	return b_result;
}

/*!
*	@fn v_error_handler_task_init(void)
* @brief Init error handler task.
* @return None
*/
void v_error_handler_task_init(void)
{
	//clear handler table
	v_error_table_clear();
	//Task init
	xTaskCreateStatic(v_error_handler_task, "ERROR_HANDLER TASK", 
				ERROR_TASK_STACK_SIZE, NULL, ERROR_TASK_PRIORITY,
				xError_Stack, &xError_TaskBuffer);
}
/*!
*	Private functions
*/

/*!
*	@fn v_error_table_clear(void)
* @brief Clear entire error handler table, call in the starting of error_handler task
* @return None
*/
static void v_error_table_clear(void)
{
	memset(astru_error_table, 0, ERROR_MAX_HANDLER * sizeof(STRU_ERROR));
}

/*!
*	@fn v_error_handler_task (void *pvParameters)
* @brief main error handler task
* @param None
* @return None
*/
static void v_error_handler_task (void *pvParameters)
{
	static uint8_t u8_error_handler_task_id = 0;
	/* get watchdog timer id */
	while(b_wdt_reg_new_task("error_task", &u8_error_handler_task_id) != true){}
	for( ; ; )
	{
		/* reload watchdog counter */
		b_wdt_task_reload_counter(u8_error_handler_task_id);
		/* Handle errors */
		for(uint8_t i = 0; i < ERROR_MAX_HANDLER; i++)
		{
			if(astru_error_table[i].u8_error_id != 0)
			{
				//id available
				if(astru_error_table[i].u32_error_counter >= astru_error_table[i].u32_error_timeout)
				{
					astru_error_table[i].u32_error_counter = 0;
					//timeout -> call error function
					astru_error_table[i].pv_error_func();
				}
				else
				{
					astru_error_table[i].u32_error_counter++;
				}
			}
		}
		/* Handle reset */
		if(b_is_reset)
		{
			b_is_reset = false;
			/* Check hardfault */
			uint32_t au32_hardfault_param[7];
			memset(au32_hardfault_param, 0, 7 * sizeof(uint32_t));
			v_hardfault_params_get(au32_hardfault_param);
			if(1 == au32_hardfault_param[0])
			{
				#if WRITE_ACTION_LOG
				b_write_action_log("Hard Fault corrupted\r\n");
				b_write_action_log("r1: %d\r\n", au32_hardfault_param[1]);
				b_write_action_log("pc 5: %d\r\n", au32_hardfault_param[2]);
				b_write_action_log("pc 6: %d\r\n", au32_hardfault_param[3]);
				b_write_action_log("Link register: %d\r\n", au32_hardfault_param[4]);
				b_write_action_log("Program counter: %d\r\n", au32_hardfault_param[5]);
				b_write_action_log("Program status register: %d\r\n", au32_hardfault_param[6]);
				#endif /* WRITE_ACTION_LOG */
			}
			else
			{
				#if WRITE_ACTION_LOG
				b_write_action_log("Reset\r\n");
				#endif /* WRITE_ACTION_LOG */
			}
		}
		vTaskDelay(ERROR_TASK_DELAY);
	}
}
