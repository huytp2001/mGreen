/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file mqtt_control_callbacks.c
 * @author Danh Pham
 * @date 01 Dec 2020
 * @version: 1.0.0
 * @brief This file contains callback functions to process frame from 
 * server.
 */ 
 
 #include <stdint.h>
 #include <stdbool.h>
 #include "mqtt_callbacks.h"
 #include "sensor.h"
 #include "setting.h"
 #include "mqtt_publish.h"
 #include "ports_control.h"
 #include "neutralize.h"
 #include "vpd_control.h"
 #include "manual.h"
 #include "relay_control.h"
 
static bool b_setting_parser( mqtt_data_frame_t stru_mqtt_data_frame);

/*!
* @fn mqtt_control_callback(mqtt_data_frame_t stru_mqtt_data_frame)
* @brief Handle frame from server. Must be registered before mqtt tast start
* @param[in] stru_mqtt_data_frame Data struct
* @return None
*/
void mqtt_control_callback(mqtt_data_frame_t stru_mqtt_data_frame)
{
	switch(stru_mqtt_data_frame.u8_frame_type)
	{
		case 0x30: 		//Manual relay control
		{
			v_manual_port_control(stru_mqtt_data_frame.stru_data_field[0].u16_id,
														stru_mqtt_data_frame.stru_data_field[0].u32_value);
		}
		break;
		case 0x31: 	//request sensors
		{
			switch(stru_mqtt_data_frame.stru_data_field[0].u16_id)
			{
				case 0x0001:		//request pressure switch status
				{
				}
				break;
				case 0x009B:		//control phec sensor
				{
					v_phec_control(stru_mqtt_data_frame.stru_data_field[0].u32_value);
				}
				break;
				default:
					break;
			}
		}
		break;
		case 0x32: 		//Manual timer style control
		{
			if(0x2000 == stru_mqtt_data_frame.stru_data_field[0].u16_id
				&& 0x2001 == stru_mqtt_data_frame.stru_data_field[1].u16_id
				&& 0x2002 == stru_mqtt_data_frame.stru_data_field[2].u16_id
				&& 0x2003 == stru_mqtt_data_frame.stru_data_field[3].u16_id)
			{
				if(0 == stru_mqtt_data_frame.stru_data_field[1].u32_value)	// turn off
				{
					stru_mqtt_data_frame.stru_data_field[0].u32_value = 0; 	//clear time on
				}
//				v_manual_port_timer_control(stru_mqtt_data_frame.u32_node_id & 0xFF,
//																		stru_mqtt_data_frame.stru_data_field[0].u32_value,
//													(uint64_t)stru_mqtt_data_frame.stru_data_field[2].u32_value 
//													| (uint64_t)stru_mqtt_data_frame.stru_data_field[3].u32_value << 32);
				b_manual_add_event(stru_mqtt_data_frame.u32_node_id & 0xFF,
													 stru_mqtt_data_frame.stru_data_field[0].u32_value,
													(uint64_t)stru_mqtt_data_frame.stru_data_field[2].u32_value,
												  (uint64_t)stru_mqtt_data_frame.stru_data_field[3].u32_value);
			}													
		}
		break;
		case 0x35:		//machine state control
		{
			switch(stru_mqtt_data_frame.stru_data_field[0].u16_id)
			{
				case 0x0001:		//start
				{
					v_irrigation_event_run();
				}
				break;
				case 0x0002:			//continue
				{
					v_irrigation_event_continue();	
				}
				break;
				case 0x0003:			//pause
				{
					v_irrigation_event_pause();
				}
				break;
				default:
					break;
			}
		}
		break;
		case 0x50:		//setting
		{
			if(b_setting_parser(stru_mqtt_data_frame))
			{
				v_mqtt_feeback_setting(&stru_mqtt_data_frame, true);
				v_neces_valid_set(true);
			}
			else
			{
				v_mqtt_feeback_setting(&stru_mqtt_data_frame, false);
			}
		}
		break;
		case 0x90:		//download schedule file
		{
			/* TODO: schedule csv file download request  */
			stru_ftp_request_t stru_ftp_request = {.e_file_type = FILE_TYPE_SCHEDULE, .u32_crc = 0, .c_file_name = {0}};
			uint8_t u8_name_len = stru_mqtt_data_frame.stru_data_field[0].u8_value_len;
			if(3 == stru_mqtt_data_frame.stru_data_field[0].u8_type_id) //string type
			{
				memcpy(stru_ftp_request.c_file_name, stru_mqtt_data_frame.stru_data_field[0].au8_payload,
								u8_name_len * sizeof(uint8_t));	//get file name
				v_mqtt_feedback_ftp(FILE_TYPE_SCHEDULE, FEEDBACK_REQ_DOWNLOAD, stru_ftp_request.c_file_name);
				strcpy(stru_ftp_request.c_file_name + u8_name_len, ".csv"); //append file type extension
				stru_ftp_request.u32_crc = stru_mqtt_data_frame.stru_data_field[1].u32_value;
				v_file_download_request(stru_ftp_request);
			}
			else // frame gui sau cau truc
			{
				v_mqtt_feedback_ftp(FILE_TYPE_SCHEDULE, FEEDBACK_DOWNLOAD_FAIL, "");
			}
		}
		break;
		case 0xA0: 		//OTA request
		{
			/* firmware hex file download request  */
			stru_ftp_request_t stru_ftp_request = {.e_file_type = FILE_TYPE_FIRMWARE, .u32_crc = 0, .c_file_name = {0}};
			uint8_t u8_name_len = stru_mqtt_data_frame.stru_data_field[0].u8_value_len;
			if(1 == stru_mqtt_data_frame.stru_data_field[0].u8_type_id) //string type
			{
				memcpy(stru_ftp_request.c_file_name, stru_mqtt_data_frame.stru_data_field[0].au8_payload,
								u8_name_len * sizeof(uint8_t));	//get file name
				v_mqtt_feedback_ftp(FILE_TYPE_FIRMWARE, FEEDBACK_REQ_DOWNLOAD, stru_ftp_request.c_file_name);
				strcpy(stru_ftp_request.c_file_name + u8_name_len, ".hex"); //append file type extension
				stru_ftp_request.u32_crc = stru_mqtt_data_frame.stru_data_field[1].u32_value;
				v_file_download_request(stru_ftp_request);
			}
			else
			{
				v_mqtt_feedback_ftp(FILE_TYPE_FIRMWARE, FEEDBACK_DOWNLOAD_FAIL, "");
			}
			
		}
		break;
		default:
			break;
	}
}
 
