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
**   File Name   :
**   Project     :
**   Description :
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "driverlib.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "can_drv.h"

//#define TEST_MODE

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Block time for waiting CAN TX mutex: 1second. */
#define CAN_TX_MUTEX_BLOCK_TIME           (portTickType)(1000 / portTICK_RATE_MS)

/* Maximum items in received CAN queue. */
#define CAN_RX_QUEUE_LEN                  (10)
/* Size in bytes of each CAN RX queue item. */
#define CAN_RX_MSG_SIZE                   (sizeof(STRU_CANDRV_MSG_T))

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                          EXTERN VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/* Mutex to guard CAN TX function. */
static xSemaphoreHandle candrv_tx_mutex;
StaticSemaphore_t xCanDrvTxMutexBuffer;

/* Queue for CAN RX messages. */
static xQueueHandle candrv_rx_msg_queue;

StaticQueue_t xRxMsgQueueBuffer;
uint8_t ucRxMsgQueueStorage[CAN_RX_QUEUE_LEN*CAN_RX_MSG_SIZE];

//Declare CAN-Message variables
tCANMsgObject CANMsgMstTx, CANMsgMstRx;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static void CANMasterIntHandler(void);

  
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/**
  * @brief CAN MSP Initialization 
  *        This function configures the hardware resources used in this example: 
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration  
  *           - NVIC configuration for DMA interrupt request enable
  * @param hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_MspInit(void)
{
	//Config CAN
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

	MAP_GPIOPinConfigure(GPIO_PB0_CAN1RX);
	MAP_GPIOPinConfigure(GPIO_PB1_CAN1TX);
	MAP_GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);
	MAP_CANInit(CAN1_BASE);
	MAP_CANBitRateSet(CAN1_BASE, SYSTEM_CLOCK_GET(), 125000);
#ifdef TEST_MODE
	HWREG(CAN1_BASE + CAN_O_CTL) |= CAN_CTL_TEST;
	HWREG(CAN1_BASE + CAN_O_TST) |= CAN_TST_LBACK;
#endif
	CANIntEnable(CAN1_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
	CANIntRegister(CAN1_BASE, &CANMasterIntHandler);
	IntPrioritySet(INT_CAN1, (5<<5) );
	IntEnable(INT_CAN1);
	CANEnable(CAN1_BASE);
}


/**
  * @brief CAN MSP De-Initialization 
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO to their default state
  * @param hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_MspDeInit(void)
{
	CANDisable(CAN1_BASE);
}


static void v_CANDRV_Configuration (void)
{
	/*##-2- Configure the CAN Filter ###########################################*/
	//
	// Initialize a message object to be used for receiving CAN messages with
	// any CAN ID.  In order to receive any CAN ID, the ID and mask must both
	// be set to 0, and the ID filter enabled.
	//
	CANMsgMstRx.ui32MsgID = 0x000;
	CANMsgMstRx.ui32MsgIDMask = 0x000;
	CANMsgMstRx.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
	CANMsgMstRx.ui32MsgLen = 8;
	CANMessageSet(CAN1_BASE, 0x000, &CANMsgMstRx, MSG_OBJ_TYPE_RX);

   /*##-3- Configure Transmission process #####################################*/
   CANMsgMstTx.ui32MsgID = 0x321;
   CANMsgMstTx.ui32Flags = 0x000;;
   CANMsgMstTx.ui32MsgIDMask = 0x000;
   CANMsgMstTx.ui32MsgLen = 2;
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      void v_CANDRV_Configuration_Init (void)
**
**   Arguments:
**      N/A
**
**   Return:
**      N/A
**
**   Description:
**      Initialize CAN hardware, GPIOs and interrupt.
**      Create required mutex and queue.
**
**   Notes:
**      Must be called before using any other CAN Driver functions.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANDRV_Configuration_Init (void)
{
  /* -1- Configure the CAN peripheral. */
   v_CANDRV_Configuration();

   /* Create mutex for CAN TX function. */
   candrv_tx_mutex = xSemaphoreCreateMutexStatic(&xCanDrvTxMutexBuffer);
   /* CAN RX queue. */
   candrv_rx_msg_queue = xQueueCreateStatic(CAN_RX_QUEUE_LEN, CAN_RX_MSG_SIZE, &( ucRxMsgQueueStorage[ 0 ]), &xRxMsgQueueBuffer);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      int32_t s32_CANDRV_Send_Msg (STRU_CANDRV_MSG_T *pstru_candrv_msg)
**
**   Arguments:
**      (in) pstru_candrv_msg  - Pointer to CAN Driver Tx message.
**      (in) s32_block_time_ms - Block time in ms to wait for CAN resource available.
**
**   Return:
**      0 if message sent OK.
**     -1 if error occured.
**
**   Description:
**      Send message to CAN bus.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANDRV_Send_Msg (STRU_CANDRV_MSG_T *pstru_candrv_msg, int32_t s32_block_time_ms)
{
   portTickType   u32_block_ticks;

   if (s32_block_time_ms < 0)
   {
      u32_block_ticks = portMAX_DELAY;
   }
   else
   {
      u32_block_ticks = (portTickType)(s32_block_time_ms / portTICK_RATE_MS);
   }
   
   /*
   ** Check if we can obtain the CAN TX resource.
   */
   if (xSemaphoreTake(candrv_tx_mutex, u32_block_ticks) == pdTRUE)
   {
		static uint8_t msg_obj=1;		 
		/* Setup CAN message structure. */
		CANMsgMstTx.ui32MsgID = pstru_candrv_msg->u16_id;
		CANMsgMstTx.ui32MsgLen = pstru_candrv_msg->u8_len;
		CANMsgMstTx.pui8MsgData = pstru_candrv_msg->au8_data;
		CANMessageSet(CAN1_BASE, msg_obj, &CANMsgMstTx, MSG_OBJ_TYPE_TX);
		 msg_obj++;
		 if (msg_obj == 32)
			 msg_obj = 1;
		/* We have finished using CAN resource, release the mutex. */
		xSemaphoreGive(candrv_tx_mutex);
		/* Return CAN TX status. */
		return (0);
   }
   /* Can't get the mutex, return error. */
   else
   {
      return (-1);
   }
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function:
**      int32_t s32_CANDRV_Receive_Msg (STRU_CANDRV_MSG_T *pstru_candrv_msg, int32_t s32_block_time_ms)
**
**   Arguments:
**      (out) pstru_candrv_msg - Pointer to CAN RX message.
**      (in) s32_block_time_ms - Block time to wait for CAN RX message in milliseconds.
**
**   Return:
**       0 if message was received OK.
**      -1 if error occured or timeout
**
**   Description:
**      Receive message from CAN bus.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_CANDRV_Receive_Msg (STRU_CANDRV_MSG_T *pstru_candrv_msg, int32_t s32_block_time_ms)
{
   portTickType  u32_block_ticks;
   portBASE_TYPE s32_result;

   if (s32_block_time_ms < 0)
   {
      u32_block_ticks = portMAX_DELAY;
   }
   else
   {
      u32_block_ticks = (portTickType)(s32_block_time_ms / portTICK_RATE_MS);
   }
   
   if (xQueueReceive(candrv_rx_msg_queue,
                     pstru_candrv_msg,
                     u32_block_ticks) == pdTRUE)
   {
      s32_result = 0;
   }
   else
   {
      s32_result = -1;
   }
   
   return (s32_result);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* pstru_can_handle)
**
**   Arguments: pstru_can_handle: pointer to a CAN_HandleTypeDef structure that 
**                   contains the configuration information for the specified CAN.
**
**   Return:
**      N/A
**
**   Description: Transmission complete callback in non blocking mode.
**      Interrupt service routine to receive CAN messages.
**
**   Notes:
**      
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void HAL_CAN_RxCpltCallback(tCANMsgObject *pstru_can_handle)
{
   uint8_t u8_idx;

   /* CAN RX message.  */
   STRU_CANDRV_MSG_T stru_candrv_msg;
   /* Flag to indicate if we need to switch to higher priority task. */
   portBASE_TYPE x_higher_prio_task;

   /* Init to false - no higher priority task switch. */
   x_higher_prio_task = pdFALSE;

   /* Put message into CAN Driver message structure. */
   stru_candrv_msg.u16_id = pstru_can_handle->ui32MsgID;
   stru_candrv_msg.u8_len = pstru_can_handle->ui32MsgLen;
   for (u8_idx = 0; u8_idx <= stru_candrv_msg.u8_len; u8_idx++)
   {
      stru_candrv_msg.au8_data[u8_idx] = (uint8_t)(pstru_can_handle->pui8MsgData[u8_idx]);
   }
   /* Send CAN RX message to queue. */
   xQueueSendFromISR(candrv_rx_msg_queue, &stru_candrv_msg, &x_higher_prio_task);
  
   /* We can switch context if necessary. */
   portEND_SWITCHING_ISR(x_higher_prio_task);
}

/*******************************************************************
 * CAN interrupt service routine
 ******************************************************************/
static void CANMasterIntHandler(void)
{
    uint32_t ui32Status;
	volatile tCANMsgObject tCANObj_Rx;
	static uint8_t au8_can_data[8];
	
    ui32Status = CANIntStatus(CAN1_BASE, CAN_INT_STS_CAUSE);
	
    switch (ui32Status)
    {
    	case CAN_INT_INTID_STATUS:		//CAN Error
    		ui32Status = CANStatusGet(CAN1_BASE, CAN_STS_CONTROL);
			break;
    	default:
    		CANIntClear(CAN1_BASE, ui32Status);
				memset(au8_can_data, 0, sizeof(au8_can_data));
				tCANObj_Rx.pui8MsgData = au8_can_data;
				CANMessageGet(CAN1_BASE, ui32Status, (tCANMsgObject *)&tCANObj_Rx, 0);
				HAL_CAN_RxCpltCallback((tCANMsgObject *)&tCANObj_Rx);
			break;
    }
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
