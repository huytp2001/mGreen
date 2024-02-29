/*! @file can_mgreen.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/
#ifndef _CAN_MGREEN_H
#define _CAN_MGREEN_H
#ifdef __cplusplus
extern “C” {
#endif
#include "driverlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "config.h"

#ifdef HARDWARE_V5
#define CAN_MGREEN_PER											SYSCTL_PERIPH_CAN1
#define CAN_MGREEN_BASE											CAN1_BASE
#define CAN_MGREEN_BIT_RATE									125000
#define CAN_MGREEN_RX_TX_PER								SYSCTL_PERIPH_GPIOB
#define CAN_MGREEN_TX_RX_PORT								GPIO_PORTB_BASE
#define CAN_MGREEN_TX_PIN										GPIO_PIN_1
#define CAN_MGREEN_RX_PIN										GPIO_PIN_0
#define CAN_MGREEN_TX_AF										GPIO_PB1_CAN1TX
#define CAN_MGREEN_RX_AF										GPIO_PB0_CAN1RX
#define CAN_MGREEN_INT											INT_CAN1
#endif

#ifdef HARDWARE_V5_1
#define CAN_MGREEN_PER											SYSCTL_PERIPH_CAN0
#define CAN_MGREEN_BASE											CAN0_BASE
#define CAN_MGREEN_BIT_RATE									125000
#define CAN_MGREEN_RX_TX_PER								SYSCTL_PERIPH_GPIOA
#define CAN_MGREEN_TX_RX_PORT								GPIO_PORTA_BASE
#define CAN_MGREEN_TX_PIN										GPIO_PIN_1
#define CAN_MGREEN_RX_PIN										GPIO_PIN_0
#define CAN_MGREEN_TX_AF										GPIO_PA1_CAN0TX
#define CAN_MGREEN_RX_AF										GPIO_PA0_CAN0RX
#define CAN_MGREEN_INT											INT_CAN0
#endif

#define CAN_MGREEN_DATA_LEN_TX_MAX					7
#define CAN_MGREEN_DATA_LEN_RX							6
#define CAN_MGREEN_QUEUE_LEN								8
#define CAN_MGREEN_QUEUE_IN_SIZE						sizeof(can_psr_t)

//*****************************************************************************
//
// Message Identifiers and Objects
// RXID is set to 0 so all messages are received
//
//*****************************************************************************
#define CAN0RXID                			0
#define RXOBJECT                			2
#define CAN0TXID                			1
#define TXOBJECT                			1
#define CANMSG_TYPE_RELAY	 						0x30

#define CANMSG_ADDR_PROXIMITY					0x10
#define CANMSG_ADDR_RELAY 						0x30


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
	CANMSG_CMD_CALIB_POINT_6			= 10,
	CANMSG_CMD_SET_CAN_SPEED			= 11,
	CANMSG_CMD_DIAGNOSTIC_MODE		=	12,
	CANMSG_CMD_DEBOUNCE						= 13,
	CANMSG_CMD_CHANGE							= 14,
	CANMSG_CMD_REQUEST_DATA				= 15,
//	#warning add relay cmd
	CANMSG_CMD_RELAY							= 16,	
	CANMSG_CMD_ACK_SEND_DATA			= 17,
	CANMSG_CMD_MAX
}can_msg_cmd_id_t;

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
	CANMSG_EVENT_MAX								
}can_msg_event_id_t;

typedef enum
{
	MSG_TYPE_DATA = 0x04,		//Controller <-> Nodes
	MSG_TYPE_EVEN = 0x03, 		//Controller <-> Nodes
	MSG_TYPE_SETTING = 0x02,	//Controller  -> Nodes
	MSG_TYPE_ALERT = 0x01,		//Controller <-  Nodes
	MSG_TYPE_ALARM  = 0x00, 		//Controller <-  Nodes
}can_msg_type_t;

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
}can_msg_argument_type_t;

typedef struct
{
	uint8_t u8_msg_type;
	uint8_t u8_dest_addr;
	uint8_t u8_src_addr;
	uint8_t u8_msg_id;
	uint8_t u8_sensor_id;
	can_msg_cmd_id_t e_cmd;
	can_msg_argument_type_t e_arg_type;
	uint8_t u8_data_len;
	uint8_t au8_data[4];
}can_psr_t;

/* CAN Message. */
typedef struct
{
   uint16_t u16_id;          /* Standard ID: 11-bit. */
   uint8_t  u8_len;          /* Data length: 0 - 8. */
   uint8_t  au8_data[8];     /* Data buffer. */
}can_msg_data_t;

void v_can_mgreen_hardware_init (void);
void v_can_mgreen_hardware_deinit (void);
void v_can_mgreen_init (void);
void v_can_mgreen_process (void);
uint8_t u8_can_mgreen_get_msg_id (void);
bool b_write_msg_to_can_mgreen (can_psr_t*  stru_can_psr_msg);
#endif
