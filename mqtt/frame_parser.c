/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file frame_parser.c
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
#include "gsm_cmd.h"
#include "gsm_hal.h"
#include "rtc.h"
#include "ustdlib.h"
/*
* Private variables
*/

static uint16_t u16_seq_number = 0;
/*
*	Private functions prototype
*/
/*
* Public functions
*/

/*!
* @fn u16_seq_number_get(void)
* @brief Get auto increment number 
* @param None
* @return uint16_t auto increment number.
*/
uint16_t u16_seq_number_get(void)
{
	u16_seq_number++;
	return u16_seq_number;
}

/*!
* @fn b_u16_to_array(uint16_t u16_in_value, uint8_t* pu8_out_array, 
*																									uint8_t* pu8_cnt)
* @brief Parse a 16bit number to an array
* @param[in] u16_in_value Source value.
* @param[out] pu8_out_array Dest array.
* @param[inout] pu8_cnt counter
* @return true if we have enough space to storage 16bit value
*/
bool b_u16_to_array(uint16_t u16_in_value, uint8_t* pu8_out_array, 
																								uint8_t* pu8_cnt)
{
	if (NULL == pu8_cnt)
	{
		*(pu8_out_array) = (uint8_t)((u16_in_value>>8) & 0xFF);
		*(pu8_out_array + 1) = (uint8_t)(u16_in_value & 0xFF);
		return true;		
	}
	if ((*pu8_cnt + 2) < FRAME_MAX_LEN)
	{
		*(pu8_out_array + *pu8_cnt) = (uint8_t)((u16_in_value>>8) & 0xFF);
		*(pu8_out_array + *pu8_cnt + 1) = (uint8_t)(u16_in_value & 0xFF);
		*pu8_cnt += 2;
		return true;
	}
	return false;
}

/*!
* @fn b_u32_to_array(uint32_t u32_in_value, uint8_t* pu8_out_array, 
*																									uint8_t* pu8_cnt)
* @brief Parse a 32bit number to an array
* @param[in] u32_in_value Source value.
* @param[out] pu8_out_array Dest array.
* @param[inout] pu8_cnt counter
* @return true if we have enough space to storage 32bit value
*/
bool b_u32_to_array(uint32_t u32_in_value, uint8_t* pu8_out_array, 
																								uint8_t* pu8_cnt)
{
	if (NULL == pu8_cnt)
	{
		*(pu8_out_array) = (uint8_t)((u32_in_value>>24) & 0xFF);
		*(pu8_out_array + 1) = (uint8_t)((u32_in_value>>16) & 0xFF);
		*(pu8_out_array + 2) = (uint8_t)((u32_in_value>>8) & 0xFF);
		*(pu8_out_array + 3) = (uint8_t)(u32_in_value & 0xFF);
		return true;		
	}
	if ((*pu8_cnt + 4) < FRAME_MAX_LEN)
	{
		*(pu8_out_array + *pu8_cnt) = (uint8_t)((u32_in_value>>24) & 0xFF);
		*(pu8_out_array + *pu8_cnt + 1) = (uint8_t)((u32_in_value>>16) & 0xFF);
		*(pu8_out_array + *pu8_cnt + 2) = (uint8_t)((u32_in_value>>8) & 0xFF);
		*(pu8_out_array + *pu8_cnt + 3) = (uint8_t)(u32_in_value & 0xFF);
		*pu8_cnt += 4;
		return true;
	}
	return false;
}

/*!
* @fn uint16_t u16_array_to_u16(uint8_t* pu8_in_array)
* @brief Convert Array to 16bit value
* @param[in] pu8_in_array Source arry.
* @return 16bit value
*/
uint16_t u16_array_to_u16(uint8_t* pu8_in_array)
{
	uint16_t u16_tmp = ((uint16_t)pu8_in_array[0]<<8) +
										 ((uint16_t)pu8_in_array[1]);
	return u16_tmp;
}

/*!
* @fn uint32_t u32_array_to_u32(uint8_t* pu8_in_array)
* @brief Convert Array to 32bit value
* @param[in] pu8_in_array Source arry.
* @return 32bit value
*/
uint32_t u32_array_to_u32(uint8_t* pu8_in_array)
{
	uint32_t u32_tmp = ((uint32_t)pu8_in_array[0]<<24) +
										 ((uint32_t)pu8_in_array[1]<<16) +
										 ((uint32_t)pu8_in_array[2]<<8) +
										 ((uint32_t)pu8_in_array[3]);
	return u32_tmp;
}

