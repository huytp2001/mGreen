/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.c
 * @author Danh Pham
 * @date 13 Nov 2020
 * @version: 1.0.0
 * @brief This file contains functions and tasks use to find the proper irrigation event .
 *
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <string.h>
 
 #include "FreeRTOS.h"
 #include "task.h"
 
 #include "rom.h"
 #include "rtc.h"
 #include "HAL_BSP.h"
 #include "schedule_task.h"
 #include "schedule_parser.h"
 #include "config.h"
 #include "sd_app.h"
 #include "sd_manage_lib.h"
 #include "mqtt_publish.h"
 #include "error_handler.h"
 #include "wdt.h"
 #include "mqtt_task.h"
/*!
* @def SCHEDULE_TASK_SIZE
* Memory size of schedule task (minsize * 4)
*/
#define SCHEDULE_TASK_SIZE			(configMINIMAL_STACK_SIZE * 4)

/*!
*	@def SCHEDULE_TASK_PRIORITY
* Priority of schedule task (2)
*/
#define SCHEDULE_TASK_PRIORITY	(tskIDLE_PRIORITY + 2)

/*!
*	@def SCHEDULE_TASK_DELAY
* Schedule task delay (1000ms)
*/
#define SCHEDULE_TASK_DELAY			(portTickType)(1000 / portTICK_RATE_MS)

/*!
* @def SCHEDULE_RETRY_TIME
*/
#define SCHEDULE_RETRY_TIME			3

/*!
*	Variables declare
*/
static StaticTask_t xFTG_Runtime_TaskBuffer;
static StackType_t  xFTG_Runtime_Stack[SCHEDULE_TASK_SIZE];

static uint16_t u16_current_order = 0;	/**<Current event order read from schedule table
																				start form 1*/
static uint8_t u8_schedule_task_id = 0;	/**<ID which watchdog allocated for schedule task */
static T_DATETIME t_remain_time;	/**<Duration from now to next irrigation event */
static bool b_is_new_schedule = true;
static uint8_t u8_retry_load_file = 0;
static E_SCHEDULE_ERR_CODE e_schedule_err = SCHE_ERR_NULL;
static uint16_t u16_done_order = 0;
static E_SCHEDULE_STAGE e_schedule_stage = SCHE_STAGE_LOAD;
static bool b_is_download_file = false;

/*!
*	Private functions prototype
*/

static void v_schedule_task(void* pv_parameters);
static void v_schedule_parse_err(void);
static void v_set_schedule_err(E_SCHEDULE_ERR_CODE);
/*!
*	Public functions
*/

/*!
*	@fn t_schedule_remain_time_get(void)
* @brief return duration from now to next irrigation event
* @return T_DATETIME
*/
T_DATETIME t_schedule_remain_time_get(void)
{
	return t_remain_time;
}

void v_new_schedule_available_set(void)
{
	b_is_new_schedule = true;
	b_is_download_file = true;
	// reset error flag and handler id
	u8_retry_load_file = 0;
	//clear last pointer in sdcard
	i8_sdcard_save_schedule_order(0);
}

void v_schedule_back(void)
{
	uint16_t u16_order = u16_done_order - 1;
	i8_sdcard_save_schedule_order(u16_order);
}

E_SCHEDULE_STAGE e_schedule_sate_get(void)
{
	return e_schedule_stage;
}
/*!
* @fn v_schedule_task_init(void)
* @brief Init schedule task, must be called in main function.
*/
void v_schedule_task_init(void)
{
	xTaskCreateStatic(v_schedule_task, "SCHEDULE_TASK", 
										SCHEDULE_TASK_SIZE, NULL, 
										SCHEDULE_TASK_PRIORITY, 
										xFTG_Runtime_Stack, 
										&xFTG_Runtime_TaskBuffer);
}

/*!
*	Private functions
*/

