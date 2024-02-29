/*! @file manual.c
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
#include "manual.h"
#include "sensor.h"
#include "spi_mgreen.h"
#include "pHEC.h"
#include "frame_parser.h"
//#include "frame_builder.h"
//#include "mmt_publish_stream.h"
#include "wdt.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "rtc.h"

#include "mqtt_publish.h"

/*!
* static data declaration
*/

/*!
* private function prototype
*/
static STRU_MANUAL_CONTROL t_manual_table[MAX_MANUAL_AREA];
static bool b_man_running = false;
/*!
* public function bodies
*/
/*!
 * @fn 
 * @brief 
 * @param[in] none
 * @return none
 */
 void v_manual_process (uint64_t* pu64_curr_port_state)
 {
	 uint64_t u64_temp_port = *pu64_curr_port_state;
	 T_DATETIME t_time;  
	 v_rtc_read_time(&t_time);
	 uint32_t u32_cur_time = u32_datetime_2_long(&t_time);
	 for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	 {
		if(t_manual_table[i].u16_location_ID != 0)
		 {
			 switch(t_manual_table[i].e_manual_state)
			 {
				 case MANUAL_REQ_RUN:
				 {
					 u64_temp_port |= (t_manual_table[i].u32_bit_low | (uint64_t )t_manual_table[i].u32_bit_high << 32);
//					 vMQTTPubAutoparaBegin(t_manual_table[i].u16_location_ID, true,
//								0, 0,t_manual_table[i].u32_time_stop - t_manual_table[i].u32_time_start, 0);
					 v_mqtt_fertigation_start(t_manual_table[i].u16_location_ID);
					 t_manual_table[i].e_manual_state = MANUAL_RUNNING;
				 }break;
				 case MANUAL_RUNNING:
				 {
					 u64_temp_port |= (t_manual_table[i].u32_bit_low | (uint64_t )t_manual_table[i].u32_bit_high << 32);
					 if(u32_cur_time >= t_manual_table[i].u32_time_stop)
					 {
						 t_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
					 }
				 }break;
				 case MANUAL_REQ_STOP:
				 {
					 STRU_DOSING temp_dosing;
					 STRU_STATISTICAL_MODULE temp_statistic;
					 float temp_data[5] = {0, 0, 0, 0, 0};
					 /* TODO: find overlap port */
					 uint64_t temp_port = 0;
					 for(uint8_t k = 0; k < MAX_MANUAL_AREA; k++)
					 {
						 if(k != i && t_manual_table[k].u16_location_ID != 0)
						 {
							 temp_port |= (uint64_t)t_manual_table[k].u32_bit_low | (uint64_t )t_manual_table[k].u32_bit_high << 32; //find all ports are using
						 }
					 }
					 temp_port &= (uint64_t)t_manual_table[i].u32_bit_low | (uint64_t )t_manual_table[i].u32_bit_high << 32;	//find overlap port
					 temp_port = ~temp_port & ((uint64_t)t_manual_table[i].u32_bit_low | (uint64_t )t_manual_table[i].u32_bit_high << 32);
					 //clear port without overlap
					 u64_temp_port &= ~temp_port;
					 //vMQTTPubAutoparaFinish(t_manual_table[i].u16_location_ID, 0, 0, 0, temp_data, t_manual_table[i].u32_time_start);
					 v_mqtt_fertigation_finish_pub(t_manual_table[i].u16_location_ID, t_manual_table[i].u32_time_start - u16_get_time_zone()*60);
					 t_manual_table[i].u16_location_ID = 0;
					 t_manual_table[i].u32_time_start = 0;
					 t_manual_table[i].u32_time_stop = 0;
					 t_manual_table[i].e_manual_state = MANUAL_STOPPING;
				 }break;
				 case MANUAL_STOPPING:
				 {
				 }break;
			 }
		 }
	 }
	 //update current valve state
	if(u64_temp_port == 0)
		b_man_running = false;
	else
		b_man_running = true;
	*pu64_curr_port_state = u64_temp_port;
 }
void v_manual_stop_by_port(uint64_t u64_port)
{
	for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	{
		if(t_manual_table[i].u16_location_ID)
		{
			if(u64_port & ((uint64_t)t_manual_table[i].u32_bit_low | (uint64_t)t_manual_table[i].u32_bit_high << 32))
			{
				t_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
			}
		}
	}
}
bool b_manual_add_event(uint16_t u16_id, uint32_t u32_time_run, uint32_t u32_bit_low, uint32_t u32_bit_high)
{
	T_DATETIME t_time;  
	v_rtc_read_time(&t_time);
	uint32_t u32_cur_time = u32_datetime_2_long(&t_time);
	for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	{
		if(t_manual_table[i].u16_location_ID == u16_id)		//overide existed location
		{
			if(u32_time_run > 0)
				t_manual_table[i].e_manual_state = MANUAL_REQ_RUN;
			else
				t_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
			t_manual_table[i].u32_time_start = u32_cur_time;
			t_manual_table[i].u32_time_stop = u32_cur_time + u32_time_run;
			t_manual_table[i].u32_bit_low = u32_bit_low;
			t_manual_table[i].u32_bit_high = u32_bit_high;
			return true;
		}
	}
	//if not existed, append new event
	
	for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	{
		if(t_manual_table[i].u16_location_ID == 0)		//overide existed location
		{
			t_manual_table[i].u16_location_ID = u16_id;
			if(u32_time_run > 0)
				t_manual_table[i].e_manual_state = MANUAL_REQ_RUN;
			else
				t_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
			t_manual_table[i].u32_time_start = u32_cur_time;
			t_manual_table[i].u32_time_stop = u32_cur_time + u32_time_run;
			t_manual_table[i].u32_bit_low = u32_bit_low;
			t_manual_table[i].u32_bit_high = u32_bit_high;
			return true;
		}
	}
	return false;
}
bool b_manual_stop_all(void)
{
	for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	{
		if(t_manual_table[i].u16_location_ID != 0)
			t_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
	}
	return true;
}

bool b_manual_is_running(void)
{
	return b_man_running;
}
/*!
* private function bodies
*/


