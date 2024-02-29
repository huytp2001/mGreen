/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2013 PIF Lab Ltd.
**   All Rights Reserved.
**
**   The information contained herein is copyrighted by and
**   is the sole property of PIF Lab Ltd.
**   Any unauthorized use, copying, transmission, distribution
**   or disclosure of such information is strictly prohibited.
**
**   This Copyright notice shall not be removed or modified
**   without prior written consent of PIF Lab Ltd.
**
**   PIF Lab Ltd. reserves the right to modify this software
**   without notice.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : 
**   Project     : 
**   Description :
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifndef __CAN_PARSE_H__
#define __CAN_PARSE_H__


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include "can_drv.h"
#include "can_app.h"
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Message type. */
#define CANMSG_TYPE_ACK								 0x05	//Controller <-> Nodes
#define CANMSG_TYPE_DATA               0x04	//Controller <-> Nodes
#define CANMSG_TYPE_EVENT							 0x03	//Controller <-  Nodes
#define CANMSG_TYPE_SETTING            0x02	//Controller  -> Nodes
#define CANMSG_TYPE_ALERT              0x01	//Controller <-  Nodes
#define CANMSG_TYPE_ALARM              0x00 //Controller <-  Nodes

/* Pre-defined Address. */
#define CANMSG_ADDR_MGREEN_CONTROLLER	 0x00
#define CANMSG_ADDR_PC_DEBUG_TOOL      0xFD
#define CANMSG_ADDR_WAIT_ADDRESSING    0xFE
#define CANMSG_ADDR_BROADCAST          0xFF

//#define CANMSG_ADDR_RESERVED           0x00
#define CANMSG_ADDR_DYNAMIC            0xA5

#define CANMSG_ADDR_SONAR							 0x20	//0x20-0x24
#define CANMSG_ADDR_RELAY							 0x30//0x30-0x3f
#define CANMSG_ADDR_DO_PH							 0x08//0x08-0x0f

/* Command ID from Controller. */
typedef enum
{
	CANMSG_CMD_NONE 							= 0,
	CANMSG_CMD_DISCOVERY					= 1,
	CANMSG_CMD_CHECK_ALIVE				= 2,
	CANMSG_CMD_SET_ADDR						= 3,
	CANMSG_CMD_SET_DATE_TIME			= 4,
	CANMSG_CMD_CALIB_POINT_1			= 5,
	CANMSG_CMD_CALIB_POINT_2			= 6,
	CANMSG_CMD_CALIB_POINT_3			= 7,
	CANMSG_CMD_CALIB_POINT_4			= 8,
	CANMSG_CMD_CALIB_POINT_5			= 9,
	CANMSG_CMD_RESET							= 10,
	CANMSG_CMD_SET_CAN_SPEED			= 11,
	CANMSG_CMD_DIAGNOSTIC_MODE		=	12,
	CANMSG_CMD_DEBOUNCE						= 13,
	CANMSG_CMD_CHANGE							= 14,
	CANMSG_CMD_REQUEST_DATA				= 15,
	CANMSG_CMD_CONTROL_RELAY				= 16,
	CANMSG_CMD_MAX								= 31
} E_CANMSG_CMD_ID;

typedef enum
{
	CANMSG_EVENT_NONE 									= 0,
	CANMSG_EVENT_BROADCAST_ADDR					= 1,
	CANMSG_EVENT_IS_ALIVE								= 2,
	CANMSG_EVENT_ACK_SET_ADDR						= 3,
	CANMSG_EVENT_ACK_SET_DATE_TIME			= 4,
	CANMSG_EVENT_ACK_CALIB_POINT_1			= 5,
	CANMSG_EVENT_ACK_CALIB_POINT_2			= 6,
	CANMSG_EVENT_ACK_CALIB_POINT_3			= 7,
	CANMSG_EVENT_ACK_CALIB_POINT_4			= 8,
	CANMSG_EVENT_ACK_CALIB_POINT_5			= 9,
	CANMSG_EVENT_ACK_CALIB_POINT_6			= 10,
	CANMSG_EVENT_ACK_SET_CAN_SPEED			= 11,
	CANMSG_EVENT_ACK_DIAGNOSTIC_MODE		=	12,
	CANMSG_EVENT_ACK_DEBOUNCE						= 13,
	CANMSG_EVENT_ACK_CHANGE							= 14,
	CANMSG_EVENT_ACK_REQUEST_DATA				= 15,
   
	CANMSG_EVENT_MAX										= 31
} E_CANMSG_EVENT_ID;

