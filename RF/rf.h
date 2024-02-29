/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    	*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file rf.h
 * @author Linh Nguyen (edit)
 * @date 27 Nov 2020
 * @version: draft 1.0
 * @brief Manage RF hardware. 
 */
#ifndef RF_H_
#define RF_H_
#ifdef __cplusplus
extern �C� {
#endif
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
/*!
* @data types, constants and macro defintions
*/

	
	
#define RF_TASK_STACK_SIZE				    (configMINIMAL_STACK_SIZE * 6)
#define RF_TASK_PRIORITY					(tskIDLE_PRIORITY + 1)
#define RF_TASK_DELAY						(portTickType)(2 / portTICK_RATE_MS)			// 2ms
	
/* RF configuration */
/* RF configuration */
#ifdef HARDWARE_V5
#define RF_UART_PER								SYSCTL_PERIPH_UART4
#define RF_UART_BASE							UART4_BASE
#define RF_UART_INT								INT_UART4
#define	RF_UART_STUDIO						4
#define RF_UART_RXTX_PER					SYSCTL_PERIPH_GPIOA
#define RF_UART_PORT							GPIO_PORTA_BASE
#define RF_UART_RX								GPIO_PA2_U4RX
#define RF_UART_TX								GPIO_PA3_U4TX
#define RF_UART_RX_PIN						GPIO_PIN_2
#define RF_UART_TX_PIN						GPIO_PIN_3

#define RF_SETA_PER								SYSCTL_PERIPH_GPIOA
#define RF_SETA_PORT							GPIO_PORTA_BASE
#define RF_SETA_PIN								GPIO_PIN_4

#define RF_SETB_PER								SYSCTL_PERIPH_GPIOA
#define RF_SETB_PORT							GPIO_PORTA_BASE
#define RF_SETB_PIN								GPIO_PIN_5

#define RF_AUX_PER								SYSCTL_PERIPH_GPIOA
#define RF_AUX_PORT								GPIO_PORTA_BASE
#define RF_AUX_PIN								GPIO_PIN_6
#define RF_AUX_INTERRUPT					GPIO_INT_PIN_6

#define RF_RST_PER								SYSCTL_PERIPH_GPIOA
#define RF_RST_PORT								GPIO_PORTA_BASE
#define RF_RST_PIN								GPIO_PIN_7
#endif

#ifdef HARDWARE_V5_1
#define RF_UART_PER								SYSCTL_PERIPH_UART4
#define RF_UART_BASE							UART4_BASE
#define RF_UART_INT								INT_UART4
#define	RF_UART_STUDIO						4
#define RF_UART_RXTX_PER					SYSCTL_PERIPH_GPIOA
#define RF_UART_PORT							GPIO_PORTA_BASE
#define RF_UART_RX								GPIO_PA2_U4RX
#define RF_UART_TX								GPIO_PA3_U4TX
#define RF_UART_RX_PIN						GPIO_PIN_2
#define RF_UART_TX_PIN						GPIO_PIN_3

#define RF_SETA_PER								SYSCTL_PERIPH_GPIOA
#define RF_SETA_PORT							GPIO_PORTA_BASE
#define RF_SETA_PIN								GPIO_PIN_4

#define RF_SETB_PER								SYSCTL_PERIPH_GPIOA
#define RF_SETB_PORT							GPIO_PORTA_BASE
#define RF_SETB_PIN								GPIO_PIN_6

#define RF_AUX_PER								SYSCTL_PERIPH_GPIOA
#define RF_AUX_PORT								GPIO_PORTA_BASE
#define RF_AUX_PIN								GPIO_PIN_5
#define RF_AUX_INTERRUPT					GPIO_INT_PIN_5

#define RF_PWR_EN_PER							SYSCTL_PERIPH_GPIOA
#define RF_PWR_EN_PORT						GPIO_PORTA_BASE
#define RF_PWR_EN_PIN							GPIO_PIN_7
#endif

#define RF_AUX_PULSE_VALID				5 //ms

#define RF_BUF_MAX								75

#define RF_QUEUE_LEN							5

#define MQTT_DATA_FIELD_LEN_IN_BYTES            8
#define FRAME_TYPE_DATA_NODE		0x20

/*!
*	Extern functions
*/
void v_rf_task_init (void);
void v_rf_clear_higher_prio_task (void);
void v_rf_end_switching_context (void);
bool b_write_frame_to_rf_from_isr (uint8_t* pu8_data_frame);
bool b_write_frame_to_rf (uint8_t* pu8_data_frame);
bool b_rf_has_frame (void);
bool b_get_frame_from_rf (uint8_t* pu8_data_frame);
void v_rf_aux_isr (void);
#ifdef __cplusplus
}
#endif

#endif /* RF_H_ */

