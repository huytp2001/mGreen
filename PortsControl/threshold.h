/*! @file threshold.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _THRESHOLD_H
#define _THRESHOLD_H
#ifdef __cplusplus
extern �C� {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "schedule.h"
#include "frame_parser.h"
#include "irrigation.h"
/*!
* @data types, constants and macro defintions
*/	
#define MAIN_THD_FILE							"THD.CSV"
#define MAX_THD_ROW								128	
#define MAX_THD_DATA							8	

#define THD_IDX_EVENT_ID					0
#define THD_IDX_LOCAL_ID					1
#define THD_IDX_PORT_L						2
#define THD_IDX_PORT_H						3
#define THD_IDX_ACT								4

#define THD_IDX_NODE_ID_1					8
#define THD_IDX_SENSOR_ID_1				9
#define THD_IDX_CDT_1							10
#define THD_IDX_VALUE_1						11


typedef enum
{
	THD_ACT_OFF = 0,
	THD_ACT_ON = 1,
	THD_ACT_TIMER = 2,
	THD_ACT_NONE = 0xFF
}E_THD_ACT_TYPE;

typedef enum
{
	THD_CDT_L = 0,
	THD_CDT_H = 1,
	THD_CDT_NONE = 0xFF
}E_THD_CDT_TYPE;

typedef struct
{
	uint32_t u32_node_ID;
	uint16_t u16_sensor_ID;
	E_THD_CDT_TYPE e_thd_cdt_type;
	uint32_t u32_thd_value;
	uint32_t u32_curr_value;
	bool b_has_data;
	bool b_status;
}STRU_THD_DATA;

typedef struct
{
	uint32_t u32_thd_event_id;
	uint16_t u16_location_id;
	E_THD_ACT_TYPE e_thd_act_type;
	uint32_t u32_start_time;	//in second, from 0 to 86399
	uint32_t u32_stop_time;		//in second, from 0 to 86399
	uint16_t u16_max_num_turn;
	uint32_t u32_min_time_bwt_2_turn;
	uint32_t u32_max_time_bwt_2_turn;
	E_DURATION_UNIT e_unit;
	uint32_t u32_time_run;					//in second, from 0 to 86399
	uint32_t u32_time_fer_start;
	uint32_t u32_time_fer_stop;
	uint64_t u64_port;
	uint8_t au8_fer_port[5];
	uint32_t u32_pump1_port;
	uint32_t u32_pump2_port;
	uint32_t u32_boost_pump_port;
	uint8_t u8_num_thd_data;
	STRU_THD_DATA astru_thd_data[MAX_THD_DATA];
	bool b_on_duty;
	bool b_pre_on_duty;
	uint32_t u32_unix_time_start;
	uint32_t u32_time_runed;
}STRU_THD;

typedef struct
{
	uint16_t u16_location_id;
	uint32_t u32_unix_time_start;
	uint32_t u32_time_runed;
}STRU_THD_LOG_RUNTIME;



/*!
* @public functions prototype
*/
uint32_t u32_convert_raw_als_to_lux(uint16_t u16_raw_als);
int i_load_thd_from_sd_card(void);
void v_apppend_sensor_data_to_thd_table(mqtt_data_frame_t stru_sensor_data);
void v_threshold_process (uint64_t* pu64_curr_port_state);
bool b_add_time_start_to_thd_log_runtime(uint16_t u16_location_id,
																				 uint32_t u32_unixtime_start);
bool b_get_time_start_from_thd_log_runtime(uint16_t u16_location_id, 
																					uint32_t* pu32_unix_time_start);
bool b_clear_time_start_from_thd_log_runtime(uint16_t u16_location_id);

#ifdef __cplusplus
}
#endif

#endif /* _THRESHOLD_H */

