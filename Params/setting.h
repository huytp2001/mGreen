/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file setting.h
 * @author Danh Pham
 * @date 01 Dec 2020
 * @version 1.0.0
 * @brief .
 */
 
#ifndef SETTING_H_
#define SETTING_H_

#include <stdint.h>
#include <stdbool.h>
/*!
* @enum e_warning_action_type_t
* Type of action options when warning occur.
*/
typedef enum
{
	WARNING_ACTION_CONTINUE = 0,		/**<Does nothing */
	WARNING_ACTION_STOP_DOSING = 1, /**<Stop fertilizer valves, only water is irrigated */
	WARNING_ACTION_STOP = 2,			/**<Stop all process and wait human's action */
}e_warning_action_type_t;

/*!
* @enum e_rf_type_t
* Types of RF module
*/
typedef enum
{
	RF_TYPE_CC1310 = 0,
	RF_TYPE_LORA = 1,
}e_rf_type_t;


/*!
* @enum e_fertilizer_flowmeter_type_t
* Types of fertilizer flowmeter
*/
typedef enum
{
	FERTI_FLOW_NORMAL = 0,
	FERTI_FLOW_HUBA = 1,
	FERTI_FLOW_NOT_USE = 2,
	FLOWMETER_TYPE_MAX,
}e_fertilizer_flowmeter_type_t;

typedef enum
{
	MODULE_4G = 0,
	MODULE_WIFI,
	MODULE_3G,  //swap 3G = 4G
	MODULE_MAX,
}e_internet_module_type;

typedef enum
{
	E_SCHEDULE_MODE = 0,
	E_FS_MODE,
	E_NEUTRALIZER_MODE,
	E_THRESHOLD_MODE,
	E_VPD_CONTROL_MODE,
	MAX_OPERATING_MODE,
}e_operating_mode;

typedef enum
{
	FS300A = 0,
	FS400A,
	OF05ZAT,
	MAX_NORMAL_FLOW_TYPE,
}e_normal_flowmeter_type;

/*!
* @struct stru_setting_t
* Parameters for fertigation program 
*/
typedef struct
{
	double d_ph_warning_range; /**<pH warning range [0..1] */
	double d_ec_warning_range; /**<EC warning range [0..1] */
	uint8_t u8_wait_pressure_enough_time; /**<Time to wait for enough pressure (seconds) */
	e_warning_action_type_t e_ph_low_action; /**<Action when pH too low */
	e_warning_action_type_t e_ph_high_action; /**<Action when pH to high */
	e_warning_action_type_t e_ec_low_action;  /**<Action when EC to low */
	e_warning_action_type_t e_ec_high_action; /**<ACtion when EC to high */
}stru_setting_t;

/*!
* @struct stru_nes_params_t
* Necessary params for machine
*/
typedef struct
{
	uint32_t u32_mqtt_topic; 	/**<Mqtt topic */
	bool b_is_use_RF;		/**<Use RF (1) or not (0) */
	e_rf_type_t e_rf_type; /**<Type of RF module (long range or short range) */
	bool b_is_use_main_flow; /**<Use main flow (1) or not (0) */
	e_fertilizer_flowmeter_type_t e_ferti_flow_type; /**<Type of fertilizer flowmeter */
	uint8_t u8_main_flow_pulses; /**<Number of pulses to calculate flow rate */
	double d_main_flowmeter_factor; /**<Factor of main flowmeter (Liters/pulse) */
	e_operating_mode e_mode;
	uint8_t u8_flow_rate;
	e_normal_flowmeter_type e_normal_flow_type;
	uint8_t au8_tank_division[4];
	uint8_t u8_main_flow_time;
	uint8_t u8_wait_pressure_enough_time;
}stru_neces_params_t;

extern void v_set_ph_warning_range(uint8_t u8_range);
extern void v_set_ec_warning_range(uint8_t u8_range);
extern void v_set_time_wait_pressure(uint8_t u8_seconds);
extern void v_set_ph_low_action(e_warning_action_type_t e_warning_type);
extern void v_set_ph_high_action(e_warning_action_type_t e_warning_type);
extern void v_set_ec_high_action(e_warning_action_type_t e_warning_type);
extern void v_set_ec_low_action(e_warning_action_type_t e_warning_type);
extern stru_setting_t stru_setting_params_get(void);
extern void v_fertigation_params_init(void);
extern stru_neces_params_t stru_neces_params_get(void);
extern void v_params_load(void);
extern void v_neces_valid_set(bool b_is_valid);
extern void v_topic_set(uint32_t u32_new_topic);
extern void v_use_rf_set(bool b_is_use_rf);
extern void v_type_rf_set(e_rf_type_t e_rf_type);
extern void v_use_main_flow_set(bool b_use_main_flow);
extern void v_main_flow_pulses_2_cal_set(uint8_t u8_num_of_pulses);
extern void v_main_flow_factor_set(double d_factor);
extern void v_ferti_flow_type_set(e_fertilizer_flowmeter_type_t e_flow_type);
void v_operating_mode(e_operating_mode e_mode);
void v_flow_rate_set(uint32_t u32_rate);
void v_tank_division_set(uint8_t u8_tank_id, uint32_t u32_division);
void v_set_normal_flowmeter_type(e_normal_flowmeter_type e_type_flowmeter);
void v_use_main_flow_set(bool b_use_main_flow);
void v_main_flow_pulses_2_cal_set(uint8_t u8_num_of_pulses);
void v_main_flow_factor_set(double d_factor);
void v_set_time_wait_pressure(uint8_t u8_seconds);
void v_set_time_flowmeter(uint8_t u8_seconds);
#endif /* SETTING_H_ */
