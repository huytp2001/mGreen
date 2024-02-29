/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file frame_parser.h
 * @author Danh Pham
 * @date 25 Nov 2020
 * @version: 1.0.0
 * @brief 
 */
#ifndef FRAME_PARSER_H_
#define FRAME_PARSER_H_ 
#include <stdint.h>
#include <stdbool.h>
#include "mqtt_task.h"

/*
|1byte FrameType|1byte NodeTypeID|4byte NodeID|2byte SeqNumber|1byte RSSI|4byte UnixTime|
|1byte DataTypeID|2byte DataID|1byte ValueLen|ValueLen bytes Value|
*/
#define FRAME_MAX_DATA_FIELD					(7)
#define FRAME_MAX_DATA_VALUE					(20)
#define FRAME_MAX_LEN									(127)
/* Frame type */
#define FRAME_TYPE_DATA_NO_GA					0x20 /**<Data of Sensor Node (GW transparent)*/
#define FRAME_TYPE_DATA_CON						0x25 /**<Data of Controller*/
#define FRAME_TYPE_CON_MA							0x30 /**<Manual control frame*/
#define FRAME_TYPE_QUERY							0x3F /**<Request device's infor*/
#define FRAME_TYPE_CON_ME							0x35 /**<Emergency control frame*/
#define FRAME_TYPE_CONFIG							0x50 /**<Configuration frame from cloud*/
#define FRAME_TYPE_INIT								0x60 /**<Init frame from Controller/GW*/
#define FRAME_TYPE_KA									0x70 /**<Keep alive frame from Controller/GW*/
#define FRAME_TYPE_PING								0x72 /**<Ping frame from cloud*/
#define FRAME_TYPE_NOTI								0x80 /**<Notify frame from Controller*/
#define	FRAME_TYPE_SCHEDULE_FILE 			0x90 /**<Frame used to download schedule file */
#define	FRAME_TYPE_THRESHOLD_FILE			0x92 /**<Frame used to download threshold file */
#define	FRAME_TYPE_FIRMWARE_FILE  		0xA0 /**<Frame used to update firmware */
 
/* Node Type */
#define NODETYPEID_SOIMOITURE					0x01
#define NODETYPEID_CLIMATE						0x02
#define NODE_TYPE_ID_VPD							0x02
#define NODETYPEID_WEATHER						0x03
#define NODETYPEID_PHEC								0x04
#define NODETYPEID_RATA_MACHINE				0x04
#define NODETYPEID_GREENPOT						0x05

/* Data type */
#define DATATYPEID_SENSOR							0x01
#define DATATYPEID_CONTROLLER					0x02
#define DATATYPEID_STRING							0x03

/* Data ID */
#define DATAID_PH1										0x0011
#define DATAID_PH2										0x0012
#define DATAID_EC1										0x0013
#define DATAID_EC2										0x0014
#define DATAID_PRESSURE								0x0015
#define DATAID_PRE_PRESSURE						0x0016
#define DATAID_FLOW_OUT								0x0017
#define DATAID_FLOW_IN								0x0018
#define DATAID_SONAR									0x0019
#define DATAID_VALVE									0x0020
#define DATAID_CALCULATED_EC					0x0025

#define DATAID_LOCATION_ID						0x0031
#define DATAID_USE_FER								0x0038
#define DATAID_TYPE_RUN								0x0039
#define DATAID_TIME_BEGIN							0x003B
#define DATAID_TIME_FINISH						0x003C
#define DATAID_NEXT_LOCATION					0x003F
#define DATAID_WATER_VOLUME						0x0040
#define DATAID_FER_VOLUME							0x0041
#define DATAID_FER_WEIGHT							0x0042

#define DATAID_PH_SET									0x0043
#define DATAID_PH_MIN									0x0044
#define DATAID_PH_AVG									0x0045
#define DATAID_PH_MAX									0x0046
#define DATAID_EC_SET									0x0047
#define DATAID_EC_MIN									0x0048
#define DATAID_EC_AVG									0x0049
#define DATAID_EC_MAX									0x004A
#define DATAID_EC_AVG_FLOW						0x0025

