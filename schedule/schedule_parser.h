/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file schedule.h
 * @author Danh Pham
 * @date 12 Nov 2020
 * @version: 2.0.0
 * @brief This file contains structs and function support for fertigation schedule.
 * 
 */
 
 #ifndef _SCHEDULE_H_
 #define _SCHEDULE_H_
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #include "irrigation.h"
 
typedef enum
{
	SCHE_ERR_NULL = 0,
	SCHE_ERR_OPEN_FILE,
	SCHE_ERR_MAX_ROW,
	SCHE_ERR_UNIT,
	SCHE_ERR_TOTAL_TIME,
	SCHE_ERR_BEFORE,
	SCHE_ERR_AFTER,
	SCHE_ERR_AREA,
	SCHE_ERR_TANK,
	SCHE_ERR_DOSING_TYPE,
	SCHE_ERR_MAX,
}E_SCHEDULE_ERR_CODE;
/*!
* @struct STRU_IRRIGATION_SCHEDULE
* Defines parameters of irrigation scheldule, 
* used to save information from schedule csv file
*/
typedef struct
{
	uint8_t u8_location_id; 	/**<id of location to be irrigated */
	uint8_t u8_num_of_valves;	/**<number of used valves */
	E_DOSING_TYPE e_dosing_type; /**<type of mixing program */
	E_DURATION_UNIT e_duration_unit; /**<unit of fertigation program */
	bool b_is_fertigation;	/**<Identify the program use fertilzer (fertigation) or not
															(irrigation) */
	uint16_t u16_ph;				/**<pH set point (= real pH value * 100) */
	uint16_t u16_ec;				/**<pH set point (= real EC value * 100) */
	uint32_t u32_total_duration; /**<total duration of fertigation program,
																	in second if unit is time or m3 * 100 if unit is m3 */
	uint32_t u32_before_duration; /**<watering duration before fertilizing, 
																	in second if unit is time or m3 * 100 if unit is m3 */
	uint32_t u32_after_duration;	/**<watering duration after fertilizing,
																	in second if unit is time or m3 * 100 if unit is m3 */
	uint16_t u16_row;				/**<order of csv row */
	uint32_t u32_time_start;	/**<start point of irrigation event, in second, from 0 to 86399 */
	uint32_t u32_low_valve;		/**<identify port of first 32 valve by bit */
	uint16_t u16_high_valve;	/**<identify port of last 16 valve by bit */	
	double d_flow_rate;		/**<total flow rate of location to be irrigated */
	uint32_t au32_dosing_rate[5]; /**<Dosing rate of each fetigation valve = real value * 100 */
	STRU_VALVE a_stru_valves[10];		/**<Array of valves in current area */
	char c_location_name[21];				/**<Name of location to be irrigated */
}STRU_IRRIGATION_SCHEDULE;
/*!
*	Extern functions
*/
extern void v_schedule_version_get(char* pc_version);
extern E_SCHEDULE_ERR_CODE e_schedule_parse(char* pc_schedule_filename, bool b_check_crc);
extern STRU_IRRIGATION_SCHEDULE stru_schedule_event_get(uint16_t u16_order);
extern uint16_t u16_schedule_total_row_get(void);
extern uint16_t u16_checksum(uint32_t u32_value);
extern uint16_t u16_schedule_checksum_get(uint16_t u16_order);
#endif /* _SCHEDULE_H_ */
