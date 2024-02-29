/*! @file uart_mgreen.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/
#ifndef _UART_MGREEN_H
#define _UART_MGREEN_H
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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "config.h"
/*!
* @data types, constants and macro defintions
*/
#ifdef HARDWARE_V5
#define	UART_MGREEN_PER								SYSCTL_PERIPH_UART1
#define UART_MGREEN_BASE							UART1_BASE
#define	UART_MGREEN_BAUD							115200
#define UART_MGREEN_INT								INT_UART1
#define	UART_MGREEN_STUDIO						1
#define	UART_MGREEN_RXTX_PER					SYSCTL_PERIPH_GPIOB
#define	UART_MGREEN_RXTX_PORT					GPIO_PORTB_BASE
#define	UART_MGREEN_RX_PIN						GPIO_PIN_0
#define	UART_MGREEN_TX_PIN						GPIO_PIN_1
#define	UART_MGREEN_RX_AF							GPIO_PB0_U1RX
#define	UART_MGREEN_TX_AF							GPIO_PB1_U1TX
#endif

#ifdef HARDWARE_V5_1
#define	UART_MGREEN_PER								SYSCTL_PERIPH_UART7
#define UART_MGREEN_BASE							UART7_BASE
#define	UART_MGREEN_BAUD							115200
#define UART_MGREEN_INT								INT_UART7
#define	UART_MGREEN_STUDIO						7
#define	UART_MGREEN_RXTX_PER					SYSCTL_PERIPH_GPIOC
#define	UART_MGREEN_RXTX_PORT					GPIO_PORTC_BASE
#define	UART_MGREEN_RX_PIN						GPIO_PIN_4
#define	UART_MGREEN_TX_PIN						GPIO_PIN_5
#define	UART_MGREEN_RX_AF							GPIO_PC4_U7RX
#define	UART_MGREEN_TX_AF							GPIO_PC5_U7TX
#endif

#define	MAX_UART_MGREEN_RX_BUF				250

void v_uart_mgreen_init (void);
#ifdef __cplusplus
}
#endif

#endif /* _UART_MGREEN_H */
