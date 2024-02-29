/*! @file protocol_mgreen.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
* All rights reserved.
*/
#ifndef _PROTOCOL_MGREEN_H
#define _PROTOCOL_MGREEN_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_types.h"
#include "driverlib.h"
#include "uartstdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/*!
* @data types, constants and macro defintions
*/
#define PROTOCOL_MGREEN_TASK_STACK_SIZE												(configMINIMAL_STACK_SIZE * 4)
#define PROTOCOL_MGREEN_TASK_PRIORITY													(tskIDLE_PRIORITY + 3)
#define PROTOCOL_MGREEN_TASK_DELAY														10

void v_protocol_mgreen_task_init(void);
#ifdef __cplusplus
}
#endif
#endif /* _PROTOCOL_MGREEN_H */
