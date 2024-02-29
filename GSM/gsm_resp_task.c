/*! @file gsm_resp_task.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdlib.h>
#include <stdint.h>
#include "gsm_resp_task.h"
#include "gsm_hal.h"
#include "gsm_cmd.h"
#include "gsm_check_at.h"
#include "mqtt_task.h"
#include "wdt.h"
#include "rtc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*!
* @def GSM_RESP_TASK_SIZE
* Memory size of gsm resp task (minsize * 8)
*/
#define GSM_RESP_TASK_SIZE				(configMINIMAL_STACK_SIZE * 8)

/*!
*	@def GSM_RESP_TASK_PRIORITY
* Priority of gsm resp task (3)
*/
#define GSM_RESP_TASK_PRIORITY	(tskIDLE_PRIORITY + 3)

/*!
*	@def GSM_RESP_TASK_DELAY
* gsm resp task delay (20ms)
*/
#define GSM_RESP_TASK_DELAY			(portTickType)(20 / portTICK_RATE_MS)

/*!
* static data declaration
*/
static StaticTask_t x_gsm_resp_task_buffer;
static StackType_t  x_gsm_resp_stack[GSM_RESP_TASK_SIZE];

static uint8_t u8_gsm_resp_task_id = 0;	/**<ID which watchdog allocated for gsm response task */
/*!
* private function prototype
*/
static void v_gsm_resp_task(void *pvParameters);
/*!
* public function bodies
*/

/*!
* @fn v_gsm_resp_task_init(void)
* @brief Init gsm resp task, must be called in main function.
*/
void v_gsm_resp_task_init(void)
{
	v_gsm_hal_init();
	xTaskCreateStatic(v_gsm_resp_task, "GSM_RESP", 
										GSM_RESP_TASK_SIZE, NULL, 
										GSM_RESP_TASK_PRIORITY, 
										x_gsm_resp_stack, 
										&x_gsm_resp_task_buffer);

}
/**
* private function bodies
*/
static void v_gsm_resp_task(void *pvParameters)
{
	//register task with watchdog timer
	while(b_wdt_reg_new_task("gsm_resp_task", &u8_gsm_resp_task_id) != true){}
	for (;;)
	{		
		//reset wdt flag
		b_wdt_task_reload_counter(u8_gsm_resp_task_id);
		v_gsm_check_at_respone();
		vTaskDelay(GSM_RESP_TASK_DELAY);
	}
}
