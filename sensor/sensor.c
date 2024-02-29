/*! @file sensor.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
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
#include "sensor.h"
#include "spi_mgreen.h"
#include "pHEC.h"
#include "frame_parser.h"
//#include "frame_builder.h"
#include "mqtt_publish.h"
#include "wdt.h"
#include "neutralize.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//#include "firmware_config.h"


#define MAX_PHEC_QUEUE			5

xQueueHandle x_pHEC_Queue;
/*!
* static data declaration
*/
static StaticTask_t xInSMTaskBuffer;
static StackType_t  xInSM_Stack[SENSOR_TASK_STACK_SIZE];
static uint8_t u8_calib_point = 0;

static StaticQueue_t pHECQueueBuffer;
static uint8_t pHECQueueStorage[MAX_PHEC_QUEUE*sizeof(uint32_t)];
static ec_statistic_t stru_ec_statistic;
static ph_statistic_t stru_ph_statistic;

/*!
* private function prototype
*/
static void v_sensor_task (void *pvParameters);

/*!
* public function bodies
*/

/*!
* @fn v_phec_control(uint8_t u8_control_state)
* @brief Turn phec on/off and calibrate sensors.
* @param[in] u8_control_state 0: Turn phec off   
*															1: Turn phec on  
*															2: Calib EC at 1.413  
*															3: Calib EC at 3.290  
*															4: Calib EC at 5.000  
*															5: Calib EC at 5.000  
*															6: Save params of EC calibration process  
*															7: Calib pH at 4.01  
*															8: Calib pH at 7.01  
*															9: Calib pH at 10.01  
*															10: Save params of pH calibration process  
* @return None
*/
void v_phec_control(uint8_t u8_control_state)
{
	switch(u8_control_state)
	{
		case 0x00:		//turn off sensor
		{
			//v_phec_turn_off(0);
			v_pH_EC_set_state(TURN_OFF_PH_EC, 0);
		}
		break;
		case 0x01:		//turn on sensor
		{
			//v_phec_turn_on(0, 1, 1);
			v_pH_EC_set_state(TURN_ON_PH_EC, 5);
		}
		break;
		case 0x02:		//calib 1413
		{
			//v_phec_calib(0, 1);
			u8_calib_point = CALIB_EC_AT_1413;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x03:		//calib 3290
		{
			//v_phec_calib(0, 3);
			u8_calib_point = CALIB_EC_AT_3290;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x04:		//calib 5000
		{
			//v_phec_calib(0, 2);
			u8_calib_point = CALIB_EC_AT_5000;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x05:		//save ec calib params
		{
			//v_phec_calib(0, 4);
			u8_calib_point = SAVE_EC_CALIB;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x06:		//calib ph 4.01
		{
			//v_phec_calib(0, 5);
			u8_calib_point = CALIB_PH_AT_401;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x07:		//calib ph 7.01
		{
			//v_phec_calib(0, 6);
			u8_calib_point = CALIB_PH_AT_701;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x08:		//calib ph 10.01
		{
			//v_phec_calib(0, 7);
			u8_calib_point = CALIB_PH_AT_1001;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
		case 0x09:		//save ph calib params
		{
			//v_phec_calib(0, 8);
			u8_calib_point = SAVE_PH_CALIB;
			v_pH_EC_set_state(u8_calib_point, 0);
		}
		break;
	}
}

/*!
 * @fn void v_sensor_task_init (void)
 * @brief 
 * @param[in] none
 * @return none
 */
void v_sensor_task_init (void)
{				
#ifdef PH_EC_SENSOR_ENABLE
	v_spi_mgreen_init();
	v_reset_all_ec_statistic();
	v_reset_all_ph_statistic();
#endif
	
	xTaskCreateStatic(v_sensor_task,"sensor_task", SENSOR_TASK_STACK_SIZE,
							NULL, SENSOR_TASK_PRIORITY, xInSM_Stack, &xInSMTaskBuffer);
	x_pHEC_Queue = xQueueCreateStatic(MAX_PHEC_QUEUE, sizeof(uint32_t), (void*)&( pHECQueueStorage[ 0 ]), &pHECQueueBuffer);
}

void v_pH_EC_set_state(uint8_t u8_point, uint32_t u32_time_get_sample)
{
	switch(u8_point)
	{
		case TURN_OFF_PH_EC: //turn off pHEC
		{
			v_enable_pHEC(false, 0);
			v_mqtt_data_pub(DATAID_CALIB_PHEC1, 0);
		}break;
		case TURN_ON_PH_EC:
		{
			v_enable_pHEC(true, u32_time_get_sample);
			v_mqtt_data_pub( DATAID_CALIB_PHEC1, 1);
		}break;
		case CALIB_EC_AT_1413:	//calib 1413
		case CALIB_EC_AT_3290:	//calib 3290
		case CALIB_EC_AT_5000:	//calib 5000
		case SAVE_EC_CALIB:	//save EC parameters
		case CALIB_PH_AT_401:	//calib 4.01
		case CALIB_PH_AT_701:	//calib 7.01
		case CALIB_PH_AT_1001:	//calib 10.01
		case SAVE_PH_CALIB:	//save pH parameters
		{
			u8_calib_point = u8_point - 1;
			v_calib_pHEC(u8_calib_point);
		}break;
		default: break;
	}
}

void v_reset_ec_statistic (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		stru_ec_statistic.ab_req_ec_statistic[u8_index] = false;
		stru_ec_statistic.au32_ec_total[u8_index] = 0;
		stru_ec_statistic.au16_ec_count[u8_index] = 0;
		stru_ec_statistic.au16_ec_min[u8_index] = 0;
		stru_ec_statistic.au16_ec_max[u8_index] = 0;
		stru_ec_statistic.af_ec_avg[u8_index] = 0.0;
	}
}

void v_reset_ph_statistic (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		stru_ph_statistic.ab_req_ph_statistic[u8_index] = false;
		stru_ph_statistic.au32_ph_total[u8_index] = 0;
		stru_ph_statistic.au16_ph_count[u8_index] = 0;
		stru_ph_statistic.au16_ph_min[u8_index] = 0;
		stru_ph_statistic.au16_ph_max[u8_index] = 0;
		stru_ph_statistic.af_ph_avg[u8_index] = 0.0;
	}
}

void v_reset_all_ec_statistic (void)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < NUM_OF_EC_PH_STATISTIC; u8_i++)
	{
		v_reset_ec_statistic(u8_i);
	}
}

void v_reset_all_ph_statistic (void)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < NUM_OF_EC_PH_STATISTIC; u8_i++)
	{
		v_reset_ph_statistic(u8_i);
	}
}

uint8_t u8_start_ec_statistic (void)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < NUM_OF_EC_PH_STATISTIC; u8_i++)
	{
		if(stru_ec_statistic.ab_req_ec_statistic[u8_i] == false)
		{
			stru_ec_statistic.ab_req_ec_statistic[u8_i] = true;
			stru_ec_statistic.au32_ec_total[u8_i] = 0;
			stru_ec_statistic.au16_ec_count[u8_i] = 0;
			stru_ec_statistic.au16_ec_min[u8_i] = 0;
			stru_ec_statistic.au16_ec_max[u8_i] = 0;
			stru_ec_statistic.af_ec_avg[u8_i] = 0.0;
			break;
		}
	}
	return u8_i;
}

