/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.c
 * @author Danh Pham
 * @date 13 Nov 2020
 * @version: 1.0.0
 * @brief This file contains functions used to parser the schedule from csv file.
 * Function in this file MUST be call in the begining of schedule manage program.
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 #include "HAL_BSP.h"
 #include "sdram.h"
 #include "rtc.h"
 #include "schedule_parser.h"
 #include "sd_app.h"
 #include "sdram_utils.h"
 #include "params.h"

/*!
* @def SCHEDULE_MAX_CSV_ROW
* Maximum row of csv file
*/
#define SCHEDULE_MAX_CSV_ROW					(100)

/*!
*	Variables declare
*/
static STRU_IRRIGATION_SCHEDULE astr_irrigation_sche_table[SCHEDULE_MAX_CSV_ROW];

static uint16_t au16_checksum_table[SCHEDULE_MAX_CSV_ROW]; /**<Contains
																					checksum of start time of each event */
static char c_schedule_version[21] = {0}; /**<Contains version of schedule file */
static uint8_t u8_csv_field_counter = 0; /**<Identify current field in csv file */
static uint16_t u16_current_row = 0; /**<Identify current row in csv file */
static bool b_save_event = false; /**<Identify the event can be saved or not */
static uint8_t u8_current_valve = 0; /**<Identify current parsed valve */
static uint32_t u32_time_begin = 0; /**<Start time of current parsed event */
static uint32_t u32_date_finish = 0; /**<End date of current parsed event */
static bool b_weekly = false; /**<Identify period type of event */
static uint16_t u16_total_row = 0; /**<Identify total parsed row */
static E_SCHEDULE_ERR_CODE e_sch_error_code = SCHE_ERR_NULL;	/**<Contains error code */
static uint16_t u16_sch_row_error = 0;												/**<Contains the order of error row */
static uint32_t u32_test_array[10] = {0};
/*!
*	Private functions prototype
*/
static void v_load_sche_field_callback(void *pv_s, size_t len, void *pv_data);
static void v_load_sche_row_callback(int c, void * data);
static bool b_sche_parse_schedule(uint8_t u8_field, char *pc_s);
static void v_set_csv_err_flag(E_SCHEDULE_ERR_CODE e_err_flag, uint16_t u16_row);
static void v_set_checksum(uint16_t u16_id, uint16_t u16_checksum);
static void v_schedule_clear(void);

/*!
*	Public functions
*/

/*!
*	@fn v_schedule_version_get(char* version)
* @brief Get version of schedule file
* @param[out] pc_version string contains version
* @return None
*/
void v_schedule_version_get(char* pc_version)
{
	memcpy(pc_version, c_schedule_version, strlen(c_schedule_version) + 1);
}

/*!
*	@fn e_schedule_parse(char* pc_schedule_filename)
* @brief Parse the schedule csv file
* @param[in] pc_schedule_filename Name of file
* @param[in] b_check_crc Check CRC of schedule file or not
* @return E_SCHEDULE_ERR_CODE
*/
E_SCHEDULE_ERR_CODE e_schedule_parse(char* pc_schedule_filename, bool b_check_crc)
{
	STRU_CSV_COUNT stru_c;
	v_schedule_clear();
	if(b_sdcard_ready())
	{
		if(i8_handle_csv_file_callback(pc_schedule_filename,
					v_load_sche_field_callback, 
					v_load_sche_row_callback, 
					&stru_c, b_check_crc) != 0)
		{
			v_set_csv_err_flag(SCHE_ERR_OPEN_FILE, 0);
		}
		else
		{
			v_set_csv_err_flag(SCHE_ERR_NULL, 0);
		}
	}
	else
	{
		uint8_t u8_temp_data[4] = {0, 0, 0, 0};
		uint32_t u32_csv_crc = 0;
		s32_params_get(PARAMS_ID_SCHEDULE_CSV_CRC, u8_temp_data);
		u32_csv_crc = ((uint32_t)u8_temp_data[3]<< 24) | ((uint32_t)u8_temp_data[2]<< 16) 
										|((uint32_t)u8_temp_data[1]<< 8) | ((uint32_t)u8_temp_data[0]);
		
		if(i8_sdram_parse_schedule_csv(v_load_sche_field_callback, 
					v_load_sche_row_callback, 
					&stru_c, u32_csv_crc) != 0)
		{
			v_set_csv_err_flag(SCHE_ERR_OPEN_FILE, 0);
		}
		else
		{
			v_set_csv_err_flag(SCHE_ERR_NULL, 0);
		}
	}
	return e_sch_error_code;
}

