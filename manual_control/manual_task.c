/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file manual_task.c
 * @author Danh Pham
 * @date 01 Dec 2020
 * @version 1.0.0
 * @brief .
 */ 
 
 #include <stdint.h>
 #include <stdbool.h>
 #include "manual_task.h"
 #include "relay_control.h"
 #include "wdt.h"
 #include "mqtt_publish.h"
 #include "rtc.h"
 
 #include "FreeRTOS.h"
 #include "task.h"
 
#define MANUAL_PUMP_PORT			1
#define MANUAL_VALVE1_PORT		5

#define FTG_MANUAL_TASK_SIZE			(configMINIMAL_STACK_SIZE * 6)
#define FTG_MANUAL_TASK_PRIORITY	(tskIDLE_PRIORITY + 2)
#define FTG_MANUAL_TASK_DELAY			(portTickType)(1000 / portTICK_RATE_MS)

/*
* Static variables 
*/
static uint64_t u64_curr_port_state = 0;
static uint64_t u64_pre_port_state = 0;
static manual_control_t astru_manual_table[MAX_MANUAL_AREA];

static StaticTask_t xManual_TaskBuffer;
static StackType_t  xManual_Stack[FTG_MANUAL_TASK_SIZE];

/* 
* Private functions prototype
*/
static void v_manual_process(void);
static void v_manual_task(void *pv_parameters);

/*
* Public functions
*/

/*!
* @fn vInitManualTask(void)
* @brief Init manual task
* @param None
* @return None
*/
void v_manual_task_init(void)
{
	xTaskCreateStatic(v_manual_task, 
										"manual_task",
										FTG_MANUAL_TASK_SIZE,
										NULL, FTG_MANUAL_TASK_PRIORITY, 
										xManual_Stack, &xManual_TaskBuffer);
}

/*!
* @fn v_manual_port_control(uint8_t u8_port, bool b_turn_on)
* @brief Turn on/off relay without condition
* @param[in] u8_port Port need to control
* @param[in] b_turn_on True: On  
*											 False: Off
* @return None
*/
void v_manual_port_control(uint8_t u8_port, bool b_turn_on)
{
	if(u8_port <= 64)		//relay control
	{
		if(b_turn_on)
		{
			b_relay_turn_on(u8_port);
		}
		else
		{
			b_relay_turn_off(u8_port);
		}
	}
}

/*!
* @fn v_manual_port_timer_control(uint8_t u8_node_id, uint32_t u32_time_on,
*								uint64_t u64_valves)
* @brief Turn on ports of a irrigation location
* @param[in] u8_node_id Id of irrigated location
* @param[in] u32_time_on Duration
* @param[in] u64_valves Port of relay need to control (bitwise)
* @return None
*/
void v_manual_port_timer_control(uint8_t u8_location_id, uint32_t u32_time_on,
								uint64_t u64_valves)
{
	bool b_reuslt = false;
	T_DATETIME t_time;
	v_rtc_read_time(&t_time);
	uint32_t u32_cur_time = u32_datetime_2_long(&t_time);
	
	for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	{
		if(astru_manual_table[i].u8_location_id == u8_location_id)		//overide existed location
		{
			if(u32_time_on > 0)
				astru_manual_table[i].e_manual_state = MANUAL_REQ_RUN;
			else
				astru_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
			astru_manual_table[i].u32_time_start = u32_cur_time;
			astru_manual_table[i].u32_time_stop = u32_cur_time + u32_time_on;
			astru_manual_table[i].u64_bitwise_port = u64_valves;
			b_reuslt = true;
		}
	}
	//if not existed, append new event
	
	if(!b_reuslt)
	{
		for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
		{
			if(astru_manual_table[i].u8_location_id == 0)		//overide existed location
			{
				astru_manual_table[i].u8_location_id = u8_location_id;
				if(u32_time_on > 0)
					astru_manual_table[i].e_manual_state = MANUAL_REQ_RUN;
				else
					astru_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
				astru_manual_table[i].u32_time_start = u32_cur_time;
				astru_manual_table[i].u32_time_stop = u32_cur_time + u32_time_on;
				astru_manual_table[i].u64_bitwise_port = u64_valves;
				break;
			}
		}
	}
}

