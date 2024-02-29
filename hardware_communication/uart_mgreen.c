/*! @file uart_mgreen.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdlib.h>
#include "uart_mgreen.h"
#include "uartstdio.h"
/*!
* static data declaration
*/
portBASE_TYPE x_higher_prio_uart_mgreen_rx = pdFALSE;
static uint8_t u8_uart_mgreen_data_buf[MAX_UART_MGREEN_RX_BUF];
static uint8_t u8_uart_mgreen_data_counter = 0;					
/*!
* private function prototype
*/
static void uart_mgreen_handle(void);
/*!
* public function bodies
*/
/*!
 * @fn void v_sim800_clear_err(void)
 * @brief Clear sim800 error: set b_sim_err = false
 * @param[in] none
 * @see b_sim800_check_err(void)
 * @return none
 */
 void v_uart_mgreen_init (void)
 {
	 SysCtlPeripheralEnable(UART_MGREEN_PER);
	while(!(SysCtlPeripheralReady(UART_MGREEN_PER)));	
	
	SysCtlPeripheralEnable(UART_MGREEN_RXTX_PER);
	while(!(SysCtlPeripheralReady(UART_MGREEN_RXTX_PER)));	
	
	GPIOPinConfigure(UART_MGREEN_RX_AF);
	GPIOPinConfigure(UART_MGREEN_TX_AF);
	GPIOPinTypeUART(UART_MGREEN_RXTX_PORT, UART_MGREEN_RX_PIN | UART_MGREEN_TX_PIN);
	UARTConfigSetExpClk(UART_MGREEN_BASE, SYSTEM_CLOCK_GET(), UART_MGREEN_BAUD, 
											(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	SysCtlDelay(2); 					// Insert a few cycles after enabling the UART to allow the clock
														// to be fully activated												
	UARTStdioConfig(UART_MGREEN_STUDIO,UART_MGREEN_BAUD,SYSTEM_CLOCK_GET());
	
	ROM_UARTIntEnable(UART_MGREEN_BASE,UART_INT_RX|UART_INT_RT);
	UARTIntRegister(UART_MGREEN_BASE, &uart_mgreen_handle);
	ROM_IntEnable(UART_MGREEN_INT);	
 }
 /*!
 * @fn void v_sim800_clear_task_woken(void)
 * @brief Clear task-woken.
 * This flag is used for RTOS incase of return from interrupt.
 * @param[in] none
 * @see none
 * @return none
 */
void v_uart_mgreen_clear_task_woken(void)
{
	x_higher_prio_uart_mgreen_rx = pdFALSE;
}
/*!
 * @fn bool b_sim800_get_task_woken(void)
 * @brief Get task-woken.
 * @param[in] none
 * @see none
 * @return true if RTOS need to switch context.
 */
bool b_uart_mgreen_get_task_woken(void)
{
	if(x_higher_prio_uart_mgreen_rx == pdTRUE)
	{
		return true;
	}
	return false;
}
 /**
* @private function bodies
*/
 /*!
 * @fn void sim800_handle(void)
 * @brief handle sim800 uart RX interrupt.
 * This function seperate the data from sim800 into 2 thread:
 * one is used for TCP data and another for sms, ussd data.
 * For TCP data: This function will check for finding AT_Resp,
 * promoting mark ('>' indicate that sim800 is ready to send data),
 * and finding MQTT frame.
 * For sms, ussd data: nothing for processing, just get and store data
 * to buffer.
 * In the end of funtion, it will check to decide RTOS need to switch context or not.
 * @param[in] 
 * @see v_sim800_clear_task_woken
 * @see v_mqtt_clear_task_woken
 * @see check_respone_at
 * @see check_promoting_mark
 * @see e_mqtt_frame_check
 * @see b_sim800_get_task_woken
 * @see b_mqtt_get_task_woken
 * @return none.
 */
static void uart_mgreen_handle(void)
{
	uint32_t ui32Status;
	ui32Status = UARTIntStatus(UART_MGREEN_BASE, true); //get interrupt status
	UARTIntClear(UART_MGREEN_BASE, ui32Status);
	v_uart_mgreen_clear_task_woken();
	
	while (ROM_UARTCharsAvail(UART_MGREEN_BASE))
	{
		if(u8_uart_mgreen_data_counter >= MAX_UART_MGREEN_RX_BUF)
		{
			u8_uart_mgreen_data_counter = 0;
		}
		u8_uart_mgreen_data_buf[u8_uart_mgreen_data_counter] = 	UARTCharGet(UART_MGREEN_BASE);
		u8_uart_mgreen_data_counter++;	
	}
	//TODO!: call function to process uart receive data buffer.
 /* We can switch context if necessary. */
	if(b_uart_mgreen_get_task_woken() == true)
	{
		portEND_SWITCHING_ISR(pdTRUE);
	}
	else
	{
		portEND_SWITCHING_ISR(pdFALSE);		
	}
}
