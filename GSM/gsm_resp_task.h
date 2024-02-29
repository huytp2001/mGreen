/*! @file gsm_resp_task.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef GSM_RESP_TASK_H
#define GSM_RESP_TASK_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib.h"
#include "board_config.h"
/*!
* @data types, constants and macro defintions
*/
/*!
* @public functions prototype
*/
void v_gsm_resp_task_init(void);
#ifdef __cplusplus
}
#endif

#endif /* GSM_RESP_TASK_H */
