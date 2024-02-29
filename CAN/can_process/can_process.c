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
**   File Name   : can_proc.c
**   Project     : MGreen Controller
**   Description : Functions for CANPROC layer.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include <stdbool.h>
#include <string.h>

/* FreeRTOS. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "can_api.h"
#include "can_process.h"
#include "can_parser.h"
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
typedef struct
{
   CANAPI_Evt_CB f_evt_cb;
} STRU_CANPROC_LOAD_EVT_T;


#define CANPROC_CMD_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 4)
#define CANPROC_CMD_TASK_PRIO           (tskIDLE_PRIORITY + 2)

#define CANPROC_REQ_QUEUE_LEN           5
#define CANPROC_REQ_MSG_SIZE            (sizeof(STRU_CANPROC_REQUEST_T))

/* Wait period for CAN Command to be available. */
#define CANPROC_CMD_WAIT                (10)

/* Time-out for waiting the response is 5s. */
#define CANPROC_RESPONSE_TIMEOUT        5000



static StaticTask_t xCanProcessTaskBuffer;
static StackType_t  xanProcess_Stack[CANPROC_CMD_TASK_STACK_SIZE];


static StaticTask_t xCanRsp_TaskBuffer;
static StackType_t  xCanRsp_Stack[CANPROC_CMD_TASK_STACK_SIZE];


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
uint32_t u32_time_reported;
uint32_t u32_evt_id_reported;

STRU_CANPROC_LOAD_EVT_T stru_lpicked_evt;

static xSemaphoreHandle canproc_request_mutex;
static xQueueHandle canproc_req_queue;
static xQueueHandle canproc_res_queue;

static STRU_CANPROC_REQUEST_T stru_canproc_req;
static STRU_CANPROC_T stru_canproc_res;

static uint8_t u8_response_id = 0;
static bool b_return_cb = false;
   
StaticSemaphore_t xCanProcReqxMutexBuffer;   

StaticQueue_t xCANProcReqQueueBuffer;
uint8_t ucCANProcReqQueueStorage[CANPROC_REQ_QUEUE_LEN*CANPROC_REQ_MSG_SIZE];


StaticQueue_t xCANProcResQueueBuffer;
uint8_t ucCANProcResQueueStorage[CANPROC_REQ_QUEUE_LEN*CANPROC_REQ_MSG_SIZE];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPROC_Cmd_Task (void *pvParameters);
static void v_CANPROC_Response_Task (void *pvParameters);
static void v_CANPROC_Msg_To_CANPSR_Msg (STRU_CANPROC_T *pstru_canproc,
                                          STRU_CANPSR_MSG_T *pstru_canpsr_msg);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void v_CANPROC_Init (void)
