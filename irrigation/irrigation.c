/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file irrigation.c
 * @brief functions for irrigation process
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 
 #include "FreeRTOS.h"
 #include "task.h"
 #include "queue.h"
 #include "math.h"
 
 #include "irrigation.h"
 #include "wdt.h"
 #include "sensor.h"
 #include "sdram.h"
 #include "relay_control.h"
 #include "mqtt_publish.h"
 #include "rtc.h"
 #include "setting.h"
 #include "params.h"
 #include "error_handler.h"
 #include "phec.h"
 #include "schedule_task.h"
 #include "i2c_mgreen.h"
 #include "schedule_task.h"
 #include "sd_app.h"
 #include "digital_input.h"
/*!
*	@def IRRIGATION_TASK_PRIORITY	(tskIDLE_PRIORITY + 3)
*/
/*!
*	@def IRRIGATION_TASK_SIZE				(configMINIMAL_STACK_SIZE * 6)
*/
/*!
*	@def IRRIGATION_QUEUE_LENGHT			10
*/
/*!
*	@def IRRIGATION_ITEM_SIZE				sizeof(STRU_IRRIGATION)
*/
/*!
*	@def IRRIGATION_TASK_DELAY				1000ms
*/
#define IRRIGATION_TASK_PRIORITY		(tskIDLE_PRIORITY + 3)
#define IRRIGATION_TASK_SIZE				(configMINIMAL_STACK_SIZE * 6)
#define IRRIGATION_QUEUE_LENGHT		10
#define IRRIGATION_ITEM_SIZE				sizeof(STRU_IRRIGATION_PROGRAM)
#define IRRIGATION_PHEC_QUEUE_LENGHT		3
#define IRRIGATION_PHEC_ITEM_SIZE			sizeof(uint32_t)
#define IRRIGATION_TASK_DELAY 			(portTickType)(1000 / portTICK_RATE_MS)	

/*!
* @def PHEC_ONBOARD
* Address of phec board on controller board 
*/
#define PHEC_ONBOARD								0
/*!
* @def SAMPLING_TIMES
* The number of time to read phec before sending to main process 
*/
#define SAMPLING_TIMES							1 
/*!
* @def PHEC_PERIOD
* Duration between 2 reading phec signal
*/
#define PHEC_PERIOD									1 //s
/*!
* @def WARNING_MAX_COUNT
*	Maximum number of warning before rise a alarm signal
*/
#define WARNING_MAX_COUNT						8

#define CHANNEL_FLOWRATE_DEFAULT		180 // liters/h

#define TURNON_VALVE_MINIMUM				50
#define TIME_SEND_FRAME							17//17s
#define PUMP_BOOST_PORT							2

#define RATE_MAX										2000 //L/1000L *100
#define RATE_MIN										100  //L/1000L *100
#define AUTO_UPDATE_RATE						(stru_setting_params_get().b_auto_update_rate)
#define CHANNEL_THRESHOLD						(0.2) //20%
#define CHANNEL_MAX									5
/*!
*	Private Variables declare
*/
static StackType_t x_irrigation_stack[IRRIGATION_TASK_SIZE];
static StaticTask_t x_irrigation_task_buffer;

static xQueueHandle irrigation_event_queue;
static xQueueHandle irrigation_phec_queue;
static StaticQueue_t x_irrigation_queue_buffer; 
static uint8_t uc_irrigation_queue_storage[IRRIGATION_QUEUE_LENGHT * IRRIGATION_ITEM_SIZE];

static StaticQueue_t x_irrigation_phec_queue_buffer; 
static uint8_t uc_irrigation_phec_queue_storage[IRRIGATION_PHEC_QUEUE_LENGHT * IRRIGATION_PHEC_ITEM_SIZE];

static E_IRRIGATION_EVENT_STATE e_irrigation_event_state = IRRIGATION_EVENT_WAIT;	/**< state of irrigation event*/
static E_IRRIGATION_PROCESS_STATE e_irrigation_process_state = IRRIGATION_PROCESS_BEGIN; 			/**< state of irrigation process*/

static uint32_t u32_remain_duration;		/**<remain duration of irrigation program,
																						in second if unit is time or m3 * 100 if unit is m3 */
static double d_channel_flow[5] = {0,0,0,0,0};
static double d_channel_volume[5] = {0};
static double d_main_volume = 0;
static uint32_t u32_phec = 0;
static uint16_t u16_ec = 0;
static uint16_t u16_ph = 0;
static uint32_t u32_ec_average = 0;
static uint32_t u32_ph_average = 0;

static uint16_t u16_calculated_ec = 0;
static STRU_STATISTICAL_MODULE stru_ph_statis;
static STRU_STATISTICAL_MODULE stru_ec_statis;

static STRU_IRRIGATION_PROGRAM stru_irrigation_queue;
static STRU_IRRIGATION_PROGRAM stru_irrigation_program;
static STRU_IRRIGATION_EVENT stru_irri_event_running;

