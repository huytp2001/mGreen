/*! @file ports_control.c
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
#include "ports_control.h"
#include "sensor.h"
#include "spi_mgreen.h"
#include "pHEC.h"
#include "frame_parser.h"
#include "mqtt_publish.h"
#include "wdt.h"
#include "schedule.h"
#include "threshold.h"
#include "manual.h"
#include "neutralize.h"
#include "vpd_control.h"

#include "relay_control.h"
#include "params.h"
#include "digital_input.h"
#include "mgreen_timeout.h"
#include "device_setting.h"
#include "display_task.h"
#include "setting.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "sd_card.h"
#include "rtc.h"
#include "download.h"
#include "MCP23017.h"
/*!
* static data declaration
*/
uint64_t u64_curr_port_state = 0x00;
uint64_t u64_pre_port_state = 0x00;
static E_PORTS_CONTROL_STATE e_ports_control_state = STATE_BEGIN;

static StaticTask_t xPortControlTaskBuffer;
static StackType_t  xPortControlStack[PORTS_CONTROL_TASK_STACK_SIZE];

/*!
* private function prototype
*/
static void v_ports_control_task (void *pvParameters);
static void v_ports_control (uint64_t u64_old_state, uint64_t u64_new_state);
static void v_ports_control_off_all (void);

/*!
* public function bodies
*/
/*!
 * @fn void v_ports_control_task_init (void)
 * @brief 
 * @param[in] none
 * @return none
 */
void v_ports_control_task_init (void)
{		
	#ifdef VPD_CONTROL_MODE
//	v_vpd_process_init();
	#endif
	xTaskCreateStatic(v_ports_control_task,"ports_control task", PORTS_CONTROL_TASK_STACK_SIZE,
							NULL, PORTS_CONTROL_TASK_PRIORITY, xPortControlStack, &xPortControlTaskBuffer);
}

void v_set_ports_control_state (E_PORTS_CONTROL_STATE e_state)
{
	e_ports_control_state = e_state;
	if ((STATE_REQ_RUN == e_state) || (STATE_CON_RUN == e_state))
	{
		
	}
}

E_PORTS_CONTROL_STATE e_get_ports_control_state(void)
{
	return e_ports_control_state;
}

void v_new_schedule_set(void)
{
	E_PORTS_CONTROL_STATE e_state = e_get_ports_control_state();
	if ((STATE_RUN == e_state) || (STATE_REQ_RUN == e_state) || (STATE_CON_RUN == e_state))
	{
		v_ports_control_off_all();
		v_set_ports_control_state(STATE_REQ_RUN);	
	}
	else
	{
		
	}
}

uint64_t u64_port_control_get_status(void)
{
	return u64_curr_port_state;
}

/*!
* private function bodies
*/
static void v_ports_control (uint64_t u64_old_state, uint64_t u64_new_state)
{
	if(u64_new_state != u64_old_state)
	{
		b_relay_control(u64_new_state, &u64_curr_port_state);
		u64_pre_port_state = u64_curr_port_state;		//update latest value for port pointer
		//Send to MQTT to notify change port state
		v_mqtt_ports_state(u64_curr_port_state);
		//check and retry
		if(u64_curr_port_state != u64_new_state)
		{
			b_relay_control(u64_new_state, &u64_curr_port_state);
			u64_curr_port_state = u64_new_state;
			u64_pre_port_state = u64_curr_port_state;
		}
	}
}

static void v_ports_control_off_all (void)
{
	b_relay_turn_off_all();
	u64_curr_port_state = 0x00;
	u64_pre_port_state = u64_curr_port_state;		//update latest value for port pointer
	//Send to MQTT to notify change port state
	v_mqtt_ports_state(u64_curr_port_state);
}

