#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "neutralize.h"
#include "rtc.h"
#include "sensor.h"
#include "mqtt_publish.h"
#include "eeprom_manage.h"
#include "digital_input.h"
#include "display_task.h"

#include "FreeRTOS.h"
#include "queue.h"

extern xQueueHandle x_pHEC_Queue;

static E_NEUTRALIZE_STATE e_neutral_state = NEUTRAL_START;
static uint32_t u32_time_stop_valve = 0;
static uint32_t u32_time_stop_aeration = 0;

static uint16_t u16_neutral_sample_duration = 10;	//seconds
static uint16_t u16_neutral_time_inject_acid = 5;	//seconds
static uint16_t u16_neutral_time_aeration = 900; //seconds
static uint16_t u16_neutral_start_point = 650;	//strart neutralization at pH 6.5
static uint16_t u16_neutral_stop_point = 580;	//stop neutralization at pH 5.8

static uint32_t u32_neutral_start_time = 25200;	//07:00
static uint32_t u32_neutral_stop_time	= 57600;	//16:00

static uint8_t u8_count_pHEC = 0;
static uint8_t u8_count_err = 0;

static SCHEDULE_STATE e_neutralize_schedule_state = SCHE_STATE_LOAD;
static uint16_t u16_current_order = 0;
static uint32_t u32_start_time;

static bool b_float_switch = true;

static void v_neutral_load_config(void)
{
	uint32_t u32_valid_value = 0;
	EEPROMRead(&u32_valid_value, EEPROM_NEUTRAL_VALID_ADDR, sizeof(uint32_t));
	if(u32_valid_value == 1)
	{
		EEPROMRead((uint32_t *)&u16_neutral_sample_duration, EEPROM_NEUTRAL_TIME_SAMPLE_ADDR, sizeof(uint32_t));
		EEPROMRead((uint32_t *)&u16_neutral_time_inject_acid, EEPROM_NEUTRAL_TIME_INJECT_ACID_ADDR, sizeof(uint32_t));
		EEPROMRead((uint32_t *)&u16_neutral_time_aeration, EEPROM_NEUTRAL_TIME_AERATION_ADDR, sizeof(uint32_t));
		EEPROMRead((uint32_t *)&u16_neutral_start_point, EEPROM_NEUTRAL_START_POINT_ADDR, sizeof(uint32_t));
		EEPROMRead((uint32_t *)&u16_neutral_stop_point, EEPROM_NEUTRAL_STOP_POINT_ADDR, sizeof(uint32_t));
		EEPROMRead(&u32_neutral_start_time, EEPROM_NEUTRAL_START_TIME_ADDR, sizeof(uint32_t));
		EEPROMRead(&u32_neutral_stop_time, EEPROM_NEUTRAL_STOP_TIME_ADDR, sizeof(uint32_t));
	}
	else
	{
		u32_valid_value = 1;
		EEPROMProgram(&u32_valid_value, EEPROM_NEUTRAL_VALID_ADDR, sizeof(uint32_t));
		EEPROMProgram((uint32_t *)&u16_neutral_sample_duration, EEPROM_NEUTRAL_TIME_SAMPLE_ADDR, sizeof(uint32_t));
		EEPROMProgram((uint32_t *)&u16_neutral_time_inject_acid, EEPROM_NEUTRAL_TIME_INJECT_ACID_ADDR, sizeof(uint32_t));
		EEPROMProgram((uint32_t *)&u16_neutral_time_aeration, EEPROM_NEUTRAL_TIME_AERATION_ADDR, sizeof(uint32_t));
		EEPROMProgram((uint32_t *)&u16_neutral_start_point, EEPROM_NEUTRAL_START_POINT_ADDR, sizeof(uint32_t));
		EEPROMProgram((uint32_t *)&u16_neutral_stop_point, EEPROM_NEUTRAL_STOP_POINT_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_neutral_start_time, EEPROM_NEUTRAL_START_TIME_ADDR, sizeof(uint32_t));
		EEPROMProgram(&u32_neutral_stop_time, EEPROM_NEUTRAL_STOP_TIME_ADDR, sizeof(u32_neutral_stop_time));
	}
}

