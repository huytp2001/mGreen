/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2014 PIF Lab Ltd.
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

#ifndef __CAN_APP_H__
#define __CAN_APP_H__


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#include "stdint.h"
//#include "can_parser.h"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

typedef enum
{
	CANMSG_ARGUMENT_TYPE_INVALID 				= 0,
	CANMSG_ARGUMENT_TYPE_U8							= 1,
	CANMSG_ARGUMENT_TYPE_I8							= 2,
	CANMSG_ARGUMENT_TYPE_U16						= 3,
	CANMSG_ARGUMENT_TYPE_I16						= 4,
	CANMSG_ARGUMENT_TYPE_U32						= 5,
	CANMSG_ARGUMENT_TYPE_I32						= 6,
	CANMSG_ARGUMENT_TYPE_FLOAT					= 7
} E_CANMSG_ARGUMENT_TYPE;

typedef struct
{
	uint8_t u8_node_addr;
	uint8_t u8_sensor_type;
	uint8_t u8_evt_id;
	E_CANMSG_ARGUMENT_TYPE e_data_type;
	uint8_t au8_data[4];
	uint8_t u8_data_len;
	int32_t s32_res_status;
//	char *pi8_node_name;
//	char *pi8_sensor_name;
//	char *pi8_unit;
} STRU_CANAPP_DATA_T;


typedef enum
{
	CANAPP_EVENT_NONE = 0,
	CANAPP_EVENT_SENSOR_IS_READY,
	CANAPP_EVENT_SENSOR_IS_ALIVE,
	CANAPP_EVENT_SENSOR_IS_VALID,
	CANAPP_EVENT_SENSOR_GET_TIME,
	CANAPP_EVENT_SENSOR_FLOW_RAW,
	CANAPP_EVENT_SENSOR_TEMP_RAW,
	CANAPP_EVENT_SENSOR_WIND_SPD_RAW,
	CANAPP_EVENT_SENSOR_WIND_DIR_RAW,
	CANAPP_EVENT_SENSOR_RAIN_RAW,
	CANAPP_EVENT_SENSOR_HUMID_RAW,
	CANAPP_EVENT_SENSOR_PH_RAW,
	CANAPP_EVENT_SENSOR_EC_RAW,
	CANAPP_EVENT_SENSOR_EC_TEMP_RAW,
	CANAPP_EVENT_SENSOR_CALIB_DONE,
	CANAPP_EVENT_SENSOR_POLL_RAW,
	CANAPP_EVENT_MAX
} E_CANAPP_EVENT_ID;


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPP_Init (void);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/



#endif /* __CAN_APP_H__ */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
