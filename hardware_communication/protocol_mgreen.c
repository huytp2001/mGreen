/*! @file protocol_mgreen.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdlib.h>
#include "protocol_mgreen.h"
#include "config.h"
#include "uart_mgreen.h"
#include "uartstdio.h"
#include "i2c_mgreen.h"
#include "can_mgreen.h"
#include "spi_mgreen.h"
#include "relay.h"

#include "firmware_config.h"

/**
* @static data declaration
*/				
/**
* @private function prototype
*/
static void v_protocol_mgreen_task(void *pvParameters);

static StaticTask_t xMgreenProtoTaskBuffer;
static StackType_t  xMgreenProtoStack[PROTOCOL_MGREEN_TASK_STACK_SIZE];

/**
* @public function bodies
*/
void v_protocol_mgreen_task_init(void)
{
	v_relay_software_init();	
	xTaskCreateStatic(v_protocol_mgreen_task, "protocol_mgreen_task", PROTOCOL_MGREEN_TASK_STACK_SIZE,
														NULL, PROTOCOL_MGREEN_TASK_PRIORITY, xMgreenProtoStack, &xMgreenProtoTaskBuffer);
}
 /**
* @private function bodies
*/

static void v_protocol_mgreen_task(void *pvParameters)
{
	vTaskSetApplicationTaskTag( NULL, ( void * ) TASK_TRACE_CAN_APP_ID );
	for(;;)
	{
		vTaskDelay(PROTOCOL_MGREEN_TASK_DELAY);
	}
}