static uint32_t u32_start_time;
static bool b_has_error = false;
static uint8_t au8_flowmeter_division[4] = {TANK_DIVISION_DEFAULT, TANK_DIVISION_DEFAULT, TANK_DIVISION_DEFAULT, TANK_DIVISION_DEFAULT};
static uint8_t u8_error_flowmeter_id = 0;
static uint8_t u8_repeat_flow_err_id = 0; /**<Id of repeating error*/

static uint8_t u8_main_flowmeter_id = 0; /**< id of error manager*/
static uint8_t u8_pressure_id = 0; /**< id of error manager*/

/*!
* Private function prototype
*/
static void v_irrigation_task( void *pvParameters );
static bool b_irrigation_process(void);
static void v_valves_control(bool b_control_on, STRU_IRRIGATION_PROGRAM stru_irrigation_program);
static void v_irri_event_run_init(STRU_IRRIGATION_PROGRAM stru_irri_program);
static void v_flowmeter_repeat_err(void);
static void v_flowmeter_stop_repeat_err(void);
static void v_fer_valve_control(uint8_t u8_port, relay_onboard_status_t status);
static void v_error_register(void);
static void v_error_unregister(void);
static void v_main_flowmeter_error(void);
static void v_pressure_error(void);
/*!
*	Public functions
*/
/*!
*	@fn b_IRRIGATION_event_add(STRU_IRRIGATION_PROGRAM IRRIGATION_event)
* @brief Add irrigation event to queue
* @param[in] irrigation preparation event 
* @return true: send to queue success
*					false: send to queue fail
*/
bool b_irrigation_event_add(STRU_IRRIGATION_PROGRAM irrigation_event)
{
	if (NULL != irrigation_event_queue)
	{
		xQueueSend(irrigation_event_queue, &irrigation_event, 10);
		return true;
	}
	else
	{
		return false;
	}
}

/*!
* @fn u8_irrigation_prog_state_get(void)
* @brief Get overall state of machine
* 0: Machine is in stop state
* 1: Machine is running (pump is on and valve is open)
* 2: Machine is counting down to next irrigation event
* @return State
*/
uint8_t u8_irrigation_prog_state_get(void)
{
	uint8_t u8_state = MACHINE_STATE_STOPED;
	switch(e_irrigation_event_state)
	{
		case IRRIGATION_EVENT_BEGIN: 		
    case IRRIGATION_EVENT_RUN:
		{
			u8_state = MACHINE_STATE_RUNNING;
		} 	 		
		break;
    case IRRIGATION_EVENT_PAUSE:		
    case IRRIGATION_EVENT_WAIT_CONTINUE:
		{
			u8_state = MACHINE_STATE_STOPED;
		}
		break;
    case IRRIGATION_EVENT_CONTINUE:
    case IRRIGATION_EVENT_STOP:	
		case IRRIGATION_EVENT_WAIT:
		{
			u8_state = MACHINE_STATE_PREPARE;
		}
		break;		
	}
	return u8_state;
}

/*!
* @fn e_irrigation_state_get(void)
* @brief get current irrigation event state
* @param[in] None
* @return irrigation event state
*/
E_IRRIGATION_EVENT_STATE e_get_irrigation_state(void)
{
	return e_irrigation_event_state;
}
/*!
* @fn e_get_IRRIGATION_process_state(void)
* @brief get current irrigation process state
* @param[in] None
* @return irrigation process state
*/
E_IRRIGATION_PROCESS_STATE e_get_irrigation_process_state(void)
{
	return e_irrigation_process_state;
}
/*!
* @fn e_duration_unit_get(void)
* @brief get duration unit of irrigation program
* @param[in] None
* @return duration unit
*/
E_DURATION_UNIT e_duration_unit_get(void)
{
	return stru_irrigation_program.e_duration_unit;
}

bool b_irrigation_phec_get(void)
{
	return stru_irrigation_program.b_phec_adjust;
}

uint8_t u8_location_running_get(void)
{
	return stru_irrigation_program.u8_location_id;
}
/*!
* @fn v_irrigation_event_pause(void)
* @brief set current irrigation event state is pause
* @param[in] None
* @return None
*/
void v_irrigation_event_pause(void)
{
	if((e_irrigation_event_state != IRRIGATION_EVENT_PAUSE) &&
		(e_irrigation_event_state != IRRIGATION_EVENT_WAIT_CONTINUE))
	{
		e_irrigation_event_state = IRRIGATION_EVENT_PAUSE;
		uint8_t u8_temp_value = (uint8_t)IRRIGATION_EVENT_WAIT_CONTINUE;
		s32_params_set(PARAMS_ID_PORTS_CTR_STATE, 1, &u8_temp_value);
	}
	if (IRRIGATION_EVENT_WAIT_CONTINUE == e_irrigation_event_state)
	{
		v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_STOPED);
	}
}