typedef enum
{
	CANMSG_SENSOR_TYPE_INVALID 					= 0x00,
	CANMSG_SENSOR_TYPE_AMB_TEMP 				= 0x01,
	CANMSG_SENSOR_TYPE_SOIL_TEMP_2 			= 0x02,
	CANMSG_SENSOR_TYPE_LIGHT 						= 0x03,
	CANMSG_SENSOR_TYPE_WIND_SPEED 			= 0x04,
	CANMSG_SENSOR_TYPE_AMB_HUMIDITY			= 0x05,
	CANMSG_SENSOR_TYPE_WIND_DIR					= 0x06,
	CANMSG_SENSOR_TYPE_SOIL_TEMP_1			= 0x07,
	CANMSG_SENSOR_TYPE_SOIL_HUMIDITY_1 	= 0x08,
	CANMSG_SENSOR_TYPE_SOIL_HUMIDITY_2	= 0x09,
	CANMSG_SENSOR_TYPE_RAIN							= 0x0A,
	CANMSG_SENSOR_TYPE_TUBE_PRESSURE		= 0x0D,
	CANMSG_SENSOR_TYPE_OUT_TEMP					= 0x0E,
	CANMSG_SENSOR_TYPE_OXY							= 0x0F,	
	CANMSG_SENSOR_TYPE_PH								= 0x10,
	CANMSG_SENSOR_TYPE_PH_CAY						= 0x11,
	CANMSG_SENSOR_TYPE_EC								= 0x13,
	CANMSG_SENSOR_TYPE_FLOW							= 0x19,
	CANMSG_SENSOR_TYPE_SONAR_1					= 0x20,
	CANMSG_SENSOR_TYPE_SONAR_2					= 0x21,
	CANMSG_SENSOR_TYPE_SONAR_3					= 0x22,
	CANMSG_SENSOR_TYPE_SONAR_4					= 0x23,
	CANMSG_SENSOR_TYPE_SONAR_5					= 0x24,
	CANMSG_SENSOR_TYPE_RELAY						= 0x30,
	CANMSG_SENSOR_TYPE_TANKLEVEL				= 0xA0,
	CANMSG_SENSOR_TYPE_SONAR_DISTANCE		= 0xA2,
	CANMSG_SENSOR_TYPE_IN_TEMP					= 0xFE,
	CANMSG_SENSOR_TYPE_BATT_LEVEL				= 0xFF
} E_CANMSG_SENSOR_TYPE;

/* Sensor: CAN Message Structure. */
typedef struct
{
	uint8_t u8_msg_type;
	uint8_t u8_dest_addr;
	uint8_t u8_src_addr;
	uint8_t u8_msg_id;
	uint8_t u8_sensor_id;
	uint8_t u8_cmd;
	E_CANMSG_ARGUMENT_TYPE e_arg_type;
	uint8_t u8_data_len;
	uint8_t au8_data[4];
} STRU_CANPSR_MSG_T;

typedef bool (*CAN_EVENT_HANDLE)(STRU_CANPSR_MSG_T *pstru_canpsr_msg, STRU_CANDRV_MSG_T *pstru_candrv_msg);
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Initialize CANPSR Message module. */
void v_CANPRS_Init (void);

int32_t s32_CANPSR_Proc_Send_Cmd (STRU_CANPSR_MSG_T *pstru_canpsr_msg);
extern void v_CANPRS_Send_Command(E_CANMSG_CMD_ID e_cmdId, STRU_CANAPP_DATA_T *pstru_node_info);

#endif /* __TEMPLATE_H__ */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