static bool b_setting_parser( mqtt_data_frame_t stru_mqtt_data_frame)
{
	bool b_result = true;
	uint8_t u8_fertigation_setting_counter = 0;
	uint8_t u8_neces_setting_counter = 0;
	for(uint8_t i = 0; i < stru_mqtt_data_frame.u8_num_data_field; i++)
	{
		switch(stru_mqtt_data_frame.stru_data_field[i].u16_id)
		{
			case 0x0096:		//time for enough pressure
			{
				v_set_time_wait_pressure(stru_mqtt_data_frame.stru_data_field[i].u32_value & 0xFF);
				u8_fertigation_setting_counter++;
			}
			break;
			case 0x019B:		//topic
			{
				v_topic_set(stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x019C:	//use RF
			{
				v_use_rf_set(stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x019D:	//RF module type
			{
				v_type_rf_set((e_rf_type_t)stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x019E:	//Enable flowmeter main line
			{
				v_use_main_flow_set(stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x019F:	//num pulses
			{
				v_main_flow_pulses_2_cal_set(stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x01A0:	//mL/pulse
			{
				double d_factor = (double)stru_mqtt_data_frame.stru_data_field[i].u32_value/1000000;
				v_main_flow_factor_set(d_factor);
				b_result = true;
			}
			break;
			case 0x01A1:	//type of fertilizer flowmeter
			{
				v_ferti_flow_type_set((e_fertilizer_flowmeter_type_t)stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x01B4:	//time enough 1 pulse from flowmetter
			{
				if (stru_mqtt_data_frame.stru_data_field[i].u32_value > 255)
				{
					stru_mqtt_data_frame.stru_data_field[i].u32_value = 255;
				}
				v_set_time_flowmeter(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			}
			break;
			case 0x01B7:
			{
				v_set_normal_flowmeter_type((e_normal_flowmeter_type)stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x01c4:
			{
				v_tank_division_set(0, stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x01c5:
			{
				v_tank_division_set(1, stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x01C9:	//operating mode
			{
				v_operating_mode((e_operating_mode)stru_mqtt_data_frame.stru_data_field[i].u32_value);
				b_result = true;
			}
			break;
			case 0x01ca: //flow rate default
			{
				if (stru_mqtt_data_frame.stru_data_field[i].u32_value <= FLOW_RATE_MAX)
				{
					v_flow_rate_set(stru_mqtt_data_frame.stru_data_field[i].u32_value);
				}
				b_result = true;
			}
			break;
			//todo: setting config
			#ifdef NEUTRALIZER_MODE
			case 0x009B:
				v_set_neutral_sample_duration(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x009C:
				v_set_neutral_time_inject_acid(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x009D:
				v_set_neutral_time_aeration(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x009E:
				v_set_netral_start_point(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x009F:
				v_set_neutral_stop_point(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x00A0:
				v_set_neutral_start_time(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x00A1:
				v_set_neutral_stop_time(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
		#endif
		#ifdef VPD_CONTROL_MODE
			case 0x01A3:	//vpd monitored node id
				v_vpd_set_node_id(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x01A4:	//vpd start time
				v_vpd_set_start_time(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x01A5:	//vpd stop time
				v_vpd_set_stop_time(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x01A6:	// vpd min temperature (point to fan turn off)
				v_vpd_set_min_temperature(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
			case 0x01A7:	//vpd max temperature (point the fan turn on)
				v_vpd_set_max_temperature(stru_mqtt_data_frame.stru_data_field[i].u32_value);
			break;
		#endif
			default:
				break;
		}
	}
//	if(FRAME_MAX_DATA_FIELD == u8_fertigation_setting_counter)
//	{
//		b_result = true;
//	}
//	if(FRAME_MAX_DATA_FIELD == u8_neces_setting_counter)
//	{
//		b_result = true;
//		v_neces_valid_set(true);
//	}
	
	
	return b_result;
}