/*!
* @fn v_irrigation_event_continue(void)
* @brief set current irrigation event state is continue
* @param[in] None
* @return None
*/
void v_irrigation_event_continue(void)
{
	if((IRRIGATION_EVENT_PAUSE == e_irrigation_event_state) ||
		(IRRIGATION_EVENT_WAIT_CONTINUE == e_irrigation_event_state))
	{
		if (0 == stru_irrigation_program.u8_location_id)
		{
			e_irrigation_event_state = IRRIGATION_EVENT_WAIT;
			v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
		}
		else
		{
			e_irrigation_event_state = IRRIGATION_EVENT_CONTINUE;
			v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_RUNNING);
		}
		uint8_t u8_temp_value = (uint8_t)IRRIGATION_EVENT_WAIT;
		s32_params_set(PARAMS_ID_PORTS_CTR_STATE, 1, &u8_temp_value);
	}
	v_clear_all_error();
}

/*!
* @fn v_irrigation_event_run(void)
* @brief set current irrigation event state is wait to prepare for next ferigation event
* @param[in] None
* @return None
*/
void v_irrigation_event_run(void)
{
	if((IRRIGATION_EVENT_PAUSE == e_irrigation_event_state) ||
		(IRRIGATION_EVENT_WAIT_CONTINUE == e_irrigation_event_state))
	{
		/* reset queue */
		v_irrigation_queue_reset();		
		uint8_t u8_temp_value = (uint8_t)IRRIGATION_EVENT_WAIT;
		s32_params_set(PARAMS_ID_PORTS_CTR_STATE, 1, &u8_temp_value);
		e_irrigation_event_state = IRRIGATION_EVENT_WAIT;
	}
	if(IRRIGATION_EVENT_RUN == e_irrigation_event_state)
	{
		v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_RUNNING);
	}
	else
	{
		v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
	}
	v_clear_all_error();
}

bool b_has_error_get(void)
{
	return b_has_error;
}

void v_irrigation_queue_reset(void)
{
	/* reset queue */
	xQueueReset(irrigation_event_queue);
}


void v_irrigation_error(void)
{
	v_schedule_back();
	vTaskDelay(1000);
	ROM_SysCtlReset();
}

void v_clear_all_error(void)
{
	/* Stop all error repeating */
//	v_pressure_stop_repeat_err();
	v_flowmeter_stop_repeat_err();
//	v_stop_repeat_sensor_err();
//	v_stop_repeat_sensor_err_ss_task();
}


/*!
* @fn v_IRRIGATION_manager_taskinit(void)
* @brief Creat task, queue
* @param[in] None
* @return None
*/
void v_irrigation_manager_taskinit(void)
{ 
	xTaskCreateStatic(v_irrigation_task, 
										"irrigation TASK", 
										IRRIGATION_TASK_SIZE, 
										NULL, 
										IRRIGATION_TASK_PRIORITY, 
										x_irrigation_stack, 
										&x_irrigation_task_buffer);
	
	irrigation_event_queue = xQueueCreateStatic(IRRIGATION_QUEUE_LENGHT, 
																							 IRRIGATION_ITEM_SIZE,
																							 uc_irrigation_queue_storage, 
																							 &x_irrigation_queue_buffer);
	irrigation_phec_queue = xQueueCreateStatic(IRRIGATION_PHEC_QUEUE_LENGHT, 
																							 IRRIGATION_PHEC_ITEM_SIZE,
																							 uc_irrigation_phec_queue_storage, 
																							 &x_irrigation_phec_queue_buffer);
}

