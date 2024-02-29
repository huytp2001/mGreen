/*! @file can_mgreen.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "driverlib.h"
#include "can_mgreen.h"
#include "relay.h"

/*!
* static data declaration
*/
xQueueHandle q_can_mgreen_in;

static tCANMsgObject stru_can_rx;
static tCANMsgObject stru_can_tx;
static can_psr_t stru_can_psr_rx;
static can_psr_t stru_can_psr_tx;

static uint8_t u8_can_msg_id = 0;
static uint8_t au8_can_mgreen_data[CAN_MGREEN_DATA_LEN_TX_MAX];

static bool b_restart_can = false;
static bool b_can_bus_idle = false;

StaticQueue_t xCANProcReqQueueBuffer;
uint8_t ucCANProcReqQueueStorage[CAN_MGREEN_QUEUE_LEN*CAN_MGREEN_QUEUE_IN_SIZE];


/*!
* private function prototype
*/
static bool b_can_mgreen_send_msg (void);
static bool b_can_send_ACK(uint8_t addr);
static void v_can_mgreen_handler(void);
static void v_can_mgreen_rx_callback(tCANMsgObject * p_stru_can_handle);

portBASE_TYPE x_higher_prio_can_rx;
/*!
* public function bodies
*/  
void v_can_mgreen_hardware_init (void)
{
	SysCtlPeripheralEnable(CAN_MGREEN_RX_TX_PER);
	while(!(SysCtlPeripheralReady(CAN_MGREEN_RX_TX_PER)));

	GPIOPinConfigure(CAN_MGREEN_TX_AF);
	GPIOPinConfigure(CAN_MGREEN_RX_AF);	
	GPIOPinTypeCAN(CAN_MGREEN_TX_RX_PORT, CAN_MGREEN_TX_PIN|CAN_MGREEN_RX_PIN);
	
	SysCtlPeripheralEnable(CAN_MGREEN_PER);
	while(!(SysCtlPeripheralReady(CAN_MGREEN_PER)));	
	CANInit(CAN_MGREEN_BASE);
	CANBitRateSet(CAN_MGREEN_BASE, SYSTEM_CLOCK_GET(), CAN_MGREEN_BIT_RATE);
	CANIntRegister(CAN_MGREEN_BASE, &v_can_mgreen_handler);
	CANIntEnable(CAN_MGREEN_BASE, CAN_INT_MASTER|CAN_INT_ERROR|CAN_INT_STATUS);
	IntEnable(CAN_MGREEN_INT);
	CANEnable(CAN_MGREEN_BASE);

	stru_can_tx.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
	
	stru_can_rx.ui32MsgID = CAN0RXID;
	stru_can_rx.ui32MsgIDMask = 0;
	stru_can_rx.ui32Flags = MSG_OBJ_RX_INT_ENABLE|MSG_OBJ_USE_ID_FILTER;
	stru_can_rx.ui32MsgLen = CAN_MGREEN_DATA_LEN_RX;
	CANMessageSet(CAN_MGREEN_BASE, RXOBJECT, &stru_can_rx, MSG_OBJ_TYPE_RX);
}
void v_can_mgreen_hardware_deinit (void)
{
	CANDisable(CAN_MGREEN_BASE);
	SysCtlPeripheralDisable(CAN_MGREEN_PER);
	GPIOPinTypeGPIOInput(CAN_MGREEN_TX_RX_PORT, CAN_MGREEN_RX_PIN|CAN_MGREEN_TX_PIN);
	vTaskDelay(10);
}
void v_can_mgreen_init (void)
{
	q_can_mgreen_in = xQueueCreateStatic(CAN_MGREEN_QUEUE_LEN, CAN_MGREEN_QUEUE_IN_SIZE, &( ucCANProcReqQueueStorage[ 0 ]), &xCANProcReqQueueBuffer);
}

void v_can_mgreen_process (void)
{
	//TODO: Get and send data
	if(uxQueueMessagesWaiting(q_can_mgreen_in))
	{
		if(xQueueReceive(q_can_mgreen_in, &stru_can_psr_tx, (portTickType)(10 / portTICK_RATE_MS)) == pdPASS)
		{	
			b_can_mgreen_send_msg();
		}
	}
}

uint8_t u8_can_mgreen_get_msg_id (void)
{
	u8_can_msg_id++;
	if(u8_can_msg_id == 0)
		u8_can_msg_id = 1;
	return u8_can_msg_id;
}

bool b_write_msg_to_can_mgreen (can_psr_t*  stru_can_psr_msg)
{
	if(xQueueSend(q_can_mgreen_in, stru_can_psr_msg, (portTickType)(10 / portTICK_RATE_MS)) == pdTRUE)
	{
			return true;
	}
	return false;
}

