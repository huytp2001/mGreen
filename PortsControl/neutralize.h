#ifndef _NEUTRALIZE_H_
#define _NEUTRALIZE_H_

#include <stdint.h>
#include <stdbool.h>
#include "schedule.h"

#define NEUTRAL_PUMP_PORT					(1<<0)
#define NEUTRAL_VALVE_PORT				(1<<1)
#define NEUTRAL_PH_OFSET					(20)

typedef enum
{
	NEUTRAL_IDLE = 0,
	NEUTRAL_START,
	NEUTRAL_CHECK_PH,
	NEUTRAL_ADD_ACID,
	NEUTRAL_AERATION,
	NEUTRAL_REQUEST_STOP,
	NEUTRAL_STOP,
}E_NEUTRALIZE_STATE;

/* Public functions */
void v_neutralize_set_state(E_NEUTRALIZE_STATE e_state);
bool b_neutralize_process(uint64_t *u64_port_control_value, STRU_IRRIGATION str_irr_row);
void v_neutralize_schedule(uint64_t *u64_port_control_value);
void v_set_neutral_sample_duration(uint16_t u16_duration);
void v_set_neutral_time_inject_acid(uint16_t u16_time);
void v_set_neutral_time_aeration(uint16_t u16_time);
void v_set_netral_start_point(uint16_t u16_point);
void v_set_neutral_stop_point(uint16_t u16_point);
void v_set_neutral_start_time(uint32_t u32_time);
void v_set_neutral_stop_time(uint32_t u32_time);
void v_clear_neu_schedule(void);

#endif /* _NEUTRALIZE_H_ */
