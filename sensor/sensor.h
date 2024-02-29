/*! @file sensor.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
* All rights reserved.
*/
#ifndef _SENSOR_H
#define _SENSOR_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
/*!
* @data types, constants and macro defintions
*/	
#define SENSOR_TASK_STACK_SIZE												(configMINIMAL_STACK_SIZE * 2)
#define SENSOR_TASK_PRIORITY													(tskIDLE_PRIORITY + 3)
#define SENSOR_TASK_DELAY															(portTickType)(50 / portTICK_RATE_MS)

#define INTERVAL_GET_PH_EC														5//5000/TIMER_PERIOD_MS
#define TURN_OFF_PH_EC																0x00
#define TURN_ON_PH_EC																	0x01
#define CALIB_EC_AT_1413															0x02
#define CALIB_EC_AT_3290															0x03
#define CALIB_EC_AT_5000															0x04
#define SAVE_EC_CALIB																	0x05
#define CALIB_PH_AT_401																0x06
#define CALIB_PH_AT_701																0x07
#define CALIB_PH_AT_1001															0x08
#define SAVE_PH_CALIB																	0x09

#define NUM_OF_EC_PH_STATISTIC												1

typedef struct
{
	uint8_t u8_index;
	bool ab_req_ec_statistic[NUM_OF_EC_PH_STATISTIC];
	uint32_t au32_ec_total[NUM_OF_EC_PH_STATISTIC];
	uint16_t au16_ec_count[NUM_OF_EC_PH_STATISTIC];
	uint16_t au16_ec_max[NUM_OF_EC_PH_STATISTIC];
	uint16_t au16_ec_min[NUM_OF_EC_PH_STATISTIC];
	float af_ec_avg[NUM_OF_EC_PH_STATISTIC];
}ec_statistic_t;

typedef struct
{
	uint8_t u8_index;
	bool ab_req_ph_statistic[NUM_OF_EC_PH_STATISTIC];
	uint32_t au32_ph_total[NUM_OF_EC_PH_STATISTIC];
	uint16_t au16_ph_count[NUM_OF_EC_PH_STATISTIC];
	uint16_t au16_ph_max[NUM_OF_EC_PH_STATISTIC];
	uint16_t au16_ph_min[NUM_OF_EC_PH_STATISTIC];
	float af_ph_avg[NUM_OF_EC_PH_STATISTIC];
}ph_statistic_t;

/*!
* @public functions prototype
*/
void v_sensor_task_init (void);
void v_pH_EC_set_state(uint8_t u8_point, uint32_t u32_time_get_sample);		
void v_reset_ec_statistic (uint8_t u8_index);
void v_reset_ph_statistic (uint8_t u8_index);
void v_reset_all_ec_statistic (void);
void v_reset_all_ph_statistic (void);
uint8_t u8_start_ec_statistic (void);
uint8_t u8_start_ph_statistic (void);
void v_do_ec_statistic (uint16_t u16_ec_value);
void v_do_ph_statistic (uint16_t u16_ph_value);
uint16_t u16_get_ec_avg (uint8_t u8_index);
uint16_t u16_get_ph_avg (uint8_t u8_index);
uint16_t u16_get_ec_min (uint8_t u8_index);
uint16_t u16_get_ph_min (uint8_t u8_index);
uint16_t u16_get_ec_max (uint8_t u8_index);
uint16_t u16_get_ph_max (uint8_t u8_index);
void v_phec_control(uint8_t u8_control_state);
#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_H */

