/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file setting.c
 * @author Danh Pham
 * @date 02 Dec 2020
 * @version 1.0.0
 * @brief .
 */ 

#include <stdint.h>
#include <stdbool.h>
#include "params.h"
#include "setting.h"
#include "sdram.h"
#include "config.h"

#include "FreeRTOS.h"
#include "task.h"

static stru_setting_t stru_setting; /**<Contains seting params */
static stru_neces_params_t stru_neces_params;

/*!
* @fn stru_neces_params_get(void)
* @brief Get necessary parameters
* @param None
* @return None
*/
stru_neces_params_t stru_neces_params_get(void)
{
	return stru_neces_params;
}
/*!
* @fn v_params_load(void)
* @brief Load necessary parammeters from EEPROM namely:  
* 	MQTT Topic  
* 	Use RF or not  
* 	Type of RF module  
*		Use main flowmeter or not  
* 	Type of main flowmeter  
*		Type of fertilizer flowmeter  
* This function MUST be called before taks run.
* @param None
* @return None
*/
void v_params_load(void)
{
	uint8_t u8_valid_value = 0;
	uint32_t u32_factor = 0;
	memset(&stru_neces_params, 0, sizeof(stru_neces_params_t));
	while(s32_params_get(PARAMS_ID_SETTING_VALID, &u8_valid_value) != 0)
	{
		vTaskDelay(2);
	}
	if(1 == u8_valid_value)
	{
		while(s32_params_get(PARAMS_ID_TOPIC,(uint8_t *)&stru_neces_params.u32_mqtt_topic) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_USE_RF,(uint8_t *)&stru_neces_params.b_is_use_RF) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_LORA,(uint8_t *)&stru_neces_params.e_rf_type) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_FERTI_FLOW_TYPE, (uint8_t *)&stru_neces_params.e_ferti_flow_type) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_OPERATING_MODE, (uint8_t *)&stru_neces_params.e_mode) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_FLOW_RATE_DEFAULT, (uint8_t *)&stru_neces_params.u8_flow_rate) != 0)
		{
			vTaskDelay(2);
		}
		if (stru_neces_params.u8_flow_rate > FLOW_RATE_MAX)
		{
			stru_neces_params.u8_flow_rate = FLOW_RATE_DEFAULT;
		}
		while(s32_params_get(PARAMS_ID_NORMAL_FLOWMTER_TYPE, (uint8_t *)&stru_neces_params.e_normal_flow_type) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_TANK1_DIVISION, (uint8_t *)&stru_neces_params.au8_tank_division[0]) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_TANK2_DIVISION, (uint8_t *)&stru_neces_params.au8_tank_division[1]) != 0)
		{
			vTaskDelay(2);
		}
		if ((0xFF == stru_neces_params.au8_tank_division[0]) && (0xFF == stru_neces_params.au8_tank_division[1]))
		{
			memset(stru_neces_params.au8_tank_division, TANK_DIVISION_DEFAULT, sizeof(stru_neces_params.au8_tank_division));
			v_tank_division_set(0, TANK_DIVISION_DEFAULT);
			v_tank_division_set(1, TANK_DIVISION_DEFAULT);
		}
		while(s32_params_get(PARAMS_ID_USE_MAIN_FLOW, (uint8_t *)&stru_neces_params.b_is_use_main_flow) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_FLOWMETER_COUNT, &stru_neces_params.u8_main_flow_pulses) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_FLOWMETER_FACTOR, (uint8_t *)&u32_factor) != 0)
		{
			vTaskDelay(2);
		}
		stru_neces_params.d_main_flowmeter_factor = (double)u32_factor/1000000;
		while(s32_params_get(PARAMS_ID_PRESSURE_TIME, (uint8_t *)&stru_neces_params.u8_wait_pressure_enough_time) != 0)
		{
			vTaskDelay(2);
		}
		while(s32_params_get(PARAMS_ID_FLOWMETER_TIME, (uint8_t *)&stru_neces_params.u8_main_flow_time) != 0)
		{
			vTaskDelay(2);
		}
	}
	else
	{
		stru_neces_params.u32_mqtt_topic = DEVICE_ID;
		stru_neces_params.b_is_use_RF = 0; //NOT USE
		stru_neces_params.e_rf_type = (e_rf_type_t)RF_TYPE_CC1310;
		stru_neces_params.e_ferti_flow_type = (e_fertilizer_flowmeter_type_t)FERTI_FLOW_NORMAL;
		stru_neces_params.e_normal_flow_type = (e_normal_flowmeter_type)OF05ZAT;
		stru_neces_params.e_mode = (e_operating_mode)E_SCHEDULE_MODE;
		stru_neces_params.u8_flow_rate = FLOW_RATE_DEFAULT;
		memset(stru_neces_params.au8_tank_division, TANK_DIVISION_DEFAULT, sizeof(stru_neces_params.au8_tank_division));
		stru_neces_params.u8_main_flow_time = TIME_WARNING_MAIN_FLOWMETER;
		stru_neces_params.u8_wait_pressure_enough_time = TIME_WAIT_PRESSURE;
		
		//v_topic_set(DEVICE_ID);
		//set epprom mode mac dinh, do phai cau hinh tat ca
		v_use_rf_set(0);
		v_type_rf_set((e_rf_type_t)RF_TYPE_CC1310);
		v_operating_mode((e_operating_mode)E_SCHEDULE_MODE);
		v_flow_rate_set(FLOW_RATE_DEFAULT);
		v_tank_division_set(0, TANK_DIVISION_DEFAULT);
		v_tank_division_set(1, TANK_DIVISION_DEFAULT);
		v_set_time_wait_pressure(TIME_WAIT_PRESSURE);
		v_set_time_flowmeter(TIME_WARNING_MAIN_FLOWMETER);
	}
}

