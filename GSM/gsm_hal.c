/*! @file gsm_hal.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2017.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdlib.h>
#include <stdint.h>
#include "gsm_hal.h"
#include "uartstdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/*!
* static data declaration
*/
static volatile uint8_t u8_sim_rssi = 0;
/*!
* private function prototype
*/
static inline void v_gsm_hal_uart_cfg (void);
static inline void v_gsm_hal_gpio_cfg (void);
static void v_gsm_hal_isr (void);

/*!
* public function bodies
*/
/*!
*	@fn void v_gsm_hal_init (void)
* @brief Init GSM Hal
* @param[in] none
* @param[out] none
* @return
*/
void v_gsm_hal_init (void)
{
	// Step 1: Configure gsm gpio
	v_gsm_hal_gpio_cfg();
	// Step 2: Configure gsm uart
	v_gsm_hal_uart_cfg();
}
/*!
*	@fn void v_gsm_hal_power_off (void)
* @brief Turn off GSM module
* @param[in] none
* @param[out] none
* @return
*/
void v_gsm_hal_power_off (void)
{
	GSM_PWR_EN_HIGH();
}

/*!
*	@fn void v_gsm_hal_power_on (void)
* @brief Turn on GSM module
* @param[in] none
* @param[out] none
* @return
*/
void v_gsm_hal_power_on (void)
{
	// Step 1: Turn on LDO
	GSM_PWR_EN_LOW();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/1000); //3ms
	// Step 2: Generate Power-On signal
	//Power on: Keep power key as low for 2s, then pull to high.
	GSM_PWR_KEY_LOW();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/3*2); //2s
	GSM_PWR_KEY_HIGH();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/1000); //3ms
}

/*!
*	@fn void v_gsm_hal_set_sim_rssi (uint8_t u8_value)
* @brief Set sim RSSI value
* @param[in] uint8_t u8_value
* @param[out] none
* @return
*/
void v_gsm_hal_set_sim_rssi (uint8_t u8_value)
{
	u8_sim_rssi = u8_value;
}

/*!
*	@fn uint8_t u8_gsm_hal_get_sim_rssi (void)
* @brief Get sim RSSI value
* @param[in] none
* @param[out] none
* @return \u u8_sim_rssi
*/
uint8_t u8_gsm_hal_get_sim_rssi (void)
{
	return u8_sim_rssi;
}

void v_wifi_reset(void)
{
	//Power on: Keep power key as low for 2s, then pull to high.
	GSM_PWR_KEY_LOW();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/3*1); //1s
	GSM_PWR_KEY_HIGH();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/1000); //3ms
}

void v_module_4g_hal_power_off(void)
{
	//Power on: Keep power key as low for 2s, then pull to high.
	GSM_PWR_KEY_HIGH();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/3); //1s
	GSM_PWR_KEY_LOW();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/3*3); //3s
	GSM_PWR_KEY_HIGH();
}

void v_module_4g_hal_power_on(void)
{
	//Power on: Keep power key as low for 2s, then pull to high.
	GSM_PWR_KEY_HIGH();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/3); //1s
	GSM_PWR_KEY_LOW();
	ROM_SysCtlDelay(SYSTEM_CLOCK_GET()/3); //100ms
	GSM_PWR_KEY_HIGH();
}
/**
* private function bodies
*/
/*!
*	@fn static inline void v_gsm_hal_uart_cfg (void)
* @brief Configure gsm uart, uart RX flow control, FIFO, Uart Studio, Interupt handler.
* @param[in] none
* @param[out] none
* @return
*/
static inline void v_gsm_hal_uart_cfg (void)
{
#if GSM_USE_RTOS
	portENTER_CRITICAL();
#endif // GSM_USE_RTOS
	SysCtlPeripheralEnable(GSM_UART_PER);
	while(!(SysCtlPeripheralReady(GSM_UART_PER)));
	
	SysCtlPeripheralEnable(GSM_UART_PORT_PER);
	while(!(SysCtlPeripheralReady(GSM_UART_PORT_PER)));		
	
	GPIOPinConfigure(GSM_UART_RX_AF);
	GPIOPinConfigure(GSM_UART_TX_AF);
	GPIOPinTypeUART(GSM_UART_PORT_BASE, GSM_UART_RX_PIN | GSM_UART_TX_PIN);
	
	// Configure the UART for 115,200, 8-N-1 operation.
	UARTConfigSetExpClk(GSM_UART_BASE, SYSTEM_CLOCK_GET(), GSM_UART_BAUD,
													(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
													 UART_CONFIG_PAR_NONE));
	// Insert a few cycles after enabling the UART to allow the clock
	// to be fully activated		
	ROM_SysCtlDelay(3);
#if GSM_USE_3G_FLOW_CONTROL
    UARTFlowControlSet(GSM_UART_BASE, UART_FLOWCONTROL_RX); // Only apply flow control for RX
#endif // GSM_USE_3G_FLOW_CONTROL

#if GSM_USE_FIFO
	UARTFIFOEnable(GSM_UART_BASE);
	// Set the UART to interrupt whenever the TX FIFO is almost empty or
	// when any character is received.
	UARTFIFOLevelSet(GSM_UART_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
#endif // GSM_USE_FIFO

#if GSM_USE_UART_STUDIO
	UARTStdioConfig(GSM_UART_STUDIO, GSM_UART_BAUD, SYSTEM_CLOCK_GET());
#endif // GSM_USE_UART_STUDIO
	
	// Config the UART interrupt.
	ROM_UARTIntEnable(GSM_UART_BASE, UART_INT_RX | UART_INT_RT);
	// Set interrupt resgister.
	UARTIntRegister(GSM_UART_BASE, &v_gsm_hal_isr);	
	// Enable the UART interrupt.
	ROM_IntEnable(GSM_UART_INT);
#if GSM_USE_RTOS
	portEXIT_CRITICAL();
#endif // GSM_USE_RTOS	
}

/*!
*	@fn static inline void v_gsm_hal_gpio_cfg (void)
* @brief Configure gsm gpio
* @param[in] none
* @param[out] none
* @return
*/
static inline void v_gsm_hal_gpio_cfg (void)
{
	ROM_SysCtlPeripheralEnable(GSM_PWR_EN_PER);
	while(!(ROM_SysCtlPeripheralReady(GSM_PWR_EN_PER)));
	
	ROM_SysCtlPeripheralEnable(GSM_PWR_KEY_PER);
	while(!(ROM_SysCtlPeripheralReady(GSM_PWR_KEY_PER)));

	GPIOPinTypeGPIOOutput(GSM_PWR_EN_PORT, GSM_PWR_EN_PIN);
	GSM_PWR_EN_HIGH(); /* turn-off ower at startup*/

	GPIOPinTypeGPIOOutput(GSM_PWR_KEY_PORT, GSM_PWR_KEY_PIN);
	GSM_PWR_KEY_HIGH();
}
/*!
*	@fn static void v_gsm_hal_isr (void)
* @brief Configure gsm uart, uart RX flow control, FIFO, Uart Studio, Interupt handler.
* @param[in] none
* @param[out] none
* @return
*/
static void v_gsm_hal_isr (void)
{
#if GSM_USE_UART_STUDIO	
	UARTStdioIntHandler();
#endif // GSM_USE_UART_STUDIO
}