#define DATAID_CH1_SET								0x0050
#define DATAID_CH2_SET								0x0051
#define DATAID_CH3_SET								0x0052
#define DATAID_CH4_SET								0x0053
#define DATAID_CH5_SET								0x0054
#define DATAID_CH1_RATE								0x0055
#define DATAID_CH2_RATE								0x0056
#define DATAID_CH3_RATE								0x0057
#define DATAID_CH4_RATE								0x0058
#define DATAID_CH5_RATE								0x0059

#define DATAID_CH1_VOLUME							0x005A
#define DATAID_CH2_VOLUME							0x005B
#define DATAID_CH3_VOLUME							0x005C
#define DATAID_CH4_VOLUME							0x005D
#define DATAID_CH5_VOLUME							0x005E

#define DATAID_CH1_WEIGHT							0x0060
#define DATAID_CH2_WEIGHT							0x0061
#define DATAID_CH3_WEIGHT							0x0062
#define DATAID_CH4_WEIGHT							0x0063
#define DATAID_CH5_WEIGHT							0x0064

#define DATAID_TANK1_TYPE							0x0065
#define DATAID_TANK2_TYPE							0x0066
#define DATAID_TANK3_TYPE							0x0067
#define DATAID_TANK4_TYPE							0x0068
#define DATAID_TANK5_TYPE							0x0069

#define DATAID_CALIB_PHEC1						0x009B
#define DATAID_CALIB_PHEC2						0x009C

#define DATA_ID_INPUT_1								0x00A0
#define DATA_ID_INPUT_2								0x00A1
#define DATA_ID_INPUT_3								0x00A2
#define DATA_ID_INPUT_4								0x00A3

#define DATAID_MACHINE_STATE					0x00F0
#define DATAID_FERTIGATION_STATE			0x00F1

#define DATAID_ERR_CODE								0x00F3
#define DATAID_PRO_TIMEPAUSE					0x00F4

#define DATAID_DOWNLOAD_FILE_STATE		0x0081
#define DATAID_CSV_VERSION						0x0082
#define DATAID_FIRMWARE_VER						0x00F6
#define DATAID_HARDWARE_VER						0x00F7
#define DATAID_SYSTEN_STATUS					0x00F8


#define DATAID_MANUAL_CONTROL					0x1001

/* Notification data ID */
#define DATAID_VALVE1_ERROR						0xFFE0
#define DATAID_VALVE2_ERROR						0xFFE1
#define DATAID_VALVE3_ERROR						0xFFE2
#define DATAID_VALVE4_ERROR						0xFFE3
#define DATAID_VALVE5_ERROR						0xFFE4

#define DATAID_PARSECSV_ERROR					0xFFE5
#define DATAID_PUMP_ERR								0xFFE6
#define DATAID_MAINFLOW_ERR						0xFFE7
#define DATAID_SD_LOST								0xFFE8
#define DATAID_EXRELAY1_ERR						0xFFF9
#define DATAID_EXRELAY2_ERR						0xFFFA

#define DATAID_PRO_FAIL								0xFFEA
#define DATAID_FLOW_OUT_WARNING				0xFFEB
#define DATAID_FLOW_IN_WARNING				0xFFEC
#define DATAID_EC2_WARNING						0xFFED
#define DATAID_EC1_WARNING						0xFFEE
#define DATAID_PH2_WARNING						0xFFEF
#define DATAID_PH1_WARNING						0xFFF0

#define DATAID_TANK1_EMPTY						0xFFF1
#define DATAID_TANK2_EMPTY						0xFFF2
#define DATAID_TANK3_EMPTY						0xFFF3
#define DATAID_TANK4_EMPTY						0xFFF4
#define DATAID_TANK5_EMPTY						0xFFF5


/* Public value */
#define PUBLISH_STATE_STOPED					0x00
#define PUBLISH_STATE_RUNNING					0x01
#define PUBLISH_STATE_PREPARE					0x02


