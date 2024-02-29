/*! @file schedule.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib.h"
#include "config.h"
#include "schedule.h"
#include "sensor.h"
#include "spi_mgreen.h"
#include "pHEC.h"
#include "sensor.h"
#include "frame_parser.h"
#include "wdt.h"
#include "storage.h"
#include "csv.h"
#include "rtc.h"
#include "digital_input.h"
#include "sd_card.h"
#include "relay_control.h"
#include "ports_control.h"
#include "error_handler.h"
#include "HAL_BSP.h"
#include "config.h"
#include "setting.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "mqtt_publish.h"
#include "sd_manage_lib.h"
#include "sd_app.h"
#include "mqtt_task.h"
/*!
* static data declaration
*/
struct STRU_CSV_COUNT c;
static STRU_IRRIGATION astr_irrigation_table[MAX_CSV_ROW];
static uint16_t u16_total_row = 0;
static uint8_t u8_csv_field_counter = 0;
static char csv_version[21];
static uint8_t u8_current_cycle = 0;
static bool b_save_event = false;
static bool b_weekly = true;
static bool b_sche_is_running = false;
static uint16_t u16_current_event = 0;
static uint16_t u16_current_order = 0;
static uint16_t u16_ontime_order = 0;
static uint32_t u32_time_begin = 0;
static uint32_t u32_date_finish = 0;
static uint64_t u64_valve_of_pump1 = 0;
static uint64_t u64_valve_of_pump2 = 0;
static STRU_SCH_EVENT stru_sch_event_list[MAX_SCHEDULE_EVENT];
static SCHEDULE_STATE e_schedule_state = SCHE_STATE_LOAD;
static uint8_t au8_dependent_master_port[5] = {0, 0, 0, 0, 0};
static uint64_t au64_dependent_slave_port[5] = {0, 0, 0, 0, 0};
static bool b_event_type_short = false;

STRU_TIME_TABLE time_running, next_time_running;
STRU_DOSING dosing_running, temp_dosing;
STRU_DATA_BASE database_running;
STRU_WATER_MODULE water_running;
STRU_STATISTICAL_MODULE str_EC_statis, str_pH_statis;

static void v_set_csv_err_flag(E_CSV_ERR_CODE e_err_flag, uint16_t u16_row, uint8_t u8_cylce)
{
}