static void v_check_ports_condition(uint64_t *pu64_curr_valve_state)
{
	uint64_t u64_curr_valve_state_tmp = *pu64_curr_valve_state;
	STRU_PORT_OUTPUT port_output;
	STRU_PORT_INPUT port_input;
	uint32_t au32_input_status[4] = {0, 0, 0, 0};
	v_get_all_di_status(au32_input_status);
	for(uint8_t i = 0; i < MAX_OUTPUT_SETTING; i++)
	{
		b_output_setting_get(i, &port_output);
		if(u64_curr_valve_state_tmp & (1 << i))
		{
			for(uint8_t j = 0; j < MAX_INPUT_SETTING; j++)
			{
				b_input_setting_get(j, &port_input);
				if(port_input.u8_map_output == (i + 1) && port_input.u8_port != 0)
				{
					switch(port_input.e_input_type)
					{
						case TYPE_I_ON_INDICATOR:
						{
							if(i16_get_timeout_status(e_timeout_input_table[port_input.u8_port - 1]) == -1)	//timeout flag is not exist
							{
								v_timeout_reg(e_timeout_input_table[port_input.u8_port - 1], port_input.u32_time_effect);					
							}
							if(au32_input_status[port_input.u8_port - 1] != 0)														//input show that port is turnon -> OK
							{
								v_timeout_reset(e_timeout_input_table[port_input.u8_port - 1]);							 
							}
							else
							{
								if(i16_get_timeout_status(e_timeout_input_table[port_input.u8_port - 1]) == 0)	//timeout is occurred
								{
									v_timeout_unreg(e_timeout_input_table[port_input.u8_port - 1]);
									u64_curr_valve_state_tmp &= ~(1 << i);
									u64_curr_valve_state_tmp &= ~(u64_getclear_dependent_slave_port(i + 1));
									v_manual_stop_by_port(1 << i);
									v_mqtt_noti_pub(DATAID_PUMP_ERR, TYPE_NOTI_ERROR);
								}
							}
						}
						break;
						case TYPE_I_OFF_INDICATOR:
						{
							if(au32_input_status[port_input.u8_port - 1] != 0)					//input show that port need to turn off -> OK
							{
								if(i16_get_timeout_status(e_timeout_input_table[port_input.u8_port - 1]) == -1)
								{
									v_timeout_reg(e_timeout_input_table[port_input.u8_port - 1], port_input.u32_time_effect);	
								}
								else if(i16_get_timeout_status(e_timeout_input_table[port_input.u8_port - 1]) == 0)
								{
									v_timeout_unreg(e_timeout_input_table[port_input.u8_port - 1]);
									u64_curr_valve_state_tmp &= ~(1 << i);
									u64_curr_valve_state_tmp &= ~(u64_getclear_dependent_slave_port(i + 1));
									v_manual_stop_by_port(1 << i);
									v_mqtt_noti_pub(DATA_ID_INPUT_1 + port_input.u8_port, TYPE_NOTI_HIGH);
								}
							}
							else
							{
								v_timeout_unreg(e_timeout_input_table[port_input.u8_port - 1]);
							}
						}
						break;
						case TYPE_I_SWITCH_NC:
						{
							if(au32_input_status[port_input.u8_port - 1] == 0) //NC => open
							{
								u64_curr_valve_state_tmp &= ~(1 << i);
								v_manual_stop_by_port(1 << i);
								v_mqtt_noti_pub(DATA_ID_INPUT_1 + port_input.u8_port, TYPE_NOTI_LOW);
							}
						}
						break;
						case TYPE_I_SWITCH_NO:
						{
							if(au32_input_status[port_input.u8_port - 1] != 0) //NO => close
							{
								u64_curr_valve_state_tmp &= ~(1 << i);
								v_manual_stop_by_port(1 << i);
								v_mqtt_noti_pub(DATA_ID_INPUT_1 + port_input.u8_port, TYPE_NOTI_HIGH);
							}
						}
						break;
						case TYPE_I_COUNTER:
						{
							if(i16_get_timeout_status(e_timeout_input_table[port_input.u8_port - 1]) == -1)	//timeout flag is not exist
							{
								v_timeout_reg(e_timeout_input_table[port_input.u8_port - 1], port_input.u32_time_effect);					
							}
							if(au32_input_status[port_input.u8_port - 1] != 0)														//input show that port is turnon -> OK
							{
								v_timeout_reset(e_timeout_input_table[port_input.u8_port - 1]);							 
							}
							else
							{
								if(i16_get_timeout_status(e_timeout_input_table[port_input.u8_port - 1]) == 0)	//timeout is occurred
								{
									v_timeout_unreg(e_timeout_input_table[port_input.u8_port - 1]);
									u64_curr_valve_state_tmp &= ~(1 << i);
									u64_curr_valve_state_tmp &= ~(u64_getclear_dependent_slave_port(i + 1));
									v_manual_stop_by_port(1 << i);
									v_mqtt_noti_pub(DATAID_PUMP_ERR, TYPE_NOTI_ERROR);
								}
							}
						}
						break;
						default:
							break;
					}
				}
			}
		}
		else
		{
			for(uint8_t j = 0; j < MAX_INPUT_SETTING; j++)
			{
				b_input_setting_get(j, &port_input);
				if(port_input.u8_map_output == (i + 1) && port_input.u8_port != 0)
				{
					v_timeout_unreg(e_timeout_input_table[port_input.u8_port - 1]);
				}
			}
		}
	}
	*pu64_curr_valve_state = u64_curr_valve_state_tmp;
}

