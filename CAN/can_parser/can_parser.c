/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2013 - 2014.
**   All Rights Reserved.
**   Developed by: PIF Lab Ltd.
**
**   The information contained herein is copyrighted by and
**   is the sole property of PIF.
**   Any unauthorized use, copying, transmission, distribution
**   or disclosure of such information is strictly prohibited.
**
**   This Copyright notice shall not be removed or modified
**   without prior written consent of PIF.
**
**   PIF reserves the right to modify this software
**   without notice.
**
**   This software is developed by PIF Lab Ltd.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : can_parser.c
**   Project     : MGreen Controller
**   Description : Functions for CANPSR layer.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* FreeRTOS. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Application includes. */
#include "can_parser.h"
#include "can_process.h"
#include "can_app.h"
#include "CAN_task.h"
#include "wdt.h"
#include "HAL_BSP.h"
#include "sensor.h"
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#define CANPRS_PROC_EVT_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 4)
#define CANPRS_PROC_EVT_TASK_PRIO           (tskIDLE_PRIORITY + 4)

/* Wait period for CAN Command to be available. */
#define CANPRS_PROC_CMD_WAIT                (10)

#define CANPRS_AUTO_MSG_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE)
#define CANPRS_AUTO_MSG_TASK_PRIO           (tskIDLE_PRIORITY + 1)

/* Wait period to check for auto message generation. */
#define CANPRS_AUTO_MSG_WAIT                (1000)

#define CANPRS_PROC_CMD_RESULT_OK            0
#define CANPRS_PROC_CMD_RESULT_OK_SEND       1
#define CANPRS_PROC_CMD_RESULT_ERROR        -1

#define CANPRS_LED_BLINK_FAST_INTERVAL       250
#define CANPRS_LED_BLINK_SLOW_INTERVAL       2500


static StaticTask_t xCanParser_TaskBuffer;
static StackType_t  xCanParser_Stack[CANPRS_PROC_EVT_TASK_STACK_SIZE];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static void v_CANPRS_Proc_Evt_Task (void *pvParameters);

static void v_CANPRS_Build_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                                        uint8_t                u8_msg_type,
                                        uint8_t                u8_dest_addr,
                                        uint8_t                u8_msg_id,
                                        uint8_t                u8_cmd,
                                        uint8_t                u8_data_len,
                                        uint8_t               *pau8_data,
																			  uint8_t 								u8_sensor_id,
																			  E_CANMSG_ARGUMENT_TYPE e_arg_type);

static void v_CANPSR_Msg_From_CANDRV_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                                                  STRU_CANDRV_MSG_T *pstru_candrv_msg);

static void v_CANPRS_Msg_To_CANDRV_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                                                STRU_CANDRV_MSG_T     *pstru_candrv_msg);

static void v_CANPRS_Proc_Evt (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                               STRU_CANDRV_MSG_T     *pstru_candrv_msg);



/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Address of host who assigns the MGreen address. */
static uint8_t u8_my_addr;

