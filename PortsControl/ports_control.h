/*! @file ports_control.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _PORTS_CONTROL_H
#define _PORTS_CONTROL_H
#ifdef __cplusplus
extern �C� {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
/*!
* @data types, constants and macro defintions
*/	
/* TODO: do we need large stack size 128x16 = 2KB??*/
#define PORTS_CONTROL_TASK_STACK_SIZE												(configMINIMAL_STACK_SIZE * 20)
#define PORTS_CONTROL_TASK_PRIORITY													(tskIDLE_PRIORITY + 3)
#define PORTS_CONTROL_TASK_DELAY														500	//ms	
#define PORTS_CONTROL_RETRY_TIMEOUT													20000	//ms
#define PORTS_CONTROL_MAX_RETRY															5			//times

typedef enum
{
	STATE_BEGIN = 0,
	STATE_STOP,
	STATE_REQ_RUN,
	STATE_CON_RUN,
	STATE_RUN,
	STATE_REQ_STOP,
	NUM_PORTS_CONTROL_STATE,
}E_PORTS_CONTROL_STATE;

	
/*!
* @public functions prototype
*/	
void v_ports_control_task_init (void);
//void v_ports_control (uint64_t u64_old_state, uint64_t u64_new_state);
void v_set_ports_control_state (E_PORTS_CONTROL_STATE e_state);
uint64_t u64_port_control_get_status(void);
E_PORTS_CONTROL_STATE e_get_ports_control_state(void);
void v_new_schedule_set(void);
#ifdef __cplusplus
}
#endif

#endif /* _PORTS_CONTROL_H */
