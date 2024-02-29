/*! @file gsm_hal.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _GSM_HAL_H
#define _GSM_HAL_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_types.h"
#include "driverlib.h"
#include "uartstdio.h"
#include "board_config.h"
/*!
* @data types, constants and macro defintions
*/
#define GSM_USE_RTOS							(1)
#define GSM_USE_3G_FLOW_CONTROL		(0) 
#define GSM_USE_FIFO							(1)
#define GSM_USE_UART_STUDIO				(1)

#define	GSM_UART_PER							BOARD_UART_PER
#define GSM_UART_BASE							BOARD_UART_BASE
#define	GSM_UART_BAUD							BOARD_UART_BAUD
#define GSM_UART_INT							BOARD_UART_INT
#define	GSM_UART_STUDIO						BOARD_UART_STUDIO

#define	GSM_UART_PORT_PER					BOARD_UART_PORT_PER
#define	GSM_UART_PORT_BASE				BOARD_UART_PORT_BASE
#define	GSM_UART_RX_PIN						BOARD_UART_RX_PIN
#define	GSM_UART_TX_PIN						BOARD_UART_TX_PIN
#define	GSM_UART_RX_AF						BOARD_UART_RX_AF
#define	GSM_UART_TX_AF						BOARD_UART_TX_AF

#define	GSM_PWR_KEY_PER						BOARD_PWR_KEY_PER
#define GSM_PWR_KEY_PORT					BOARD_PWR_KEY_PORT
#define GSM_PWR_KEY_PIN						BOARD_PWR_KEY_PIN

#define	GSM_PWR_EN_PER						BOARD_PWR_EN_PER
#define GSM_PWR_EN_PORT						BOARD_PWR_EN_PORT
#define GSM_PWR_EN_PIN						BOARD_PWR_EN_PIN

#define GSM_PWR_KEY_HIGH()       	ROM_GPIOPinWrite(GSM_PWR_KEY_PORT, GSM_PWR_KEY_PIN, 0x00);
#define GSM_PWR_KEY_LOW()        	ROM_GPIOPinWrite(GSM_PWR_KEY_PORT, GSM_PWR_KEY_PIN, 0xFF);

#define GSM_PWR_EN_HIGH()        	ROM_GPIOPinWrite(GSM_PWR_EN_PORT, GSM_PWR_EN_PIN, 0xFF);
#define GSM_PWR_EN_LOW()         	ROM_GPIOPinWrite(GSM_PWR_EN_PORT, GSM_PWR_EN_PIN, 0x00);
/*!
* @public functions prototype
*/
void v_gsm_hal_init (void);
void v_gsm_hal_power_off (void);
void v_gsm_hal_power_on (void);
void v_gsm_hal_set_sim_rssi (uint8_t u8_value);
void v_gsm_mqtt_network_close(bool b_close);
bool b_gsm_is_mqtt_network_closed(void);
uint8_t u8_gsm_hal_get_sim_rssi (void);
void v_wifi_reset(void);
void v_module_4g_hal_power_off(void);
void v_module_4g_hal_power_on(void);
#ifdef __cplusplus
}
#endif

#endif /* GSM_HAL_H_ */