uint8_t u8_start_ph_statistic (void)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < NUM_OF_EC_PH_STATISTIC; u8_i++)
	{
		if(stru_ph_statistic.ab_req_ph_statistic[u8_i] == false)
		{
			stru_ph_statistic.ab_req_ph_statistic[u8_i] = true;
			stru_ph_statistic.au32_ph_total[u8_i] = 0;
			stru_ph_statistic.au16_ph_count[u8_i] = 0;
			stru_ph_statistic.au16_ph_min[u8_i] = 0;
			stru_ph_statistic.au16_ph_max[u8_i] = 0;
			stru_ph_statistic.af_ph_avg[u8_i] = 0.0;
			break;
		}
	}
	return u8_i;
}

void v_do_ec_statistic (uint16_t u16_ec_value)
{
	//TODO: check u16_ec_value is valid or not
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < NUM_OF_EC_PH_STATISTIC; u8_i++)
	{
		//Check b_req_statistic
		if(stru_ec_statistic.ab_req_ec_statistic[u8_i] == true)
		{
			if(stru_ec_statistic.au16_ec_count[u8_i] == 0)
			{
				stru_ec_statistic.au16_ec_min[u8_i] = u16_ec_value;
				stru_ec_statistic.au16_ec_max[u8_i] = u16_ec_value;
			}
			else
			{
				if(u16_ec_value < stru_ec_statistic.au16_ec_min[u8_i])
				{
					stru_ec_statistic.au16_ec_min[u8_i] = u16_ec_value;
				}
				if(u16_ec_value > stru_ec_statistic.au16_ec_max[u8_i])
				{
					stru_ec_statistic.au16_ec_max[u8_i] = u16_ec_value;
				}
			}
			stru_ec_statistic.au32_ec_total[u8_i] += u16_ec_value;
			stru_ec_statistic.au16_ec_count[u8_i] += 1;
		}
	}
}

