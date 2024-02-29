/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    	*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file cc1310.c
 * @author Linh Nguyen (editor)
 * @date 27 Nov 2020
 * @version: draft 1.0
 * @brief Manage functionalities of CC1310 Module.
 */
/*!
 * Add more include here
 */
#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "rf.h"
#include "cc1310.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*!
*	Variables declare
*/
static uint8_t au8_cc1310_data[RF_BUF_MAX];
static uint8_t u8_cc1310_data_count = 0;

/*!
*	Functions prototype
*/
static uint8_t u8_cc1310_check_init (void);
static void v_cc1310_uart_isr (void);
static uint8_t u8_cc1310_check_frame (void);

/*!
*	Public functions 
*/

/*!
* @fn v_cc1310_init (void)
* @brief 
* @param[in] None
* @return None
*/
bool v_cc1310_init (void)
{	
	SysCtlDelay(SysCtlClockGet()/3);
	
	if (!u8_cc1310_check_init())
	{
		return false;
	}
	
	SysCtlDelay(SysCtlClockGet()/3);
	
	//RX mode
	/*
	 * |M0(SETA)|M2(SETB)  |Mode   |
	 * |0       |0         |SLEEP  |
	 * |0       |1         |RX     |
	 * |1       |0         |TX     |
	 * |1       |1         |RESERVE|
	 */	
	//SLEEP
//	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, 0);
//	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, 0);
	
	//RX
	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, 0);
	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, RF_SETB_PIN);

	//TX
//	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, RF_SETA_PIN);
//	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, 0);	
	
	ROM_UARTIntEnable(RF_UART_BASE, UART_INT_RX|UART_INT_RT);
	UARTIntRegister(RF_UART_BASE, &v_cc1310_uart_isr);
	return true;	
}

/*!
* @fn v_cc1310_process_data (void)
* @brief 
* @param[in] None
* @return None
*/
void v_cc1310_process_data (void)
{
	switch(u8_cc1310_check_frame())
	{
		case 0:
		{
		}
		break;
		case 1: //wrong check sum
		{
			//TODO: set node id = 0  to indicate wrong frame
			//TODO: 
			//b_write_frame_to_rf_from_isr(au8_cc1310_data);			
		}
		break;			
		case 2: //
		{
			b_write_frame_to_rf(au8_cc1310_data);
		}
		break;
	}			
	u8_cc1310_data_count = 0;
	memset(au8_cc1310_data, 0, RF_BUF_MAX);
}

void v_cc1310_send_package (uint8_t* pu8_package, uint8_t u8_size)
{
	//TX
	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, RF_SETA_PIN);
	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, 0);	
	
	//SysCtlDelay(SysCtlClockGet()/1000);
	vTaskDelay(2000);
	
	for (uint8_t u8_i = 0; u8_i < u8_size; u8_i++)
	{
		UARTCharPut(RF_UART_BASE, *(pu8_package + u8_i));
	}
	
	//SysCtlDelay(SysCtlClockGet()/1000);
	vTaskDelay(2000);
	
	//SLEEP
	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, 0);
	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, 0);
}

/*!
* Private Functions
*/

/*!
* @fn u8_cc1310_check_init (void)
* @brief 
* @param[in] None
* @return None
*/
static uint8_t u8_cc1310_check_init (void)
{
	return 1;
}

/*!
* @fn void v_cc1310_uart_isr (void)
* @brief 
* @param[in] None
* @return None
*/
static void v_cc1310_uart_isr (void)
{
	uint32_t u32_status;
	u32_status = UARTIntStatus(RF_UART_BASE, true); //get interrupt status
	UARTIntClear(RF_UART_BASE, u32_status);
	
	v_rf_clear_higher_prio_task();
	
	if(u32_status & (UART_INT_RX | UART_INT_RT))
	{
		while(UARTCharsAvail(RF_UART_BASE))
		{
			au8_cc1310_data[u8_cc1310_data_count] = UARTCharGetNonBlocking(RF_UART_BASE);
			u8_cc1310_data_count++;
		}
	}
//	if(!(HWREG(RF_UART_BASE + UART_O_FR) & UART_FR_RXFE))//read data
//	{
//		au8_cc1310_data[u8_cc1310_data_count] = HWREG(RF_UART_BASE + UART_O_DR);
//		u8_cc1310_data_count++;
//	}
	/* We can switch context if necessary. */
	v_rf_end_switching_context();
}

/*!
* @fn u8_cc1310_check_frame (void)
* @brief 
* @param[in] None
* @return None
*/
static uint8_t u8_cc1310_check_frame (void)
{
	if (u8_cc1310_data_count == 0)
	{
		return 0;
	}
	if (u8_cc1310_data_count != (au8_cc1310_data[0] + 1))
	{
		return 0;
	}
	//Check sum of frame: MSB
	uint16_t u16_sum_in_frame = 0;
	u16_sum_in_frame = au8_cc1310_data[au8_cc1310_data[0] - 3] |
										(au8_cc1310_data[au8_cc1310_data[0] - 4] << 8);
	uint16_t u16_sum_of_frame = 0;
	for (uint8_t u8_i = 1; u8_i < au8_cc1310_data[0] - 4; u8_i++)
	{
		u16_sum_of_frame += au8_cc1310_data[u8_i];
	}
	if (u16_sum_of_frame != u16_sum_in_frame)
	{
		return 1;
	}
	return 2;
}
