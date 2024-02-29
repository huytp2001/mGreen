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
**   File Name   : can_app.c
**   Project     : MGreen Controller
**   Description : CAN APP functions.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#include "string.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "can_parser.h"
#include "can_api.h"
#include "can_app.h"
#include "firmware_config.h"
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#define CANAPP_CMD_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 16)
#define CANAPP_CMD_TASK_PRIO              (tskIDLE_PRIORITY + 2)

/* Wait period for CANAPP Data Handle Task. */
#define CANAPP_CMD_WAIT                   (100)

#define CANAPP_QUEUE_LEN                  16
#define CANAPP_MSG_SIZE                   (sizeof(STRU_CANAPP_DATA_T))

#define CANMSG_NUM_OF_ADDR                (CANMSG_ADDR_BROADCAST + 1)

/* Time-out of the configured address exists the non-volatile memory of device is 60s. */
#define CANAPP_ADDR_TIMEOUT               60000


#define CAN_APP_TASK_PRIORITY					( CANAPP_CMD_TASK_PRIO)
static StaticTask_t xCanApp_TaskBuffer;
static StackType_t  xCanAppStack[CANAPP_CMD_TASK_STACK_SIZE];


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static xQueueHandle canapp_queue;
static uint32_t au32_dev_addr[CANMSG_NUM_OF_ADDR];
static uint32_t au32_addr_timeout[CANMSG_NUM_OF_ADDR];


StaticQueue_t xRxCANAppQueueBuffer;
static uint8_t ucCANAppQueueStorage[CANAPP_QUEUE_LEN*CANAPP_MSG_SIZE];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANAPP_Data_Handler_Task (void *pvParameters);