/*!
*	@fn v_set_schedule_err(E_SCHEDULE_ERR_CODE)
* @brief Save the error code when parsing schedule
* @param[in] e_code
* @return None
*/
static void v_set_schedule_err(E_SCHEDULE_ERR_CODE e_code)
{
	e_schedule_err = e_code;
}
/*!
*	@fn void v_schedule_task(void* pv_parameters)
* @brief Schedule program task. In the case the schedule file is  copied manually to SDcard,
* turn DIP Switch 3 to ON 
* @param[in] pv_parameters
* @return None
*/
static void v_schedule_task(void* pv_parameters)
{
	static E_SCHEDULE_ERR_CODE e_code;
	static uint8_t u8_parse_err_id = 0;
	static bool b_pub_csv_version = true;
	static uint16_t u16_fer_day = 0;
	//register task with watchdog timer
	while(b_wdt_reg_new_task("schedule_task", &u8_schedule_task_id) != true){}

	#if (0)	/* Use for debug */
		uint32_t u32_time = 1616029550 + 7*3600 ; 
		T_DATETIME t_time;
		v_long_2_datetime(&t_time, u32_time);
		b_rtc_set_time(&t_time);
		i8_sdcard_save_schedule_order(1);
		i8_sdcard_save_IRRIGATION_day(21);
	#endif
		
	for( ; ; )
	{
		static T_DATETIME t_today;  
		static uint32_t u32_realtime = 0;
		static STRU_IRRIGATION_SCHEDULE stru_irrigation_event;
		static uint16_t u16_total_row = 0;		
		
		//reset wdt flag
		b_wdt_task_reload_counter(u8_schedule_task_id);
		//Get time
		v_rtc_read_time(&t_today);
		u32_realtime = u32_datetime_2_long(&t_today)%86400;
		//Reset at the begining of day 
		if(t_today.u8_hour == 0 && t_today.u8_minute == 0 && t_today.u8_second < 4)
		{
			i8_sdcard_save_schedule_order(0);
			i8_sdcard_save_fertigation_day(t_today.u8_date);
			vTaskDelay(5000);
			ROM_SysCtlReset();
		}
		
		if (t_today.u16_year > 2020)
		{
			//parse schedule
			if(b_is_new_schedule)
			{
				b_is_new_schedule = false;
				e_code = e_schedule_parse(CONFIG_SCHEDULE_FILENAME, false);	/* Parse file with CRC */
				//load from begin of schedule 
				e_schedule_stage = SCHE_STAGE_LOAD;
				// reset queue IRRIGATION
				v_irrigation_queue_reset();
				v_mqtt_data_pub(0x000d, e_code);
				LED_DB_1_OFF();
			}
			//process schedule
			u16_total_row = u16_schedule_total_row_get();
			if(SCHE_ERR_NULL == e_code)
			{
				if (b_connection_get() && b_pub_csv_version)
				{
					b_pub_csv_version = false;
					char c_version[21] = {0};
					v_schedule_version_get(c_version);
					v_mqtt_schedule_version_pub(c_version);
				}
				
				if(u8_parse_err_id != 0)
				{
					if(b_error_unreg(u8_parse_err_id) == true)
					{
						u8_parse_err_id = 0;
					}
				}
								
				//seek the irrigation event
				switch(e_schedule_stage)
				{
					case SCHE_STAGE_LOAD:
					{
						u16_current_order = 1;
						e_schedule_stage = SCHE_STAGE_SEEK_EVENT;
						i8_sdcard_get_fertigation_day(&u16_fer_day);
						if (u16_fer_day != t_today.u8_date)
						{
							i8_sdcard_save_schedule_order(0);
							i8_sdcard_save_fertigation_day(t_today.u8_date);
							//v_new_schedule_available_set();
							v_mqtt_data_pub(0xaaaa, 0);
						}
					}
					break;
					case SCHE_STAGE_SEEK_EVENT:
					{
						uint16_t u16_last_order = 0;
						/* TODO: disable to debug */
						i8_sdcard_get_schedule_order(&u16_last_order);
						/* When current time greater start time of the event or the event was
						* irrigated, it will be skip. The loop also check if the current order
						* is still in range of total event.
						*/
						while(((u32_realtime > (stru_schedule_event_get(u16_current_order - 1)
																	.u32_time_start + 50))
									|| (u16_last_order >= u16_current_order))
									&& (u16_current_order <= u16_total_row))
						{
							if(u16_current_order >= u16_last_order)
							{
								i8_sdcard_save_schedule_order(u16_current_order);
							}
							u16_current_order++;
						}
						/* When above process can find an event which have start time less than
						* current time, it will compare the checksum to ensure the event is in right form
						*/
						if(u16_current_order <= u16_total_row)
						{
							memset(&stru_irrigation_event, 0, sizeof(STRU_IRRIGATION_PROGRAM));
							stru_irrigation_event = stru_schedule_event_get(u16_current_order - 1);
							static uint16_t u16_current_checksum = 0;
							static uint16_t u16_saved_checksum = 0;
							u16_current_checksum = u16_checksum(stru_irrigation_event.u32_time_start);
							u16_saved_checksum = u16_schedule_checksum_get(u16_current_order - 1);
							if(u16_current_checksum == u16_saved_checksum)
							{
								e_schedule_stage = SCHE_STAGE_CHECK_TIME;
							}
							else
							{
								v_mqtt_noti_pub(DATAID_PARSECSV_ERROR, TYPE_NOTI_CRC);
							}							
						}
						else
						{
							e_schedule_stage = SCHE_STAGE_TIME_OVER;
						}
					}
					break;
					case SCHE_STAGE_CHECK_TIME:
					{
						if(u16_current_order <= u16_total_row)
						{
							/* Start the aeration pump */
							if(e_get_irrigation_state() == IRRIGATION_EVENT_WAIT)
							{
								if(u32_realtime < stru_irrigation_event.u32_time_start)
								{
									LED_DB_1_ON();
								}
							}
							/* Parse irrigation event to IRRIGATION program */
							if(u32_realtime >= stru_irrigation_event.u32_time_start)
							{
								LED_DB_1_OFF();
								STRU_IRRIGATION_PROGRAM stru_irrigation_program;
								memcpy(stru_irrigation_program.astru_valve, 
											stru_irrigation_event.a_stru_valves, 
											stru_irrigation_event.u8_num_of_valves);
								memcpy(stru_irrigation_program.au32_rate, stru_irrigation_event.au32_dosing_rate, 5 * sizeof(uint32_t));
								stru_irrigation_program.b_phec_adjust = false;
								if( (stru_irrigation_event.u16_ec != 0) || (stru_irrigation_event.u16_ph != 0))
								{
									stru_irrigation_program.b_phec_adjust = true;
								}
								stru_irrigation_program.e_dosing_type = stru_irrigation_event.e_dosing_type;
								stru_irrigation_program.e_duration_unit = stru_irrigation_event.e_duration_unit;
								stru_irrigation_program.u16_ec = stru_irrigation_event.u16_ec;
								stru_irrigation_program.u16_ph = stru_irrigation_event.u16_ph;
								stru_irrigation_program.u32_after_duration = stru_irrigation_event.u32_after_duration;
								stru_irrigation_program.u32_before_duration = stru_irrigation_event.u32_before_duration;
								stru_irrigation_program.u32_total_duration = stru_irrigation_event.u32_total_duration;
								stru_irrigation_program.u8_location_id = stru_irrigation_event.u8_location_id;
								stru_irrigation_program.u8_no_valves = stru_irrigation_event.u8_num_of_valves;
								stru_irrigation_program.d_default_flowrate = stru_irrigation_event.d_flow_rate;
								memcpy(stru_irrigation_program.location_name, stru_irrigation_event.c_location_name,
																			strlen(stru_irrigation_event.c_location_name));
								for(uint8_t i = 0; i < stru_irrigation_program.u8_no_valves; i++)
								{
									stru_irrigation_program.astru_valve[i] = stru_irrigation_event.a_stru_valves[i];
								}
								/* Call function from IRRIGATION program to update new program */
								if((e_get_irrigation_state() == IRRIGATION_EVENT_BEGIN) ||
									 (e_get_irrigation_state() == IRRIGATION_EVENT_RUN) ||
									 (e_get_irrigation_state() == IRRIGATION_EVENT_WAIT) ||
									 (e_get_irrigation_state() == IRRIGATION_EVENT_WAIT_CONTINUE) ||
									 (e_get_irrigation_state() == IRRIGATION_EVENT_STOP) )
								{
									while(b_irrigation_event_add(stru_irrigation_program) != true)
									{
										vTaskDelay(100);
									}
								}
								i8_sdcard_save_schedule_order(u16_current_order);
								u16_done_order = u16_current_order;
								u16_current_order++;
								e_schedule_stage = SCHE_STAGE_SEEK_EVENT;
							}
							else
							{
								static uint32_t u32_remain_time = 0;
								u32_remain_time = stru_irrigation_event.u32_time_start - u32_realtime;
							}
						}
					}
					break;
					case SCHE_STAGE_TIME_OVER:
					{
						static bool b_clear_err = false;
						if ((t_today.u8_hour >= TIME_STOP_REPEAT_ERROR) && (!b_clear_err))
						{
							b_clear_err = true;
							//todo: clear repeat error
							//v_clear_all_error();
						}
					}
					break;
					default:
						break;
				}
			}
			else		//file is not available or error
			{			
				if(u8_retry_load_file < SCHEDULE_RETRY_TIME)
				{
					u8_retry_load_file++;
					b_is_new_schedule = true;
				}
				else
				{
					if(SCHEDULE_RETRY_TIME == u8_retry_load_file)
					{
						u8_retry_load_file++;
						v_mqtt_noti_pub(DATAID_PARSECSV_ERROR, (E_NOTI_TYPE)e_schedule_err);
						b_error_handler_reg("parse csv error", 600, v_schedule_parse_err, &u8_parse_err_id);
						v_set_schedule_err(e_code);
					}
					if (b_is_download_file)
					{
						if(b_is_publish_free())
						{
							ROM_SysCtlReset();
						}
					}
				}
			}
		}
		vTaskDelay(SCHEDULE_TASK_DELAY);
	}
}

/*!
* @fn 
* @brief
*/
static void v_schedule_parse_err(void)
{
	v_mqtt_noti_pub(DATAID_PARSECSV_ERROR, (E_NOTI_TYPE)e_schedule_err);
}
