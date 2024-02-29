/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file mqtt_publish.c
 * @author Danh Pham
 * @date 25 Nov 2020
 * @version: 1.0.0
 * @brief Contains functions used to send data to server.
 */ 
 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "mqtt_publish.h"
#include "frame_parser.h"
#include "mqtt_task.h"
#include "gsm_hal.h"
#include "rtc.h"
/*
* Private variables
*/
static mqtt_data_frame_t stru_mqtt_data_frame;
static mqtt_t stru_mqtt;
/*
*	Private functions prototype
*/
/*
* Public functions
*/
/*!
* @fn v_mqtt_pub_hello(uint32_t u32_hw_version, uint32_t u32_fw_version,
*											mqtt_connect_code_t e_mqtt_connect_code)
* @brief Publish Hello frame to server 
* @param[in] u32_hw_version
* @param[in] u32_fw_version
* @param[in] u32_system_status
* @return None
*/
void v_mqtt_pub_hello(uint32_t u32_hw_version, uint32_t u32_fw_version,
											mqtt_connect_code_t e_mqtt_connect_code)
{		
	v_frame_build_header(FRAME_TYPE_INIT, NODETYPEID_RATA_MACHINE, 0, 3,
											&stru_mqtt_data_frame);
	
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_HARDWARE_VER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	b_u32_to_array(u32_hw_version, stru_mqtt_data_frame.stru_data_field[0].au8_payload,NULL);
	
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = DATAID_FIRMWARE_VER;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = 4;
	b_u32_to_array(u32_fw_version, stru_mqtt_data_frame.stru_data_field[1].au8_payload,NULL);
	
	stru_mqtt_data_frame.stru_data_field[2].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[2].u16_id = DATAID_SYSTEN_STATUS;
	stru_mqtt_data_frame.stru_data_field[2].u8_value_len = 4;
	b_u32_to_array(e_mqtt_connect_code, stru_mqtt_data_frame.stru_data_field[2].au8_payload,NULL);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_pub_keep_alive(uint32_t u32_value)
* @brief Publish Keep alive frame to server 
* @param[in] u32_value
* @return None
*/
void v_mqtt_pub_keep_alive(uint32_t u32_value)
{		
	v_frame_build_header(FRAME_TYPE_KA, NODETYPEID_RATA_MACHINE, 0, 1,
											&stru_mqtt_data_frame);
	
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = 0;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	b_u32_to_array(u32_value, stru_mqtt_data_frame.stru_data_field[0].au8_payload,NULL);

	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_pub_keep_alive(uint32_t u32_value)
* @brief Publish Fertigation start frame to server 
* @param[in] u16_location_id
* @return None
*/
void v_mqtt_fertigation_start(uint16_t u16_location_id)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id, 1,
											&stru_mqtt_data_frame);
	/* Start time */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_TIME_BEGIN;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	b_u32_to_array(u32_rtc_unix_time_get(), stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_fertigation_info_pub(uint16_t u16_location_id, uint32_t *pu32_rate,
*													double *pd_fertilizer_volume, double d_water_volume)
* @brief Publish fertigation info. 
* @param[in] u16_location_id
* @param[in] *pu32_rate
* @param[in] *pd_fertilizer_volume
* @param[in] d_water_volume
* @return None
*/
void v_mqtt_fertigation_info_pub(uint16_t u16_location_id, uint32_t *pu32_rate,
													double *pd_fertilizer_volume, double d_water_volume)
{
	v_mqtt_fertilizer_rate_pub(u16_location_id, pu32_rate);
	v_mqtt_fertilizer_volume_pub(u16_location_id, pd_fertilizer_volume);
	v_mqtt_water_volume_pub(u16_location_id, d_water_volume);
}

/*!
* @fn v_mqtt_ph_statistic_pub(uint16_t u16_location_id, 
*											uint16_t u16_min_ph, uint16_t u16_mean_ph, uint16_t u16_max_ph)
* @brief Send statistics of pH values . 
* Must be called at the end of a fertigation event, before the finish frame.
* @param[in] u16_location_id Id of irrigated location
* @param[in] u16_min_ph Min value of pH
* @param[in] u16_mean_ph Mean (average) value of pH
* @param[in] u16_max_ph Max value of pH
* @return None
*/
void v_mqtt_ph_statistic_pub(uint16_t u16_location_id, 
											uint16_t u16_min_ph, uint16_t u16_mean_ph, uint16_t u16_max_ph)
{	
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id, 3,
											&stru_mqtt_data_frame);
	/* Min pH value */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_PH_MIN;
	b_u32_to_array(u16_min_ph, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);

	/* Mean pH value */
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = DATAID_PH_AVG;
	b_u32_to_array(u16_mean_ph, stru_mqtt_data_frame.stru_data_field[1].au8_payload, 0);
	
	/* Max pH value */	
	stru_mqtt_data_frame.stru_data_field[2].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[2].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[2].u16_id = DATAID_PH_MAX;
	b_u32_to_array(u16_max_ph, stru_mqtt_data_frame.stru_data_field[2].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_ec_statistic_pub(uint16_t u16_location_id,
*										uint16_t u16_min_ec, uint16_t u16_mean_ec, uint16_t u16_max_ec)
* @brief Send statistics of EC values . 
* Must be called at the end of a fertigation event, before the finish frame.
* @param[in] u16_location_id Id of irrigated location
* @param[in] u16_min_ec Min value of EC
* @param[in] u16_mean_ec Mean (average) value of EC
* @param[in] u16_max_ec Max value of EC
* @return None
*/
void v_mqtt_ec_statistic_pub(uint16_t u16_location_id,
										uint16_t u16_min_ec, uint16_t u16_mean_ec, uint16_t u16_max_ec)
{		
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id, 3,
											&stru_mqtt_data_frame);
	/* Min EC value */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_EC_MIN;
	b_u32_to_array(u16_min_ec, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);

	/* Mean EC value */
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = DATAID_EC_AVG;
	b_u32_to_array(u16_mean_ec, stru_mqtt_data_frame.stru_data_field[1].au8_payload, 0);
	
	/* Max EC value */	
	stru_mqtt_data_frame.stru_data_field[2].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[2].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[2].u16_id = DATAID_EC_MAX;
	b_u32_to_array(u16_max_ec, stru_mqtt_data_frame.stru_data_field[2].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

void v_mqtt_ec_calculated_pub(uint16_t u16_location_id,
										uint16_t u16_calculated_ec)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id, 1,
											&stru_mqtt_data_frame);
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_CALCULATED_EC;
	b_u32_to_array(u16_calculated_ec, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_fertigation_finish_pub(uint16_t u16_location_id, uint32_t u32_time_start)
* @brief Send start time of a fertigation event to identify the finishing. 
* Must be called at the of a fertigation event, after all statistic and info frames
* @param[in] u16_location_id Id of irrigated location
* @param[in] u32_time_start Start time of fertigation event
* @return None
*/
void v_mqtt_fertigation_finish_pub(uint16_t u16_location_id, uint32_t u32_time_start)
{	
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id, 1,
											&stru_mqtt_data_frame);
	/* Finish time */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_TIME_FINISH;
	b_u32_to_array(u32_time_start, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_phec_pub(uint16_t u16_location_id, uint16_t u16_ph, uint16_t u16_ec)
* @brief Send value of pHEC to server
* @param[in] u16_location_id ID of irrigated location
* @param[in] u16_ph pH value
* @param[in] u16_ec EC value
* @return None
*/
void v_mqtt_phec_pub(uint16_t u16_location_id, uint16_t u16_ph, uint16_t u16_ec)
{	
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id, 2,
											&stru_mqtt_data_frame);
	/* pH */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_PH1;
	b_u32_to_array(u16_ph, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	/* EC */
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = DATAID_EC1;
	b_u32_to_array(u16_ec, stru_mqtt_data_frame.stru_data_field[1].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}	

/*!
* @fn v_mqtt_fw_version_pub(uint32_t u32_fw_version)
* @brief Send firmware version to server
* @param[in] u32_fw_version Current version of firmware
* @return None
*/
void v_mqtt_fw_version_pub(uint32_t u32_fw_version)
{
	v_frame_build_header(FRAME_TYPE_QUERY, NODETYPEID_RATA_MACHINE, 0, 1,
											&stru_mqtt_data_frame);
	/* Firmware version */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_FIRMWARE_VER;
	b_u32_to_array(u32_fw_version, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_schedule_version_pub(char *c_csv_ver)
* @brief Send schedule csv version to server
* @param[in] *c_csv_ver Current version of schedule csv file
* @return None
*/
void v_mqtt_schedule_version_pub(char *c_csv_ver)
{	
	v_frame_build_header(FRAME_TYPE_SCHEDULE_FILE, NODETYPEID_RATA_MACHINE, 0, 1,
											&stru_mqtt_data_frame);
	/* Firmware version */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_STRING;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = strlen(c_csv_ver);
	if (stru_mqtt_data_frame.stru_data_field[0].u8_value_len > FRAME_MAX_DATA_VALUE)
	{
		stru_mqtt_data_frame.stru_data_field[0].u8_value_len = FRAME_MAX_DATA_VALUE;
	}
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_CSV_VERSION;
	memcpy(stru_mqtt_data_frame.stru_data_field[0].au8_payload, c_csv_ver, 
											stru_mqtt_data_frame.stru_data_field[0].u8_value_len);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
	
}

/*!
* @fn v_mqtt_noti_pub(uint16_t u16_noti_id, uint8_t u8_type_noti)
* @brief Publish notification
* @param[in] u16_noti_id Data id of notification
* @param[in] e_type_noti Type of notification
* @return None
*/
void v_mqtt_noti_pub(uint16_t u16_noti_id, E_NOTI_TYPE e_type_noti)
{	
	v_frame_build_header(FRAME_TYPE_NOTI, NODETYPEID_RATA_MACHINE, 0, 1,
											&stru_mqtt_data_frame);
	/* Firmware version */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = u16_noti_id;
	b_u32_to_array(e_type_noti, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_fertigation_state(uint8_t u8_state)
* @brief Publish state of fertigaiton process
* @param[in] u8_state State of fertigaiton process
* @return None
*/
void v_mqtt_fertigation_state(uint8_t u8_state)
{
	v_mqtt_data_pub(DATAID_FERTIGATION_STATE, u8_state);
}

/*!
* @fn v_mqtt_pub_feedback(mqtt_data_frame_t* pstru_mqtt_data_frame, uint32_t u32_value)
* @brief Feedback for received frame
* @param[in] pstru_mqtt_data_frame
* @param[in] u32_value
* @return None
*/
void v_mqtt_pub_feedback(mqtt_data_frame_t* pstru_mqtt_data_frame, uint32_t u32_value)
{
	pstru_mqtt_data_frame->u8_frame_type = 0x2F;
	pstru_mqtt_data_frame->u8_node_type_id = NODETYPEID_RATA_MACHINE;
	pstru_mqtt_data_frame->u8_rssi = u8_gsm_hal_get_sim_rssi();;
	pstru_mqtt_data_frame->u16_seq_number = u16_seq_number_get();
	pstru_mqtt_data_frame->u32_node_id = 0x00;
	pstru_mqtt_data_frame->u32_unix_time = u32_rtc_unix_time_get();

	/* feedback value */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = 0xFB;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	b_u32_to_array(u32_value, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_feeback_setting(mqtt_data_frame_t *stru_mqtt_frame, bool b_successed)
* @brief Feedback for setting process
* @param[in] stru_mqtt_frame Setting frame
* @param[in] b_successed Is setting successed or not
* @return None
*/
void v_mqtt_feeback_setting(mqtt_data_frame_t* pstru_mqtt_data_frame, bool b_successed)
{
	pstru_mqtt_data_frame->u8_frame_type = FRAME_TYPE_DATA_CON;
	pstru_mqtt_data_frame->u8_node_type_id = NODETYPEID_RATA_MACHINE;
	pstru_mqtt_data_frame->u8_rssi = u8_gsm_hal_get_sim_rssi();;
	pstru_mqtt_data_frame->u16_seq_number = u16_seq_number_get();
	pstru_mqtt_data_frame->u32_node_id = 0x00;
	pstru_mqtt_data_frame->u32_unix_time = u32_rtc_unix_time_get();
	if(false == b_successed)
	{
		for(uint8_t i = 0; i < pstru_mqtt_data_frame->u8_num_data_field; i++)
		{
			b_u32_to_array(0, pstru_mqtt_data_frame->stru_data_field[i].au8_payload, 0);			
		}
	}
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, pstru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_feedback_ftp(E_FTP_FRAME_TYPE e_file_type,
*												uint8_t u8_state, const char *c_filename)
* @brief Feedback for ftp download process
* @param[in] e_file_type Type of file
* @param[in] e_feedback_status Status of downloading process
* @param[in] *c_filename File name
* @return None
*/
void v_mqtt_feedback_ftp(E_DOWNLOAD_FILE_TYPE e_file_type,
							E_FTP_FEEDBACK_STATUS e_feedback_status, const char *c_filename)
{
	uint8_t u8_frame_type = 0;
	switch(e_file_type)
	{
		case FILE_TYPE_SCHEDULE:
		{
			u8_frame_type = 0x90;
		}
		break;
		case FILE_TYPE_THRESHOLD:
		{
			u8_frame_type = 0x92;
		}
		break;
		case FILE_TYPE_FIRMWARE:
		{
			u8_frame_type = 0xA0;
		}
		break;
		default:
			break;
	}
	v_frame_build_header(u8_frame_type, NODETYPEID_RATA_MACHINE, 0, 2,
											&stru_mqtt_data_frame);

	/* File name */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_DOWNLOAD_FILE_STATE;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	b_u32_to_array(e_feedback_status, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	/* File name */
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_STRING;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = DATAID_CSV_VERSION;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = strlen(c_filename);
	
	if(strlen(c_filename) < FRAME_MAX_DATA_VALUE)
	{
		stru_mqtt_data_frame.stru_data_field[1].u8_value_len = strlen(c_filename);
	}
	else
	{
		stru_mqtt_data_frame.stru_data_field[1].u8_value_len = FRAME_MAX_DATA_VALUE;
	}
	memcpy(stru_mqtt_data_frame.stru_data_field[1].au8_payload, c_filename, 
					stru_mqtt_data_frame.stru_data_field[1].u8_value_len);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_data_pub(uint16_t u16_data_id, uint32_t u32_value)
* @brief Publish data with special data ID
* @param[in] u16_data_id Data ID
* @param[in] u32_value Data value
* @return None
*/
void v_mqtt_data_pub(uint16_t u16_data_id, uint32_t u32_value)
{	
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, 0, 1,
											&stru_mqtt_data_frame);
	/* Firmware version */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = u16_data_id;
	b_u32_to_array(u32_value, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}
/*!
* @fn v_mqtt_fertilizer_rate_pub(uint16_t u16_location_id, uint32_t *pu32_rate)
* @brief Send fertilizer absorb rates to server
* @param[in] u16_location_id ID of irrigated location
* @param[in] *pu32_rate Array contains rates
* @return None
*/
void v_mqtt_fertilizer_rate_pub(uint16_t u16_location_id, uint32_t *pu32_rate)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id,
											5, &stru_mqtt_data_frame);	
	
	for(uint8_t i = 0; i < 5; i++)
	{
		stru_mqtt_data_frame.stru_data_field[i].u8_type_id = DATATYPEID_CONTROLLER;
		stru_mqtt_data_frame.stru_data_field[i].u8_value_len = 4;
		stru_mqtt_data_frame.stru_data_field[i].u16_id = DATAID_CH1_RATE + i;
		b_u32_to_array(pu32_rate[i], stru_mqtt_data_frame.stru_data_field[i].au8_payload, 0);
	}

	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_fertilizer_volume_pub(uint16_t u16_location_id, double *pd_volume)
* @brief Send absorbed fertilizer volumne to server
* @param[in] u16_location_id ID of irrigated location
* @param[in] *pd_volume Array contains volume
* @return None
*/
void v_mqtt_fertilizer_volume_pub(uint16_t u16_location_id, double *pd_volume)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id,
											5, &stru_mqtt_data_frame);	
	double d_volume[5] = {0,0,0,0,0};
	memcpy(d_volume, pd_volume, 4*sizeof(double));
	for(uint8_t i = 0; i < 5; i++)
	{
		stru_mqtt_data_frame.stru_data_field[i].u8_type_id = DATATYPEID_CONTROLLER;
		stru_mqtt_data_frame.stru_data_field[i].u8_value_len = 4;
		stru_mqtt_data_frame.stru_data_field[i].u16_id = DATAID_CH1_VOLUME + i;
		b_u32_to_array((uint32_t)(d_volume[i] * 100), 
										stru_mqtt_data_frame.stru_data_field[i].au8_payload, 0);
	}
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

/*!
* @fn v_mqtt_water_volume_pub(uint16_t u16_location_id, double d_volume)
* @brief Send irrigated water volumne to server
* @param[in] u16_location_id ID of irrigated location
* @param[in] d_volume Volume
* @return None
*/
void v_mqtt_water_volume_pub(uint16_t u16_location_id, double d_volume)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, u16_location_id,
											1, &stru_mqtt_data_frame);	
	
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_WATER_VOLUME;
		b_u32_to_array((uint32_t)(d_volume), 
										stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

void v_mqtt_threshold_event_pub(STRU_THD stru_thd_data)
{
	/* TODO: */
	v_frame_build_header(FRAME_TYPE_NOTI, NODETYPEID_RATA_MACHINE, 0, stru_thd_data.u8_num_thd_data,
											&stru_mqtt_data_frame);
	
	uint8_t u8_num_data = 0;
	for(uint8_t u8_i = 0; u8_i < stru_thd_data.u8_num_thd_data; u8_i++)
	{
		if((stru_thd_data.astru_thd_data[u8_i].b_status == true) &&
			 (stru_thd_data.astru_thd_data[u8_i].b_has_data == true))
		{
			if(stru_thd_data.astru_thd_data[u8_i].e_thd_cdt_type == THD_CDT_L)
			{
				stru_mqtt_data_frame.stru_data_field[u8_num_data].u8_type_id = DATATYPE_ID_NOTI_LOW;
			}
			else if(stru_thd_data.astru_thd_data[u8_i].e_thd_cdt_type == THD_CDT_H)
			{
				stru_mqtt_data_frame.stru_data_field[u8_num_data].u8_type_id = DATATYPE_ID_NOTI_HIGH;
			}
			stru_mqtt_data_frame.stru_data_field[u8_num_data].u16_id = 
									stru_thd_data.astru_thd_data[u8_i].u16_sensor_ID;
			stru_mqtt_data_frame.stru_data_field[u8_num_data].u8_value_len = 4;	
			stru_mqtt_data_frame.stru_data_field[u8_num_data].u32_value = 
									stru_thd_data.astru_thd_data[u8_i].u32_curr_value;
			b_u32_to_array(stru_thd_data.astru_thd_data[u8_i].u32_curr_value,
					stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
			u8_num_data += 1;
		}
	}	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
	
}

void v_vpd_data_pub(uint32_t u32_node_id, uint16_t u16_data_id, uint32_t u32_value)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODE_TYPE_ID_VPD, u32_node_id, 1,
											&stru_mqtt_data_frame);
	/* Firmware version */
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = u16_data_id;
	b_u32_to_array(u32_value, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);
	
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

void v_mqtt_thd_info_pub(STRU_THD stru_thd_data)
{
	//Node ID = Location ID	
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, stru_thd_data.u16_location_id, 1,
											&stru_mqtt_data_frame);	
	
	//Threshold event ID: 
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = DATAID_THD_EVENT_ID;		
	b_u32_to_array(stru_thd_data.u32_thd_event_id, stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);	
	//Threshold Action type:
	// 1: single ON/OFF
	// 2: timer ON/OFF
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = DATAID_THD_ACT_TYPE;
	b_u32_to_array(stru_thd_data.e_thd_act_type, stru_mqtt_data_frame.stru_data_field[1].au8_payload, 0);		
	// Duration unit
	// 0: time in second
	// 1: water volume in litter
	stru_mqtt_data_frame.stru_data_field[2].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[2].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[2].u16_id = DATAID_THD_DURATION_UNIT;
	b_u32_to_array(stru_thd_data.e_unit, stru_mqtt_data_frame.stru_data_field[2].au8_payload, 0);
	//Duration
	stru_mqtt_data_frame.stru_data_field[3].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[3].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[3].u16_id = DATAID_THD_DURATION;
	b_u32_to_array(stru_thd_data.u32_time_run, stru_mqtt_data_frame.stru_data_field[3].au8_payload, 0);
	//Port control: 32 LSB bit 
	stru_mqtt_data_frame.stru_data_field[4].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[4].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[4].u16_id = DATAID_PORT_32_BIT_L;
	b_u32_to_array((uint32_t)stru_thd_data.u64_port, stru_mqtt_data_frame.stru_data_field[4].au8_payload, 0);
	//Port control: 32 MSB bit
	stru_mqtt_data_frame.stru_data_field[5].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[5].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[5].u16_id = DATAID_PORT_32_BIT_H;
	b_u32_to_array((uint32_t)(stru_thd_data.u64_port >> 32), stru_mqtt_data_frame.stru_data_field[5].au8_payload, 0);

	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}

void v_mqtt_ports_state(uint64_t u64_ports_state)
{
#ifdef PROTOCOL_VER_2
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, 0,
											1, &stru_mqtt_data_frame);	
	
	stru_mqtt_data_frame.stru_data_field[0].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[0].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[0].u16_id = 0x00F3;
		b_u32_to_array((uint32_t)(u64_ports_state & 0x00FFFFFFFF), 
										stru_mqtt_data_frame.stru_data_field[0].au8_payload, 0);	
	
	stru_mqtt_data_frame.stru_data_field[1].u8_type_id = DATATYPEID_CONTROLLER;
	stru_mqtt_data_frame.stru_data_field[1].u8_value_len = 4;
	stru_mqtt_data_frame.stru_data_field[1].u16_id = 0x00F4;
		b_u32_to_array((uint32_t)((u64_ports_state >> 32) & 0x00FFFFFFFF), 
										stru_mqtt_data_frame.stru_data_field[1].au8_payload, 0);
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
#endif
}

void v_mqtt_debug_rate_pub(uint8_t u8_index, uint32_t *u32_volume)
{
	v_frame_build_header(FRAME_TYPE_DATA_CON, NODETYPEID_RATA_MACHINE, 0,
											5, &stru_mqtt_data_frame);	
	
	for(uint8_t i = 0; i < 5; i++)
	{
		stru_mqtt_data_frame.stru_data_field[i].u8_type_id = DATATYPEID_CONTROLLER;
		stru_mqtt_data_frame.stru_data_field[i].u8_value_len = 4;
		stru_mqtt_data_frame.stru_data_field[i].u16_id = u8_index + i;
		b_u32_to_array((uint32_t)(u32_volume[i]), 
										stru_mqtt_data_frame.stru_data_field[i].au8_payload, 0);
	}
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_mqtt_data_frame, &stru_mqtt);
	/* Send frame */
	b_write_mqtt_pub_to_server(&stru_mqtt);
}