/* Incremental ID to identify message event from Sensor. */
static uint8_t u8_my_msg_id;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      void v_CANPRS_Init (void)
**
**   Arguments:
**      N/A
**
**   Return:
**      N/A
**
**   Description:
**      Initialize CAN driver, create task to process CAN command from
**      AssetPro device.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANPRS_Init (void)
{
   /* Create task to receive and process commands from AssetPro device. */
   xTaskCreateStatic(v_CANPRS_Proc_Evt_Task,
               "CANPRS PROC EVT",
               CANPRS_PROC_EVT_TASK_STACK_SIZE,
               NULL,
               CANPRS_PROC_EVT_TASK_PRIO,
								xCanParser_Stack,
               &xCanParser_TaskBuffer);
	// vTaskSuspend(handle);
   /* Init device address: waiting to be addressed. */
   u8_my_addr = CANMSG_ADDR_MGREEN_CONTROLLER;
   /* Init message ID. This will be increased for event generated by MGreen without request from Server. */
   u8_my_msg_id = 0;
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      static void v_CANPRS_Proc_Evt_Task (void *pvParameters)
**
**   Arguments:
**      (in) pvParameters - Task input (not used).
**
**   Return:
**      N/A
**
**   Description:
**      Task to receive the CAN events from device, process and send them to CANPROC.
*
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPRS_Proc_Evt_Task (void *pvParameters)
{
	static STRU_CANDRV_MSG_T stru_candrv_msg;
	static STRU_CANPSR_MSG_T stru_canpsr_msg;
	static uint8_t u8_can_proc_task_id = 0;
	/* Get task id from wdt */
	while(b_wdt_reg_new_task("can_process", &u8_can_proc_task_id) != true){}
	for( ; ; )
	{
		/* Reload wdt */
		b_wdt_task_reload_counter(u8_can_proc_task_id);
		if(s32_CANDRV_Receive_Msg(&stru_candrv_msg, CANPRS_PROC_CMD_WAIT) == 0)
		{
			 /* We received new message from CAN driver. */
			 /* Parse message into CANPSR message structure. */
			 v_CANPSR_Msg_From_CANDRV_Msg(&stru_canpsr_msg, &stru_candrv_msg);
			 /* Process message. */
			 v_CANPRS_Proc_Evt(&stru_canpsr_msg, &stru_candrv_msg);
		}
		else
		{
			 /* No command received. */
		}
		vTaskDelay(100);
	}
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      static void v_CANPRS_Proc_Evt (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
**                                     STRU_CANDRV_MSG_T     *pstru_candrv_msg)
**
**   Arguments:
**      (in)  pstru_canpsr_msg - Pointer to CANPSR message data structure to process
**      (in)  pstru_candrv_msg  - Pointer to CAN Driver message data structure to send response if needed
**
**   Return:
**      N/A
**
**   Description:
**      Process events received from sensor device. May send
**      responses if needed.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPRS_Proc_Evt (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                               STRU_CANDRV_MSG_T *pstru_candrv_msg)
{
	static STRU_CANAPP_DATA_T t_can_app_msg;
	/*
	** If destination address is not our or broadcast address,
	** we don't need to do any further processing.
	*/
	if ((pstru_canpsr_msg->u8_dest_addr != u8_my_addr) && (pstru_canpsr_msg->u8_dest_addr != CANMSG_ADDR_BROADCAST))
	{
		return;
	}

	t_can_app_msg.u8_node_addr = pstru_canpsr_msg->u8_src_addr;
	t_can_app_msg.u8_sensor_type = pstru_canpsr_msg->u8_sensor_id;
	t_can_app_msg.e_data_type = pstru_canpsr_msg->e_arg_type;
	t_can_app_msg.u8_data_len = pstru_canpsr_msg->u8_data_len;
	memcpy(t_can_app_msg.au8_data, pstru_canpsr_msg->au8_data, pstru_canpsr_msg->u8_data_len);
	t_can_app_msg.u8_evt_id = pstru_canpsr_msg->u8_cmd;
	t_can_app_msg.s32_res_status = 0;

	switch (pstru_canpsr_msg->u8_msg_type)
	{
	case CANMSG_TYPE_DATA:
	{
		switch (pstru_canpsr_msg->u8_sensor_id)
		{
			case CANMSG_SENSOR_TYPE_INVALID:
			 break;
			case CANMSG_SENSOR_TYPE_AMB_TEMP:
			 break;
			case CANMSG_SENSOR_TYPE_SOIL_TEMP_2:
			 break;
			case CANMSG_SENSOR_TYPE_LIGHT:
			 break;
			case CANMSG_SENSOR_TYPE_WIND_SPEED:
			 break;
			case CANMSG_SENSOR_TYPE_AMB_HUMIDITY:
			 break;
			case CANMSG_SENSOR_TYPE_WIND_DIR:
			 break;
			case CANMSG_SENSOR_TYPE_SOIL_TEMP_1:
			 break;
			case CANMSG_SENSOR_TYPE_SOIL_HUMIDITY_1:
			 break;
			case CANMSG_SENSOR_TYPE_SOIL_HUMIDITY_2:
			 break;
			case CANMSG_SENSOR_TYPE_RAIN:
			 break;
			case CANMSG_SENSOR_TYPE_TUBE_PRESSURE:
			 break;
			case CANMSG_SENSOR_TYPE_OUT_TEMP:
			 break;
			case CANMSG_SENSOR_TYPE_OXY:					 
			 break;
			case CANMSG_SENSOR_TYPE_RELAY:	
			{					 
			}
			 break;				 
			case CANMSG_SENSOR_TYPE_PH:
			{
				if(t_can_app_msg.u8_evt_id == CANMSG_CMD_CALIB_POINT_1)
				{
					//v_ext_phec_calib_step_update(t_can_app_msg.au8_data[0]);
				}
				else
				{
					static uint32_t u32_data = 0;
					u32_data = t_can_app_msg.au8_data[3] << 24 | t_can_app_msg.au8_data[2] << 16 | t_can_app_msg.au8_data[1] << 8 | t_can_app_msg.au8_data[0];
					//v_ext_phec_value_update(u32_data);
				}
			}
			break;
			case CANMSG_SENSOR_TYPE_EC:
			break;
			case CANMSG_SENSOR_TYPE_FLOW:
			break;
			case CANMSG_SENSOR_TYPE_IN_TEMP:
			break;
			case CANMSG_SENSOR_TYPE_BATT_LEVEL:
			break;
			case CANMSG_SENSOR_TYPE_TANKLEVEL:
			{
			}
			break;
			case CANMSG_SENSOR_TYPE_SONAR_DISTANCE:
			break;
			case CANMSG_SENSOR_TYPE_SONAR_1:
			break;
			case CANMSG_SENSOR_TYPE_SONAR_2:
			break;
			case CANMSG_SENSOR_TYPE_SONAR_3:
			break;
			case CANMSG_SENSOR_TYPE_SONAR_4:
			break;
			case CANMSG_SENSOR_TYPE_SONAR_5:
			break;
			default:
			break;
		}
	}
	case CANMSG_TYPE_SETTING:
	{
	 switch (pstru_canpsr_msg->u8_cmd)
	 {
		 default:
			 break;
	 }
	}
	break;
	case CANMSG_TYPE_ALERT:
	 break;
	case CANMSG_TYPE_ALARM:
	 break;
	case CANMSG_TYPE_EVENT:
	{
		switch (pstru_canpsr_msg->u8_cmd)
		{
		 case CANMSG_EVENT_BROADCAST_ADDR:
		 {
		 }
			break;
		 case CANMSG_EVENT_IS_ALIVE:
		 {
		 }
		 break;
		 default:
			 break;
		}
	}
	break;
	case CANMSG_TYPE_ACK:
	{
	 v_can_in_send_queue(&t_can_app_msg);
	}
	break;
	default:
	 break;
	}
}


int32_t s32_CANPSR_Proc_Send_Cmd (STRU_CANPSR_MSG_T *pstru_canpsr_msg)
{
   /*
   ** Command processing result.
   **  0 - OK
   **  1 - OK and need to send response stored in CANPSR message structure.
   ** -1 - Error occured.
   */
   int32_t s32_err = 0;
   
   STRU_CANDRV_MSG_T stru_candrv_msg;
   STRU_CANPSR_MSG_T stru_canpsr_msg;
   
   u8_my_msg_id++;
//   v_CANPRS_Build_Msg (&stru_canpsr_msg, 
//                       CANMSG_TYPE_DATA,
//                       pstru_canpsr_msg->u8_dest_addr,
//                       u8_my_msg_id,
//                       pstru_canpsr_msg->u8_cmd,
//                       pstru_canpsr_msg->u8_data_len,
//                       pstru_canpsr_msg->au8_data);
      
   /* Build and send command. */
   v_CANPRS_Msg_To_CANDRV_Msg(&stru_canpsr_msg, &stru_candrv_msg);
   s32_err = s32_CANDRV_Send_Msg(&stru_candrv_msg, CANPRS_PROC_CMD_WAIT);

   return (s32_err);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      static void v_CANPRS_Build_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
**                                     uint8_t                u8_msg_type,
**                                     uint8_t                u8_dest_addr,
**                                     uint8_t                u8_msg_id,
**                                     uint8_t                u8_cmd,
**                                     uint8_t                u8_data_len,
**                                     uint8_t               *pau8_data)
**
**   Arguments:
**      (out) pstru_canpsr_msg - Pointer to CANPSR message data structure to build.
**      (in)  u8_msg_type       - Message type to be used.
**      (in)  u8_dest_addr      - Destination address to send message.
**      (in)  u8_msg_id         - Message ID.
**      (in)  u8_cmd            - Command/Event ID.
**      (in)  u8_data_len       - Data length in CANPSR message.
**      (in)  pau8_data         - Pointer to data buffer to populate CANPSR message data.
**
**   Return:
**      N/A
**
**   Description:
**      Build CANPSR message from individual elements.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPRS_Build_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                               uint8_t                u8_msg_type,
                               uint8_t                u8_dest_addr,
                               uint8_t                u8_msg_id,
                               uint8_t                u8_cmd,
                               uint8_t                u8_data_len,
                               uint8_t               *pau8_data,
															 uint8_t 								u8_sensor_id,
															 E_CANMSG_ARGUMENT_TYPE e_arg_type)
{
   /* Message type: Data. */
   pstru_canpsr_msg->u8_msg_type = u8_msg_type;
   /* Destination address. */
   pstru_canpsr_msg->u8_dest_addr = u8_dest_addr;
   /* Source address = my address. */
   pstru_canpsr_msg->u8_src_addr = u8_my_addr;
   /* Message ID. */
   pstru_canpsr_msg->u8_msg_id = u8_msg_id;
   /* Command/Event code. */
   pstru_canpsr_msg->u8_cmd = u8_cmd;
   /* Data length. */
   pstru_canpsr_msg->u8_data_len = u8_data_len;
	
	pstru_canpsr_msg->e_arg_type = e_arg_type;
	pstru_canpsr_msg->u8_sensor_id = u8_sensor_id;
	
   /* Data content. */
   if ((u8_data_len != 0) && (pau8_data != NULL))
   {
      memcpy(pstru_canpsr_msg->au8_data, pau8_data, u8_data_len);
   }
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      static void v_CANPSR_Msg_From_CANDRV_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
**                                              STRU_CANDRV_MSG_T *pstru_candrv_msg)
**
**   Arguments:
**      (out) pstru_canpsr_msg - Pointer to CANPSR message data structure.
**      (in)  pstru_candrv_msg  - Pointer to CAN Driver message data structure.
**
**   Return:
**      N/A
**
**   Description:
**      Build CANPSR message information and data from CAN Driver message.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPSR_Msg_From_CANDRV_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                                         STRU_CANDRV_MSG_T *pstru_candrv_msg)
{
   /* Message type is 3-bit (b10 - b8) of CAN ID. */
   pstru_canpsr_msg->u8_msg_type = (uint8_t)((pstru_candrv_msg->u16_id & 0x0700) >> 8);
   /* Destination address is 8-bit (b7 - b0) of CAN ID. */
   pstru_canpsr_msg->u8_dest_addr = (uint8_t)(pstru_candrv_msg->u16_id & 0x00FF);
   /* Source address is first byte of data. */
   pstru_canpsr_msg->u8_src_addr = pstru_candrv_msg->au8_data[0];
   /* Message ID is second byte of data. */
   pstru_canpsr_msg->u8_msg_id = pstru_candrv_msg->au8_data[1];
   /* Sensor ID is the third byte of data. */
   pstru_canpsr_msg->u8_sensor_id = pstru_candrv_msg->au8_data[2];
   /* Command ID is the first 5 high bits of the fouth byte of data. */
   pstru_canpsr_msg->u8_cmd = (pstru_candrv_msg->au8_data[3] >> 3);
   /* Argument type is the last 3 bits of the fouth byte of data. */
   pstru_canpsr_msg->e_arg_type = (E_CANMSG_ARGUMENT_TYPE)(pstru_candrv_msg->au8_data[3] & 0x07);
   /* Data length = CAN data length - 3 (source id, message id and command id). */
   pstru_canpsr_msg->u8_data_len = pstru_candrv_msg->u8_len - 4;
   /* Command data. */
   memcpy(pstru_canpsr_msg->au8_data, &(pstru_candrv_msg->au8_data[4]), pstru_canpsr_msg->u8_data_len);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      static void v_CANPRS_Msg_To_CANDRV_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
**                                              STRU_CANDRV_MSG_T *pstru_candrv_msg)
**
**   Arguments:
**      (in)  pstru_canpsr_msg - Pointer to CANPSR message data structure.
**      (out) pstru_candrv_msg  - Pointer to CAN Driver message data structure.
**
**   Return:
**      N/A
**
**   Description:
**      Build CAN Driver message from CANPSR message information and data.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPRS_Msg_To_CANDRV_Msg (STRU_CANPSR_MSG_T *pstru_canpsr_msg,
                                       STRU_CANDRV_MSG_T *pstru_candrv_msg)
{
   if (pstru_canpsr_msg->u8_data_len > 4)
     pstru_canpsr_msg->u8_data_len = 4;
   /* Parse CAN message ID from message type, destination and source addresses. */
   pstru_candrv_msg->u16_id = (uint16_t)((pstru_canpsr_msg->u8_msg_type & 0x07) << 8);
   pstru_candrv_msg->u16_id |= (uint16_t)(pstru_canpsr_msg->u8_dest_addr);
	
   /* CAN data length = CANPSR data length + 3 bytes for CANPSR message ID and command ID. */
   pstru_candrv_msg->u8_len = pstru_canpsr_msg->u8_data_len + 8;
   
   /* Put CANPSR message ID, command and data into CAN data buffer. */
   pstru_candrv_msg->au8_data[0] = pstru_canpsr_msg->u8_src_addr;
   pstru_candrv_msg->au8_data[1] = pstru_canpsr_msg->u8_msg_id;
   pstru_candrv_msg->au8_data[2] = pstru_canpsr_msg->u8_sensor_id;
   pstru_candrv_msg->au8_data[3] = ((pstru_canpsr_msg->u8_cmd << 3) & 0xF8) | (pstru_canpsr_msg->e_arg_type % 0x07);
   memcpy(&(pstru_candrv_msg->au8_data[4]), pstru_canpsr_msg->au8_data, pstru_canpsr_msg->u8_data_len);
}

void v_CANPRS_Send_Command(E_CANMSG_CMD_ID e_cmdId, STRU_CANAPP_DATA_T *pstru_node_info)
{
	static STRU_CANPSR_MSG_T stru_canpsr_msg;
	static STRU_CANDRV_MSG_T stru_candrv_msg;
	
	v_CANPRS_Build_Msg(&stru_canpsr_msg, CANMSG_TYPE_SETTING, pstru_node_info->u8_node_addr, u8_my_msg_id++, e_cmdId,
											pstru_node_info->u8_data_len, pstru_node_info->au8_data, pstru_node_info->u8_sensor_type, pstru_node_info->e_data_type);
	
	v_CANPRS_Msg_To_CANDRV_Msg (&stru_canpsr_msg, &stru_candrv_msg);
	
	s32_CANDRV_Send_Msg(&stru_candrv_msg, 100);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