/*!
* @fn v_neces_valid_set(bool b_is_valid)
* @brief Set the valid flag to identify the setting is available or not.
* @param[in] b_is_valid True: Setting is avalid
*												False: Setting is not avalid 
* @return None
*/
void v_neces_valid_set(bool b_is_valid)
{
	while(s32_params_set(PARAMS_ID_SETTING_VALID, 1, (uint8_t *)&b_is_valid) != 0)
	{
		vTaskDelay(2);
	}
}
/*!
* @fn v_topic_set(uint32_t u32_new_topic)
* @brief Set a new topic. 
* The valid flag must be set and the device MUST be reset to apply the change
* @param[in] u32_new_topic New topic
* @return None
*/
void v_topic_set(uint32_t u32_new_topic)
{
	while(s32_params_set(PARAMS_ID_TOPIC, 4, (uint8_t *)&u32_new_topic) != 0)
	{
		vTaskDelay(2);
	}
}
/*!
* @fn v_use_rf_set(bool b_is_use_rf)
* @brief Set the flag to identify RF is used or not
* The valid flag must be set and the device MUST be reset to apply the change
* @param[in] b_is_use_rf True: Use RF  
*													False: Not use RF
* @return None
*/
void v_use_rf_set(bool b_is_use_rf)
{
	while(s32_params_set(PARAMS_ID_USE_RF, 1, (uint8_t *)&b_is_use_rf) != 0)
	{
		vTaskDelay(2);
	}
}

/*!
* @fn v_type_rf_set(e_rf_type_t e_rf_type)
* @brief Type of RF module (CC1310, LoRa, etc...)
* The valid flag must be set and the device MUST be reset to apply the change
* @param[in] e_rf_type Type of module, see e_rf_type_t
* @return None
*/
void v_type_rf_set(e_rf_type_t e_rf_type)
{
	while(s32_params_set(PARAMS_ID_LORA, 1, (uint8_t *)&e_rf_type) != 0)
	{
		vTaskDelay(2);
	}
}

/*!
* @fn v_use_main_flow_set(bool b_use_main_flow)
* @brief Set the flag to identify flowmeter for mainline is used or not.
* The valid flag must be set and the device MUST be reset to apply the change
* @param[in] b_use_main_flow True: Use  
*													False: Not use 
* @return None
*/
void v_use_main_flow_set(bool b_use_main_flow)
{
	while(s32_params_set(PARAMS_ID_USE_MAIN_FLOW, 1, (uint8_t *)&b_use_main_flow) != 0)
	{
		vTaskDelay(2);
	}
}