void v_do_ph_statistic (uint16_t u16_ph_value)
{
	//TODO: check u16_ec_value is valid or not
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < NUM_OF_EC_PH_STATISTIC; u8_i++)
	{
		//Chphk b_req_statistic
		if(stru_ph_statistic.ab_req_ph_statistic[u8_i] == true)
		{
			if(stru_ph_statistic.au16_ph_count[u8_i] == 0)
			{
				stru_ph_statistic.au16_ph_min[u8_i] = u16_ph_value;
				stru_ph_statistic.au16_ph_max[u8_i] = u16_ph_value;
			}
			else
			{
				if(u16_ph_value < stru_ph_statistic.au16_ph_min[u8_i])
				{
					stru_ph_statistic.au16_ph_min[u8_i] = u16_ph_value;
				}
				if(u16_ph_value > stru_ph_statistic.au16_ph_max[u8_i])
				{
					stru_ph_statistic.au16_ph_max[u8_i] = u16_ph_value;
				}
			}
			stru_ph_statistic.au32_ph_total[u8_i] += u16_ph_value;
			stru_ph_statistic.au16_ph_count[u8_i] += 1;
		}
	}
}

uint16_t u16_get_ec_avg (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		if(stru_ec_statistic.au16_ec_count[u8_index] != 0)
		{
			stru_ec_statistic.af_ec_avg[u8_index] = (float)(stru_ec_statistic.au32_ec_total[u8_index] / 
																						stru_ec_statistic.au16_ec_count[u8_index]);
		}
		return (uint16_t) (stru_ec_statistic.af_ec_avg[u8_index]);
	}
	return 0;
}

uint16_t u16_get_ph_avg (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		if(stru_ph_statistic.au16_ph_count[u8_index] != 0)
		{
			stru_ph_statistic.af_ph_avg[u8_index] = (float)(stru_ph_statistic.au32_ph_total[u8_index] / 
																						stru_ph_statistic.au16_ph_count[u8_index]);
		}
		return (uint16_t) (stru_ph_statistic.af_ph_avg[u8_index]);
	}
	return 0;
}

uint16_t u16_get_ec_min (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		return (uint16_t) (stru_ec_statistic.au16_ec_min[u8_index]);
	}
	return 0;
}

uint16_t u16_get_ph_min (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		return (uint16_t) (stru_ph_statistic.au16_ph_min[u8_index]);
	}
	return 0;
}

uint16_t u16_get_ec_max (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		return (uint16_t) (stru_ec_statistic.au16_ec_max[u8_index]);
	}
	return 0;
}

uint16_t u16_get_ph_max (uint8_t u8_index)
{
	if (u8_index < NUM_OF_EC_PH_STATISTIC)
	{
		return (uint16_t) (stru_ph_statistic.au16_ph_max[u8_index]);
	}
	return 0;
}

#if SENSOR_ENABLE 
/*!
* private function bodies
*/
static uint8_t u8_count_err = 0;
static void v_sensor_task (void *pvParameters)
{	
	static uint8_t u8_sensor_task_id = 0;	
	while(b_wdt_reg_new_task("sensor_task", &u8_sensor_task_id) != true){}	
	pHECInit();
	v_phec_control(0);
	for(;;)
	{
		//reload wdt counter
		b_wdt_task_reload_counter(u8_sensor_task_id);
#ifdef PH_EC_SENSOR_ENABLE
	uint32_t u32_pHEC_data = 0;
	bool b_process_done = b_pH_EC_process(&u32_pHEC_data);
	if(b_process_done && u32_pHEC_data != 0xFFFFFFFF)
	{
		//send value to process
		u8_count_err = 0;
		if(x_pHEC_Queue != NULL)
			xQueueSend(x_pHEC_Queue, &u32_pHEC_data, pdFAIL);
		uint16_t u16_ph = u32_pHEC_data & 0xFFFF;
		v_do_ph_statistic(u16_ph);
		//v_mqtt_data_pub(DATAID_PH1, u16_parse_data);
		uint16_t u16_ec = u32_pHEC_data >> 16;
		v_do_ec_statistic(u16_ec);
		//v_mqtt_data_pub(DATAID_EC1, u16_parse_data);
		v_mqtt_phec_pub(0, u16_ph, u16_ec);
	}
	else if(u32_pHEC_data == 0)
	{
		//read sensor fail
		u8_count_err++;
		if(u8_count_err > 3)
		{
			u8_count_err = 0;
			v_mqtt_noti_pub(DATAID_PH1_WARNING, TYPE_NOTI_SENSOR_LOST);
			v_neutralize_set_state(NEUTRAL_REQUEST_STOP);
		}
	}
	u32_pHEC_data = 0x00;
	b_process_done = b_phEC_calib_process((uint8_t *)&u32_pHEC_data);
	if(b_process_done)	//calib done
	{
		if(u32_pHEC_data == 0)
		{
			v_mqtt_data_pub( DATAID_CALIB_PHEC1, 0xF0 + u8_calib_point + 1);
		}
		else
		{
			v_mqtt_data_pub( DATAID_CALIB_PHEC1, (u32_pHEC_data & 0xFF) + 1);
		}
	}
#endif		
		vTaskDelay(SENSOR_TASK_DELAY);
	}		
}
#else
static void v_sensor_task (void *pvParameters)
{
}
#endif /* SENSOR_ENABLE */