/*!
*	Private functions
*/
static void v_fer_valve_control(uint8_t u8_port, relay_onboard_status_t status)
{
	if (ON_RELAY_ONBOARD == status)
	{
		if (0 == u8_port)
		{
			b_relay_turn_on(VALVE_1_PORT);
			b_relay_turn_on(VALVE_2_PORT);
		}
		else if (1 == u8_port)
		{
			b_relay_turn_on(VALVE_1_PORT);
		}
		else if (2 == u8_port)
		{
			b_relay_turn_on(VALVE_2_PORT);
		}
	}
	else
	{
		if (0 == u8_port)
		{
			b_relay_turn_off(VALVE_1_PORT);
			b_relay_turn_off(VALVE_2_PORT);
		}
		else if (1 == u8_port)
		{
			b_relay_turn_off(VALVE_1_PORT);
		}
		else if (2 == u8_port)
		{
			b_relay_turn_off(VALVE_2_PORT);
		}
	}
}
/*!
* @fn v_irrigation_task( void *pvParameters )
* @brief irrigation task
* @param[in] None
* @return None
*/
static void v_irrigation_task( void *pvParameters )
{
	static uint8_t u8_irrigation_task_id = 0;
	static uint8_t u8_time_task_run = 0;
	static bool b_repeat_err = false;
	uint8_t param_event_state;
	stru_irrigation_program.u8_location_id = 0;	
	while(b_wdt_reg_new_task("irrigation_task", &u8_irrigation_task_id) != true){}	
	//while( b_phec_read_queue_reg(irrigation_phec_queue) != true) {} 
	s32_params_get(PARAMS_ID_PORTS_CTR_STATE, &param_event_state);
	e_irrigation_event_state = (E_IRRIGATION_EVENT_STATE)param_event_state;
	au8_flowmeter_division[0] = TANK_1_DIVISION;
	au8_flowmeter_division[1] = TANK_2_DIVISION;
	for(;;)
	{
		//reload wdt counter
		b_wdt_task_reload_counter(u8_irrigation_task_id);
		if (IRRIGATION_EVENT_WAIT == e_irrigation_event_state)
		{
			if (pdTRUE == xQueueReceive(irrigation_event_queue, &stru_irrigation_queue, 0))
			{
				e_irrigation_event_state = IRRIGATION_EVENT_BEGIN;
			}
			else{
				
			}
		}
		else{
		}
		
		switch (e_irrigation_event_state)
		{
			case IRRIGATION_EVENT_BEGIN:
			{
				stru_irrigation_program = stru_irrigation_queue;			
				e_irrigation_event_state = IRRIGATION_EVENT_RUN;
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_RUNNING);

				/* reset counters */
							
				b_has_error = false;
			}
			break;
			case IRRIGATION_EVENT_RUN:
			{
				if (b_irrigation_process())
				{
					v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
					e_irrigation_event_state = IRRIGATION_EVENT_STOP;
				}
				else
				{
					u8_time_task_run++;
					if (u8_time_task_run > TIME_SEND_FRAME)
					{
						u8_time_task_run = 0;
						v_mqtt_fertilizer_volume_pub(stru_irri_event_running.u8_id, stru_irri_event_running.d_volume);
						v_mqtt_water_volume_pub(stru_irri_event_running.u8_id, stru_irri_event_running.d_main_volume);
					}
				}
			}
			break;
			case IRRIGATION_EVENT_PAUSE:
			{
				// pause by error or user remote

				if (IRRIGATION_PROCESS_RUN == e_irrigation_process_state)
				{
					stru_irrigation_program.u32_before_duration = 0;
				}
				v_valves_control(false, stru_irrigation_program);
				//turn off all fer valve
				v_fer_valve_control(0, OFF_RELAY_ONBOARD);
				e_irrigation_process_state = IRRIGATION_PROCESS_BEGIN;
				e_irrigation_event_state = IRRIGATION_EVENT_WAIT_CONTINUE;
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_STOPED);
				
				v_error_unregister();
				v_i2c_hardware_reset();
				
				b_has_error = true;
			}
			break;
			case IRRIGATION_EVENT_WAIT_CONTINUE:
			{
				//do nothing
			}
			break;
			case IRRIGATION_EVENT_CONTINUE:
			{
				e_irrigation_event_state = IRRIGATION_EVENT_RUN;
				
			}
			break;
			case IRRIGATION_EVENT_STOP:
			{					
				u32_remain_duration = 0;
				v_mqtt_fertilizer_volume_pub(stru_irri_event_running.u8_id, stru_irri_event_running.d_volume);
				v_mqtt_fertilizer_rate_pub(stru_irri_event_running.u8_id, stru_irri_event_running.u32_volume_set);
				v_mqtt_water_volume_pub(stru_irri_event_running.u8_id, stru_irri_event_running.d_main_volume);
				
				#ifdef PH_EC_SENSOR_ENABLE	
				stru_ec_statis.u16_MinValue = u16_get_ec_min(0);
				u32_ec_average = u16_get_ec_avg(0);
				stru_ec_statis.u16_MaxValue = u16_get_ec_max(0);
				
				stru_ph_statis.u16_MinValue = u16_get_ph_min(0);
				u32_ph_average = u16_get_ph_avg(0);
				stru_ph_statis.u16_MaxValue = u16_get_ph_max(0);
				
				v_reset_ec_statistic(0);
				v_reset_ph_statistic(0);				
				#endif	
				v_mqtt_ph_statistic_pub(stru_irri_event_running.u8_id, 
												stru_ph_statis.u16_MinValue, u32_ph_average, stru_ph_statis.u16_MaxValue);
				v_mqtt_ec_statistic_pub(stru_irrigation_program.u8_location_id, 
																stru_ec_statis.u16_MinValue, u32_ec_average, stru_ec_statis.u16_MaxValue);
																
				v_mqtt_fertigation_finish_pub(stru_irri_event_running.u8_id, u32_start_time);								
				
				v_error_unregister();
				v_i2c_hardware_reset();
				
				stru_irrigation_program.u8_location_id = 0;
				e_irrigation_event_state = IRRIGATION_EVENT_WAIT;

			}
			break;
			case IRRIGATION_EVENT_WAIT:
			{
				// do nothing
				if (!b_repeat_err)
				{
					b_repeat_err = true;
				}
			}
			break;
			default:
			{
				e_irrigation_event_state = IRRIGATION_EVENT_WAIT;
			}
			break;
		}		
		vTaskDelay(IRRIGATION_TASK_DELAY);
	}
}