/**
* @private function bodies
*/
static bool b_can_mgreen_send_msg (void)
{
	au8_can_mgreen_data[0] = stru_can_psr_tx.u8_src_addr;
	au8_can_mgreen_data[1] = stru_can_psr_tx.u8_msg_id;
	au8_can_mgreen_data[2] = stru_can_psr_tx.u8_sensor_id;
	au8_can_mgreen_data[3] = ((stru_can_psr_tx.e_cmd)<<3) | ((stru_can_psr_tx.e_arg_type)&0x07);
	
	uint8_t u8_i = 0;
	for (u8_i = 0; u8_i < stru_can_psr_tx.u8_data_len; u8_i++)
	{
		au8_can_mgreen_data[4 + u8_i] = stru_can_psr_tx.au8_data[u8_i];
	}
	
	stru_can_tx.ui32MsgID = (stru_can_psr_tx.u8_msg_type<<8) | stru_can_psr_tx.u8_dest_addr;
	if(stru_can_tx.ui32MsgID == 0x830)
		stru_can_tx.ui32MsgID = 0x730;
	stru_can_tx.ui32MsgLen = 4 + stru_can_psr_tx.u8_data_len;
	stru_can_tx.pui8MsgData = au8_can_mgreen_data;	
	
	if (b_restart_can)
	{
		v_can_mgreen_hardware_deinit();
		v_can_mgreen_hardware_init();
		b_restart_can = false;		
	}
	CANMessageSet(CAN_MGREEN_BASE, TXOBJECT, &stru_can_tx, MSG_OBJ_TYPE_TX);
	
	b_can_bus_idle = false;
	for(u8_i = 200; u8_i > 0; u8_i--)
	{
		if(b_can_bus_idle == true)
		{
			return true;
		}
		vTaskDelay(10);
	}
	
	v_can_mgreen_hardware_deinit();
	v_can_mgreen_hardware_init();
	return false;	
}

static bool b_can_send_ACK(uint8_t addr)
{
	stru_can_psr_tx.u8_msg_type = MSG_TYPE_SETTING;
	stru_can_psr_tx.u8_dest_addr = addr;
	stru_can_psr_tx.u8_src_addr = 0;
	stru_can_psr_tx.e_cmd = CANMSG_CMD_SET_ADDR;
	stru_can_psr_tx.u8_sensor_id = 0;
	stru_can_psr_tx.e_arg_type = CANMSG_ARGUMENT_TYPE_INVALID ;
	stru_can_psr_tx.u8_msg_id = u8_can_msg_id++;

	au8_can_mgreen_data[0] = stru_can_psr_tx.u8_src_addr;
	au8_can_mgreen_data[1] = stru_can_psr_tx.u8_msg_id;
	au8_can_mgreen_data[2] = stru_can_psr_tx.u8_sensor_id;
	au8_can_mgreen_data[3] = ((stru_can_psr_tx.e_cmd)<<3) | ((stru_can_psr_tx.e_arg_type)&0x07);

	stru_can_tx.ui32MsgID = (stru_can_psr_tx.u8_msg_type<<8) | stru_can_psr_tx.u8_dest_addr;
	stru_can_tx.ui32MsgLen = 4;
	stru_can_tx.pui8MsgData = au8_can_mgreen_data;
	CANMessageSet(CAN_MGREEN_BASE, TXOBJECT, &stru_can_tx, MSG_OBJ_TYPE_TX);
	b_can_bus_idle = false;
	
	uint8_t u8_i = 200;
	for(u8_i = 200; u8_i > 0; u8_i--)
	{
		if(b_can_bus_idle == true)
		{
			return true;
		}
		vTaskDelay(10);
	}
	return false;
}

static void v_can_mgreen_handler(void)
{
	uint32_t u32_status;
	volatile tCANMsgObject tCANObj_Rx;
	static uint8_t au8_can_data[8];

	u32_status = CANIntStatus(CAN_MGREEN_BASE, CAN_INT_STS_CAUSE);
	switch(u32_status)
	{
		case CAN_INT_INTID_STATUS:
		{
			u32_status = CANStatusGet(CAN_MGREEN_BASE, CAN_STS_CONTROL);
			if ((u32_status & CAN_STATUS_BUS_OFF) || (u32_status & CAN_STATUS_EWARN))
			{
				b_restart_can = true;
			}
		}
		break;
	 case TXOBJECT:
	 {
		 CANIntClear(CAN_MGREEN_BASE, u32_status);
	 }
	 break;
	 case RXOBJECT:
	 default:
	 {
		 CANIntClear(CAN_MGREEN_BASE, u32_status);
		 memset(au8_can_data, 0, sizeof(au8_can_data));
		 tCANObj_Rx.pui8MsgData = au8_can_data;
		 CANMessageGet(CAN_MGREEN_BASE, RXOBJECT, (tCANMsgObject *)&tCANObj_Rx, false);
		 v_can_mgreen_rx_callback((tCANMsgObject *)&tCANObj_Rx);
	 }
	 break;
	}
	portEND_SWITCHING_ISR(x_higher_prio_can_rx);
}

static void v_can_mgreen_rx_callback(tCANMsgObject * p_stru_can_handle)
{
	stru_can_psr_rx.u8_src_addr = p_stru_can_handle->pui8MsgData[0];
	stru_can_psr_rx.u8_msg_id = p_stru_can_handle->pui8MsgData[1];
	stru_can_psr_rx.u8_sensor_id = p_stru_can_handle->pui8MsgData[2];
	stru_can_psr_rx.e_cmd = (p_stru_can_handle->pui8MsgData[3])>>3;
	stru_can_psr_rx.e_arg_type = (p_stru_can_handle->pui8MsgData[3]) &0x07;
	stru_can_psr_rx.u8_data_len = p_stru_can_handle->ui32MsgLen - 4;
	
	uint8_t i = 0;
	for(i=0; i<stru_can_psr_rx.u8_data_len; i++)
	{
		stru_can_psr_rx.au8_data[i] = p_stru_can_handle->pui8MsgData[i+4];
	}
	
	switch(stru_can_psr_rx.e_cmd)
	{
		case CANMSG_CMD_DISCOVERY:
		{
			b_can_send_ACK(stru_can_psr_rx.u8_src_addr);
		}
		break;					
		default: break;
	}
	
	switch(stru_can_psr_rx.u8_sensor_id)
	{
		case CANMSG_TYPE_RELAY:
		{
		}
		break;
		default:
		break;
	}
	b_can_bus_idle = true;
}
