/*! @file vpd.c
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
#include "math.h"
#include "rtc.h"
#include "vpd_control.h"
#include "eeprom_manage.h"
#include "mqtt_publish.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define VPD_MAX_QUEUE					2
#define VPD_MAX_THRESH				11		//milibar
#define VPD_MIN_THRESH				7
#define MIN_TEMPERATURE				15
#define MAX_TEMPERATURE				35

#define MIN_TEMPERATURE_THRESH 28
#define MAX_TEMPERATURE_THRESH 30

#define VPD_QUEUE_LEN						5
/*!
* static data declaration
*/
static xQueueHandle x_vpd_Queue;
static StaticQueue_t xQVpdQueueBuffer;
static uint8_t ucQVpdStorage[3 * 8 * VPD_QUEUE_LEN];

static E_VPD_STATE e_vpd_state = VPD_IDLE;
static uint32_t u32_vpd_start_time = 28800;	//07:00
static uint32_t u32_vpd_stop_time	= 57600;	//17:00
static uint32_t u32_vpd_t_min = 25;
static uint32_t u32_vpd_t_max = 28;
static uint32_t u32_node_ID_set = 2691;
static bool b_fans_status = false;
static bool b_coolingpad_status = false;
//temperature (oC)									15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35	
static uint8_t vpd_minRh_table[] = {20, 20, 25, 30, 30, 35, 35, 40, 40, 45, 45, 50, 50, 50, 50, 55, 55, 60, 60, 60, 60};
static uint8_t vpd_maxRh_table[] = {45, 45, 50, 50, 50, 55, 55, 55, 55, 60, 60, 65, 65, 65, 65, 70, 70, 75, 75, 75, 75};
/*!
* private function bodies
*/
static void v_vpd_load_config(void)
{
	uint32_t u32_valid_value = 0;
	EEPROMRead(&u32_valid_value, EEPROM_VPD_VALID_ADDR, sizeof(uint32_t));
	if(u32_valid_value == 1)
	{
		EEPROMRead(&u32_vpd_start_time, EEPROM_VPD_START_TIME_ADDR, sizeof(uint32_t));
		EEPROMRead(&u32_vpd_stop_time, EEPROM_VPD_STOP_TIME_ADDR, sizeof(uint32_t));
		EEPROMRead(&u32_vpd_t_min, EEPROM_VPD_MIN_TEMP_ADDR, sizeof(uint32_t));
		EEPROMRead(&u32_vpd_t_max, EEPROM_VPD_MAX_TEMP_ADDR, sizeof(uint32_t));
		EEPROMRead(&u32_node_ID_set, EEPROM_VPD_NODE_ID_ADDR, sizeof(uint32_t));
	}
	else
	{
		u32_valid_value = 1;
		EEPROMProgram(&u32_valid_value, EEPROM_VPD_VALID_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_vpd_start_time, EEPROM_VPD_START_TIME_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_vpd_stop_time, EEPROM_VPD_STOP_TIME_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_vpd_t_min, EEPROM_VPD_MIN_TEMP_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_vpd_t_max, EEPROM_VPD_MAX_TEMP_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_node_ID_set, EEPROM_VPD_NODE_ID_ADDR, sizeof(uint32_t));
	}
}
/*!
* public function bodies
*/
void v_vpd_publish_params(void)
{
	v_mqtt_data_pub(DATAID_FAN_STATUS,(int)b_coolingpad_status);
	v_mqtt_data_pub(DATAID_COOLINGPAD_STATUS, (int)b_coolingpad_status);
	v_mqtt_data_pub(DATAID_VPD_MONITOR_NODE, u32_node_ID_set);
	v_mqtt_data_pub(DATAID_VPD_START_TIME, u32_vpd_start_time);
	v_mqtt_data_pub(DATAID_VPD_STOP_TIME, u32_vpd_t_min);
	v_mqtt_data_pub(DATAID_VPD_MIN_TEMP, u32_vpd_start_time);
	v_mqtt_data_pub(DATAID_VPD_MAX_TEMP, u32_vpd_t_max);
}
void v_vpd_set_node_id(uint32_t u32_node_id)
{	
	u32_node_ID_set = u32_node_id;
	EEPROMProgram(&u32_node_id, EEPROM_VPD_NODE_ID_ADDR, sizeof(uint32_t));
}