/*!
*	@fn stru_schedule_get(uint16_t u16_order)
* @brief Get a irrigation event in schedule with predefined order
* @param[in] u16_order Order of event
* @return STRU_IRRIGATION_SCHEDULE Struct of irrigation event
*/
STRU_IRRIGATION_SCHEDULE stru_schedule_event_get(uint16_t u16_order)
{
	return astr_irrigation_sche_table[u16_order];
}

/*!
*	@fn u16_schedule_error_row_get(uint16_t u16_order)
* @brief Get error row in the case the schedule can not parse successfully.
* @param[in] u16_order Order of event
* @return Row contains error
*/
uint16_t u16_schedule_error_row_get(uint16_t u16_order)
{
	return u16_sch_row_error;
}

/*!
* @fn u16_schedule_total_row_get(void)
* @brief Get number of available rows in schedule
* @return uint16_t number of rows
*/
uint16_t u16_schedule_total_row_get(void)
{
	return u16_total_row;
}
/*!
*	@fn u16_schedule_checksum_get(uint16_t u16_order)
* @brief Get a start time checksum of event in schedule with predefined order
* @param[in] u16_order Order of event
* @return Start time checksum
*/
uint16_t u16_schedule_checksum_get(uint16_t u16_order)
{
	return au16_checksum_table[u16_order];
}

/*!
* @fn uint16_t u16_checksum(uint32_t u32_value)
* @brief Calculate sum of bytes of a 32bit value
* @param[in] u32_value Value need to calculate checksum
* @return Value of checksum
*/
uint16_t u16_checksum(uint32_t u32_value)
{
	return ((u32_value >> 24 & 0xFF) 
					| (u32_value >> 16 & 0xFF) 
					| (u32_value >> 8 & 0xFF) 
					| (u32_value & 0xFF));
}

/*!
*	Private functions
*/
/*!
*	@fn void v_load_sche_field_callback(void *pv_s, size_t len, void *pv_data)
* @brief Callback function for parsing schedule csv file field
* @param[in] pv_s string data of field
* @param[in] len size of string
* @param[in] pv_data buffer of data
* @return None
*/
static void v_load_sche_field_callback(void *pv_s, size_t len, void *pv_data)
{
	static uint8_t u8_type_of_data = 0;
	((STRU_CSV_COUNT *)pv_data)->u16_fields++;
	if(pv_s != NULL)
	{
		if(u8_csv_field_counter == 0)
		{
			u8_type_of_data = strtoul(pv_s, 0, 10);
		}
		switch(u8_type_of_data)
		{
			case 0:							//csv version
			{
				if(u8_csv_field_counter == 1)
				{
					memcpy((void *)c_schedule_version, (void *)pv_s, strlen(pv_s) + 1);
				}
			}
			break;
			case 1:						//fertilizer tank setting
			{
				//do nothing
			}
			break;
			case 3:							//schedule data
			{
				b_sche_parse_schedule(u8_csv_field_counter, pv_s);
			}
			break;
			case 4:							//pressure set
			{
				//do nothing
			}
			break;
			default: break;
		}
		u8_csv_field_counter++;
	}
}

/*!
* @fn v_load_sche_row_callback(int c, void * data)
* @param[in] c
* @param[in] data struct contains parameters of csv file
* @return None
*/
static void v_load_sche_row_callback(int c, void * data)
{
	u8_csv_field_counter = 0;
	((STRU_CSV_COUNT *)data)->u16_rows++;
}	