/*!
* @fn b_IRRIGATION_process()
* @brief doing irrigation process
* @param[in] None
* @return true: process complete
*					false: process don't complete
*/
static bool b_irrigation_process(void)
{
	bool b_done = false;
	static double Qv_channel = 0.0;
	static double f_channel = 0.0;
	static uint32_t au32_input_counter[5] = {0,0,0,0};
	static uint32_t au32_input_counter_tmp[5] = {0,0,0,0};
	static uint32_t au32_input_counter_volume[5] = {0,0,0,0};
	static uint32_t au32_input_counter_flow[5] = {0,0,0,0};
	static uint32_t u32_curr_time = 0;
	static uint32_t au32_pre_time[5] = {0,0,0,0,0};
	static uint16_t u16_retry_check_valve[5] = {0,0,0,0};
	static double d_flow_debug[30];
	static double d_volume_debug[30];
	static double d_flow_calculate[30];
	static uint32_t u32_cnt_debug[30];
	static uint8_t u8_if_debug = 0;
	static uint8_t u8_iv_debug = 0;
	static uint32_t u32_pre_main_flow_counter = 0;
	switch (e_irrigation_process_state)
	{
		case IRRIGATION_PROCESS_BEGIN:
		{			
			/*
			* reset bien
			* bat van, bom, phec
			* kiem tra van phan cai dat
			* cai dat thoi gian/luu luong tuoi
			*/
			v_mqtt_data_pub(0xFFFF, (E_IRRIGATION_PROCESS_STATE)IRRIGATION_PROCESS_BEGIN);
			
			Qv_channel = 0.0;
			f_channel = 0.0;
			memset(au32_input_counter, 0, sizeof(au32_input_counter));
			memset(au32_input_counter_tmp, 0, sizeof(au32_input_counter_tmp));
			memset(au32_input_counter_volume, 0, sizeof(au32_input_counter_volume));
			memset(au32_input_counter_flow, 0, sizeof(au32_input_counter_flow));
			memset(d_flow_debug, 0, sizeof(d_flow_debug));
			memset(d_volume_debug, 0, sizeof(d_volume_debug));
			memset(au32_pre_time, 0, sizeof(au32_pre_time));
			u32_curr_time = 0;
			u8_if_debug = 0;
			u8_iv_debug = 0;
			memset(u16_retry_check_valve, 0, sizeof(u16_retry_check_valve));			
			
			v_clear_flow_meter(0);
			
			/* Turn on valve */
			v_valves_control(true, stru_irrigation_program);	
			
			#ifdef PH_EC_SENSOR_ENABLE
			v_pH_EC_set_state(TURN_ON_PH_EC, INTERVAL_GET_PH_EC);
			u8_start_ec_statistic();
			u8_start_ph_statistic();
			#endif	
			v_error_register();
			
			if (b_has_error)
			{
				for (uint8_t i=0; i<MAX_CHANNEL; i++)
				{
					if (VALVE_STATE_ON == stru_irri_event_running.e_valve_state[i])
					{
						//turn on fer valve
						v_fer_valve_control(i+1, ON_RELAY_ONBOARD);
						v_mqtt_data_pub(0xE0+i, 1); 
					}
				}
			}
			else
			{
				/* Get start time */
				u32_start_time = u32_rtc_unix_time_get();
				v_irri_event_run_init(stru_irrigation_program);
				v_mqtt_fertigation_start(stru_irrigation_program.u8_location_id);
				for(uint8_t i=0; i<5; i++)
				{
					d_channel_flow[i] = FLOW_RATE_DEFAULT;
				}
			}
			e_irrigation_process_state = IRRIGATION_PROCESS_BEFORE_RUN;
			v_mqtt_data_pub(0xFFFF, (E_IRRIGATION_PROCESS_STATE)IRRIGATION_PROCESS_BEFORE_RUN);
		}
		break;
		case IRRIGATION_PROCESS_BEFORE_RUN:
		{
			/*
			* check time before
			*/
			if (stru_irri_event_running.d_running_duration >= stru_irrigation_program.u32_before_duration)
			{
				e_irrigation_process_state = IRRIGATION_PROCESS_RUN;
				v_mqtt_data_pub(0xFFFF, (E_IRRIGATION_PROCESS_STATE)IRRIGATION_PROCESS_RUN);
				for (uint8_t i=0; i<MAX_CHANNEL; i++)
				{
					if (VALVE_STATE_ON == stru_irri_event_running.e_valve_state[i])
					{
						//turn on fer valve
						v_fer_valve_control(i+1, ON_RELAY_ONBOARD);
						v_mqtt_data_pub(0xE0+i, 1); 
					}
				}
			}
			else
			{			
				//do nothing
			}
		}
		break;
		case IRRIGATION_PROCESS_RUN:
		{
			if (stru_irri_event_running.d_running_duration >= (stru_irrigation_program.u32_total_duration - stru_irrigation_program.u32_after_duration))
			{
				e_irrigation_process_state = IRRIGATION_PROCESS_AFTER_RUN;
				v_mqtt_data_pub(0xFFFF, (E_IRRIGATION_PROCESS_STATE)IRRIGATION_PROCESS_AFTER_RUN);
				//turn off all fer valve again
				v_fer_valve_control(0, OFF_RELAY_ONBOARD);
			}
			else
			{
				/*
				* get channel volume, main volume
				* check volume, warning
				* check volume, turn off valve
				* thong ke ph ec?
				*/
				v_di_get_value(au32_input_counter_tmp);
				u32_curr_time++;
				//v_mqtt_debug_rate_pub(0, au32_input_counter_tmp);
				if (FERTILIZER_FLOWMETER_TYPE != FERTI_FLOW_NOT_USE)
				{
					for (uint8_t i=0; i<MAX_CHANNEL; i++)
					{
						if (VALVE_STATE_ON == stru_irri_event_running.e_valve_state[i])
						{						
							if (au32_input_counter_tmp[i] !=0)
							{
								au32_input_counter[i] += au32_input_counter_tmp[i];
								au32_input_counter_tmp[i] = 0;
								if (FERTILIZER_FLOWMETER_TYPE == FERTI_FLOW_NORMAL)
								{
									if (NORMAL_FLOWMETER_TYPE == OF05ZAT)
									{
										stru_irri_event_running.d_volume[i] = (au32_input_counter[i] * FLOW_FACTOR_OF05ZAT)/1000*((double)au8_flowmeter_division[i]/100); //Lit
									}
									else if (NORMAL_FLOWMETER_TYPE == FS400A)
									{
										stru_irri_event_running.d_volume[i] = (au32_input_counter[i]/(FLOW_FACTOR_FS400 * 60))* FS400_DIFF *((double)au8_flowmeter_division[i]/100);
									}
									else if (NORMAL_FLOWMETER_TYPE == FS300A)
									{
										stru_irri_event_running.d_volume[i] = (au32_input_counter[i]/(FLOW_FACTOR_FS300 * 60)) * ((double)au8_flowmeter_division[i]/100);
									}
								}
								else if (FERTILIZER_FLOWMETER_TYPE == FERTI_FLOW_HUBA)
								{
									//update flow all channel
									if (u32_curr_time - au32_pre_time[i] > TIME_UPDATE_FLOW_HUBA)
									{
										f_channel = (double)(au32_input_counter[i] - au32_input_counter_flow[i])/TIME_UPDATE_FLOW_HUBA;
										Qv_channel = Kf * f_channel + Q0;		//liters/min
										if(Qv_channel <= 18.0 && Qv_channel > 0)
										{
											d_channel_flow[i] = Qv_channel; //liters/h
										}
										else
										{
											Qv_channel = d_channel_flow[i];
										}
										au32_input_counter_flow[i] = au32_input_counter[i];									
										au32_pre_time[i] = u32_curr_time;;										
										//todo: debug pub flow			
//										if (i==0)
//										{
//											d_flow_debug[u8_if_debug] = d_channel_flow[i];
//											u8_if_debug++;
//										}
									}
									//update volume
									if (au32_input_counter[i] >= au32_input_counter_volume[i] + HUBA_UPDATE_VOLUME_PULSES)
									{
										if ( d_channel_flow[i] <= 18 && d_channel_flow[i] >0)
										{
											Qv_channel = d_channel_flow[i];
										}
										else
										{
											Qv_channel = FLOW_RATE_DEFAULT;
										}
										stru_irri_event_running.d_volume[i] += (double)(au32_input_counter[i] - au32_input_counter_volume[i]) * Kf * Qv_channel/(60.0 * (Qv_channel - Q0));	//liters			
										stru_irri_event_running.d_volume[i] = stru_irri_event_running.d_volume[i] *((double)au8_flowmeter_division[i]/100);
										au32_input_counter_volume[i] = au32_input_counter[i];
										//todo: debug volume 
//										if (i==0)
//										{
//											d_flow_calculate[u8_iv_debug] = Qv_channel;
//											d_volume_debug[u8_iv_debug] = stru_irri_event_running.d_volume[i];
//											u32_cnt_debug[u8_iv_debug] = au32_input_counter[i];
//											u8_iv_debug++;
//										}
									}
								}
								
								if (stru_irri_event_running.d_volume[i]*100 >= (stru_irri_event_running.u32_volume_set[i]))
								{
									v_fer_valve_control(i+1, OFF_RELAY_ONBOARD);
									stru_irri_event_running.e_valve_state[i] = VALVE_STATE_OFF;
									stru_irri_event_running.b_valve_use[i] = false;
								}						
							}
							else
							{
								u16_retry_check_valve[i]++;
								if (u16_retry_check_valve[i] >= MAX_RETRY_CHECK)
								{
									u16_retry_check_valve[i] = 0;
									//dung may, canh bao
									v_mqtt_noti_pub(DATAID_VALVE1_ERROR + i, TYPE_NOTI_SENSOR_LOST);
									v_irrigation_event_pause();
									//turn off all fer valve
									v_fer_valve_control(0, OFF_RELAY_ONBOARD);
									//memset(stru_irri_event_running.e_valve_state, VALVE_STATE_NONE, sizeof(stru_irri_event_running.e_valve_state));
									//todo: error_handler: lap lai canh bao 5p
									u8_error_flowmeter_id = i;
									b_error_handler_reg("Flowmeter error", ERROR_REPEAT_TIMEOUT, v_flowmeter_repeat_err, &u8_repeat_flow_err_id);
								}
							}
						}
					}
				}
				else
				{
					//1s
					for (uint8_t i1=0; i1<MAX_CHANNEL; i1++)
					{
						if ((VALVE_STATE_ON == stru_irri_event_running.e_valve_state[i1]) && (au32_pre_time[i1] != u32_curr_time))
						{									
							stru_irri_event_running.d_volume[i1] += (double)FLOW_RATE/60;
							au32_pre_time[i1] = u32_curr_time;
							if ((stru_irri_event_running.d_volume[i1]*100) >= stru_irri_event_running.u32_volume_set[i1])
							{
								v_fer_valve_control(i1+1, OFF_RELAY_ONBOARD);
								stru_irri_event_running.e_valve_state[i1] = VALVE_STATE_OFF;
								stru_irri_event_running.b_valve_use[i1] = false;
							}
						}								
					}
				}
			}
		}
		break;
		case IRRIGATION_PROCESS_AFTER_RUN:
		{
			if (stru_irri_event_running.d_running_duration >= stru_irrigation_program.u32_total_duration)
			{
				e_irrigation_process_state = IRRIGATION_PROCESS_BEFORE_STOP;
				v_mqtt_data_pub(0xFFFF, (E_IRRIGATION_PROCESS_STATE)IRRIGATION_PROCESS_BEFORE_STOP);
			}
		}
		break;
		case IRRIGATION_PROCESS_BEFORE_STOP:
		{
			//turnoff pump, valve
			v_valves_control(false, stru_irrigation_program);				
			//state process statis, pub infor
			#ifdef PH_EC_SENSOR_ENABLE
			v_pH_EC_set_state(TURN_OFF_PH_EC, 0);	
			#endif
			
			if (stru_irri_event_running.d_volume[0]*120 < stru_irri_event_running.u32_volume_set[0])
			{
				v_mqtt_noti_pub(DATAID_VALVE1_ERROR, TYPE_NOTI_SENSOR_LOST);
			}
			if (stru_irri_event_running.d_volume[1]*120 < stru_irri_event_running.u32_volume_set[1])
			{
				v_mqtt_noti_pub(DATAID_VALVE2_ERROR, TYPE_NOTI_SENSOR_LOST);
			}
		
			e_irrigation_process_state = IRRIGATION_PROCESS_STOP;
			v_mqtt_data_pub(0xFFFF, (E_IRRIGATION_PROCESS_STATE)IRRIGATION_PROCESS_STOP);
		}
		break;
		case IRRIGATION_PROCESS_STOP:
		{
			//do nothing
			//state end
		}
		break;
		default:
		{
			e_irrigation_process_state = IRRIGATION_PROCESS_BEGIN;
		}
		break;
	}
	/*update running time*/
	if (USE_MAIN_FLOW)
	{
		//xu ly con don, reset xung khi bat dau chay lai
		au32_input_counter[MAIN_FLOW_INPUT_PORT-1] = u32_get_di3_status();
		stru_irri_event_running.d_main_volume = au32_input_counter[MAIN_FLOW_INPUT_PORT-1] * MAIN_FLOW_FACTOR; //m3
		stru_irri_event_running.d_main_volume = stru_irri_event_running.d_main_volume * 100;
		if(u32_pre_main_flow_counter !=au32_input_counter[MAIN_FLOW_INPUT_PORT-1])
		{
			u32_pre_main_flow_counter = au32_input_counter[MAIN_FLOW_INPUT_PORT-1];
			b_error_reset_counter(u8_main_flowmeter_id);
		}
	}
	stru_irri_event_running.u16_main_time++;
	if (UNIT_TIME == stru_irrigation_program.e_duration_unit)
	{
		stru_irri_event_running.d_running_duration = stru_irri_event_running.u16_main_time;
	}
	else if (UNIT_M3 == stru_irrigation_program.e_duration_unit)
	{
		stru_irri_event_running.d_running_duration = stru_irri_event_running.d_main_volume;
	}
	
	if (CHECK_PRESSURE_ENABLE)
	{
		//check pressure switch: port 4
		if (!u32_get_di4_status())
		{
			b_error_reset_counter(u8_pressure_id);
		}
	}
	
	if (IRRIGATION_PROCESS_STOP == e_irrigation_process_state)
	{
		e_irrigation_process_state = IRRIGATION_PROCESS_BEGIN;
		b_done = true;
	}
	else
	{
		b_done = false;
	}
	return b_done;
}