/*!
* @fn v_frame_build_header(uint8_t u8_frame_type, uint8_t u8_node_type_id, 
*																	uint32_t u32_node_id, uint8_t u8_num_of_data, 
*																	mqtt_data_frame_t *pstru_mqtt_data_frame)
* @brief Create a frame header. Must be call before parsing data field
* @param[in] u8_frame_type 							Frame type
* @param[in] u8_node_type_id 						Node type ID
* @param[in] u32_node_id 								ID of node
* @param[in] u8_num_of_data 						Number of data fields
* @param[out] *pstru_mqtt_data_frame 		Frame after builded
* @return None
*/
void v_frame_build_header(uint8_t u8_frame_type, uint8_t u8_node_type_id, 
														uint32_t u32_node_id, uint8_t u8_num_of_data, 
														mqtt_data_frame_t *pstru_mqtt_data_frame)
{
	memset(pstru_mqtt_data_frame, 0, sizeof(mqtt_data_frame_t));
	
	pstru_mqtt_data_frame->u8_frame_type = u8_frame_type;
	pstru_mqtt_data_frame->u8_node_type_id = NODETYPEID_RATA_MACHINE;
	pstru_mqtt_data_frame->u32_node_id = u32_node_id;
	pstru_mqtt_data_frame->u32_unix_time = u32_rtc_unix_time_get();
	pstru_mqtt_data_frame->u16_seq_number = u16_seq_number_get();
	pstru_mqtt_data_frame->u8_rssi = u8_gsm_hal_get_sim_rssi();
	pstru_mqtt_data_frame->u8_num_data_field = u8_num_of_data;
}

/*!
* @fn v_build_mqtt_struct(const char* topic, uint8_t u8_qos, uint8_t u8_retain,
*									mqtt_data_frame_t* pstru_mqtt_data_frame, mqtt_t* pstru_mqtt)
* @brief Build mqtt struct before writing to mqtt pub queue. 
* @param[in] str_topic String type. Topic that needs to be published
* @param[in] u8_qos Integer type. The QoS level at which the client wants to publish the messages.
*						0 At most once
*						1 At least once
*						2 Exactly once
* @param[in] u8_retain Integer type. Whether to enable the server to retain the message after 
* it has been delivered to the current subscribers.
*						0 Disable the server to retain the message after it has been delivered to the
*							current subscribers
*						1 Enable the server to retain the message after it has been delivered to the
*							current subscribers			
* @param[in] pstru_mqtt_data_frame Pointer of struct mqtt_data_frame that be used to build stru_mqtt.
* @param[out] pstru_mqtt Pointer of struct mqtt will be built.
* @return None
*/
void v_build_mqtt_struct(const char* str_topic, uint8_t u8_qos, uint8_t u8_retain,
									mqtt_data_frame_t* pstru_mqtt_data_frame, mqtt_t* pstru_mqtt)
{
	memset(pstru_mqtt, 0, sizeof(mqtt_t));
	
	ustrnlcpy(pstru_mqtt->au8_topic, str_topic, MQTT_TOPIC_LEN_MAX);
	pstru_mqtt->u8_qos = u8_qos;
	pstru_mqtt->u8_retain = u8_retain;
	/*
	|1byte FrameType|1byte NodeTypeID|4byte NodeID|2byte SeqNumber|1byte RSSI|4byte UnixTime|
	|1byte DataTypeID|2byte DataID|1byte ValueLen|ValueLen bytes Value|	
	*/
	uint8_t u8_cnt = 0;
	// FrameType
	pstru_mqtt->au8_payload[u8_cnt++] = pstru_mqtt_data_frame->u8_frame_type;
	// NodeTypeID
	pstru_mqtt->au8_payload[u8_cnt++] = pstru_mqtt_data_frame->u8_node_type_id;
	// NodeID: MSB first!
	b_u32_to_array(pstru_mqtt_data_frame->u32_node_id, pstru_mqtt->au8_payload, &u8_cnt);
	// SeqNumber: MSB first!
	b_u16_to_array(pstru_mqtt_data_frame->u16_seq_number, pstru_mqtt->au8_payload, &u8_cnt);
	// RSSI
	pstru_mqtt->au8_payload[u8_cnt++] = pstru_mqtt_data_frame->u8_rssi;
	// UinxTime: MSB first
	uint32_t u32_tmp = pstru_mqtt_data_frame->u32_unix_time;
	b_u32_to_array(u32_tmp, pstru_mqtt->au8_payload, &u8_cnt);
	// DataField
	if (pstru_mqtt_data_frame->u8_num_data_field > FRAME_MAX_DATA_FIELD)
	{
		pstru_mqtt_data_frame->u8_num_data_field = FRAME_MAX_DATA_FIELD;
	}
	for(uint8_t i = 0; i < pstru_mqtt_data_frame->u8_num_data_field; i++)
	{
		// DataTypeID
		pstru_mqtt->au8_payload[u8_cnt++] = pstru_mqtt_data_frame->stru_data_field[i].u8_type_id;
		// DataID
		b_u16_to_array(pstru_mqtt_data_frame->stru_data_field[i].u16_id, pstru_mqtt->au8_payload, &u8_cnt);
		// DataLen
		pstru_mqtt->au8_payload[u8_cnt++] = (uint8_t)((pstru_mqtt_data_frame->stru_data_field[i].u8_value_len) & 0xff);;
		// DataValue
		memcpy(pstru_mqtt->au8_payload + u8_cnt, pstru_mqtt_data_frame->stru_data_field[i].au8_payload,
							pstru_mqtt_data_frame->stru_data_field[i].u8_value_len);
		u8_cnt += pstru_mqtt_data_frame->stru_data_field[i].u8_value_len;
	}
	pstru_mqtt->u8_payload_len = u8_cnt;
}

