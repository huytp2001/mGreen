/*! @file threshold.c
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
#include "threshold.h"
#include "schedule.h"
#include "csv.h"
#include "storage.h"
#include "sensor.h"
#include "spi_mgreen.h"
#include "pHEC.h"
#include "frame_parser.h"
//#include "frame_builder.h"
#include "mqtt_publish.h"
#include "wdt.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "sd_card.h"
#include "mqtt_publish.h"
#include "rtc.h"
#include "sd_manage_lib.h"
/*!
* static data declaration
*/
static STRU_THD astru_thd[MAX_THD_ROW];
static STRU_THD_LOG_RUNTIME astru_thd_log_runtime[MAX_THD_ROW];
static uint16_t u16_total_thd_row = 0;
static uint8_t u8_csv_thd_field_counter = 0;
static char csv_thd_version[21];
struct STRU_CSV_COUNT stru_csv;

static void v_set_csv_threshold_err_flag(E_CSV_ERR_CODE e_err_flag, uint16_t u16_row)
{
}

/*!
* private function prototype
*/
static void v_clear_thd(void);
static bool b_parse_threshold_csv(uint8_t u8_field, char *s);
static void v_LoadTHDCSV_FieldCallback(void *s, size_t len, void *data);
static void v_LoadTHDCSV_RowCallback(int c, void * data);
/*!
* public function bodies
*/
/*!
 * @fn 
 * @brief 
 * @param[in] none
 * @return none
 */
/*!
* private function bodies
*/
static void v_clear_thd(void)
{
	for(uint16_t i = 0; i < MAX_THD_ROW; i++)
	{
		astru_thd[i].u64_port = 0;
		astru_thd[i].e_thd_act_type = THD_ACT_NONE;
		astru_thd[i].b_on_duty = false;
		astru_thd[i].b_pre_on_duty = false;
		for (uint8_t u8_j = 0; u8_j < MAX_THD_DATA; u8_j++)
		{
			astru_thd[i].astru_thd_data[u8_j].u32_node_ID = 0;
			astru_thd[i].astru_thd_data[u8_j].u16_sensor_ID = 0;
			astru_thd[i].astru_thd_data[u8_j].e_thd_cdt_type = THD_CDT_NONE;
			astru_thd[i].astru_thd_data[u8_j].u32_thd_value = 0;
			astru_thd[i].astru_thd_data[u8_j].b_has_data = false;
			astru_thd[i].astru_thd_data[u8_j].u32_curr_value = 0;
			astru_thd[i].astru_thd_data[u8_j].b_status = false;
		}
		u16_total_thd_row = 0;
		u8_csv_thd_field_counter = 0;
		stru_csv.u16_fields = 0;
		stru_csv.u16_rows = 0;
		
		//runtime log
		astru_thd_log_runtime[i].u16_location_id = 0;
		astru_thd_log_runtime[i].u32_time_runed = 0;
		astru_thd_log_runtime[i].u32_unix_time_start = 0;
	}
}

