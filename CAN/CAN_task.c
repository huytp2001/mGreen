/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file CAN_task.c
 * @author Danh Pham
 * @date 10 Nov 2020
 * @version: 1.0.0
 * @brief Contains functions and tasks used to manage CAN bus.
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 
 #include "FreeRTOS.h"
 #include "task.h"
 #include "queue.h"
 
 #include "can_api.h"
 #include "can_parser.h" 
 #include "can_process.h"
 #include "CAN_task.h"
 #include "wdt.h"
 
/*!
* @def CAN_MAN_QUEUE_LEN 
* Maximum len of CAN's queues
*/
#define CAN_MAN_QUEUE_LEN                 10

/*!
* @def CAN_MAN_MSG_SIZE
* Size of CAN's message
*/
#define CAN_MAN_MSG_SIZE               (sizeof(STRU_CANAPP_DATA_T))

/*!
* @def CAN_IN_TASK_STACK_SIZE
* Size of CAN in task
*/
#define CAN_IN_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE * 1)

/*!
* @def CAN_IN_TASK_PRIORITY
* Priority of CAN in task
*/
#define CAN_IN_TASK_PRIORITY             (tskIDLE_PRIORITY + 1)

/*!
* @def CAN_IN_TASK_DELAY
*/
#define CAN_IN_TASK_DELAY                (portTickType)(10/ portTICK_RATE_MS)

/*!
* @def CAN_OUT_TASK_STACK_SIZE
* Size of CAN out task
*/
#define CAN_OUT_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE * 4)

/*!
* @def CAN_OUT_TASK_PRIORITY
* Priority of CAN in task
*/
#define CAN_OUT_TASK_PRIORITY             (tskIDLE_PRIORITY + 2)

/*!
* @def CAN_OUT_TASK_DELAY
*/
#define CAN_OUT_TASK_DELAY                (portTickType)(50/ portTICK_RATE_MS)

/*!
* @enum E_CAN_OUT_HANDLE_STEP
* @brief Steps of send data via CAN bus process
*/
typedef enum
{
	CAN_STEP_IDLE = 0,				/**< No message to send */
	CAN_STEP_SEND,						/**< Send message */
	CAN_STEP_CHECK_ACK,				/**< Check ACK after a message sent */
	CAN_STEP_WAITING_REACT,		/**< Check reaction of relay board (optional) */
	CAN_STEP_MAX,
}E_CAN_OUT_HANDLE_STEP;
/*!
*	Variables declare
*/

static xQueueHandle xqueue_can_in_man;	/**<CAN input queue */
static xQueueHandle xqueue_can_out_man;	/**<CAN output queue */
static xQueueHandle xqueue_can_ack;			/**<CAN ACK queue */

static StaticQueue_t x_can_in_storage;
static uint8_t uc_can_in_queue_buffer[CAN_MAN_QUEUE_LEN
																		*CAN_MAN_MSG_SIZE];

static StaticQueue_t x_can_out_storage;
static uint8_t uc_can_out_queue_buffer[CAN_MAN_QUEUE_LEN
																		*CAN_MAN_MSG_SIZE];

static StaticQueue_t x_can_ack_storage;
static uint8_t uc_can_ack_queue_buffer[CAN_MAN_QUEUE_LEN*sizeof(uint8_t)];

static StaticTask_t xCanInTaskBuffer;
static StackType_t  xCanInStack[CAN_IN_TASK_STACK_SIZE];

static StaticTask_t xCanOutTaskBuffer;
static StackType_t  xCanOutStack[CAN_OUT_TASK_STACK_SIZE];

/*!
*	Private functions prototype
*/
static void v_can_in_task(void *pvParameters);
static void v_can_out_task(void *pvParameters);

/*!
*	Public functions
*/

/*!
*	@fn v_can_task_init(void)
* @brief Init tasks for CAN bus
* @param None
* @return None
*/

void v_can_task_init(void)
{
	/* CAN Hardware Init */
	HAL_CAN_MspInit();
	/* CAN Process Event Init */
	v_CANAPI_Evt_Init();
	/* Queues init */
	xqueue_can_in_man = xQueueCreateStatic(CAN_MAN_QUEUE_LEN,
																				CAN_MAN_MSG_SIZE,
																				&(uc_can_in_queue_buffer[0]), 
																				&x_can_in_storage);
  xqueue_can_out_man = xQueueCreateStatic(CAN_MAN_QUEUE_LEN,
																					CAN_MAN_MSG_SIZE, 
																					&(uc_can_out_queue_buffer[0]), 
																					&x_can_out_storage);
	xqueue_can_ack = xQueueCreateStatic(CAN_MAN_QUEUE_LEN, 
																			sizeof(uint8_t), 
																			&(uc_can_ack_queue_buffer[0]), 
																			&x_can_ack_storage); 
	/* Tasks init */
	xTaskCreateStatic(v_can_in_task,
               "can_in_task",
               CAN_IN_TASK_STACK_SIZE,
               NULL,
               CAN_IN_TASK_PRIORITY,
               xCanInStack,
			   &xCanInTaskBuffer);
	// vTaskSuspend(handle);
	
   xTaskCreateStatic(v_can_out_task,
               "can_out_task",
               CAN_OUT_TASK_STACK_SIZE,
               NULL,
               CAN_OUT_TASK_PRIORITY,
               xCanOutStack,
			   &xCanOutTaskBuffer);
}