/*!
* @fn v_valves_control(bool b_control_on, STRU_FERTIGATION_PROGRAM stru_fertigation_program)
* @brief turn on/off valves of a irrigated area
* @param[in] b_control_on True: turn on valves. False: turn off valves
* @param[in] stru_fertigation_program Current struct contain information of fertigation program
* @return None
*/
static void v_valves_control(bool b_control_on, STRU_IRRIGATION_PROGRAM stru_irrigation_program)
{
	uint8_t u8_cnt = 0;
	static uint8_t i_v = 0;
	if(b_control_on)
	{
		for(i_v = 0; i_v < stru_irrigation_program.u8_no_valves; i_v++)
		{
			u8_cnt = 0;
			if(b_relay_turn_on(stru_irrigation_program.astru_valve[i_v].u8_port) == false)
			{
				u8_cnt++;
				if(b_relay_turn_on(stru_irrigation_program.astru_valve[i_v].u8_port) == false)
				{
					u8_cnt++;
				}
			}
			if (u8_cnt >= 2)
			{
				v_irrigation_error();
			}
		}
	}
	else
	{
		for( uint8_t i = 0; i < stru_irrigation_program.u8_no_valves; i++)
		{
			if(b_relay_turn_off(stru_irrigation_program.astru_valve[i].u8_port) == false)
			{
				b_relay_turn_off(stru_irrigation_program.astru_valve[i].u8_port);
			}
		}
	}
}