static bool b_parse_thd_csv(uint8_t u8_field, char *s)
{	
	if(u8_field < THD_IDX_NODE_ID_1)
	{
		switch(u8_field)
		{
			case THD_IDX_LOCAL_ID:
			{
				u16_total_thd_row += 1;
				astru_thd[u16_total_thd_row - 1].u16_location_id = strtoul(s, 0, 10);
			}
			break;			
			case THD_IDX_PORT_L:
			{		
				if(u16_total_thd_row > MAX_THD_ROW)
				{
					u16_total_thd_row = MAX_THD_ROW;
				}
				uint32_t u32_port_low = strtoul(s, 0, 10);
				astru_thd[u16_total_thd_row - 1].u64_port |= u32_port_low;
			}
			break;
			case THD_IDX_PORT_H:
			{
				uint64_t u64_port_high = strtoul(s, 0, 10);
				astru_thd[u16_total_thd_row - 1].u64_port |= u64_port_high << 32;				
			}
			break;
			case THD_IDX_ACT:
			{
				astru_thd[u16_total_thd_row - 1].e_thd_act_type = (E_THD_ACT_TYPE)strtoul(s, 0, 10);
			}
			break;
		}
	}
	else if(u8_field >= THD_IDX_NODE_ID_1)
	{
		uint8_t u8_thd_data_idx = 0;
		u8_thd_data_idx = (u8_field - THD_IDX_NODE_ID_1)/(THD_IDX_VALUE_1 - THD_IDX_NODE_ID_1 + 1);
		if(u8_thd_data_idx >= MAX_THD_DATA)
		{
			u8_thd_data_idx = MAX_THD_DATA - 1;
		}
		uint8_t u8_data_field = 0;
		u8_data_field = (u8_field - THD_IDX_NODE_ID_1)%(THD_IDX_VALUE_1 - THD_IDX_NODE_ID_1 + 1)
										+ THD_IDX_NODE_ID_1;
		switch(u8_data_field)
		{
			case THD_IDX_NODE_ID_1:
			{
				astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].u32_node_ID = 
																				strtoul(s, 0, 10);
				if(astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].u32_node_ID
					!= 0)
				{
					astru_thd[u16_total_thd_row - 1].u8_num_thd_data = u8_thd_data_idx + 1;
				}
			}
			break;
			case THD_IDX_SENSOR_ID_1:
			{
				astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].u16_sensor_ID = 
																				strtoul(s, 0, 10);
			}
			break;		
			case THD_IDX_CDT_1:
			{
				if(astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].u32_node_ID
					== 0)
				{
					astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].e_thd_cdt_type =
					THD_CDT_NONE;
				}
				else
				{
					astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].e_thd_cdt_type = 
																				(E_THD_CDT_TYPE)strtoul(s, 0, 10);
				}
			}
			break;		
			case THD_IDX_VALUE_1:
			{
				astru_thd[u16_total_thd_row - 1].astru_thd_data[u8_thd_data_idx].u32_thd_value = 
																				strtoul(s, 0, 10);
			}
			break;			
		}
	}
	return true;
}

