/*! @file board_config.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_
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
#include "config.h"
/*!
* @data types, constants and macro defintions
*/

/*!
*	@def BOARD_FF_CONTROLLER_V4_3
*	@def BOARD_MGREEN_NG_V5
*/	
#define BOARD_MGREEN_NG_V5	

/* GSM UART Port */
#ifdef BOARD_FF_CONTROLLER_V4_3
#define BOARD_UART_PER								SYSCTL_PERIPH_UART2
#define BOARD_UART_BASE								UART2_BASE
#define	BOARD_UART_BAUD								115200
#define BOARD_UART_INT								INT_UART2
#define	BOARD_UART_STUDIO							2

#define BOARD_UART_PORT_PER						SYSCTL_PERIPH_GPIOD
#define BOARD_UART_PORT_BASE					GPIO_PORTD_BASE		   
#define BOARD_UART_RX_PIN							GPIO_PIN_4
#define BOARD_UART_TX_PIN							GPIO_PIN_5
#define BOARD_UART_RX_AF							GPIO_PD4_U2RX
#define BOARD_UART_TX_AF							GPIO_PD5_U2TX
	                                                                     
#define	BOARD_PWR_KEY_PER							SYSCTL_PERIPH_GPIOD
#define BOARD_PWR_KEY_PORT						GPIO_PORTD_BASE
#define	BOARD_PWR_KEY_PIN							GPIO_PIN_6	
				                                    
#define BOARD_PWR_EN_PER							SYSCTL_PERIPH_GPIOD
#define	BOARD_PWR_EN_PORT							GPIO_PORTD_BASE
#define	BOARD_PWR_EN_PIN							GPIO_PIN_7

/* Debug UART Port */
#define BOARD_DEBUG_UART_PER					SYSCTL_PERIPH_UART6
#define BOARD_DEBUG_UART_BASE					UART6_BASE
#define	BOARD_DEBUG_UART_BAUD					115200
#define BOARD_DEBUG_UART_INT					INT_UART6
#define	BOARD_DEBUG_UART_STUDIO				6

#define BOARD_DEBUG_UART_PORT_PER			SYSCTL_PERIPH_GPIOP  
#define BOARD_DEBUG_UART_PORT_BASE		GPIO_PORTP_BASE                                                
#define BOARD_DEBUG_UART_RX_AF				GPIO_PP0_U6RX
#define BOARD_DEBUG_UART_TX_AF				GPIO_PP1_U6TX                                                
#define BOARD_DEBUG_UART_RX_PIN				GPIO_PIN_0
#define BOARD_DEBUG_UART_TX_PIN				GPIO_PIN_1  
#endif

#ifdef BOARD_MGREEN_NG_V5
#define BOARD_UART_PER								SYSCTL_PERIPH_UART3
#define BOARD_UART_BASE								UART3_BASE
#define	BOARD_UART_BAUD								115200
#define BOARD_UART_INT								INT_UART3
#define	BOARD_UART_STUDIO							3

#define BOARD_UART_PORT_PER						SYSCTL_PERIPH_GPIOJ
#define BOARD_UART_PORT_BASE					GPIO_PORTJ_BASE		   
#define BOARD_UART_RX_PIN							GPIO_PIN_0
#define BOARD_UART_TX_PIN							GPIO_PIN_1
#define BOARD_UART_RX_AF							GPIO_PJ0_U3RX
#define BOARD_UART_TX_AF							GPIO_PJ1_U3TX
	                                                                     
#define	BOARD_PWR_KEY_PER							SYSCTL_PERIPH_GPIOB
#define BOARD_PWR_KEY_PORT						GPIO_PORTB_BASE
#define	BOARD_PWR_KEY_PIN							GPIO_PIN_5	
				                                    
#define BOARD_PWR_EN_PER							SYSCTL_PERIPH_GPIOP
#define	BOARD_PWR_EN_PORT							GPIO_PORTP_BASE
#define	BOARD_PWR_EN_PIN							GPIO_PIN_0

/* Debug UART Port */
#define BOARD_DEBUG_UART_PER					SYSCTL_PERIPH_UART6
#define BOARD_DEBUG_UART_BASE					UART6_BASE
#define	BOARD_DEBUG_UART_BAUD					115200
#define BOARD_DEBUG_UART_INT					INT_UART6
#define	BOARD_DEBUG_UART_STUDIO				6

#define BOARD_DEBUG_UART_PORT_PER			SYSCTL_PERIPH_GPIOP  
#define BOARD_DEBUG_UART_PORT_BASE		GPIO_PORTP_BASE                                                
#define BOARD_DEBUG_UART_RX_AF				GPIO_PP0_U6RX
#define BOARD_DEBUG_UART_TX_AF				GPIO_PP1_U6TX                                                
#define BOARD_DEBUG_UART_RX_PIN				GPIO_PIN_0
#define BOARD_DEBUG_UART_TX_PIN				GPIO_PIN_1        
#endif
/*!
* @public functions prototype
*/
#ifdef __cplusplus
}
#endif

#endif /* _BOARD_CONFIG_H_ */