/*!
* @fn bool b_sche_parse_schedule(uint8_t u8_field, char *pc_s)
* @brief Parsing fertigation schedule from csv file and save to an array
* @param[in] u8_field Order of field in row
* @param[in] pc_s String contained in field
*/
static bool b_sche_parse_schedule(uint8_t u8_field, char *pc_s)
{	
	bool b_result = true;
	if(!b_save_event && u8_field >= 7)
	{
		u8_field = 0;
	}
	switch(u8_field)
	{
		case 1: 				//order
		{
			u8_current_valve = 0;
			b_save_event = false;
			if(u16_current_row > SCHEDULE_MAX_CSV_ROW)
			{
				v_set_csv_err_flag(SCHE_ERR_MAX_ROW, u16_current_row);
				b_result = false;
			}
		}
		break;
		case 2:					//time begin
		{
			u32_time_begin = strtoul(pc_s, 0, 10) + u16_get_time_zone() * 60;
			u32_test_array[0] = strtoul(pc_s, 0, 10);
		}
		break;
		case 3:					//time finish
		{
			u32_date_finish = strtoul(pc_s, 0, 10) + u16_get_time_zone() * 60;
			u32_test_array[1] = strtoul(pc_s, 0, 10);
		}
		break;
		case 4: 				//frequency
		{
			 b_weekly = strtoul(pc_s, 0, 10);
		}
		break;
		case 5:					//interval
		{
			if(!b_weekly)	//freq = daily
			{
				uint16_t u16_interval = strtoul(pc_s, 0, 10);
				T_DATETIME t_time; 
				v_rtc_read_time(&t_time);
				uint32_t u32_today = u32_datetime_2_long(&t_time);
				u32_today = u32_today / 86400;			//get num of date
				u32_date_finish = u32_date_finish/86400;
				uint32_t u32_temp_begin = u32_time_begin / 86400;
				if(u32_today >= u32_temp_begin && u32_date_finish >= u32_today)
				{
					if(((u32_today - u32_temp_begin) % u16_interval) == 0)
					{
						v_set_checksum(u16_current_row, u16_checksum(u32_time_begin % 86400));
						astr_irrigation_sche_table[u16_current_row].u32_time_start = u32_time_begin % 86400;
						u16_current_row++;
						u16_total_row++;
						b_save_event = true;
					}
				}
			}
		}
		break;
		case 6: 				//Byday
		{
			if(b_weekly)
			{
				T_DATETIME time;
				v_rtc_read_time(&time);
				uint8_t u8_byday = strtoul(pc_s, 0, 10);
				uint8_t u8_wday = u8_week_day_get(&time);
				uint32_t u32_today = u32_datetime_2_long(&time);
				u32_today = u32_today / 86400;													//get num of date
				u32_date_finish = u32_date_finish/86400;
				uint32_t u32_temp_begin = u32_time_begin / 86400;
				if(u32_today >= u32_temp_begin && u32_date_finish >= u32_today)
				{
					if(u8_byday & (1 << u8_wday))
					{
						v_set_checksum(u16_current_row, u16_checksum(u32_time_begin % 86400));
						astr_irrigation_sche_table[u16_current_row].u32_time_start = u32_time_begin % 86400;
						u16_current_row++;
						u16_total_row++;
						b_save_event = true;
					}
				}
			}
		}
		break;
		case 7:					//date irrigation must happen
		{
		}
		break;
		case 8:					//date irrigation must skip
		{
		}
		break;
		case 9:					//location ID
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].u8_location_id = strtoul(pc_s, 0, 10);
			}
			u32_test_array[2] = strtoul(pc_s, 0, 10);
		}
		break;
		case 10:					//begin on time (not use)
		{
			
		}
		break;
		case 11:					//irrigation type (time or m3)
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].e_duration_unit = (E_DURATION_UNIT)strtoul(pc_s, 0, 10);
				if(astr_irrigation_sche_table[u16_current_row - 1].e_duration_unit >= UNIT_MAX)
				{
					v_set_csv_err_flag(SCHE_ERR_UNIT, u16_current_row);
					b_result = false;
				}
			}
		}
		break;
		case 12:					//total irrigation time or volume(real volume = csv volume/100)
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].u32_total_duration = strtoul(pc_s, 0, 10);
				//check if it's overnight event
				if(astr_irrigation_sche_table[u16_current_row - 1].e_duration_unit == UNIT_TIME 
						&& (astr_irrigation_sche_table[u16_current_row - 1].u32_total_duration 
									+ astr_irrigation_sche_table[u16_current_row - 1].u32_time_start) > 86399)	
				{
					v_set_csv_err_flag(SCHE_ERR_TOTAL_TIME, u16_current_row);
					b_result = false;
				}
				u32_test_array[3] = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 13:					//before 
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].u32_before_duration = strtoul(pc_s, 0, 10);
				//check if time before > total time
				if(astr_irrigation_sche_table[u16_current_row - 1].u32_before_duration 
								> astr_irrigation_sche_table[u16_current_row - 1].u32_total_duration)
				{
					v_set_csv_err_flag(SCHE_ERR_BEFORE, u16_current_row);
					b_result = false;
				}
				u32_test_array[4] = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 14:					//after
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].u32_after_duration = strtoul(pc_s, 0, 10);
				//check if (before + after) > total time
				if((astr_irrigation_sche_table[u16_current_row - 1].u32_before_duration
						+ astr_irrigation_sche_table[u16_current_row - 1].u32_after_duration)
						> astr_irrigation_sche_table[u16_current_row - 1].u32_total_duration)
				{
					v_set_csv_err_flag(SCHE_ERR_AFTER, u16_current_row);
					b_result = false;
				}
			}
		}
		break;
		case 15:					//fertigation (y/n)
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].b_is_fertigation = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 16:					//fertigation type
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].e_dosing_type = (E_DOSING_TYPE)strtoul(pc_s, 0, 10);
				if(astr_irrigation_sche_table[u16_current_row - 1].e_dosing_type > TYPE_DOSING_MAX)
				{
					v_set_csv_err_flag(SCHE_ERR_DOSING_TYPE, u16_current_row);
					b_result = false;
				}
			}
		}
		break;
		case 17:				//rate of fertilizer (real value = csv value/100)
		case 18:
		case 19:
		case 20:
		case 21:
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1]
										.au32_dosing_rate[u8_csv_field_counter - 17] = strtoul(pc_s, 0, 10); 
				u32_test_array[5] = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 22: 				//EC target
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].u16_ec = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 23:				//pH target
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].u16_ph = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 24:				//valve 32bit low
		{
			if(b_save_event)
			{
				uint32_t u32_valve_low = strtoul(pc_s, 0, 10);
				astr_irrigation_sche_table[u16_current_row - 1].u8_num_of_valves = 0;
				astr_irrigation_sche_table[u16_current_row - 1].u32_low_valve = u32_valve_low;
				for(uint8_t i = 0; i < 10; i++)
				{
					astr_irrigation_sche_table[u16_current_row - 1].a_stru_valves[i].u8_port = 0;
				}
				for(uint8_t i = 0; i < 32; i++)
				{
					if(u32_valve_low & (1<<i))								//valve selected <=> bit set to 1
					{
						astr_irrigation_sche_table[u16_current_row - 1].a_stru_valves[u8_current_valve].u8_port = i + 1;
						u8_current_valve++;
						astr_irrigation_sche_table[u16_current_row - 1].u8_num_of_valves++;
					}
				}
				u32_test_array[6] = strtoul(pc_s, 0, 10);
				u32_test_array[7] = astr_irrigation_sche_table[u16_current_row - 1].u8_num_of_valves;
			}
		}
		break;
		case 25:				//valve 16bit high
		{
			if(b_save_event)
			{
				uint16_t u16_valve_high = strtoul(pc_s, 0, 10);
				astr_irrigation_sche_table[u16_current_row - 1].u16_high_valve = u16_valve_high;
				for(uint8_t i = 0; i < 16; i++)
				{
					if(u16_valve_high & (1<<i))
					{
						astr_irrigation_sche_table[u16_current_row - 1].a_stru_valves[u8_current_valve].u8_port = i + 1 + 32;
						u8_current_valve++;
						astr_irrigation_sche_table[u16_current_row - 1].u8_num_of_valves++;
					}
				}
				u32_test_array[8] = astr_irrigation_sche_table[u16_current_row - 1].u8_num_of_valves;
			}
		}
		break;
		case 26:				//output flow
		{
			if(b_save_event)
			{
				astr_irrigation_sche_table[u16_current_row - 1].d_flow_rate = (double)(strtoul(pc_s, 0, 10))/100;
				u32_test_array[9] = strtoul(pc_s, 0, 10);
			}
		}
		break;
		case 27:				//location name
		{
			if(b_save_event)
			{
				if(strlen(pc_s) < 21)
					memcpy((void *)astr_irrigation_sche_table[u16_current_row - 1].c_location_name, (void *)pc_s, strlen(pc_s) + 1);
				else
					memcpy((void *)astr_irrigation_sche_table[u16_current_row - 1].c_location_name, (void *)pc_s, 20);
				astr_irrigation_sche_table[u16_current_row - 1].c_location_name[20] = 0x00;
			}
		}
		break;
		case 28:				//crop name (not use)
		{
			
		}
		break;
		case 29:				//crop state (not use)
		{
			
		}
		break;
		default: break;
	}
	return b_result;
}

