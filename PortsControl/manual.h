/*! @file manual.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _MANUAL_H
#define _MANUAL_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
/*!
* @data types, constants and macro defintions
*/	

#define MAX_MANUAL_AREA					64

typedef enum
{
	MANUAL_REQ_STOP = 0,
	MANUAL_STOPPING = 1,
	MANUAL_REQ_RUN = 2,
	MANUAL_RUNNING = 3,
}E_MANUAL_CONTROL_STATE;
typedef struct
{
	uint16_t u16_location_ID;
	E_MANUAL_CONTROL_STATE e_manual_state;
	uint32_t u32_time_start;
	uint32_t u32_time_stop;
	uint32_t u32_bit_low;
	uint32_t u32_bit_high;
}STRU_MANUAL_CONTROL;
/*!
* @public functions prototype
*/	
void v_manual_process (uint64_t* pu64_curr_port_state);
bool b_manual_add_event(uint16_t u16_id, uint32_t u32_time_run, uint32_t u32_bit_low, uint32_t u32_bit_high);
bool b_manual_stop_all(void);
void v_manual_stop_by_port(uint64_t u64_port);
bool b_manual_is_running(void);
#ifdef __cplusplus
}
#endif

#endif /* _MANUAL_H */