static void v_irri_event_run_reset(void)
{
	memset(&stru_irri_event_running, 0, sizeof(stru_irri_event_running));
}

static void v_irri_event_run_init(STRU_IRRIGATION_PROGRAM stru_irri_program)
{
	stru_irri_event_running.u8_id = stru_irri_program.u8_location_id;
	for (uint8_t i=0; i<4; i++)
	{
		if(stru_irri_program.au32_rate[i] != 0)
		{
			stru_irri_event_running.b_valve_use[i] = true;
			stru_irri_event_running.e_valve_state[i] = VALVE_STATE_ON;
		}
		else
		{
			stru_irri_event_running.b_valve_use[i] = false;
			stru_irri_event_running.e_valve_state[i] = VALVE_STATE_NONE;
		}
		stru_irri_event_running.u32_volume_set[i] = stru_irrigation_program.au32_rate[i];
		stru_irri_event_running.d_volume[i] = 0;
	}	
	stru_irri_event_running.d_main_volume = 0;
	stru_irri_event_running.u16_main_time = 0;
	stru_irri_event_running.d_running_duration = 0;	
}

void v_valve_state_set(E_FER_VALVE_STATE e_state)
{
	memset(stru_irri_event_running.e_valve_state, e_state, sizeof(stru_irri_event_running.e_valve_state));
}