static void v_ports_control_task (void *pvParameters)
{	
	static uint8_t u8_ports_control_state_tmp = 0;
	static uint8_t u8_ports_control_task_run_count = 0;
	static uint8_t au8_ports_control_retry[2] = {0, 0};
	static uint8_t u8_port_control_task_id = 0;
	static T_DATETIME t_today; 
	static bool b_pub_csv_version = true;
	static uint8_t u8_num_of_exboard;
	while(b_wdt_reg_new_task("port_control_task", &u8_port_control_task_id) != true){}	
	u8_num_of_exboard = SW1_READ() | (SW2_READ() << 1);
	for(;;)
	{
		//reload wdt counter
		b_wdt_task_reload_counter(u8_port_control_task_id);
		//Get time
		v_rtc_read_time(&t_today);
		/*
		//Reset at the begining of day 
		if(t_today.u8_hour == 0 && t_today.u8_minute == 0 && t_today.u8_second < 4)
		{
			vTaskDelay(5000);
			ROM_SysCtlReset();
		}
		if (b_connection_get() && b_pub_csv_version)
		{
			b_pub_csv_version = false;
			char c_version[21] = {0};
			//todo: pub csv version
		}
		//TODO: check port status
		u8_ports_control_task_run_count++;
		if(u8_ports_control_task_run_count > PORTS_CONTROL_RETRY_TIMEOUT / PORTS_CONTROL_TASK_DELAY)
		{
			u8_ports_control_task_run_count = 0;
			static uint64_t u64_ex_status;
			
			for(uint8_t i = RELAY_I2C1_ADDR; i <= u8_num_of_exboard; i++)
			{
				u64_ex_status = 0;
				if(b_get_ex_relay_status(i, &u64_ex_status))
				{
					if(u64_ex_status != ((u64_curr_port_state>>8)  << (i * 16)))
					{
						v_mqtt_data_pub(0x0e, 0000);
						v_relay_hardware_init();
						v_ports_control(0, u64_curr_port_state);
						au8_ports_control_retry[i]++;
						if(au8_ports_control_retry[i] > PORTS_CONTROL_MAX_RETRY)
						{
							au8_ports_control_retry[i] = 0;
							v_mqtt_noti_pub(DATAID_EXRELAY1_ERR + i, TYPE_NOTI_SENSOR_LOST);
						}
					}
					else
					{
						au8_ports_control_retry[i] = 0;
					}
				}
				else
				{
					v_relay_hardware_init();
					v_ports_control(0, u64_curr_port_state);
					au8_ports_control_retry[i]++;
					if(au8_ports_control_retry[i] > PORTS_CONTROL_MAX_RETRY)
					{
						au8_ports_control_retry[i] = 0;
						v_mqtt_noti_pub(DATAID_EXRELAY1_ERR + i, TYPE_NOTI_SENSOR_LOST);
					}
				}
			}
		}				
		//TODO: Control ports
		switch(e_get_ports_control_state())
		{
			case STATE_BEGIN:
			{
				//TODO: get ports_control_state from EEPROM
				s32_params_get(PARAMS_ID_PORTS_CTR_STATE, &u8_ports_control_state_tmp);
				if(u8_ports_control_state_tmp > (uint8_t)NUM_PORTS_CONTROL_STATE)
				{			
					v_set_ports_control_state(STATE_REQ_STOP);
				}
				else
				{
					v_set_ports_control_state((E_PORTS_CONTROL_STATE)u8_ports_control_state_tmp);					
				}
				v_ports_control_off_all();
			}
			break;
			case STATE_STOP:
			{
				//Do nothing
			}
			break;
			case STATE_REQ_RUN:
			{
				//TODO: notify to MQTT
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
				//TODO:save ports_control_state to EEPROM
				u8_ports_control_state_tmp = (uint8_t)STATE_REQ_RUN;
				s32_params_set(PARAMS_ID_PORTS_CTR_STATE, 1, &u8_ports_control_state_tmp);		
				i_load_schedule_from_sd_card();
				//TODO: switch to STATE_RUN
				v_set_ports_control_state(STATE_RUN);	
				#ifdef NEUTRALIZER_MODE
					v_clear_neu_schedule();
				#endif
			}
			break;
			case STATE_CON_RUN:
			{
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
				u8_ports_control_state_tmp = (uint8_t)STATE_REQ_RUN;
				s32_params_set(PARAMS_ID_PORTS_CTR_STATE, 1, &u8_ports_control_state_tmp);			
				v_sche_reload_event(&u64_curr_port_state);
				v_ports_control(u64_pre_port_state, u64_curr_port_state);	
				v_set_ports_control_state(STATE_RUN);		
			}
			break;
			case STATE_RUN:
			{
				if ((OPERATING_MODE == E_SCHEDULE_MODE) || (OPERATING_MODE == E_FS_MODE))
				{
					v_schedule_process (&u64_curr_port_state);
				}
				#ifdef SCHEDULE_MODE
					v_schedule_process (&u64_curr_port_state);		
				#endif
				
				#ifdef NEUTRALIZER_MODE
					v_neutralize_schedule(&u64_curr_port_state);
				#endif
				
				#ifdef VPD_CONTROL_MODE
					v_vpd_process(&u64_curr_port_state);
				#endif
				
				#ifdef THRESHOLD_MODE
					v_threshold_process(&u64_curr_port_state);
				#endif
			}
			break;
			case STATE_REQ_STOP:
			{				
				//TODO: First: turn off all ports
				u64_curr_port_state = 0;
				v_ports_control(u64_pre_port_state, u64_curr_port_state);
				u64_pre_port_state = u64_curr_port_state;
				b_manual_stop_all();
				//TODO: notify to MQTT
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_STOPED);
				//TODO:save ports_control_state to EEPROM				
				u8_ports_control_state_tmp = (uint8_t)STATE_REQ_STOP;
				s32_params_set(PARAMS_ID_PORTS_CTR_STATE, 1, &u8_ports_control_state_tmp);				
				
				//TODO: switch to STATE_STOP
				v_set_ports_control_state(STATE_STOP);
				
				v_valve_state_set(VALVE_STATE_NONE);
				#ifdef NEUTRALIZER_MODE
					v_neutralize_set_state(NEUTRAL_REQUEST_STOP);
				#endif
			}
			break;
			default: break;
		}		
*/
		v_manual_process (&u64_curr_port_state);
		//v_check_ports_condition(&u64_curr_port_state);
		//control port;
		v_ports_control(u64_pre_port_state, u64_curr_port_state);		
		bool test = false;
		if (test)
		{
			b_save_file_process(FILE_TYPE_FIRMWARE);
		}
		vTaskDelay(PORTS_CONTROL_TASK_DELAY);
	}		
}