/*!
* private function prototype
*/
static void v_clear_schedule(void);
static bool b_parse_sche_csv(uint8_t u8_cylce, uint8_t u8_field, char *s);
static void v_LoadCSV_FieldCallback_sch(void *s, size_t len, void *data);
static void v_LoadCSV_RowCallback_sch(int c, void * data);
/*!
* public function bodies
*/
/*!
* private function bodies
*/
static void v_clear_schedule(void)
{
	for(uint16_t i = 0; i < MAX_IRRIGATION_ROW; i++)
	{
		astr_irrigation_table[i].u16_order = 0;
		astr_irrigation_table[i].u32_time_start = 0;
		astr_irrigation_table[i].u64_valve = 0;
	}
	u16_total_row = 0;
	u8_csv_field_counter = 0;
	c.u16_fields = 0;
	c.u16_rows = 0;
	
	u16_current_order = 0;
	u16_ontime_order = 0;
	b_save_event = false;
}
static bool b_parse_sche_csv(uint8_t u8_cylce, uint8_t u8_field, char *s)
{	
	if((!b_save_event) && (u8_field >= 7))
	{
		u8_field = 0;
	}
	switch(u8_field)
	{
		case 1: 				//order
		{
			u16_current_event = strtoul(s, 0, 10);
			b_save_event = false;
			if(u16_current_order > MAX_CSV_ROW)
			{
				v_set_csv_err_flag(CSV_ERR_MAX_ROW, u16_current_event, u8_cylce);
				return false;
			}
		}
		break;
		case 2:					//time begin
		{
			u32_time_begin = strtoul(s, 0, 10) + u16_get_time_zone() * 60;
		}
		break;
		case 3:					//time finish
		{
			u32_date_finish = strtoul(s, 0, 10) + u16_get_time_zone() * 60;
		}
		break;
		case 4: 				//frequency
		{
			 b_weekly = strtoul(s, 0, 10);
		}
		break;
		case 5:					//interval
		{
			if(!b_weekly)	//freq = daily
			{
				uint16_t u16_interval = strtoul(s, 0, 10);
				uint32_t u32_today = 0;
				T_DATETIME t_time;  
				v_rtc_read_time(&t_time);
				u32_today = u32_datetime_2_long(&t_time);
				u32_today = u32_today / 86400;			//get num of date
				u32_date_finish = u32_date_finish/86400;
				uint32_t u32_temp_begin = u32_time_begin / 86400;
				if(u32_today >= u32_temp_begin && u32_date_finish >= u32_today)
				{
					if(((u32_today - u32_temp_begin) % u16_interval) == 0)
					{
						astr_irrigation_table[u16_current_order].u32_time_start = u32_time_begin % 86400;
						u16_current_order++;
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
				uint8_t u8_byday = strtoul(s, 0, 10);
				T_DATETIME t_time;  
				v_rtc_read_time(&t_time);
				uint8_t u8_wday = u8_week_day_get(&t_time);
				uint32_t u32_today = u32_datetime_2_long(&t_time);
				u32_today = u32_today / 86400;													//get num of date
				u32_date_finish = u32_date_finish/86400;
				uint32_t u32_temp_begin = u32_time_begin / 86400;
				if(u32_today >= u32_temp_begin && u32_date_finish >= u32_today)
				{
					if(u8_byday & (1 << u8_wday))
					{
						astr_irrigation_table[u16_current_order].u32_time_start = u32_time_begin % 86400;
						u16_current_order++;
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
		// break; /*TNPHU: is it correct?? */
		if(b_save_event)
		{
			case 9:					//location ID
			{
				astr_irrigation_table[u16_current_order - 1].u16_location_ID = strtoul(s, 0, 10);
			}
			break;
			case 10:					//begin on time
			{
			}
			break;
			case 11:					//irrigation type (time or m3)
			{
				astr_irrigation_table[u16_current_order - 1].e_unit = (E_DURATION_UNIT)(strtoul(s, 0, 10));
			}
			break;
			case 12:					//total irrigation time or volume
			{
				if(astr_irrigation_table[u16_current_order - 1].e_unit == UNIT_TIME)
				{
					astr_irrigation_table[u16_current_order - 1].u32_time_run = strtoul(s, 0, 10);
					astr_irrigation_table[u16_current_order - 1].u32_time_end = astr_irrigation_table[u16_current_order - 1].u32_time_start +
																																			astr_irrigation_table[u16_current_order - 1].u32_time_run;
					if(astr_irrigation_table[u16_current_order - 1].u32_time_end > 86399)
					{
						v_set_csv_err_flag(CSV_ERR_TOTAL_TIME, u16_current_event, u8_cylce);
						return false;
					}
				}
				else if(astr_irrigation_table[u16_current_order - 1].e_unit == UNIT_M3)
				{
					astr_irrigation_table[u16_current_order - 1].u32_time_run = strtoul(s, 0, 10);
					astr_irrigation_table[u16_current_order - 1].u32_time_end = astr_irrigation_table[u16_current_order - 1].u32_time_start +
																																			astr_irrigation_table[u16_current_order - 1].u32_time_run;
				}
			}
			break;
			case 13:					//before
			{
				uint32_t u32_temp = strtoul(s, 0, 10);
				astr_irrigation_table[u16_current_order - 1].u32_time_fer_start = u32_temp;
			}
			break;
			case 14:					//after
			{
				uint32_t u32_temp = strtoul(s, 0, 10);
				astr_irrigation_table[u16_current_order - 1].u32_time_fer_stop = u32_temp;
				if(astr_irrigation_table[u16_current_order - 1].u32_time_fer_stop < astr_irrigation_table[u16_current_order - 1].u32_time_fer_start)
				{
					v_set_csv_err_flag(CSV_ERR_DOSING, u16_current_event, u8_cylce);
				}
			}
			break;
			case 15:					//fertigation (y/n)
			{
				uint32_t u32_temp = strtoul(s, 0, 10);
				if(u32_temp == 0)
				{
					astr_irrigation_table[u16_current_order - 1].b_fer_enable = false;
				}
				else
				{
					astr_irrigation_table[u16_current_order - 1].b_fer_enable = true;
				}
			}
			break;
			case 16:					//fertigation type
			{
			}
			break;
			case 17:				//rate of fertilizer
			case 18:
			case 19:
			case 20:
			case 21:
			{
				astr_irrigation_table[u16_current_order - 1]
										.au32_dosing_rate[u8_csv_field_counter - 17] = strtoul(s, 0, 10); 
			}
			break;
			case 22: 				//EC target
			{
			}
			break;
			case 23:				//pH target
			{
				astr_irrigation_table[u16_current_order - 1].u16_pH = strtoul(s, 0, 10);
			}
			break;
			case 24:				//valve 32bit low
			{
				uint32_t u32_valve_low = strtoul(s, 0, 10);
				astr_irrigation_table[u16_current_order - 1].u64_valve = u32_valve_low;
			}
			break;
			case 25:				//valve 16bit high
			{
				uint64_t u64_valve_high = strtoul(s, 0, 10);
				astr_irrigation_table[u16_current_order - 1].u64_valve |= u64_valve_high << 32;
			}
			break;
			case 26:				//output flow
			{
			}
			break;
			case 27:				//location name
			{
				memcpy((void *)astr_irrigation_table[u16_current_order - 1].cname, (void *)s, strlen(s) + 1);
			}
			break;
			case 28:				//crop name
			{
			}
			break;
			case 29:				//crop state
			{
			}
			break;
			case 30:				//pump 1 port
			{
				astr_irrigation_table[u16_current_order - 1].u32_pump1_port = strtoul(s, 0, 10);
			}
			break;
			case 31:				//pump 2 port
			{
				astr_irrigation_table[u16_current_order - 1].u32_pump2_port = strtoul(s, 0, 10);
			}
			break;
			case 32: 			//pump boost port
			{
				astr_irrigation_table[u16_current_order - 1].u32_boost_pump_port = strtoul(s, 0, 10);
			}
			break;
			case 33:
			case 34:
			case 35:
			case 36:
			case 37:
			{
				astr_irrigation_table[u16_current_order - 1].au8_fer_port[u8_field - 33] = strtoul(s, 0, 10);
			}
			break;
		}
		default: break;
	}
	return true;
}

static void v_LoadCSV_FieldCallback_sch(void *s, size_t len, void *data)
{
	static uint8_t u8_type_of_data = 0;
	((struct STRU_CSV_COUNT *)data)->u16_fields++;
	if(s != NULL)
	{
		if(u8_csv_field_counter == 0)
		{
			u8_type_of_data = strtoul(s, 0, 10);
		}
		switch(u8_type_of_data)
		{
			case 0:							//csv version
			{
				if(u8_csv_field_counter == 1)
				{
					memcpy((void *)csv_version, (void *)s, strlen(s) + 1);
				}
			}
			break;
			case 1:						//fertilizer tank setting
			{
			}
			break;
			case 3:							//schedule data
			{
				b_parse_sche_csv(u8_current_cycle, u8_csv_field_counter, s);
			}
			break;
		}
		u8_csv_field_counter++;
	}
}
static void v_LoadCSV_RowCallback_sch(int c, void * data)
{
	u8_csv_field_counter = 0;
	((struct STRU_CSV_COUNT *)data)->u16_rows++;
}
static void v_sche_clear_event(void)
{
	e_schedule_state = SCHE_STATE_LOAD;
	for(int i = 0; i < MAX_SCHEDULE_EVENT; i++)
	{
		stru_sch_event_list[i].u16_location_ID = 0;
		stru_sch_event_list[i].u16_irrigation_pointer = 0;
		stru_sch_event_list[i].u32_fer_remain_time = 0;
		stru_sch_event_list[i].u32_total_remain_time = 0;
		stru_sch_event_list[i].u32_time_off = 0;
		stru_sch_event_list[i].b_fer_is_running = false;
	}
}


static void v_set_dependent_port(uint8_t u8_master, uint64_t u64_slave)
{
	for(uint8_t i = 0; i < 5; i++)
	{
		if(au8_dependent_master_port[i] == u8_master)
		{
			au64_dependent_slave_port[i] = u64_slave;
			return;
		}
	}
	for(uint8_t i = 0; i < 5; i++)
	{
		if(au8_dependent_master_port[i] == 0)
		{
			au8_dependent_master_port[i] = u8_master;
			au64_dependent_slave_port[i] = u64_slave;
			return;
		}
	}
}
uint64_t u64_getclear_dependent_slave_port(uint8_t u8_master_port)
{
	uint64_t u64_result = 0;
	for(uint8_t i = 0; i < 5; i++)
	{
		if(au8_dependent_master_port[i] == u8_master_port)
		{
			u64_result = au64_dependent_slave_port[i];
			au64_dependent_slave_port[i] = 0;
			return u64_result;
		}
	}
	return 0;
}
static bool b_schedule_event_reg(STRU_SCH_EVENT event)
{
	/* TODO: check if the location is existed in event's table or not */
	for(uint8_t i = 0; i < MAX_SCHEDULE_EVENT; i ++)
	{
		if(stru_sch_event_list[i].u16_location_ID == event.u16_location_ID)
			return false;
	}
	/* TODO: append event to table */
	for(uint8_t i = 0; i < MAX_SCHEDULE_EVENT; i ++)
	{
		if(stru_sch_event_list[i].u16_location_ID == 0 &&
			stru_sch_event_list[i].u32_time_off == 0)
		{
			stru_sch_event_list[i] = event;
			return true;
		}
	}
	//Table is full
	return false;
}
static uint64_t u64_schedule_start_event(STRU_IRRIGATION str_irr, uint64_t u64_cur_valve_state)
{
	u64_cur_valve_state |= str_irr.u64_valve;		
	if(str_irr.u32_pump1_port != 0 )
	{
		v_clear_flow_meter(str_irr.u32_pump1_port);
		u64_cur_valve_state |= (1 << (str_irr.u32_pump1_port - 1));
		u64_valve_of_pump1  |= (str_irr.u64_valve | (1 << (str_irr.u32_pump1_port - 1)));
		if(!str_irr.b_fer_enable)
			v_set_dependent_port(str_irr.u32_pump1_port, str_irr.u64_valve);
		else
		{
			uint64_t u64_port = str_irr.u64_valve | (uint64_t)(1 << (str_irr.u32_boost_pump_port - 1));
			for(uint8_t i = 0; i < 5; i++)
			{
				u64_port |= (uint64_t)(1 << (str_irr.au8_fer_port[i] - 1));
			}
			v_set_dependent_port(str_irr.u32_pump1_port, u64_port);
		}
	}
	if(str_irr.u32_pump2_port != 0)
	{
		v_clear_flow_meter(str_irr.u32_pump2_port);
		u64_cur_valve_state |= (1 << (str_irr.u32_pump2_port - 1));
		u64_valve_of_pump2 |= (str_irr.u64_valve | (1 << (str_irr.u32_pump2_port - 1)));
		if(!str_irr.b_fer_enable)
			v_set_dependent_port(str_irr.u32_pump2_port, str_irr.u64_valve);
		else
		{
			uint64_t u64_port = str_irr.u64_valve | (uint64_t)(1 << (str_irr.u32_boost_pump_port - 1));
			for(uint8_t i = 0; i < 5; i++)
			{
				u64_port |= (uint64_t)(1 << (str_irr.au8_fer_port[i] - 1));
	}
			v_set_dependent_port(str_irr.u32_pump2_port, u64_port);
		}
	}
	//send start frame to MQTT
	v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_RUNNING);
	//vMQTTPubAutoparaBegin(str_irr.u16_location_ID, false, TYPE_DOSING_L_L, 
	//						str_irr.u32_time_run, str_irr.u32_time_fer_start, str_irr.u32_time_fer_stop);	
	v_mqtt_fertigation_start(str_irr.u16_location_ID);
	v_clear_flow_meter(2);
	return u64_cur_valve_state;
}
static uint64_t u64_schedule_start_fertilizer(STRU_IRRIGATION str_irr, uint64_t u64_cur_valve_state)
{
	if(str_irr.u32_boost_pump_port != 0)
		u64_cur_valve_state |= (1 << (str_irr.u32_boost_pump_port - 1)); 
	for(uint8_t i = 0; i < 5; i++)
	{
		if(str_irr.au8_fer_port[i] != 0)
			u64_cur_valve_state |= (1 << (str_irr.au8_fer_port[i] - 1));
	}
	return u64_cur_valve_state;	
}

static uint64_t u64_schedule_stop_fertilizer(STRU_IRRIGATION str_irr, uint64_t u64_cur_valve_state)
{
	static bool b_pump_boost_is_sharing = false;
	static bool b_fer_port_is_sharing[5] = {false, false, false, false, false};
	b_pump_boost_is_sharing = false;
	for(uint8_t i = 0; i < 5; i++)
	{
		b_fer_port_is_sharing[i] = false;
	}
	/* TODO: check sharing ports */
	for(uint8_t i = 0; i < MAX_SCHEDULE_EVENT; i ++)
	{
		if(stru_sch_event_list[i].u16_location_ID != 0 &&
			stru_sch_event_list[i].u32_time_off != 0 && str_irr.u16_location_ID != stru_sch_event_list[i].u16_location_ID)
		{
			if(str_irr.u32_boost_pump_port != 0 && astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_boost_pump_port == str_irr.u32_boost_pump_port)
			{
				b_pump_boost_is_sharing = true;
			}
			for(uint8_t ch = 0; ch < 5; ch++)
			{
				if(stru_sch_event_list[i].b_fer_is_running && str_irr.au8_fer_port[ch] != 0 
						&& astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].au8_fer_port[ch] == str_irr.au8_fer_port[ch])
				{
					b_fer_port_is_sharing[ch] = true;
				}
			}
		}
	}
	if(str_irr.u32_boost_pump_port != 0 && !b_pump_boost_is_sharing)
		u64_cur_valve_state &= ~(1 << (str_irr.u32_boost_pump_port - 1)); 
	for(uint8_t i = 0; i < 5; i++)
	{
		if(str_irr.au8_fer_port[i] != 0 && !b_fer_port_is_sharing[i])
			u64_cur_valve_state &= ~(1 << (str_irr.au8_fer_port[i] - 1));
	}
	return u64_cur_valve_state;
}

static uint64_t u64_schedule_stop(STRU_IRRIGATION str_irr, uint64_t u64_cur_valve_state, uint32_t u32_time_begin)
{
	static bool b_pump_1_is_sharing = false;
	static bool b_pump_2_is_sharing = false;
	b_pump_1_is_sharing = false;
	b_pump_2_is_sharing = false;
	u64_cur_valve_state &= ~str_irr.u64_valve;		
	/* TODO: check the pump 1, pump 2 is a shared port or not */
	for(uint8_t i = 0; i < MAX_SCHEDULE_EVENT; i ++)
	{
		if(stru_sch_event_list[i].u16_location_ID != 0 &&
			stru_sch_event_list[i].u32_time_off != 0 && str_irr.u16_location_ID != stru_sch_event_list[i].u16_location_ID)
		{
			if(str_irr.u32_pump1_port != 0 && astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump1_port == str_irr.u32_pump1_port)
			{
				b_pump_1_is_sharing = true;
			}
			if(str_irr.u32_pump2_port != 0 && astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump2_port == str_irr.u32_pump2_port)
			{
				b_pump_2_is_sharing = true;
			}
		}
	}
	if(str_irr.u32_pump1_port != 0 && !b_pump_1_is_sharing)
	{
		u64_cur_valve_state &= ~(1 << (str_irr.u32_pump1_port - 1));
		u64_valve_of_pump1 &= ~(1 << (str_irr.u32_pump1_port - 1));
	}
	if(str_irr.u32_pump2_port != 0 && !b_pump_2_is_sharing)
	{
		u64_cur_valve_state &= ~(1 << (str_irr.u32_pump2_port - 1));
		u64_valve_of_pump2 &= ~(1 << (str_irr.u32_pump2_port - 1));
	}
	u64_cur_valve_state &= (u64_schedule_stop_fertilizer(str_irr, u64_cur_valve_state));
	u64_valve_of_pump1 &= (u64_schedule_stop_fertilizer(str_irr, u64_cur_valve_state));
	u64_valve_of_pump2 &= (u64_schedule_stop_fertilizer(str_irr, u64_cur_valve_state));	
#ifdef PH_EC_SENSOR_ENABLE	
	str_EC_statis.u16_MinValue = u16_get_ec_min(0);
	str_EC_statis.u16_AvgValue = u16_get_ec_avg(0);
	str_EC_statis.u16_MaxValue = u16_get_ec_max(0);
	
	str_pH_statis.u16_MinValue = u16_get_ph_min(0);
	str_pH_statis.u16_AvgValue = u16_get_ph_avg(0);
	str_pH_statis.u16_MaxValue = u16_get_ph_max(0);
	
	v_mqtt_ph_statistic_pub(str_irr.u16_location_ID, 
													str_pH_statis.u16_MinValue, str_pH_statis.u16_AvgValue, str_pH_statis.u16_MaxValue);
	v_mqtt_ec_statistic_pub(str_irr.u16_location_ID, 
													str_EC_statis.u16_MinValue, str_EC_statis.u16_AvgValue, str_EC_statis.u16_MaxValue);
	//v_pH_EC_set_state(TURN_OFF_PH_EC, 0);	
	v_reset_ec_statistic(0);
	v_reset_ph_statistic(0);
#endif	
	//Send end frame to MQTT
	v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
	v_mqtt_fertigation_finish_pub(str_irr.u16_location_ID, u32_time_begin);
	return u64_cur_valve_state;
}

/*!
* public function bodies
*/

/*
function use to read data from csv file
*/
int iHandle_CSVfileCallback(char *txtname, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c)
{
	char str[CSV_BUFFER_LENGTH_READ_LINE] = {0};
	struct csv_parser p;
	if(csv_init(&p, CSV_APPEND_NULL) != 0)
	{
		return -1;
	}
	while(i16_sd_take_semaphore(10))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(txtname, READ_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		return -2;
	}
	
	for(int i = 0; i < MAX_CSV_ROW; i++)
	{
		if(sd_read_one_line(str, CSV_BUFFER_LENGTH_READ_LINE) != NULL)
		{
			if(str[0] != CSV_COMMENT_LINE_FIRST_CHAR && str[0] != CSV_QUOTE_CHAR)
			{
				if(csv_parse(&p, str, strlen(str), cb1, cb2, &c) != strlen(str))
				{
					i16_sd_close_file();
					i16_sd_give_semaphore();
					return -3;
				}
				//clear buffer
				memset(str, 0, sizeof(str));
			}
		}
		else							//no data read (eof or error)
		{
			break;
		}
	}
	csv_fini(&p, cb1, cb2, &c);
	csv_free(&p);
	
	i16_sd_close_file();
	i16_sd_give_semaphore();
	return 0;						//sucess
}

int i_load_schedule_from_sd_card(void)
{
	//Clear old schedule
	v_clear_schedule();
	v_sche_clear_event();
	return(iHandle_CSVfileCallback(MAIN_CSV_FILE, v_LoadCSV_FieldCallback_sch, v_LoadCSV_RowCallback_sch, &c));
}

uint16_t u16_get_total_turn(void)
{
	return u16_total_row;
}
STRU_IRRIGATION get_irrigation_row(uint16_t u16_order)
{
	return astr_irrigation_table[u16_order];
}

uint32_t u32_check_event(uint32_t u32_time, uint32_t *pu32_time_begin)
{
	uint32_t u32_result = 0;
	double d_cur_volume;
	for(uint8_t i = 0; i < MAX_SCHEDULE_EVENT; i++)
	{
		if(stru_sch_event_list[i].u16_location_ID != 0 && stru_sch_event_list[i].u32_time_off != 0)		//available event
		{
			if(stru_sch_event_list[i].e_unit == UNIT_TIME)
			{
				if(u32_time >= stru_sch_event_list[i].u32_time_fer_begin)
				{
					stru_sch_event_list[i].u32_fer_remain_time = stru_sch_event_list[i].u32_time_fer_off - u32_time;
					stru_sch_event_list[i].u32_time_fer_begin = 0xFFFFFFFF;	//clear time
					stru_sch_event_list[i].b_fer_is_running = true;
					u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_START_FERTILIZER;
				}
				if(u32_time >= stru_sch_event_list[i].u32_time_fer_off)
				{
					stru_sch_event_list[i].u32_fer_remain_time = 0;
					stru_sch_event_list[i].u32_time_fer_off = 0xFFFFFFFF; //clear time
					stru_sch_event_list[i].b_fer_is_running = false;
					u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_STOP_FERTILIZER;
				}				
				if(u32_time >= stru_sch_event_list[i].u32_time_off)
				{
					stru_sch_event_list[i].u16_location_ID = 0;
					stru_sch_event_list[i].u32_time_off = 0;
					stru_sch_event_list[i].u32_total_remain_time = 0;
					stru_sch_event_list[i].b_fer_is_running = false;
					u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_STOP_EVENT;
					*pu32_time_begin = stru_sch_event_list[i].u32_time_begin;
					v_mqtt_data_pub(0xAE, SCHE_STOP_EVENT);
				}
				/* TODO: update remain time */
				if(stru_sch_event_list[i].u32_time_fer_begin != 0xFFFFFFFF)	//valve still close
				{
					stru_sch_event_list[i].u32_fer_remain_time = stru_sch_event_list[i].u32_time_fer_begin - u32_time;
				}
				else if(stru_sch_event_list[i].u32_time_fer_begin == 0xFFFFFFFF && stru_sch_event_list[i].u32_time_fer_off != 0xFFFFFFFF)	//valve opening
				{
					stru_sch_event_list[i].u32_fer_remain_time = stru_sch_event_list[i].u32_time_fer_off - u32_time;
				}
				else
				{
					stru_sch_event_list[i].u32_fer_remain_time = 0;
				}
				stru_sch_event_list[i].u32_total_remain_time = stru_sch_event_list[i].u32_time_off - u32_time;
			}
			/* TODO: Check with unit = m3 */
			else if(stru_sch_event_list[i].e_unit == UNIT_M3)
			{
				/*TODO: Read curent flowmeter volume*/
				if(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump1_port != 0)
						d_cur_volume = d_get_flow_meter(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump1_port);
				else if(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump2_port != 0)
						d_cur_volume = d_get_flow_meter(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump2_port);
				//d_cur_flow = d_cur_flow/1000.0; //L to m3
				/*TODO: compare volume with setpoint*/
				if(d_cur_volume >= stru_sch_event_list[i].u32_time_fer_begin)
				{
					//stru_sch_event_list[i].u32_fer_remain_time = stru_sch_event_list[i].u32_time_fer_off - stru_fer_event.d_main_volume;
					stru_sch_event_list[i].u32_time_fer_begin = 0xFFFFFFFF;	//clear time
					u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_START_FERTILIZER;					
				}
				if(d_cur_volume >= stru_sch_event_list[i].u32_time_fer_off)
				{
					stru_sch_event_list[i].u32_fer_remain_time = 0;
					stru_sch_event_list[i].u32_time_fer_off = 0xFFFFFFFF; //clear time
					u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_STOP_FERTILIZER;
				}
				if (d_cur_volume >= stru_sch_event_list[i].u32_time_off)
				{
					stru_sch_event_list[i].u16_location_ID = 0;
					stru_sch_event_list[i].u32_time_off = 0;
					stru_sch_event_list[i].u32_total_remain_time = 0;
					u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_STOP_EVENT;
					*pu32_time_begin = stru_sch_event_list[i].u32_time_begin;
				}
				/* TODO: update remain time */
				if(stru_sch_event_list[i].u32_time_fer_begin != 0xFFFFFFFF)	//valve still close
				{
					stru_sch_event_list[i].u32_fer_remain_time = stru_sch_event_list[i].u32_time_fer_begin - d_cur_volume;
				}
				else if(stru_sch_event_list[i].u32_time_fer_begin == 0xFFFFFFFF && stru_sch_event_list[i].u32_time_fer_off != 0xFFFFFFFF)	//valve opening
				{
					stru_sch_event_list[i].u32_fer_remain_time = stru_sch_event_list[i].u32_time_fer_off - d_cur_volume;					
				}
				else
				{
					stru_sch_event_list[i].u32_fer_remain_time = 0;
				}
				stru_sch_event_list[i].u32_total_remain_time = stru_sch_event_list[i].u32_time_off - d_cur_volume;
			}
		}
		else if(stru_sch_event_list[i].u16_location_ID != 0 && stru_sch_event_list[i].u32_time_off == 0) //event without irrigation duration
		{
			stru_sch_event_list[i].u16_location_ID = 0;
			stru_sch_event_list[i].u32_time_off = 0;
			stru_sch_event_list[i].u32_total_remain_time = 0;
			u32_result = (stru_sch_event_list[i].u16_irrigation_pointer << 8) | SCHE_STOP_EVENT;
			*pu32_time_begin = stru_sch_event_list[i].u32_time_begin;
		}
	}
	return u32_result;
}

void v_schedule_process (uint64_t* pu64_curr_valve_state)
{
	uint64_t u64_curr_valve_state_tmp = *pu64_curr_valve_state;
	static T_DATETIME stru_curr_datetime;
	static uint32_t u32_curr_time;	
	v_rtc_read_time(&stru_curr_datetime);
	u32_curr_time = stru_curr_datetime.u8_hour * 3600 + 
									stru_curr_datetime.u8_minute *60 +
									stru_curr_datetime.u8_second;
	
	switch(e_schedule_state)
	{
		case SCHE_STATE_LOAD:
		{
			u16_current_order = 0;
			e_schedule_state = SCHE_STATE_SEEK_EVENT;
		}
		break;
		case SCHE_STATE_SEEK_EVENT:
		{
			uint16_t u16_num_row = u16_get_total_turn();
			while(u32_curr_time > (astr_irrigation_table[u16_current_order].u32_time_start + 59) && u16_current_order < u16_num_row)
			{
				u16_current_order++;
				u16_ontime_order++;
			}
			if(u16_current_order > 0 && astr_irrigation_table[u16_current_order - 1].e_unit == UNIT_TIME 
				&& u32_curr_time < astr_irrigation_table[u16_current_order - 1].u32_time_end)
			{
				u16_current_order -= 1;
				b_event_type_short = true;
			}
			if(u16_current_order < u16_num_row)
			{
				e_schedule_state = SCHE_STATE_CHECK_TIME;
			}
			else
			{
				e_schedule_state = SCHE_STATE_TIME_OVER;
			}
		}
		break;
		case SCHE_STATE_TIME_OVER:
		{
			static bool b_pub = false;
			if (!b_pub)
			{
				b_pub = true;
				v_mqtt_data_pub(0xffff, 0xffff);
			}
		}
		break;
		case SCHE_STATE_CHECK_TIME:
		{
			/* TODO: check event in event table */
			uint32_t u32_temp_time;
			uint32_t u32_even_status = u32_check_event(u32_curr_time, &u32_temp_time);
			if(u32_even_status != 0)
			{
				switch(u32_even_status & 0xFF)
				{
					case SCHE_START_FERTILIZER:
					{
						#ifdef PH_EC_SENSOR_ENABLE
						v_pH_EC_set_state(TURN_ON_PH_EC, INTERVAL_GET_PH_EC);
						u8_start_ec_statistic();
						u8_start_ph_statistic();
						#endif												
						u64_curr_valve_state_tmp |= u64_schedule_start_fertilizer(astr_irrigation_table[u32_even_status >> 8], u64_curr_valve_state_tmp);
					}
					break;
					case SCHE_STOP_FERTILIZER:
					{
						#ifdef PH_EC_SENSOR_ENABLE
						v_pH_EC_set_state(TURN_OFF_PH_EC, 0);	
						#endif
						u64_curr_valve_state_tmp &= (u64_schedule_stop_fertilizer(astr_irrigation_table[u32_even_status >> 8], u64_curr_valve_state_tmp));
					}
					break;
					case SCHE_STOP_EVENT:
					{
						#ifdef PH_EC_SENSOR_ENABLE
						v_pH_EC_set_state(TURN_OFF_PH_EC, 0);	
						#endif
						
						T_DATETIME t_time;  
						v_rtc_read_time(&t_time);
						uint32_t u32_today = u32_datetime_2_long(&t_time);
						u32_today = u32_today - (u32_today % 86400);
						double d_cur_volume_static = 0;
						double d_fertilizer_volume[4] = {0,0,0,0};
						if(astr_irrigation_table[u32_even_status >> 8].u32_pump1_port != 0)
						{
							d_cur_volume_static = d_get_flow_meter(astr_irrigation_table[u32_even_status >> 8].u32_pump1_port);
							v_clear_flow_meter(astr_irrigation_table[u32_even_status >> 8].u32_pump1_port);
						}
						else if(astr_irrigation_table[u32_even_status >> 8].u32_pump2_port != 0)
						{
							d_cur_volume_static = d_get_flow_meter(astr_irrigation_table[u32_even_status >> 8].u32_pump2_port);
							v_clear_flow_meter(astr_irrigation_table[u32_even_status >> 8].u32_pump2_port);
						}
						d_cur_volume_static = d_cur_volume_static/1000.0; //L to m3	
						//read volume of fertilizer
						for(uint8_t i = 0; i < 4; i++)
						{
							d_fertilizer_volume[i] = d_get_flow_meter(astr_irrigation_table[u32_even_status >> 8].au8_fer_port[i]);
							v_clear_flow_meter(astr_irrigation_table[u32_even_status >> 8].au8_fer_port[i]);
						}					
						u64_curr_valve_state_tmp &= (u64_schedule_stop(astr_irrigation_table[u32_even_status >> 8], u64_curr_valve_state_tmp, (u32_temp_time + u32_today - u16_get_time_zone()*60)));
					}
					break;
					default: break;
				}
			}
			/* TODO: check potential event */
			if(u16_current_order < u16_get_total_turn())
			{
				if(u32_curr_time < astr_irrigation_table[u16_current_order].u32_time_start)
				{
					LED_DB_1_ON();
				}
				if(u32_curr_time >= astr_irrigation_table[u16_current_order].u32_time_start)
				{
					LED_DB_1_OFF();
					//todo: debug event order
					//v_mqtt_data_pub(0, u16_current_order);
					//v_mqtt_data_pub(1, u32_curr_time);				
					//v_mqtt_data_pub(2, astr_irrigation_table[u16_current_order].u32_time_start);
					static STRU_SCH_EVENT irr_event;
					irr_event.u16_irrigation_pointer = u16_current_order;
					irr_event.u16_location_ID = astr_irrigation_table[u16_current_order].u16_location_ID;
					irr_event.e_unit = astr_irrigation_table[u16_current_order].e_unit;
					irr_event.u32_time_begin = u32_curr_time;
					memcpy(irr_event.au32_fer_rate, astr_irrigation_table[u16_current_order].au32_dosing_rate, 5*sizeof(uint32_t));
					if(!b_event_type_short)
					{
						if(astr_irrigation_table[u16_current_order].e_unit == UNIT_TIME)
						{
							irr_event.u32_time_off = u32_curr_time + astr_irrigation_table[u16_current_order].u32_time_run;
							if(astr_irrigation_table[u16_current_order].b_fer_enable)
							{
								irr_event.u32_time_fer_begin = u32_curr_time + astr_irrigation_table[u16_current_order].u32_time_fer_start;
								irr_event.u32_time_fer_off = astr_irrigation_table[u16_current_order].u32_time_end - astr_irrigation_table[u16_current_order].u32_time_fer_stop;
								irr_event.u32_time_fer_total = irr_event.u32_time_fer_off - irr_event.u32_time_fer_begin;
							}
							else
							{
								irr_event.u32_time_fer_begin = 0xFFFFFFFF;
								irr_event.u32_time_fer_off = 0xFFFFFFFF;
							}
						}
						else
						{
							irr_event.u32_time_off = astr_irrigation_table[u16_current_order].u32_time_run;
							if(astr_irrigation_table[u16_current_order].b_fer_enable)
							{
								irr_event.u32_time_fer_begin = astr_irrigation_table[u16_current_order].u32_time_fer_start;
								irr_event.u32_time_fer_off = irr_event.u32_time_off;
							}
							else
							{
								irr_event.u32_time_fer_begin = 0xFFFFFFFF;
								irr_event.u32_time_fer_off = 0xFFFFFFFF;
							}
						}
					}
					else
					{
						b_event_type_short = false;
						if(astr_irrigation_table[u16_current_order].e_unit == UNIT_TIME)
						{
							irr_event.u32_time_off = astr_irrigation_table[u16_current_order].u32_time_start + astr_irrigation_table[u16_current_order].u32_time_run;
							if(astr_irrigation_table[u16_current_order].b_fer_enable)
							{
								irr_event.u32_time_fer_begin = astr_irrigation_table[u16_current_order].u32_time_start + astr_irrigation_table[u16_current_order].u32_time_fer_start;
								irr_event.u32_time_fer_off = astr_irrigation_table[u16_current_order].u32_time_start + astr_irrigation_table[u16_current_order].u32_time_fer_stop;
							}
							else
							{
								irr_event.u32_time_fer_begin = 0xFFFFFFFF;
								irr_event.u32_time_fer_off = 0xFFFFFFFF;
							}
						} 
						else
						{
							irr_event.u32_time_off = astr_irrigation_table[u16_current_order].u32_time_run;
							if(astr_irrigation_table[u16_current_order].b_fer_enable)
							{
								irr_event.u32_time_fer_begin = astr_irrigation_table[u16_current_order].u32_time_fer_start;
								irr_event.u32_time_fer_off = irr_event.u32_time_off;
							}
							else
							{
								irr_event.u32_time_fer_begin = 0xFFFFFFFF;
								irr_event.u32_time_fer_off = 0xFFFFFFFF;
							}
						}
					}
					irr_event.b_fer_is_running = false;
					/* TODO: add event to event table */
					if(b_schedule_event_reg(irr_event))
					{
						astr_irrigation_table[u16_current_order].u32_time_start = 0xFFFFFFFF;
						/* TODO: start event */
						u64_curr_valve_state_tmp |= u64_schedule_start_event(astr_irrigation_table[u16_current_order], u64_curr_valve_state_tmp);
						/* TODO: increase pointer */
						u16_current_order++;
						while(astr_irrigation_table[u16_current_order].u32_time_start == 0xFFFFFFFF)
						{
							u16_current_event++;
						}
					}
					//todo: debug event order					
					//v_mqtt_data_pub(3, u16_current_order);
					e_schedule_state = SCHE_STATE_CHECK_TIME;
				}
			}
			if (OPERATING_MODE == E_SCHEDULE_MODE)
			{
				if(u16_ontime_order < u16_get_total_turn())
				{
					if(astr_irrigation_table[u16_ontime_order].u32_time_start == 0xFFFFFFFF)
					{
						u16_ontime_order++;
					}
					if(u32_curr_time >= astr_irrigation_table[u16_ontime_order].u32_time_start)
					{
							if(astr_irrigation_table[u16_ontime_order].u32_pump1_port == 0 
									&& astr_irrigation_table[u16_ontime_order].u32_pump2_port == 0)
							{
								STRU_SCH_EVENT irr_event;
								astr_irrigation_table[u16_ontime_order].u32_time_start = 0xFFFFFFFF;
								irr_event.u16_irrigation_pointer = u16_ontime_order;
								irr_event.u16_location_ID = astr_irrigation_table[u16_ontime_order].u16_location_ID;
								irr_event.e_unit = astr_irrigation_table[u16_ontime_order].e_unit;
								irr_event.u32_time_begin = u32_curr_time;
								irr_event.u32_time_off = u32_curr_time + astr_irrigation_table[u16_ontime_order].u32_time_run;
								if(astr_irrigation_table[u16_ontime_order].b_fer_enable)
								{
									irr_event.u32_time_fer_begin = u32_curr_time + astr_irrigation_table[u16_ontime_order].u32_time_fer_start;
									irr_event.u32_time_fer_off = u32_curr_time + astr_irrigation_table[u16_ontime_order].u32_time_fer_stop;
								}
								else
								{
									irr_event.u32_time_fer_begin = 0xFFFFFFFF;
									irr_event.u32_time_fer_off = 0xFFFFFFFF;
								}
								irr_event.b_fer_is_running = false;
								/* TODO: add event to event table */
								if(b_schedule_event_reg(irr_event))
								{
									/* TODO: start event */
									u64_curr_valve_state_tmp |= u64_schedule_start_event(astr_irrigation_table[u16_ontime_order], u64_curr_valve_state_tmp);
								}
							}
							/* TODO: increase pointer */
							u16_ontime_order++;                                                                        
					}
				}
				
			}
		}
		break;
		default: break;
	}
	//update current valve state
	if(u64_curr_valve_state_tmp == 0)
		b_sche_is_running = false;
	else
		b_sche_is_running = true;
	*pu64_curr_valve_state = u64_curr_valve_state_tmp;
}	

/*!
* Return the status of the machine is running or not
*/
bool b_shedule_is_running(void)
{
	return b_sche_is_running;
}

void v_sche_reload_event(uint64_t *pu64_curr_valve_state)
{
	uint64_t u64_curr_valve_state_tmp = *pu64_curr_valve_state;
	static T_DATETIME stru_curr_datetime;
	static uint32_t u32_curr_time;
	v_rtc_read_time(&stru_curr_datetime);
	u32_curr_time = stru_curr_datetime.u8_hour * 3600 + 
									stru_curr_datetime.u8_minute *60 +
									stru_curr_datetime.u8_second;
	for(int i = 0; i < MAX_SCHEDULE_EVENT; i++)
	{
		if(stru_sch_event_list[i].u16_location_ID != 0 && stru_sch_event_list[i].u32_time_off != 0)
		{
			if(stru_sch_event_list[i].e_unit == UNIT_TIME)
			{
				/* TODO: update new stop time */
				stru_sch_event_list[i].u32_time_off = u32_curr_time + stru_sch_event_list[i].u32_total_remain_time;
				u64_curr_valve_state_tmp |= u64_schedule_start_event(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer], u64_curr_valve_state_tmp);
				/* TODO: check time to open fertilizer valve */
				if(stru_sch_event_list[i].u32_time_fer_begin != 0xFFFFFFFF)		//still close
				{
					stru_sch_event_list[i].u32_time_fer_off = stru_sch_event_list[i].u32_time_fer_off - stru_sch_event_list[i].u32_time_fer_begin;
					stru_sch_event_list[i].u32_time_fer_begin = u32_curr_time + stru_sch_event_list[i].u32_fer_remain_time;
					stru_sch_event_list[i].u32_time_fer_off = stru_sch_event_list[i].u32_time_fer_off + stru_sch_event_list[i].u32_time_fer_begin;
				}
				else if(stru_sch_event_list[i].u32_time_fer_begin == 0xFFFFFFFF && stru_sch_event_list[i].u32_time_fer_off != 0xFFFFFFFF)	//openning
				{
					/* TODO: update new stop fertilizer time */
					stru_sch_event_list[i].u32_time_fer_off = u32_curr_time + stru_sch_event_list[i].u32_fer_remain_time;
					/* TODO: open valve */
					u64_curr_valve_state_tmp |= u64_schedule_start_fertilizer(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer], u64_curr_valve_state_tmp);
				}
			}
			else if(stru_sch_event_list[i].e_unit == UNIT_M3)
			{
				/* TODO: update new stop time */
				double d_cur_flow_value = 0;
				if(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump1_port != 0)
						d_cur_flow_value = d_get_flow_meter(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump1_port);
				else if(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump2_port != 0)
						d_cur_flow_value = d_get_flow_meter(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer].u32_pump2_port);
				d_cur_flow_value = d_cur_flow_value / 1000.0 * 10; //L to m3 then * 10 to rescale unit
				stru_sch_event_list[i].u32_time_off = d_cur_flow_value + stru_sch_event_list[i].u32_total_remain_time;
				u64_curr_valve_state_tmp |= u64_schedule_start_event(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer], u64_curr_valve_state_tmp);
				/* TODO: check time to open fertilizer valve */
				if(stru_sch_event_list[i].u32_time_fer_begin != 0xFFFFFFFF)		//still close
				{
					stru_sch_event_list[i].u32_time_fer_off = stru_sch_event_list[i].u32_time_fer_off - stru_sch_event_list[i].u32_time_fer_begin;
					stru_sch_event_list[i].u32_time_fer_begin = d_cur_flow_value + stru_sch_event_list[i].u32_fer_remain_time;
					stru_sch_event_list[i].u32_time_fer_off = stru_sch_event_list[i].u32_time_fer_off + stru_sch_event_list[i].u32_time_fer_begin;
				}
				else if(stru_sch_event_list[i].u32_time_fer_begin == 0xFFFFFFFF && stru_sch_event_list[i].u32_time_fer_off != 0xFFFFFFFF)	//openning
				{
					/* TODO: update new stop fertilizer time */
					stru_sch_event_list[i].u32_time_fer_off = d_cur_flow_value + stru_sch_event_list[i].u32_fer_remain_time;
					/* TODO: open valve */
					u64_curr_valve_state_tmp |= u64_schedule_start_fertilizer(astr_irrigation_table[stru_sch_event_list[i].u16_irrigation_pointer], u64_curr_valve_state_tmp);
				}
			}
		}
	}
	*pu64_curr_valve_state = u64_curr_valve_state_tmp;
}

