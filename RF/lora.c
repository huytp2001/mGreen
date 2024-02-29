/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    	*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
/**
 * @file lora.c
 * @author Linh Nguyen (editor) 
 * @date 13 Nov 2020
 * @version: draft 1.0
 * @brief Manage functionality of LORA Module.
 */
/*!
 * Add more include here
 */
#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "rf.h"
#include "lora.h"
#include "sdram.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "timers.h"

/*!
* @def LORA_TIMEOUT
*/
#define LORA_TIMEOUT  1600000
/*!
*	Variables declare
*/
extern portBASE_TYPE x_higher_prio_task;
/*!
*	Private variables
*/
static volatile uint8_t au8_lora_data[RF_BUF_MAX];
static volatile uint8_t u8_lora_data_count = 0;
static volatile uint8_t u8_frame_len = 0;
/*!
*	Functions prototype
*/
static void v_lora_uart_isr (void);
static uint8_t u8_lora_check_frame (void);
static uint8_t u8_lora_e32_check_init (void);

/*!
*	Public functions 
*/

/*!
* @fn v_lora_e32_init (void)
* @brief Init LORA E32 module
* @param[in] None
* @return None
*/
bool v_lora_e32_init (void)
{

	
	//M0 = SETA
	//M1 = SETB
	//Mode 3 sleep, M1 = 1, M0 = 1 for parameter setting.
	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, RF_SETA_PIN);
	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, RF_SETB_PIN);	
	
	SysCtlDelay(SysCtlClockGet()/3);
	//|0xC0|0x00|0x19|0x06|0xC4|
	//Freq: 868MHz| Uart: 9600, 8N1| AirRate: 1200 bps, Power: 20dbm
	//FEC: enable, Fixed mode: Enable, WOR: 500ms.
	//Address: 0, Channel: 6.
	UARTCharPut(RF_UART_BASE, 0xC2); // saved parameter when power-down.
	UARTCharPut(RF_UART_BASE, 0x00);
	UARTCharPut(RF_UART_BASE, 0x01);
	UARTCharPut(RF_UART_BASE, 0x19);
	UARTCharPut(RF_UART_BASE, 0x06);
	UARTCharPut(RF_UART_BASE, 0xC4);	
	
	
	uint8_t u8_tmp = 0;
	u8_tmp = u8_lora_e32_check_init();
	if (!u8_tmp)
	{
		//return false;
	}

	//Mode 0: Normal mode
	//M1 = 0, M0 = 0.
	SysCtlDelay(SysCtlClockGet()/3);
	GPIOPinWrite(RF_SETA_PORT, RF_SETA_PIN, 0);
	GPIOPinWrite(RF_SETB_PORT, RF_SETB_PIN, 0);
	
	UARTIntRegister(RF_UART_BASE, &v_lora_uart_isr);
	ROM_IntPrioritySet(RF_UART_INT, (5 << 5));
	ROM_UARTIntEnable(RF_UART_BASE, UART_INT_RX|UART_INT_RT);
	return true;	
}

/*!
* @fn v_lora_send_package (uint8_t* pu8_package, uint8_t u8_size)
* @brief Send package to UART
* @param[in] pu8_package Info package
* @param[in] u8_size Size of package
* @return None
*/
void v_lora_send_package (uint8_t* pu8_package, uint8_t u8_size)
{
	for (uint8_t u8_i = 0; u8_i < u8_size; u8_i++)
	{
		UARTCharPut(RF_UART_BASE, *(pu8_package + u8_i));
	}
}


/*!
* @fn v_lora_process_mess(void)
* @brief Process data of lora message
* @param None
* @return None
*/
void v_lora_process_mess(void)
{
	if (u8_lora_check_frame() == 2)
	{
		b_write_frame_to_rf((uint8_t *)au8_lora_data);
		u8_lora_data_count = 0;
		u8_frame_len = 0;
		// Reset lora data array
		memset((void *)au8_lora_data, 0, RF_BUF_MAX);		
	}
}
/*!
* Private Functions
*/

/*!
* @fn v_lora_uart_isr (void)
* @brief UART Interrupt Service Routine of LORA 
* @param[in] None
* @return None
*/
static void v_lora_uart_isr (void)
{
	uint32_t u32_status;
	u32_status = UARTIntStatus(RF_UART_BASE, true); //get interrupt status
	UARTIntClear(RF_UART_BASE, u32_status);
	
	v_rf_clear_higher_prio_task();
	
	if(!(HWREG(RF_UART_BASE + UART_O_FR) & UART_FR_RXFE)) //read data
	{
		au8_lora_data[u8_lora_data_count] = HWREG(RF_UART_BASE + UART_O_DR);
		u8_lora_data_count++;
	}
	
	/* We can switch context if necessary. */
	v_rf_end_switching_context();
}

/*!
* @fn u8_lora_check_frame (void)
* @brief Frame checksum 
* @param[in] None
* @return 	0 Return without doing checksum due to wrong frame length
*						1 Wrong checksum
* 					2 Checksum OK
*/
static uint8_t u8_lora_check_frame (void)
{
	if (u8_lora_data_count == 0)
	{
		return 0;
	}
	if ((u8_lora_data_count) != au8_lora_data[0])
	{
		return 0;
	}
	//Check sum of frame
	uint16_t u16_sum_in_frame = 0;
	u16_sum_in_frame = au8_lora_data[u8_lora_data_count - 1] |
										(au8_lora_data[u8_lora_data_count - 2] << 8);
	
	uint16_t u16_sum_of_frame = 0;
	for (uint8_t u8_i = 1; u8_i < u8_lora_data_count - 2; u8_i++)
	{
		u16_sum_of_frame += au8_lora_data[u8_i];
	}
	//u16_sum_of_frame = ~u16_sum_of_frame;
	if (u16_sum_of_frame != u16_sum_in_frame)
	{
		return 1;
	}
	return 2;
}
/*!
* @fn u8_lora_e32_check_init ()
* @brief Check E32 status and Wait for E32 init.
* @param[in] None
* @return u8_check_status
*/
static uint8_t u8_lora_e32_check_init ()
{
	uint8_t u8_check_done = 1;
	uint8_t u8_data_check = 0;
	static uint8_t u8_check_status = 0;
	uint32_t u32_check_timeout = 0;
	
	while(u8_check_done)
	{
		if(!(HWREG(RF_UART_BASE + UART_O_FR) & UART_FR_RXFE))
		{
			u8_data_check = HWREG(RF_UART_BASE + UART_O_DR);
			switch(u8_check_status)
			{
				case 0:
					if (u8_data_check == 0xC0)
					{
						u8_check_status = 1;
						break;
					}
					return 0;
				case 1:
					if(u8_data_check == 0x00)
					{
						u8_check_status = 2;
						break;
					}
					return 0;
				case 2:
					if(u8_data_check == 0x00)
					{
						u8_check_status = 3;
						break;
					}
					return 0;
				case 3:
					if(u8_data_check == 0x19)
					{
						u8_check_status = 4;
						break;
					}
					return 0;
				case 4:
					if(u8_data_check == 0x06)
					{
						u8_check_status = 5;
						break;
					}
					return 0;
				case 5:
					if(u8_data_check == 0xCC)
					{
						u8_check_done = 0;
						return 1;
					}
					return 0;
			}
			u32_check_timeout = 0;
		}
		u32_check_timeout++;
		if (u32_check_timeout > LORA_TIMEOUT)
		{
			u8_check_done = 0;
			return 0;
		}
	}	
	return 1;
}