void v_set_neutral_sample_duration(uint16_t u16_duration)
{
	u16_neutral_sample_duration = u16_duration;
	EEPROMProgram((uint32_t *)&u16_neutral_sample_duration, EEPROM_NEUTRAL_TIME_SAMPLE_ADDR, sizeof(uint32_t));
}
void v_set_neutral_time_inject_acid(uint16_t u16_time)
{
	u16_neutral_time_inject_acid = u16_time;
	EEPROMProgram((uint32_t *)&u16_neutral_time_inject_acid, EEPROM_NEUTRAL_TIME_INJECT_ACID_ADDR, sizeof(uint32_t));
}
void v_set_neutral_time_aeration(uint16_t u16_time)
{
	u16_neutral_time_aeration = u16_time;
	EEPROMProgram((uint32_t *)&u16_neutral_time_aeration, EEPROM_NEUTRAL_TIME_AERATION_ADDR, sizeof(uint32_t));
}

void v_set_netral_start_point(uint16_t u16_point)
{
	u16_neutral_start_point = u16_point;
	EEPROMProgram((uint32_t *)&u16_neutral_start_point, EEPROM_NEUTRAL_START_POINT_ADDR, sizeof(uint32_t));
}
void v_set_neutral_stop_point(uint16_t u16_point)
{
	u16_neutral_stop_point = u16_point;
	EEPROMProgram((uint32_t *)&u16_neutral_stop_point, EEPROM_NEUTRAL_STOP_POINT_ADDR, sizeof(uint32_t));
}

void v_set_neutral_start_time(uint32_t u32_time)
{
	u32_neutral_start_time = u32_time;
	EEPROMProgram(&u32_neutral_start_time, EEPROM_NEUTRAL_START_TIME_ADDR, sizeof(uint32_t));
}
void v_set_neutral_stop_time(uint32_t u32_time)
{
	u32_neutral_stop_time = u32_time;
	EEPROMProgram(&u32_neutral_stop_time, EEPROM_NEUTRAL_STOP_TIME_ADDR, sizeof(uint32_t));
}

void v_neutralize_set_state(E_NEUTRALIZE_STATE e_state)
{
	v_neutral_load_config();
	e_neutral_state = e_state;
}

void v_neutralize_schedule(uint64_t *u64_port_control_value)
{
	uint64_t u64_curr_valve_state_tmp = *u64_port_control_value;
	static T_DATETIME stru_curr_datetime;
	static uint32_t u32_curr_time;
	v_rtc_read_time(&stru_curr_datetime);
	u32_curr_time = stru_curr_datetime.u8_hour * 3600 + 
									stru_curr_datetime.u8_minute *60 +
									stru_curr_datetime.u8_second;
	switch(e_neutralize_schedule_state)
	{
		case SCHE_STATE_LOAD:
		{
			u16_current_order = 0;
			v_display_waiting_LCD();
			e_neutral_state = NEUTRAL_IDLE;
			e_neutralize_schedule_state = SCHE_STATE_SEEK_EVENT;
		}break;
		case SCHE_STATE_SEEK_EVENT:
		{
			uint16_t u16_num_row = u16_get_total_turn();
			while(u32_curr_time > get_irrigation_row(u16_current_order).u32_time_start + 59)
			{
				u16_current_order++;
			}
			if(u16_current_order < u16_num_row)
			{
				e_neutralize_schedule_state = SCHE_STATE_CHECK_TIME;
			}
			else
			{
				e_neutralize_schedule_state = SCHE_STATE_TIME_OVER;
				v_mqtt_data_pub(0,0xffff);
			}
		}break;
		case SCHE_STATE_TIME_OVER:
		{
			
		}break;
		case SCHE_STATE_CHECK_TIME:
		{
 			if(u32_curr_time >= get_irrigation_row(u16_current_order).u32_time_start)
			{
				if(b_neutralize_process(&u64_curr_valve_state_tmp, get_irrigation_row(u16_current_order)))
				{
					u16_current_order++;
					uint16_t u16_num_row = u16_get_total_turn();
					if(u16_current_order >= u16_num_row)
					{
						e_neutralize_schedule_state = SCHE_STATE_TIME_OVER;
					}
				}
			}
		}break;
		default:
			break;
	}
	*u64_port_control_value = u64_curr_valve_state_tmp;
}

