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
**   File Name   : can_api.c
**   Project     : MGreen Controller
**   Description : CAN API functions.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#include "string.h"

#include "can_api.h"
#include "can_process.h"
#include "can_parser.h"
#include "can_drv.h"


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


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

  
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void v_CANAPI_Evt_Init (void)
**
**   Arguments: n/a
**
**   Return: n/a
**
**   Description: Init CANPSR & CANPROC 
**                and call register callback functions for handling events.
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPI_Evt_Init (void)
{
	/* CAN configuration initialization. */
	v_CANDRV_Configuration_Init();

	v_CANPRS_Init();
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: 
**
**   Arguments: 
**
**   Return: s32_err: integer value
**
**   Description: 
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANAPI_Query_Discovery (uint8_t u8_dest_addr)
{
   int32_t s32_err = 0;
   STRU_CANPROC_REQUEST_T stru_canproc_query;
   
   stru_canproc_query.stru_canproc.u8_sensor_addr = u8_dest_addr;
//   stru_canproc_query.stru_canproc.u8_evt_id = CANMSG_CMD_QUERY_DISCOVERY;
   stru_canproc_query.stru_canproc.u8_data_len = 0;

   /* No response. */
   s32_err = s32_CANPROC_Send_Request_To_CANPSR (&stru_canproc_query);

   return (s32_err);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: 
**
**   Arguments: 
**
**   Return: s32_err: integer value
**
**   Description: 
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANAPI_Set_Addr (STRU_CANAPI_T *pstru_canapi)
{
   int32_t s32_err = 0;
   STRU_CANPROC_REQUEST_T stru_canproc_set_addr;
   
   /* Convert CANAPI to CANPROC. */
   stru_canproc_set_addr.stru_canproc.u8_sensor_addr = pstru_canapi->u8_sensor_addr;
   stru_canproc_set_addr.stru_canproc.u8_evt_id = CANMSG_CMD_SET_ADDR;
   stru_canproc_set_addr.stru_canproc.u8_data_len = pstru_canapi->u8_data_len;
   if (pstru_canapi->u8_data_len)
   {
      memcpy (stru_canproc_set_addr.stru_canproc.au8_data, pstru_canapi->au8_data, pstru_canapi->u8_data_len);
   }
   /* No response. */
   s32_CANPROC_Send_Request_To_CANPSR (&stru_canproc_set_addr);
   
   return (s32_err);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: 
**
**   Arguments: 
**
**   Return: s32_err: integer value
**
**   Description: 
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANAPI_Set_Date_Time (STRU_CANAPI_T *pstru_canapi)
{
   int32_t s32_err = 0;
   STRU_CANPROC_REQUEST_T stru_canproc_set_date;
   
   stru_canproc_set_date.stru_canproc.u8_sensor_addr = pstru_canapi->u8_sensor_addr;
   stru_canproc_set_date.stru_canproc.u8_evt_id = CANMSG_CMD_SET_DATE_TIME;
   stru_canproc_set_date.stru_canproc.u8_data_len = pstru_canapi->u8_data_len;
   if (pstru_canapi->u8_data_len)
   {
      memcpy (stru_canproc_set_date.stru_canproc.au8_data, pstru_canapi->au8_data, pstru_canapi->u8_data_len);
   }

   /* No response. */
   s32_err = s32_CANPROC_Send_Request_To_CANPSR (&stru_canproc_set_date);

   return (s32_err);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: 
**
**   Arguments: 
**
**   Return: s32_err: integer value
**
**   Description: 
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_Can_Speed (STRU_CANAPI_T *pstru_canapi)
{
   int32_t s32_err = 0;
   STRU_CANPROC_REQUEST_T stru_canproc_send_cmd;
   
   stru_canproc_send_cmd.stru_canproc.u8_sensor_addr = pstru_canapi->u8_sensor_addr;
   stru_canproc_send_cmd.stru_canproc.u8_evt_id = CANMSG_CMD_SET_CAN_SPEED;
   stru_canproc_send_cmd.stru_canproc.u8_data_len = pstru_canapi->u8_data_len;
   if (pstru_canapi->u8_data_len)
   {
      memcpy (stru_canproc_send_cmd.stru_canproc.au8_data, pstru_canapi->au8_data, pstru_canapi->u8_data_len);
   }

   /* No response. */
   s32_err = s32_CANPROC_Send_Request_To_CANPSR (&stru_canproc_send_cmd);

   return (s32_err);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