/* Emergency control DataID */
#define DATAID_CONTROL_START					0x0001
#define DATAID_CONTROL_CONTINUE				0x0002
#define DATAID_CONTROL_STOP						0x0003
#define DATAID_CONTROL_TEST_MODE			0x000F

// Frame type Query
//Data type ID
#define DATATYPE_ID_QUERY								0x00
#define DATATYPE_ID_NOTI_LOW						0x02
#define DATATYPE_ID_NOTI_HIGH						0x01
//Data ID
#define DATAID_QUERY_PHONE_NUM					0x0001
#define DATAID_QUERY_BUDGET							0x0002
#define DATAID_MI10_REGIS								0x0003
#define DATAID_QUERY_VERSION_CODE				0x0004
#define DATAID_MI10_UN_REGIS						0x0005
#define DATAID_MI10_UN_REGIS_ACK				0x0006
#define DATAID_QUERY_BUDGET_DATA				0x0007
#define DATAID_READ_1_SMS								0x0008
#define DATAID_SYNC_TIME								0x0100
#define DATAID_RESET										0x00F0
#define DATAID_FORMAT_SD_CARD						0x000E
#define DATAID_REQUEST_SENSOR_DATA			0x000F
#define DATAID_CHECK_SD_CARD						0x0011

//semi auto
#define DATAID_TIME_BEFORE		0x70
#define DATAID_TIME_MIX				0x71
#define DATAID_TIME_AFTER			0x72
//Threshold
#define DATAID_THD_EVENT_ID					0x73
#define DATAID_THD_ACT_TYPE					0x74
#define DATAID_THD_DURATION_UNIT		0x75
#define DATAID_THD_DURATION					0x76
//vpd
#define DATAID_VPD									0x77
#define DATAID_FAN_STATUS						0x78
#define DATAID_COOLINGPAD_STATUS		0x79
#define DATAID_VPD_MONITOR_NODE			0x01A3
#define DATAID_VPD_START_TIME				0x01A4
#define DATAID_VPD_STOP_TIME				0x01A5
#define DATAID_VPD_MIN_TEMP					0x01A6
#define DATAID_VPD_MAX_TEMP					0x01A7

#define DATAID_PORT_32_BIT_L				0x2002
#define DATAID_PORT_32_BIT_H				0x2003

//DataID from sensor
#define DATA_ID_TEMP											0x01
#define DATA_ID_TEMP_LAND_2								0x02
#define DATA_ID_ALS												0x03
#define DATA_ID_WIND											0x04
#define DATA_ID_HUMID											0x05
#define DATA_ID_TEMP_LAND_1								0x07
#define DATA_ID_SOIMOITURE_1							0x08
#define DATA_ID_SOIMOITURE_2							0x09
#define DATA_ID_RAIN											0x0A
#define DATA_ID_PH_1											0x11
#define DATA_ID_PH_2											0x12
#define DATA_ID_EC_1											0x13
#define DATA_ID_EC_2											0x14
#define DATA_ID_EC_OUTPUT_SOIL      			0x15
#define DATA_ID_PH_OUTPUT_SOIL      			0x16
#define DATA_ID_EC_OUTPUT_LIQUID    			0x20
#define DATA_ID_pH_OUTPUT_LIQUID    			0x21
#define DATA_ID_O2												0x18
#define DATA_ID_WIND_DIR									0x1F
#define DATA_ID_BAT												0xFF

#define DATA_ID_STATISTICS								0x01
#define DATA_ID_RAM_STATUS								0x02
#define DATA_ID_MCU_STATUS								0x03
/*!
* @enum E_FTP_FEEDBACK_STATUS
* Status of FTP download process 
*/
typedef enum
{
	FEEDBACK_REQ_DOWNLOAD			=	0x01, /**<Has download request signal from server */
	FEEDBACK_DOWNLOAD_SUCCESS	=	0x02, /**<Download successfully */
	FEEDBACK_CRC_FAIL   			= 0xFA,	/**<CRC don't match */
	FEEDBACK_PARSE_FAIL				=	0xFB, /**<Can not parse file */
	FEEDBACK_OPEN_FAIL				=	0xFC, /**<Can not open file */
	FEEDBACK_TRANSFER_FAIL		=	0xFD, /**<Can not tranfer file from module to sd card */
	FEEDBACK_SAVE_TO_SD_FAIL	=	0xFE, /**<Can not save file to sd card */
	FEEDBACK_DOWNLOAD_FAIL		=	0xFF, /**<Download fail with no clear reason */
}E_FTP_FEEDBACK_STATUS;