bool b_neutralize_process(uint64_t *u64_port_control_value, STRU_IRRIGATION str_irr_row)
{
	bool b_return_value = false;
	static uint8_t u8_ph_over_cnt = 0;
	uint64_t u64_temp_port_value = *u64_port_control_value;
	T_DATETIME t_time;
	v_rtc_read_time(&t_time);
	uint32_t u32_cur_time = u32_datetime_2_long(&t_time) % 86400;
	if(SW1_READ())
	{
		if(!SW2_READ())
		{
			b_float_switch = !INPUT_1_READ();
		}
		else
		{
			b_float_switch = !INPUT_2_READ();
		}
	}
	else
	{
		if(!SW2_READ())
		{
			b_float_switch = INPUT_1_READ();
		}
		else
		{
			b_float_switch = INPUT_2_READ();
		}
	}
	if(b_float_switch)
	{
		switch(e_neutral_state)
		{
			case NEUTRAL_IDLE:		//check time
			{
				e_neutral_state = NEUTRAL_START;
			}
			case NEUTRAL_START:
			{
				//display status
				v_display_active();
				v_display_active_zone(str_irr_row.cname);
				//start pump
				u64_temp_port_value |= NEUTRAL_PUMP_PORT;
				u64_temp_port_value |= str_irr_row.u64_valve;
				//turn on pH sensor
				u8_count_pHEC = 0;
				v_pH_EC_set_state(1, u16_neutral_sample_duration);
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_RUNNING);
				u8_ph_over_cnt = 0;
				u32_start_time = u32_rtc_unix_time_get();
				v_mqtt_fertigation_start(str_irr_row.u16_location_ID);
				e_neutral_state = NEUTRAL_CHECK_PH;
			}break;
			case NEUTRAL_CHECK_PH:		//turn on pump and read pH
			{
				uint32_t u32_pHEC_data = 0;
				if(xQueueReceive(x_pHEC_Queue, &u32_pHEC_data, 10))
				{
					if(u32_pHEC_data != 0)
					{
						u8_count_err = 0;
						uint16_t u16_pH = u32_pHEC_data & 0xFFFF;
						v_display_pH_2_LCD(str_irr_row.u16_pH, u16_pH);
						if(u16_pH < str_irr_row.u16_pH)
						{
							u8_count_pHEC++;
							if(u8_count_pHEC > 10)
							{
								//turn off pump and valve
								u64_temp_port_value &= ~(NEUTRAL_PUMP_PORT);
								u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
								u64_temp_port_value &= ~str_irr_row.u64_valve;
								//turn off sensor
								v_pH_EC_set_state(0, 0);
								v_display_waiting_LCD();
								e_neutral_state = NEUTRAL_IDLE;
								b_return_value = true;
							}
						}
						else
						{
							e_neutral_state = NEUTRAL_ADD_ACID;
							v_mqtt_noti_pub(DATAID_PH1_WARNING, TYPE_NOTI_HIGH);
						}
					}
					else
					{
						u8_count_err++;
						if(u8_count_err > 5)
						{
							u8_count_err =  0;
							v_mqtt_noti_pub(DATAID_PH1_WARNING, TYPE_NOTI_SENSOR_LOST);
							//turn off pump and valve
							u64_temp_port_value &= ~(NEUTRAL_PUMP_PORT);
							u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
							u64_temp_port_value &= ~str_irr_row.u64_valve;
							//turn off sensor
							v_pH_EC_set_state(0, 0);
							v_display_waiting_LCD();
							e_neutral_state = NEUTRAL_IDLE;
							b_return_value = true;
						}
					}
				}
			}break;
			case NEUTRAL_ADD_ACID:	
			{
				uint32_t u32_pHEC_data = 0;
				//check valve status
				if(u32_cur_time >= u32_time_stop_valve)
				{
					u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
				}
				//read pH
				if(xQueueReceive(x_pHEC_Queue, &u32_pHEC_data, 10))
				{
					if(u32_pHEC_data != 0)
					{
						uint16_t u16_pH = u32_pHEC_data & 0xFFFF;
						v_display_pH_2_LCD(str_irr_row.u16_pH, u16_pH);
						if(u16_pH > (str_irr_row.u16_pH - NEUTRAL_PH_OFSET))
						{
							//turn on valve
							u64_temp_port_value |= NEUTRAL_VALVE_PORT;
							u32_time_stop_valve = u32_cur_time + u16_neutral_time_inject_acid;
						}
						else
						{
							//turn off valve
							u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
							//run additional aeration
							u32_time_stop_aeration = u32_cur_time + u16_neutral_time_aeration;
							e_neutral_state = NEUTRAL_AERATION;
						}
					}
					else
					{
						//turn off pump and valve
						u64_temp_port_value &= ~(NEUTRAL_PUMP_PORT);
						u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
						u64_temp_port_value &= ~str_irr_row.u64_valve;
						//turn off sensor
						v_pH_EC_set_state(0, 0);
						v_display_waiting_LCD();
						e_neutral_state = NEUTRAL_IDLE;
						b_return_value = true;
					}
				}
			}break;
			case NEUTRAL_AERATION:		//additional run to make aeration
			{
				uint32_t u32_pHEC_data = 0;
				if(xQueueReceive(x_pHEC_Queue, &u32_pHEC_data, 10))
				{
					if(u32_pHEC_data != 0)
					{
						uint16_t u16_pH = u32_pHEC_data & 0xFFFF;
						v_display_pH_2_LCD(str_irr_row.u16_pH, u16_pH);
						if(u16_pH > str_irr_row.u16_pH)
						{
							u8_ph_over_cnt++;
							if (u8_ph_over_cnt > 5)
							{
								u8_ph_over_cnt = 0;
								//return to add acid again
								e_neutral_state = NEUTRAL_ADD_ACID;
							}
						}
					}
				}
				if(u32_cur_time >= u32_time_stop_aeration)
				{
					//turn off pump and valve
					u64_temp_port_value &= ~(NEUTRAL_PUMP_PORT);
					u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
					u64_temp_port_value &= ~str_irr_row.u64_valve;
					//turn off sensor
					v_pH_EC_set_state(0, 0);
					v_display_waiting_LCD();
					e_neutral_state = NEUTRAL_IDLE;
					b_return_value = true;
					v_mqtt_fertigation_finish_pub(str_irr_row.u16_location_ID, u32_start_time);
					v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_PREPARE);
				}
			}break;
			case NEUTRAL_REQUEST_STOP:
			{
				//turn off pump and valve
				u64_temp_port_value &= ~(NEUTRAL_PUMP_PORT);
				u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
				u64_temp_port_value &= ~str_irr_row.u64_valve;
				//turn off sensor
				v_pH_EC_set_state(0, 0);
				e_neutral_state = NEUTRAL_STOP;
				v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_STOPED);
				v_display_no_schedule();
			}break;
			case NEUTRAL_STOP:
			{
				//do nothing
			}break;
		}
	}
	else
	{
		if(e_neutral_state != NEUTRAL_IDLE)
		{
			//turn off pump and valve
			u64_temp_port_value &= ~(NEUTRAL_PUMP_PORT);
			u64_temp_port_value &= ~(NEUTRAL_VALVE_PORT);
			u64_temp_port_value &= ~str_irr_row.u64_valve;
			//turn off sensor
			v_pH_EC_set_state(0, 0);
			v_display_waiting_LCD();
			e_neutral_state = NEUTRAL_IDLE;
			
		}
		b_return_value = true;
		v_mqtt_noti_pub(DATAID_TANK1_EMPTY, TYPE_NOTI_LOW);
		v_mqtt_data_pub(DATAID_MACHINE_STATE, PUBLISH_STATE_STOPED);
	}
	*u64_port_control_value = u64_temp_port_value;
	return b_return_value;
}
void	v_clear_neu_schedule(void)
{
	v_display_no_schedule();
	e_neutralize_schedule_state = SCHE_STATE_LOAD;
}