/*!
* @fn v_main_flow_pulses_2_cal_set(uint8_t u8_num_of_pulses)
* @brief Set the number of pulses accumulated before process the calculation.
* The valid flag must be set and the device MUST be reset to apply the change
* @param[in] u8_num_of_pulses
* @return None
*/
void v_main_flow_pulses_2_cal_set(uint8_t u8_num_of_pulses)
{
	while(s32_params_set(PARAMS_ID_FLOWMETER_COUNT, 1, (uint8_t *)&u8_num_of_pulses) != 0)
	{
		vTaskDelay(2);
	}
}

/*!
* @fn v_main_flow_factor_set(double d_factor)
* @brief Set the factor (Liters/pulse) of main flowmeter.
* The valid flag must be set and the device MUST be reset to apply the change
* @param[in] d_factor Liters/pulse
* @return None
*/
void v_main_flow_factor_set(double d_factor)
{
	uint32_t u32_factor = d_factor * 1000000;
	while(s32_params_set(PARAMS_ID_FLOWMETER_FACTOR, 4, (uint8_t *)&u32_factor) != 0)
	{
		vTaskDelay(2);
	}
}

void v_ferti_flow_type_set(e_fertilizer_flowmeter_type_t e_flow_type)
{
	while(s32_params_set(PARAMS_ID_FERTI_FLOW_TYPE, 1, (uint8_t *)&e_flow_type) != 0)
	{
		vTaskDelay(2);
	}
}

void v_operating_mode(e_operating_mode e_mode)
{
	while(s32_params_set(PARAMS_ID_OPERATING_MODE, 1, (uint8_t *)&e_mode) != 0)
	{
		vTaskDelay(2);
	}
}

void v_flow_rate_set(uint32_t u32_rate)
{
	while(s32_params_set(PARAMS_ID_FLOW_RATE_DEFAULT, 1, (uint8_t *)&u32_rate) != 0)
	{
		vTaskDelay(2);
	}
}

void v_tank_division_set(uint8_t u8_tank_id, uint32_t u32_division)
{
	if (u8_tank_id == 0)
	{
		while(s32_params_set(PARAMS_ID_TANK1_DIVISION, 1, (uint8_t *)&u32_division) != 0)
		{
			vTaskDelay(2);
		}
	}
	else if(u8_tank_id == 1)
	{
		while(s32_params_set(PARAMS_ID_TANK2_DIVISION, 1, (uint8_t *)&u32_division) != 0)
		{
			vTaskDelay(2);
		}
	}
}
/*!
* @fn stru_setting_params_get(void)
* @brief Get setting parameters.
* Must be called frequently in fertigation task to update newest values.
* @param None
* @return None
*/
stru_setting_t stru_setting_params_get(void)
{
	return stru_setting;
}

void v_set_normal_flowmeter_type(e_normal_flowmeter_type e_type_flowmeter)
{
	if (e_type_flowmeter >= MAX_NORMAL_FLOW_TYPE)
	{
		e_type_flowmeter = FS300A;
	}
	stru_neces_params.e_normal_flow_type = e_type_flowmeter;
	while(s32_params_set(PARAMS_ID_NORMAL_FLOWMTER_TYPE , 1, 
																(uint8_t *)&e_type_flowmeter) != 0)
	{
		vTaskDelay(2);
	}
}

/*!
* @fn v_set_time_wait_pressure(uint8_t u8_seconds)
* @brief Set the wating time for enough pressure
* @param[in] u8_seconds Time to wait in seconds (from 0 to 255s)
* @return None
*/
void v_set_time_wait_pressure(uint8_t u8_seconds)
{
	stru_neces_params.u8_wait_pressure_enough_time = u8_seconds;
	while(s32_params_set(PARAMS_ID_PRESSURE_TIME , 1, &u8_seconds) != 0)
	{
		vTaskDelay(2);
	}	
}

/*!
* @fn v_set_time_flowmeter(uint8_t u8_seconds)
* @brief Set the wating time for enough 1 pulse flowmeter
* @param[in] u8_seconds Time to wait in seconds (from 0 to 255s)
* @return None
*/
void v_set_time_flowmeter(uint8_t u8_seconds)
{
	stru_neces_params.u8_main_flow_time = u8_seconds;
	while(s32_params_set(PARAMS_ID_FLOWMETER_TIME , 1, &u8_seconds) != 0)
	{
		vTaskDelay(2);
	}
}