int32_t s32_Set_Address_Management (uint8_t *pu8_data);

  
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void v_CANAPP_Init (void)
**
**   Arguments: n/a
**
**   Return: n/a
**
**   Description: Register callback functions for handling events.
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPP_Init (void)
{
   /* Init CANAPI. */
   v_CANAPI_Evt_Init();

   /* Init callback function for handling Load picked event. */

   /* CANAPP queue. */
   canapp_queue = xQueueCreateStatic(CANAPP_QUEUE_LEN, CANAPP_MSG_SIZE, &( ucCANAppQueueStorage[ 0 ]), &xRxCANAppQueueBuffer);

   /* Create task to process commands. */
   xTaskCreateStatic(v_CANAPP_Data_Handler_Task,
               "CANAPP Handle",
               CANAPP_CMD_TASK_STACK_SIZE,
               NULL,
               CANAPP_CMD_TASK_PRIO,
               xCanAppStack,
               &xCanApp_TaskBuffer);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: static void v_CANAPP_Data_Handle_Task (void *pvParameters)
**
**   Arguments: (in) pvParameters - Task input (not used).
**
**   Return: n/a
**
**   Description: Task to handle the Data returned from the CANAPP callback functions.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANAPP_Data_Handler_Task (void *pvParameters)
{
   static STRU_CANAPP_DATA_T stru_canapp_handle_data;
   uint8_t u8_idx;
   uint8_t u8_evt_id;
   
   vTaskSetApplicationTaskTag( NULL, ( void * ) TASK_TRACE_CAN_APP_ID );
   for (;;)
   {
      if (xQueueReceive(canapp_queue, &stru_canapp_handle_data, CANAPP_CMD_WAIT) == pdTRUE)
      {
         if (stru_canapp_handle_data.s32_res_status == CANAPI_RESULT_OK)
         {
            u8_evt_id = stru_canapp_handle_data.au8_data[0];
            switch (u8_evt_id)
            {
							default:
								 break;
            }
         }
         else
         {
         }
      }
      
      /* Free device id of device addresses. */
      for (u8_idx = CANMSG_ADDR_DYNAMIC; u8_idx <= (CANMSG_ADDR_WAIT_ADDRESSING - 1); u8_idx++)
      {
         if (au32_dev_addr[u8_idx] != 0)
         {
            au32_addr_timeout[u8_idx] += CANAPP_CMD_WAIT;
            /* Time-out 1 minute. */
            if (au32_addr_timeout[u8_idx] >= CANAPP_ADDR_TIMEOUT)
            {
               /* Free device id of device address. */
               au32_dev_addr[u8_idx] = 0;
               au32_addr_timeout[u8_idx] = 0;
            }
         }
      }

   }
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**
**   Arguments:
**
**   Return:
**
**   Description:
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPP_Weight_Change_Evt_Handler (int32_t s32_status, uint8_t *pu8_data, uint8_t u8_data_len)
{
   /* To do. */
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**
**   Arguments:
**
**   Return:
**
**   Description:
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPP_LDropped_Evt_Handler (int32_t s32_status, uint8_t *pu8_data, uint8_t u8_data_len)
{
   /* To do. */
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**
**   Arguments:
**
**   Return:
**
**   Description:
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPP_Broadcast_Query_Evt_Handler (int32_t s32_status, uint8_t *pu8_data, uint8_t u8_data_len)
{   
   /* To do. */
   STRU_CANAPP_DATA_T stru_canapp_data;
   
   /* Copy data and send to CANAPP data handler task. */
   stru_canapp_data.s32_res_status = s32_status;
//   stru_canapp_data.au8_data[0] = CANMSG_EVENT_RESPONSE_QUERY_DISCOVERY;
   /* The pu8_data includes: 1byte - device type, 4bytes - device id, 1byte - source address. */
   memcpy ((stru_canapp_data.au8_data + 1), pu8_data, u8_data_len);
   stru_canapp_data.u8_data_len = u8_data_len + 1;
   if (xQueueSend(canapp_queue, &stru_canapp_data, CANAPP_CMD_WAIT) == pdTRUE)
   {
   }
}


int32_t s32_Set_Address_Management (uint8_t *pu8_data)
{
   STRU_CANAPI_T stru_canapi;
   uint32_t u32_new_dev_id = 0;
   int32_t s32_err = 0;
   uint8_t u8_idx;
   uint8_t u8_src_addr;   

   u32_new_dev_id = *(uint32_t *)(pu8_data + 1);
   u8_src_addr = *(uint8_t *)(pu8_data + 5);
   /* Check the new device id is existing? */
   for (u8_idx = CANMSG_ADDR_DYNAMIC; u8_idx <= (CANMSG_ADDR_WAIT_ADDRESSING - 1); u8_idx++)
   {
      /* This device id is existing. */
      if (au32_dev_addr[u8_idx] == u32_new_dev_id)
      {
         if (u8_src_addr != CANMSG_ADDR_WAIT_ADDRESSING)
         {
            au32_addr_timeout[u8_idx] = 0;
            return (1);
         }
         else
         {
            /* Set the address for device. */
            break;
         }
      }
   }
   
   /* The new device id is not existing. And search the new device addr has not assigned yet. */
   if (u8_idx >= (CANMSG_ADDR_WAIT_ADDRESSING - 1))
   {
      for (u8_idx = CANMSG_ADDR_DYNAMIC; u8_idx <= (CANMSG_ADDR_WAIT_ADDRESSING - 1); u8_idx++)
      {
         /* The device address is not assigned (free). */
         if (au32_dev_addr[u8_idx] == 0)
         {
            au32_dev_addr[u8_idx] = u32_new_dev_id;
            break;
         }
      }
   }
   
   /* All device address are assigned. Ignore the new broadcast query and not set address.*/
   if (u8_idx >= (CANMSG_ADDR_WAIT_ADDRESSING - 1))
   {
      return (-1);
   }

   /* Assign a address for the new device id. */
   stru_canapi.u8_sensor_addr = 0x0f;
   stru_canapi.u8_data_len = 5;
   /* 4bytes dev id. */
   memcpy (stru_canapi.au8_data, (pu8_data + 1), (stru_canapi.u8_data_len - 1));
   /* 1byte device address. */
   stru_canapi.au8_data[4] = u8_idx;
   /* CANAPI Set address function. */
   s32_err = s32_CANAPI_Set_Addr (&stru_canapi);
   
   return (s32_err);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**
**   Arguments:
**
**   Return:
**
**   Description:
**
**   Notes:
**      
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPP_Response_Time_CB (int32_t s32_status, uint8_t *pu8_data, uint8_t u8_data_len)
{   
   STRU_CANAPP_DATA_T stru_canapp_data;
   
   /* Copy data and send to CANAPP data handler task. */
   stru_canapp_data.s32_res_status = s32_status;
//   stru_canapp_data.au8_data[0] = CANMSG_EVENT_SEND_DATE_TIME;
   /* The pu8_data includes: 4bytes - unix time, 1byte - src address. */
   memcpy ((stru_canapp_data.au8_data + 1), pu8_data, u8_data_len);
   stru_canapp_data.u8_data_len = u8_data_len + 1;
   if (xQueueSend(canapp_queue, &stru_canapp_data, CANAPP_CMD_WAIT) == pdTRUE)
   {
   }
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