/*!
* @enum E_NOTI_TYPE
* Type of notification 
*/
typedef enum
{
	TYPE_NOTI_ERROR=        0x00, /**<Error occured */
	TYPE_NOTI_HIGH = 				0x01,	/**<Value greater than setpoint */
	TYPE_NOTI_LOW = 				0x02, /**<Value less than setpoint */
	TYPE_NOTI_NOT_COMPL	= 	0x03,	/**<Program is not completed */
	TYPE_NOTI_SENSOR_LOST	= 0x04,	/**<Sensor lost */
	TYPE_NOTI_CRC = 0x05,					/**<CRC */
}E_NOTI_TYPE;
 
/*!
* @struct mqtt_data_field_t
* Struct of a data field in data frame. 
* Reference docs: 
* https://knowledge.mimosatek.com/books/m-iot-protocol/page/frame-detail
*/
typedef struct
{
	uint8_t u8_type_id; 			/**<Data type id of data field */
	uint16_t u16_id; 					/**<Data id */
	uint8_t u8_value_len;			/**<Data length (bytes) */
	uint8_t au8_payload[FRAME_MAX_DATA_VALUE + 1];  /**<Data payload */
	uint32_t u32_value;
}mqtt_data_field_t;

/*!
* @struct mqtt_data_frame_t
* Struct of a data frame. 
* Reference docs: 
* https://knowledge.mimosatek.com/books/m-iot-protocol/page/frame-detail
*/
typedef struct
{
	uint8_t u8_frame_type; 								/**<Frame type of frame */
	uint8_t u8_node_type_id;							/**<Node type id */
	uint32_t u32_node_id;									/**<Node id */
	uint16_t u16_seq_number;							/**<Sequence number (an auto increment number) */
	uint8_t u8_rssi;											/**<RSSI of module */
	uint32_t u32_unix_time;								/**<Time frame was generated GMT + 7 */
	uint8_t u8_num_data_field;						/**< Length of data fields */
	mqtt_data_field_t stru_data_field[FRAME_MAX_DATA_FIELD];	/**< Data fields */
}mqtt_data_frame_t;

typedef void (*mqtt_call_back)(mqtt_data_frame_t stru_mqtt_data_frame);

/*!
* @public functions prototype
*/
uint16_t u16_seq_number_get(void);
bool b_u16_to_array(uint16_t u16_in_value, uint8_t* pu8_out_array, 
																								uint8_t* pu8_cnt);
bool b_u32_to_array(uint32_t u32_in_value, uint8_t* pu8_out_array, 
																								uint8_t* pu8_cnt);
uint16_t u16_array_to_u16(uint8_t* pu8_in_array);
uint32_t u32_array_to_u32(uint8_t* pu8_in_array);
void v_frame_build_header(uint8_t u8_frame_type, uint8_t u8_node_type_id, 
														uint32_t u32_node_id, uint8_t u8_num_of_data, 
														mqtt_data_frame_t *pstru_mqtt_data_frame);
void v_build_mqtt_struct(const char* str_topic, uint8_t u8_qos, uint8_t u8_retain,
									mqtt_data_frame_t* pstru_mqtt_data_frame, mqtt_t* pstru_mqtt);
void v_frame_mqtt_parse(mqtt_t* pstru_mqtt, mqtt_data_frame_t* pstru_mqtt_data_frame);
bool b_mqtt_register_callback(mqtt_call_back callback);
void v_mqtt_unregister_callback(mqtt_call_back callback);
#endif /* FRAME_PARSER_H_ */