**
**   Arguments: n/a
**
**   Return: n/a
**
**   Description: 
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANPROC_Init (void)
{
   /* CANPROC request queue. */
   canproc_req_queue = xQueueCreateStatic(CANPROC_REQ_QUEUE_LEN, CANPROC_REQ_MSG_SIZE, &( ucCANProcReqQueueStorage[ 0 ]), &xCANProcReqQueueBuffer);
   /* CANPROC response queue. */
   canproc_res_queue = xQueueCreateStatic(CANPROC_REQ_QUEUE_LEN, CANPROC_REQ_MSG_SIZE, &( ucCANProcResQueueStorage[ 0 ]), &xCANProcResQueueBuffer);

   /* Create mutex for CANPROC request. */
   canproc_request_mutex = xSemaphoreCreateMutexStatic(&xCanProcReqxMutexBuffer);

   /* Create task to process commands. */
   xTaskCreateStatic(v_CANPROC_Cmd_Task,
               "CANPROC Cmd",
               CANPROC_CMD_TASK_STACK_SIZE,
               NULL,
               CANPROC_CMD_TASK_PRIO,
               xanProcess_Stack,
               &xCanProcessTaskBuffer);

   /* Create task to process commands. */
   xTaskCreateStatic(v_CANPROC_Response_Task,
               "CANPROC Response",
               CANPROC_CMD_TASK_STACK_SIZE,
               NULL,
               CANPROC_CMD_TASK_PRIO,
               xCanRsp_Stack,
               &xCanRsp_TaskBuffer);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: static void v_CANPROC_Cmd_Task (void *pvParameters)
**
**   Arguments: (in) pvParameters - Task input (not used).
**
**   Return: n/a
**
**   Description: Task to handle the CAN commands and send them to CANPSR.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPROC_Cmd_Task (void *pvParameters)
{
   for (;;)
   {
      /* Receive CANPROC request queue from CANAPI. */
      if (xQueueReceive(canproc_req_queue, &stru_canproc_req, CANPROC_CMD_WAIT) == pdTRUE)
      {
         /* Convert CANPROC to CANPSR. */
         s32_CANPROC_Send_Request_To_CANPSR (&stru_canproc_req);
         
         switch (stru_canproc_req.stru_canproc.u8_evt_id)
         {
					 default:
							break;
         }
      }
   }
}
      

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: static void v_CANPROC_Response_Task (void *pvParameters)
**
**   Arguments: (in) pvParameters - Task input (not used).
**
**   Return: n/a
**
**   Description: Task to handle the CAN responses and return callback function.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPROC_Response_Task (void *pvParameters)
{
   for (;;)
   {
      /* Receive CANPROC response queue from CANPSR. */
      if (xQueueReceive(canproc_res_queue, &stru_canproc_res, CANPROC_CMD_WAIT) == pdTRUE)
      {
         /* Check the expected responding. */
         if ((u8_response_id == stru_canproc_res.u8_evt_id) && 
             (stru_canproc_req.stru_canproc.u8_sensor_addr == stru_canproc_res.u8_sensor_addr))
         {
            b_return_cb = true;
         }
      }
      
      /* Check time-out: 3s. */
      if (u8_response_id)
      {
         stru_canproc_req.u32_timeout += CANPROC_CMD_WAIT;
         if (stru_canproc_req.u32_timeout >= CANPROC_RESPONSE_TIMEOUT)
         {
            b_return_cb = true;
         }
      }
      
      /* Return callback function. */
      if (b_return_cb)
      {
         b_return_cb = false;
         if (stru_canproc_req.u32_timeout > CANPROC_RESPONSE_TIMEOUT)
         {
            memset (stru_canproc_res.au8_data, 0, sizeof(stru_canproc_res.au8_data));
            stru_canproc_res.au8_data[0] = stru_canproc_res.u8_sensor_addr;
            stru_canproc_res.u8_data_len = 1;
            /* Status: Time-out. */
            stru_canproc_req.f_evt_cb (CANAPI_RESULT_TIMEOUT, (uint8_t *)(stru_canproc_res.au8_data), stru_canproc_res.u8_data_len);
         }
         else
         {
            stru_canproc_res.au8_data[stru_canproc_res.u8_data_len] = stru_canproc_res.u8_sensor_addr;
            stru_canproc_res.u8_data_len += 1;
            /* Status: OK. */
            stru_canproc_req.f_evt_cb (CANAPI_RESULT_OK, (uint8_t *)(stru_canproc_res.au8_data), stru_canproc_res.u8_data_len);
         }
         
         u8_response_id = 0;
         stru_canproc_req.u32_timeout = 0;
         memset (&stru_canproc_req, 0, sizeof(stru_canproc_req));
         xSemaphoreGive(canproc_request_mutex);
      }
   }
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: int32_t s32_CANPROC_Send_Request (STRU_CANPROC_REQUEST_T *pstru_canproc_request)
**
**   Arguments: pstru_canproc_request
**
**   Return:  0/(-1): integer value.
**
**   Description: 
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANPROC_Send_Request (STRU_CANPROC_REQUEST_T *pstru_canproc_request)
{
   if (xSemaphoreTake(canproc_request_mutex, CANPROC_CMD_WAIT) == pdTRUE)
   {
      pstru_canproc_request->u32_timeout = 0;
      xQueueSend(canproc_req_queue, pstru_canproc_request, CANPROC_CMD_WAIT);
      
      return (0);
   }
   else
   {
      return (-1);
   }

}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: int32_t s32_CANPROC_Send_Response (STRU_CANPROC_T *pstru_canproc_res)
**
**   Arguments: pstru_canproc_res: pointer to CANPROC structure.
**
**   Return: 0/(-1): integer value.
**
**   Description: Send a CANPROC message to the CANPROC response queue.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANPROC_Send_Response (STRU_CANPROC_T *pstru_canproc_res)
{
   if (xQueueSend(canproc_res_queue, pstru_canproc_res, CANPROC_CMD_WAIT) == pdTRUE)
   {
      return (0);
   }
   else
   {
      return (-1);
   }
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void v_CAN_Reg_LPicked_Evt_CB (CAN_LPicked_Evt_CB v_callback_function)
**
**   Arguments: v_callback_function: callback function
**
**   Return: n/a
**
**   Description: Register the callback function for the load picked event.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CAN_Reg_LPicked_Evt_CB (CANAPI_Evt_CB v_callback_function)
{
   stru_lpicked_evt.f_evt_cb = v_callback_function;
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void v_CANPROC_LPicked_Evt (STRU_CANPSR_MSG_T *pstru_canpsr_msg)
**
**   Arguments: pstru_canpsr_msg: pointer to CANPSR message structure
**
**   Return: n/a
**
**   Description: The load picked callback function called when this event causes.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANPROC_LPicked_Evt (STRU_CANPSR_MSG_T *pstru_canpsr_msg)
{
   if (stru_lpicked_evt.f_evt_cb != NULL)
   {
      pstru_canpsr_msg->au8_data[pstru_canpsr_msg->u8_data_len] = pstru_canpsr_msg->u8_src_addr;
      pstru_canpsr_msg->u8_data_len++;
      stru_lpicked_evt.f_evt_cb (CANAPI_RESULT_OK, (uint8_t *)(pstru_canpsr_msg->au8_data), pstru_canpsr_msg->u8_data_len);
   }
}

int32_t s32_CANPROC_Send_Request_To_CANPSR (STRU_CANPROC_REQUEST_T *pstru_canproc_request)
{
   int32_t s32_err = 0;
   STRU_CANPSR_MSG_T stru_canpsr_msg;
   
   v_CANPROC_Msg_To_CANPSR_Msg (&pstru_canproc_request->stru_canproc, &stru_canpsr_msg);
   /* Call CANPSR. */
   s32_err = s32_CANPSR_Proc_Send_Cmd (&stru_canpsr_msg);
   
   return (s32_err);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: 
**    static void v_CANPROC_Msg_To_CANPSR_Msg (STRU_CANPROC_T *pstru_canproc,
**                                             STRU_CANPSR_MSG_T *pstru_canpsr_msg)
**
**   Arguments: pstru_canproc: pointer to CANPROC structure
**             pstru_canpsr_msg: pointer to CANPSR message structure
**
**   Return: n/a
**
**   Description: Convert CANPROC message to CANPSR message.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void v_CANPROC_Msg_To_CANPSR_Msg (STRU_CANPROC_T *pstru_canproc,
                                         STRU_CANPSR_MSG_T *pstru_canpsr_msg)
{
   pstru_canpsr_msg->u8_cmd = pstru_canproc->u8_evt_id;
   pstru_canpsr_msg->u8_dest_addr = pstru_canproc->u8_sensor_addr;
   pstru_canpsr_msg->u8_data_len = pstru_canproc->u8_data_len;
   if (pstru_canpsr_msg->u8_data_len)
   {
      memcpy(pstru_canpsr_msg->au8_data, pstru_canproc->au8_data, pstru_canpsr_msg->u8_data_len);
   }
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
