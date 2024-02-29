/*! @file schedule.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _SCHEDULE_H
#define _SCHEDULE_H


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "config.h"
#include "irrigation.h"
/*!
* @data types, constants and macro defintions
*/	
#define MAIN_CSV_FILE							"MAINFTG.CSV"
#define MAX_CSV_ROW								800
#define MAX_NAME_LEN							20

#define MAX_IRRIGATION_ROW					800
#define MAX_SCHEDULE_EVENT					48
	
//#define CSV_COMMENT_LINE_FIRST_CHAR			'#'
//#define CSV_QUOTE_CHAR									'"'
//#define CSV_BUFFER_LENGTH_READ_LINE			200
//#define DELIMITER												"\t\r\n"

#define MAX_RETRY_CHECK					40 //20*500ms = 10s
#define UNDER_FLOW_THRESHOLD		1.4 //40%

struct STRU_CSV_COUNT
{
	uint16_t u16_fields;
	uint16_t u16_rows;
};

typedef enum
{
	SCHE_NONE = 0,
	SCHE_START_FERTILIZER = 1,
	SCHE_STOP_FERTILIZER = 2,
	SCHE_STOP_EVENT = 225
}SCHEDULE_EVENT_STATUS;

typedef enum
{
	SCHE_STATE_LOAD = 0,
	SCHE_STATE_SEEK_EVENT = 1,
	SCHE_STATE_CHECK_TIME = 2,
	SCHE_STATE_TIME_OVER = 3,
}SCHEDULE_STATE;

typedef struct
{
	uint16_t u16_order;
	uint16_t u16_location_ID;
	uint16_t u16_pH;
	E_DURATION_UNIT e_unit;
	uint32_t u32_time_start;				//in second, from 0 to 86399
	uint32_t u32_time_run;					//in second, from 0 to 86399
	uint32_t u32_time_end;					//in second, from 0 to 86399
	uint32_t u32_time_fer_start;
	uint32_t u32_time_fer_stop;
	uint64_t u64_valve;
	uint8_t au8_fer_port[5];
	uint32_t u32_pump1_port;
	uint32_t u32_pump2_port;
	uint32_t u32_boost_pump_port;
	bool b_on_duty;
	bool b_fer_enable;
	uint32_t u32_time_runed;
	char cname[20];
	uint32_t au32_dosing_rate[5];
}STRU_IRRIGATION;

typedef struct
{
	uint16_t u16_location_ID;
	uint16_t u16_irrigation_pointer;
	E_DURATION_UNIT e_unit;
	uint32_t u32_time_begin;
	uint32_t u32_time_off;
	uint32_t u32_time_fer_begin;
	uint32_t u32_time_fer_off;
	uint32_t u32_time_fer_total;
	bool b_fer_is_running;
	uint32_t u32_total_remain_time;
	uint32_t u32_fer_remain_time;
	uint32_t au32_fer_rate[5];
}STRU_SCH_EVENT;

typedef struct
{
	uint8_t u8_ID;
	uint8_t u8_Repeat;
	uint8_t u8_Current_lap;
	uint8_t u8_T_hour;								//delay to next time point
	uint8_t u8_T_min;
	uint32_t u32_Time_Delay;					// delay to next cycle (s)
	uint8_t u8_No_Except;
	uint8_t u8_Except_ID[48];
	uint8_t u8_W_hour;								//use for ftg UI
	uint8_t u8_W_min;									//use for ftg UI
	uint8_t u8_hour;									//use for ftg process, not use for ftg UI
	uint8_t u8_min;										//use for ftg process, not use for ftg UI
}STRU_TIME_TABLE;

typedef struct
{
	uint8_t u8_ID;
	uint8_t u8_No_Channel;
	uint8_t u8_aFer_ChannelID[5];
	E_DOSING_TYPE e_type;
	uint32_t u32_aRate[5];
	bool b_pHEC_Calib;
	uint16_t u16_pH;									// *100				
	uint16_t u16_EC;									// *100
}STRU_DOSING;

typedef struct
{
	uint16_t u16_ID;
	uint8_t u8_noPhase;
}STRU_DATA_BASE;



typedef struct
{
	uint8_t u8_ID;
	E_DOSING_TYPE e_unit;
	uint16_t u16_Volume_water;
	uint16_t u16_Water_before;
	uint16_t u16_Water_after;
	uint32_t u32_TimeRun;						//in second(s)
	uint32_t u32_Time_before;				//in second(s)
	uint32_t u32_Time_after;				//in second(s)
	uint8_t u8_Fer_rate;
	uint8_t u8_Water_rate;
	uint8_t u8_Date_Fer_rate;
	uint8_t u8_Date_Water_rate;
}STRU_WATER_MODULE;

typedef struct
{
	uint8_t u8_Current_Sub_PointID;
	uint8_t u8_Current_Sub_Point;
	uint8_t u8_Current_timeID;
	uint8_t u8_Current_time_Point;
	uint8_t u8_No_Sub;
	uint8_t u8_No_Time;
	uint8_t u8_aSubID[48];
}STRU_MAIN_PROG;

typedef enum
{
	TYPE_FER_NULL,
	TYPE_FER_EC,
	TYPE_FER_AC,
	TYPE_FER_AL,
	TYPE_MAX_TYPE
}E_FER_TYPE;

typedef struct
{
	uint8_t u8_ID;
	uint16_t u16_Volume;
	E_FER_TYPE e_type;
	uint16_t u16_weigh;								// Kg
	bool b_pH;
	uint16_t u16_pH;									// *100
	uint8_t u8_Port;
	bool bChecked;
}STRU_FER_CAN;

typedef enum
{
	CSV_ERR_NULL = 0,
	CSV_ERR_AREA,
	CSV_ERR_TANK,
	CSV_ERR_DOSING,
	CSV_ERR_COMBINE,
	CSV_ERR_DATABASE,
	CSV_ERR_PLAN1,
	CSV_ERR_PLAN2,
	CSV_ERR_OPERATION_TIME,
	CSV_ERR_TOTAL_TIME,
	CSV_ERR_MAX_ROW,
	CSV_ERR_MAX
}E_CSV_ERR_CODE;
/*!
* @public functions prototype
*/
int iHandle_CSVfileCallback(char *txtname, void (*cb1)(void *, uint32_t, void *),
														void (*cb2)(int c, void *), void *c);
int i_load_schedule_from_sd_card(void);
void v_schedule_process (uint64_t* pu64_curr_valve_state);
void v_sche_reload_event(uint64_t *pu64_curr_valve_state);
bool b_shedule_is_running(void);
uint64_t u64_getclear_dependent_slave_port(uint8_t u8_master_port);
uint16_t u16_get_total_turn(void);
STRU_IRRIGATION get_irrigation_row(uint16_t u16_order);
#endif /* _SCHEDULE_H */