/*!
* @fn void v_frame_mqtt_parse(mqtt_data_frame_t* pstru_mqtt_data_frame, mqtt_t* pstru_mqtt)
* @brief Parse mqqt payload from server to frame 
* @param[in] pstru_mqtt Pointer to mqtt data that received from mqtt will be parsed to frame
* @param[out] pstru_mqtt_data_frame Pointer of struct mqtt data frame.
* @return None
*/
void v_frame_mqtt_parse(mqtt_t* pstru_mqtt, mqtt_data_frame_t* pstru_mqtt_data_frame)
{
	memset(pstru_mqtt_data_frame, 0, sizeof(mqtt_data_frame_t));
		/*
	|1byte FrameType|1byte NodeTypeID|4byte NodeID|2byte SeqNumber|1byte RSSI|4byte UnixTime|
	|1byte DataTypeID|2byte DataID|1byte ValueLen|ValueLen bytes Value|	
	*/
	uint8_t u8_cnt = 0;
	// FrameType
	pstru_mqtt_data_frame->u8_frame_type = pstru_mqtt->au8_payload[u8_cnt++];
	// NodeTypeID
	pstru_mqtt_data_frame->u8_node_type_id = pstru_mqtt->au8_payload[u8_cnt++];
	// NodeID: MSB first!
	pstru_mqtt_data_frame->u32_node_id = u32_array_to_u32(pstru_mqtt->au8_payload + u8_cnt);
	u8_cnt += 4;
	// SeqNumber: MSB first!
	pstru_mqtt_data_frame->u16_seq_number = u16_array_to_u16(pstru_mqtt->au8_payload + u8_cnt);
	u8_cnt += 2;
	// RSSI
	pstru_mqtt_data_frame->u8_rssi = pstru_mqtt->au8_payload[u8_cnt++];
	// UinxTime: MSB first
	pstru_mqtt_data_frame->u32_unix_time = u32_array_to_u32(pstru_mqtt->au8_payload + u8_cnt);
	u8_cnt += 4;
	// DataField
	uint8_t i = 0;
	for(i = 0; i < FRAME_MAX_DATA_FIELD; i++)
	{
		if(pstru_mqtt->u8_payload_len > u8_cnt)
		{
			// DataTypeID
			pstru_mqtt_data_frame->stru_data_field[i].u8_type_id = pstru_mqtt->au8_payload[u8_cnt++];
			// DataID
			pstru_mqtt_data_frame->stru_data_field[i].u16_id = u16_array_to_u16(pstru_mqtt->au8_payload + u8_cnt);
			u8_cnt += 2;
			// DataLen
			pstru_mqtt_data_frame->stru_data_field[i].u8_value_len = pstru_mqtt->au8_payload[u8_cnt++];
			// DataValue
			if (pstru_mqtt_data_frame->stru_data_field[i].u8_type_id != DATATYPEID_STRING)
			{
				pstru_mqtt_data_frame->stru_data_field[i].u32_value = u32_array_to_u32(pstru_mqtt->au8_payload + u8_cnt);
			}
			memcpy(pstru_mqtt_data_frame->stru_data_field[i].au8_payload, pstru_mqtt->au8_payload + u8_cnt,
								pstru_mqtt_data_frame->stru_data_field[i].u8_value_len);
			u8_cnt += pstru_mqtt_data_frame->stru_data_field[i].u8_value_len;
		}
		else
		{
			break;
		}
	}
	pstru_mqtt_data_frame->u8_num_data_field = i;
}