static void v_flowmeter_repeat_err(void)
{
	if ((u8_error_flowmeter_id == 0) || (u8_error_flowmeter_id == 1))
	{
		v_mqtt_noti_pub(DATAID_VALVE1_ERROR + u8_error_flowmeter_id, TYPE_NOTI_SENSOR_LOST);
	}
	else if (u8_error_flowmeter_id == 2)
	{
		v_mqtt_noti_pub(DATAID_PUMP_ERR, TYPE_NOTI_ERROR);
		v_mqtt_noti_pub(DATAID_MAINFLOW_ERR, TYPE_NOTI_SENSOR_LOST);
	}
	else if (u8_error_flowmeter_id == 3)
	{
		v_mqtt_noti_pub(DATAID_PUMP_ERR, TYPE_NOTI_ERROR);
	}
}

static void v_flowmeter_stop_repeat_err(void)
{
	if(u8_repeat_flow_err_id != 0)
	{
		b_error_unreg(u8_repeat_flow_err_id);
	}
}

static void v_error_register(void)
{
	if (USE_MAIN_FLOW && (UNIT_M3 == stru_irrigation_program.e_duration_unit))
	{
		b_error_handler_reg("main_flowmeter_error", 
																MAIN_FLOWMETER_ERROR_TIME, 
																v_main_flowmeter_error, 
																&u8_main_flowmeter_id);	
	}
	if (CHECK_PRESSURE_ENABLE)
	{
		b_error_handler_reg("pressure_error", PRESSURE_ERROR_TIME, v_pressure_error, &u8_pressure_id);
	}
}

static void v_error_unregister(void)
{
	if (USE_MAIN_FLOW)
	{
		if (u8_main_flowmeter_id != 0)
		{
			b_error_unreg(u8_main_flowmeter_id);
			u8_main_flowmeter_id = 0;
		}	
	}
	if (CHECK_PRESSURE_ENABLE)
	{
		if (u8_pressure_id != 0)
		{
			b_error_unreg(u8_pressure_id);
			u8_pressure_id = 0;
		}
	}
}

static void v_main_flowmeter_error(void)
{
	v_mqtt_noti_pub(DATAID_PUMP_ERR, TYPE_NOTI_ERROR);
	v_mqtt_noti_pub(DATAID_MAINFLOW_ERR, TYPE_NOTI_SENSOR_LOST);
	u8_error_flowmeter_id = 2;
	b_error_handler_reg("Flowmeter error", ERROR_REPEAT_TIMEOUT, v_flowmeter_repeat_err, &u8_repeat_flow_err_id);
	v_irrigation_event_pause();	
}

static void v_pressure_error(void)
{
	v_mqtt_noti_pub(DATAID_PUMP_ERR, TYPE_NOTI_ERROR);
	u8_error_flowmeter_id = 3;
	b_error_handler_reg("Pressure error", ERROR_REPEAT_TIMEOUT, v_flowmeter_repeat_err, &u8_repeat_flow_err_id);
	v_irrigation_event_pause();
}