/*!
*	@fn v_can_in_send_queue(STRU_CANAPP_DATA_T *pstru_in_sm_data)
* @brief Send internal message to process.
* @param[in] pstru_in_sm_data Data need to be sent
* @return None
*/
void v_can_in_send_queue(STRU_CANAPP_DATA_T *pstru_in_sm_data)
{
   if (xqueue_can_in_man != NULL)
   {
		 xQueueSend(xqueue_can_in_man, pstru_in_sm_data, 10);
   }
}

/*!
*	@fn v_can_out_send_queue(STRU_CANAPP_DATA_T *pstru_in_sm_data)
* @brief Send message to CAN bus.
* @param[in] pstru_out_data Data need to be sent
* @return None
*/
void v_can_out_send_queue(STRU_CANAPP_DATA_T *pstru_out_data)
{
	if (xqueue_can_out_man != NULL)
	{
		xQueueSend(xqueue_can_out_man, pstru_out_data, 100);
	}
}

/*!
*	Private functions
*/

/*!
*	@fn v_can_in_task(void *pvParameters)
* @brief Receive ACK message from external board
* @param None
* @return None
*/
static void v_can_in_task(void *pvParameters)
{
	static STRU_CANAPP_DATA_T stru_in_sm_data;
	static uint8_t u8_can_in_task_id = 0;
	while(b_wdt_reg_new_task("can_in_task", &u8_can_in_task_id) != true){} 
 for( ; ; )
 {  
		//reload watchdog timer
		b_wdt_task_reload_counter(u8_can_in_task_id);
	 //process
		if (xQueueReceive(xqueue_can_in_man, &stru_in_sm_data, 10) == pdTRUE)
		{
			if(xqueue_can_ack != NULL)
			{
				xQueueSend(xqueue_can_ack, &(stru_in_sm_data.u8_node_addr), 0);
			}
		}
		vTaskDelay(10);
 }
}

/*!
* @fn v_Out_Sensor_Man_Task (void *pvParameters)
* @brief Send messages to external board
* @param None
* @return None
*/
static void v_can_out_task(void *pvParameters)
{
	static STRU_CANAPP_DATA_T stru_out_sm_data;
	static E_CAN_OUT_HANDLE_STEP e_can_out_step = CAN_STEP_IDLE;
	static uint8_t u8_retry;
	static uint8_t u8_can_out_task_id = 0;
	//get task id
	while(b_wdt_reg_new_task("can_out_task", &u8_can_out_task_id) != true){}
	for( ; ; )
	{
		//Reload watchdog timer
		b_wdt_task_reload_counter(u8_can_out_task_id);
		//Send message process
		switch(e_can_out_step)
		{
		 case CAN_STEP_IDLE:
		 {
			if (xQueueReceive(xqueue_can_out_man, &stru_out_sm_data, 10) == pdTRUE)
			{
				u8_retry = 0;
				e_can_out_step = CAN_STEP_SEND;
			}
		 }
		 break;
		 case CAN_STEP_SEND:
		 {
			 v_CANPRS_Send_Command((E_CANMSG_CMD_ID)stru_out_sm_data.u8_evt_id, &stru_out_sm_data);
			 e_can_out_step = CAN_STEP_CHECK_ACK;
		 }
		 break;
		 case CAN_STEP_CHECK_ACK:
		 {
			 uint8_t u8_ack_addr = 0;
			 if(xQueueReceive(xqueue_can_ack, &u8_ack_addr, 300))
			 {
				 if(u8_ack_addr == stru_out_sm_data.u8_node_addr)
				 {
					 if(stru_out_sm_data.u8_sensor_type == CANMSG_SENSOR_TYPE_RELAY)
						 e_can_out_step = CAN_STEP_WAITING_REACT;
					 else
						 e_can_out_step = CAN_STEP_IDLE;
				 }
			 }
			 else
			 {
				 u8_retry++;
				 if(u8_retry > 3)
				 {
					 e_can_out_step = CAN_STEP_IDLE;
				 }
				 else
				 {
					 e_can_out_step = CAN_STEP_SEND;
				 }
			 }
		 }
		 break;
		 case CAN_STEP_WAITING_REACT:
		 {
			 
		 }
		 break;
		 default: break;
	}

	vTaskDelay(50);
	}
}