/*!
* @fn void v_manual_process(void)
* @brief Find the ports need to on/off from manual event list
* @param None
* @return None
*/
static void v_manual_process(void)
{
	uint64_t u64_temp_port = u64_curr_port_state;
	T_DATETIME t_time;
	v_rtc_read_time(&t_time);
	 uint32_t u32_cur_time = u32_datetime_2_long(&t_time);
	 for(uint8_t i = 0; i < MAX_MANUAL_AREA; i++)
	 {
		if(astru_manual_table[i].u8_location_id != 0)
		 {
			 switch(astru_manual_table[i].e_manual_state)
			 {
				 case MANUAL_REQ_RUN:
				 {
					 astru_manual_table[i].e_manual_state = MANUAL_RUNNING;
					 u64_temp_port |= astru_manual_table[i].u64_bitwise_port;
					 v_mqtt_fertigation_start(astru_manual_table[i].u8_location_id);
				 }
				 break;
				 case MANUAL_RUNNING:
				 {
					 u64_temp_port |= astru_manual_table[i].u64_bitwise_port;
					 if(u32_cur_time >= astru_manual_table[i].u32_time_stop)
					 {
						 astru_manual_table[i].e_manual_state = MANUAL_REQ_STOP;
					 }
				 }
				 break;
				 case MANUAL_REQ_STOP:
				 {
					 /* Find overlap port */
					 uint64_t temp_port = 0;
					 for(uint8_t k = 0; k < MAX_MANUAL_AREA; k++)
					 {
						 if(k != i && astru_manual_table[k].u8_location_id != 0)
						 {
							 temp_port |= astru_manual_table[k].u64_bitwise_port;
						 }
					 }
					 temp_port &= astru_manual_table[i].u64_bitwise_port;
					 temp_port = ~temp_port & astru_manual_table[i].u64_bitwise_port;
					 //clear port without overlap
					 u64_temp_port &= ~temp_port;
					 v_mqtt_fertigation_finish_pub(astru_manual_table[i].u8_location_id,
																	astru_manual_table[i].u32_time_start - u32_rtc_unix_time_get());
					 astru_manual_table[i].u8_location_id = 0;
					 astru_manual_table[i].u32_time_start = 0;
					 astru_manual_table[i].u32_time_stop = 0;
					 astru_manual_table[i].e_manual_state = MANUAL_STOPPING;
				 }break;
				 case MANUAL_STOPPING:
				 {
				 }break;
			 }
		 }
	 }
	u64_curr_port_state = u64_temp_port;
}

static void v_manual_task(void *pv_parameters)
{
	static uint8_t u8_task_id = 0;
	while(b_wdt_reg_new_task("manual_task", &u8_task_id) != true) {}
	for ( ; ; )
	{
		b_wdt_task_reload_counter(u8_task_id);
		v_manual_process();
		if(u64_curr_port_state != u64_pre_port_state)
		{
			u64_pre_port_state = u64_curr_port_state;
			for(uint8_t i = 0; i < 64; i++)
			{
				if(u64_curr_port_state & ((uint64_t)1 << i))
				{
					b_relay_turn_on(MANUAL_VALVE1_PORT + i);
				}
				else
				{
					b_relay_turn_off(MANUAL_VALVE1_PORT + i);
				}
				if(u64_curr_port_state != 0)
				{
					b_relay_turn_on(MANUAL_PUMP_PORT);			//Turn on pump
				}
				else
				{
					b_relay_turn_off(MANUAL_PUMP_PORT);			//Turn off pump
				}
			}
		}
		vTaskDelay(FTG_MANUAL_TASK_DELAY);
	}
}