void v_vpd_set_start_time(uint32_t u32_start_time)
{
	u32_vpd_start_time = u32_start_time;
	EEPROMProgram(&u32_vpd_start_time, EEPROM_VPD_START_TIME_ADDR, sizeof(uint32_t));
}

void v_vpd_set_stop_time(uint32_t u32_stop_time)
{
	u32_vpd_stop_time = u32_stop_time;
	EEPROMProgram(&u32_vpd_stop_time, EEPROM_VPD_STOP_TIME_ADDR, sizeof(uint32_t));
}

void v_vpd_set_min_temperature(uint32_t u32_min_t)
{
	u32_vpd_t_min = u32_min_t;
	EEPROMProgram(&u32_vpd_t_min, EEPROM_VPD_MIN_TEMP_ADDR, sizeof(uint32_t));
}

void v_vpd_set_max_temperature(uint32_t u32_max_t)
{
	u32_vpd_t_max = u32_max_t;
	EEPROMProgram(&u32_vpd_t_max, EEPROM_VPD_MAX_TEMP_ADDR, sizeof(uint32_t));
}

void v_vpd_process_init(void)
{
	v_vpd_load_config();
	x_vpd_Queue = xQueueCreateStatic(VPD_QUEUE_LEN, 3 * sizeof(double), &( ucQVpdStorage[ 0 ]), &xQVpdQueueBuffer);	
}
void v_update_vpd_value(uint32_t u32_node_ID, double d_T, double d_RH)
{
	/*
		def calculate_es(T):
			return 6.1078 * math.exp((17.269 * T) / (237.3 + T))
		def calculate_vpd(es, rh):
			return es - (rh * es/100)
		float f_vpd_value = 0;
	*/
	if(u32_node_ID == u32_node_ID_set)
	{
		double es = 0, vpd[3];
		es = 6.1078 * exp((17.269 * d_T) / (237.3 + d_T));
		vpd[0] = es - (d_RH * es/100);
		vpd[1] = d_T;
		vpd[2] = d_RH;
		if(x_vpd_Queue != NULL)
		{
			xQueueSend(x_vpd_Queue, &vpd, (portTickType)(10 / portTICK_RATE_MS));
		}
	}
}
void v_vpd_process(uint64_t *u64_port_control_value)
{
	uint64_t u64_temp_port_value = *u64_port_control_value;
	T_DATETIME t_time;
	v_rtc_read_time(&t_time);
	uint32_t u32_cur_time = u32_datetime_2_long(&t_time) % 86400;
	double vpd[3];
	static uint8_t u8_t, u8_rh;
	
	if(x_vpd_Queue == NULL)
	{
		v_vpd_process_init();
	}
	switch(e_vpd_state)
	{
		case VPD_IDLE:		//check time
		{
			if(u32_cur_time >= u32_vpd_start_time && u32_cur_time <= u32_vpd_stop_time)
			{
					e_vpd_state = VPD_RUN;
			}
			else
			{
				e_vpd_state = VPD_REQUEST_STOP;
			}
			if(xQueueReceive(x_vpd_Queue, &vpd, 10) == pdPASS)
			{
				//read to clear queue
			}
		}break;
		case VPD_RUN:
		{
			if(u32_cur_time < u32_vpd_start_time || u32_cur_time > u32_vpd_stop_time)
			{
					e_vpd_state = VPD_REQUEST_STOP;
			}
			else
			{
				if(xQueueReceive(x_vpd_Queue, &vpd, 10) == pdPASS)
				{
					v_mqtt_data_pub(DATAID_VPD, vpd[0] * 10);
					u8_t = (uint8_t)vpd[1];
					u8_rh = (uint8_t)vpd[2];
					if(vpd[0] >= VPD_MAX_THRESH || vpd[0] <= VPD_MIN_THRESH)
					{
						/* TODO: Turnon Fans */
						u64_temp_port_value |= (1 << (VPD_CONTROL_FAN_PORT - 1));
						
						if(u8_rh < vpd_minRh_table[u8_t - MIN_TEMPERATURE])
						{
							/* TODO: turn on cooling pad */
							u64_temp_port_value |= (1 << (VPD_CONTROL_PUMP_PORT - 1));
						}
						else if(u8_rh > vpd_maxRh_table[u8_t - MIN_TEMPERATURE])
						{
							/* TODO: turn off cooling pad */
							u64_temp_port_value &= ~(1 << (VPD_CONTROL_PUMP_PORT - 1));
						}
					}
					else
					{
						/* TODO: turn off cooling pad */
						u64_temp_port_value &= ~(1 << (VPD_CONTROL_PUMP_PORT - 1));
						if(u8_t > MAX_TEMPERATURE_THRESH)
						{
							/* TODO: Turn on fans */
							u64_temp_port_value |= (1 << (VPD_CONTROL_FAN_PORT - 1));
						}
						else if(u8_t < MIN_TEMPERATURE_THRESH)
						{
							/* TODO: Turn off fans */
							u64_temp_port_value &= ~(1 << (VPD_CONTROL_FAN_PORT - 1));
						}
					}
				}
			}
		}break;
		case VPD_REQUEST_STOP:
		{
			//turn off pump and fans
			u64_temp_port_value &= ~(1 << (VPD_CONTROL_FAN_PORT - 1));
			u64_temp_port_value &= ~(1 << (VPD_CONTROL_PUMP_PORT - 1));
			e_vpd_state = VPD_STOP;
		}break;
		case VPD_STOP:
		{
			//do nothing
			if(u32_cur_time >= u32_vpd_start_time && u32_cur_time <= u32_vpd_stop_time)
			{
					e_vpd_state = VPD_IDLE;
			}
		}break;
		default: break;
	}
		if((*u64_port_control_value & (1 << (VPD_CONTROL_FAN_PORT-1))) == 0 &&  (u64_temp_port_value & (1 << (VPD_CONTROL_FAN_PORT-1))) != 0)	//fans is turned on
		{
			b_fans_status = true;
			v_mqtt_data_pub(DATAID_FAN_STATUS, 0x01);
		}
		else if((*u64_port_control_value & (1 << (VPD_CONTROL_FAN_PORT-1))) != 0 &&  (u64_temp_port_value & (1 << (VPD_CONTROL_FAN_PORT-1))) == 0)	//fans is turned off
		{
			b_fans_status = false;
			v_mqtt_data_pub(DATAID_FAN_STATUS, 0x00);
		}
		
		if((*u64_port_control_value & (1 << (VPD_CONTROL_PUMP_PORT-1))) == 0 &&  (u64_temp_port_value & (1 << (VPD_CONTROL_PUMP_PORT-1))) != 0)	//cooling pad is turned on
		{
			b_coolingpad_status = true;
			v_mqtt_data_pub(DATAID_COOLINGPAD_STATUS, 0x01);
		}
		else if((*u64_port_control_value & (1 << (VPD_CONTROL_PUMP_PORT-1))) != 0 &&  (u64_temp_port_value & (1 << (VPD_CONTROL_PUMP_PORT-1))) == 0)	//cooling pad is turned off
		{
			b_coolingpad_status = false;
			v_mqtt_data_pub(DATAID_COOLINGPAD_STATUS, 0x00);
		}
	 *u64_port_control_value = u64_temp_port_value;
}