static void v_LoadTHDCSV_FieldCallback(void *s, size_t len, void *data)
{
	static uint8_t u8_type_of_data = 0;
	((struct STRU_CSV_COUNT *)data)->u16_fields++;
	if(s != NULL)
	{
		if(u8_csv_thd_field_counter == 0)
		{
			u8_type_of_data = strtoul(s, 0, 10);
		}
		switch(u8_type_of_data)
		{
			case 0:							//csv version
			{
				if(u8_csv_thd_field_counter == 1)
				{
					memcpy((void *)csv_thd_version, (void *)s, strlen(s) + 1);
				}
			}
			break;
			case 1:						//fertilizer tank setting
			{
			}
			break;
			case 5:							//threshold data
			{
				b_parse_thd_csv(u8_csv_thd_field_counter, s);
			}
			break;
		}
		u8_csv_thd_field_counter++;
	}
}
static void v_LoadTHDCSV_RowCallback(int c, void * data)
{
	u8_csv_thd_field_counter = 0;
	((struct STRU_CSV_COUNT *)data)->u16_rows++;
}
/*!
* public function bodies
*/
/*
function use to read data from csv file
*/
int iHandle_CSVTHDfileCallback(char *txtname, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c)
{
	char str[CSV_BUFFER_LENGTH_READ_LINE] = {0};
	struct csv_parser p;
	if(csv_init(&p, CSV_APPEND_NULL) != 0)
	{
		return -1;
	}
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
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

int i_load_thd_from_sd_card(void)
{
	//Clear old schedule
	v_clear_thd();
	return(iHandle_CSVTHDfileCallback(MAIN_THD_FILE, v_LoadTHDCSV_FieldCallback, 
																						v_LoadTHDCSV_RowCallback, &stru_csv));
}

void v_apppend_sensor_data_to_thd_table(mqtt_data_frame_t stru_sensor_data)
{
	for(uint8_t u8_i = 0; u8_i < u16_total_thd_row; u8_i++)
	{
		for(uint8_t u8_j = 0; u8_j < astru_thd[u8_i].u8_num_thd_data; u8_j++)
		{
			if(astru_thd[u8_i].astru_thd_data[u8_j].u32_node_ID == stru_sensor_data.u32_node_id)
			{
				for(uint8_t u8_k = 0; u8_k < stru_sensor_data.u8_num_data_field; u8_k++)
				{
					if(astru_thd[u8_i].astru_thd_data[u8_j].u16_sensor_ID == 
						stru_sensor_data.stru_data_field[u8_k].u16_id)
					{
						switch(astru_thd[u8_i].astru_thd_data[u8_j].u16_sensor_ID)
						{
							case DATA_ID_ALS:
							{					
								astru_thd[u8_i].astru_thd_data[u8_j].u32_curr_value = 
								u32_convert_raw_als_to_lux((uint16_t)
								stru_sensor_data.stru_data_field[u8_k].u32_value);		
							}
							break;
							default:
							{			
								astru_thd[u8_i].astru_thd_data[u8_j].u32_curr_value = 
								stru_sensor_data.stru_data_field[u8_k].u32_value;	
							}
							break;
						}
						astru_thd[u8_i].astru_thd_data[u8_j].b_has_data = true;
					}
				}
			}
		}		
	}
}


void v_threshold_process (uint64_t* pu64_curr_port_state)
{
	uint64_t u64_curr_port_state_tmp = *pu64_curr_port_state;;
	for(uint8_t u8_i = 0; u8_i < u16_total_thd_row; u8_i++)
	{
		for(uint8_t u8_j = 0; u8_j < astru_thd[u8_i].u8_num_thd_data; u8_j++)
		{
			if(astru_thd[u8_i].astru_thd_data[u8_j].b_has_data == true)
			{
				if(astru_thd[u8_i].astru_thd_data[u8_j].e_thd_cdt_type == THD_CDT_L)
				{
					if(astru_thd[u8_i].astru_thd_data[u8_j].u32_curr_value < 
						astru_thd[u8_i].astru_thd_data[u8_j].u32_thd_value)
					{
						astru_thd[u8_i].astru_thd_data[u8_j].b_status = true;
					}
					else
					{
						astru_thd[u8_i].astru_thd_data[u8_j].b_status = false;
					}
				}
				else if(astru_thd[u8_i].astru_thd_data[u8_j].e_thd_cdt_type == THD_CDT_H)
				{
					if(astru_thd[u8_i].astru_thd_data[u8_j].u32_curr_value > 
						astru_thd[u8_i].astru_thd_data[u8_j].u32_thd_value)
					{
						astru_thd[u8_i].astru_thd_data[u8_j].b_status = true;
					}
					else
					{
						astru_thd[u8_i].astru_thd_data[u8_j].b_status = false;
					}				
				}
			}
			if(u8_j == 0)
			{
				astru_thd[u8_i].b_on_duty = astru_thd[u8_i].astru_thd_data[u8_j].b_status;
			}
			else
			{
				astru_thd[u8_i].b_on_duty &= astru_thd[u8_i].astru_thd_data[u8_j].b_status;
			}
		}
		if(astru_thd[u8_i].b_on_duty == true)
		{
			if(astru_thd[u8_i].e_thd_act_type == THD_ACT_OFF)
			{
				u64_curr_port_state_tmp &= ~astru_thd[u8_i].u64_port;
			}
			else if(astru_thd[u8_i].e_thd_act_type == THD_ACT_ON)
			{
				u64_curr_port_state_tmp |= astru_thd[u8_i].u64_port;
			}
			if(astru_thd[u8_i].b_pre_on_duty != astru_thd[u8_i].b_on_duty)
			{
				//TODO: Notify to MQTT
				//b_mqtt_publish_threshold_event(astru_thd[u8_i]);						
				if(astru_thd[u8_i].e_thd_act_type == THD_ACT_ON)
				{
					//TODO: Send start frame to MQTT
					T_DATETIME t_time;  
					v_rtc_read_time(&t_time);
					astru_thd[u8_i].u32_unix_time_start = u32_datetime_2_long(&t_time);
					if(b_add_time_start_to_thd_log_runtime(astru_thd[u8_i].u16_location_id,
																							astru_thd[u8_i].u32_unix_time_start))
					{
						v_mqtt_fertigation_start(astru_thd[u8_i].u16_location_id);
					}
				}
				else if(astru_thd[u8_i].e_thd_act_type == THD_ACT_OFF)
				{
					//TODO: send stop frame to MQTT
					if(b_get_time_start_from_thd_log_runtime(astru_thd[u8_i].u16_location_id, 
																								&astru_thd[u8_i].u32_unix_time_start))
					{
						b_clear_time_start_from_thd_log_runtime(astru_thd[u8_i].u16_location_id);
						v_mqtt_fertigation_finish_pub(astru_thd[u8_i].u16_location_id, astru_thd[u8_i].u32_unix_time_start);
					}
				}
			}		
		}
		astru_thd[u8_i].b_pre_on_duty = astru_thd[u8_i].b_on_duty;			
	}
	//update current valve state
	*pu64_curr_port_state = u64_curr_port_state_tmp;
}	

uint32_t u32_convert_raw_als_to_lux(uint16_t u16_raw_als)
{
	uint32_t u32_exponent = (u16_raw_als & 0xF000)  >> 12;
	uint32_t u32_body = u16_raw_als & 0x0FFF;
	u32_exponent = (uint32_t)0x00000001 << u32_exponent;
	u32_body = u32_exponent*u32_body/10;
	if(u32_body > 838656)
	{
		u32_body = 838656;
	}
	return (u32_body);
}

bool b_add_time_start_to_thd_log_runtime(uint16_t u16_location_id,
																				 uint32_t u32_unixtime_start)
{
	for(uint8_t u8_i = 0; u8_i < MAX_THD_ROW; u8_i++)
	{
		if(astru_thd_log_runtime[u8_i].u16_location_id == u16_location_id)
			return false;	
	}
	for(uint8_t u8_i = 0; u8_i < MAX_THD_ROW; u8_i++)
	{
		if(astru_thd_log_runtime[u8_i].u16_location_id == 0)
		{
			astru_thd_log_runtime[u8_i].u16_location_id = u16_location_id;
			astru_thd_log_runtime[u8_i].u32_unix_time_start = u32_unixtime_start;
			astru_thd_log_runtime[u8_i].u32_time_runed = 0;
			return true;
		}		
	}
	return false;
}

bool b_get_time_start_from_thd_log_runtime(uint16_t u16_location_id, 
																					uint32_t* pu32_unix_time_start)
{
	for(uint8_t u8_i = 0; u8_i < MAX_THD_ROW; u8_i++)
	{
		if(astru_thd_log_runtime[u8_i].u16_location_id == u16_location_id)
		{
			*pu32_unix_time_start = astru_thd_log_runtime[u8_i].u32_unix_time_start;
			return true;
		}
	}
	return false;		
}

bool b_clear_time_start_from_thd_log_runtime(uint16_t u16_location_id)
{
	for(uint8_t u8_i = 0; u8_i < MAX_THD_ROW; u8_i++)
	{
		if(astru_thd_log_runtime[u8_i].u16_location_id == u16_location_id)
		{
			astru_thd_log_runtime[u8_i].u16_location_id = 0;
			astru_thd_log_runtime[u8_i].u32_unix_time_start = 0;
			astru_thd_log_runtime[u8_i].u32_time_runed = 0;
			return true;
		}
	}
	return false;	
}