/*!
* @fn v_set_csv_err_flag(E_SCHEDULE_ERR_CODE e_err_flag, uint16_t u16_row)
* @brief Save the error code when parsing csv file
* @param[in] e_err_Flag Code of error
* @param[in] u16_row Error row
* @return None
*/
static void v_set_csv_err_flag(E_SCHEDULE_ERR_CODE e_err_flag, uint16_t u16_row)
{
	e_sch_error_code = e_err_flag;
	u16_sch_row_error = u16_row;
}

/*!
* @fn v_set_checksum(uint16_t u16_id, uint16_t u16_checksum)
* @brief Save the checksum of start time to array
* @param[in] u16_id Id of fertigation event (row of schedule)
* @param[in] u16_checksum Sum of bytes of start time
* @return None
*/
static void v_set_checksum(uint16_t u16_id, uint16_t u16_checksum)
{
	au16_checksum_table[u16_id] = u16_checksum;
}


/*!
* @fn v_schedule_clear(void)
* @brief Clear all schedule in schedule table
* @return None
*/
static void v_schedule_clear(void)
{
	u16_current_row = 0;
	memset(astr_irrigation_sche_table, 0, SCHEDULE_MAX_CSV_ROW * sizeof(STRU_IRRIGATION_SCHEDULE));
}
