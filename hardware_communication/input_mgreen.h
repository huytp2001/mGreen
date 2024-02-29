/*! @file input_mgreen.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/
#ifndef _INPUT_MGREEN_H
#define _INPUT_MGREEN_H
#ifdef __cplusplus
extern “C” {
#endif
#include "config.h"
/*!
* @data types, constants and macro defintions
*/	
#ifdef HARDWARE_V5
#define INPUT_MGREEN_GPIO_PER							SYSCTL_PERIPH_GPIOF
#define INPUT_MGREEN_GPIO_PORT						GPIO_PORTF_BASE
#define INPUT_MGREEN_GPIO_PIN_1						GPIO_PIN_0
#define INPUT_MGREEN_GPIO_PIN_2						GPIO_PIN_1	
#define INPUT_MGREEN_GPIO_PIN_3						GPIO_PIN_2
#define INPUT_MGREEN_GPIO_PIN_4						GPIO_PIN_3	

#define INPUT_MGREEN_INT_1								GPIO_INT_PIN_0
#define INPUT_MGREEN_INT_2								GPIO_INT_PIN_1
#define INPUT_MGREEN_INT_3								GPIO_INT_PIN_2
#define INPUT_MGREEN_INT_4								GPIO_INT_PIN_3
#endif

#ifdef HARDWARE_V5_1
#define INPUT_MGREEN_GPIO_PER							SYSCTL_PERIPH_GPIOD
#define INPUT_MGREEN_GPIO_PORT						GPIO_PORTD_BASE
#define INPUT_MGREEN_GPIO_PIN_1						GPIO_PIN_4
#define INPUT_MGREEN_GPIO_PIN_2						GPIO_PIN_5	
#define INPUT_MGREEN_GPIO_PIN_3						GPIO_PIN_6
#define INPUT_MGREEN_GPIO_PIN_4						GPIO_PIN_7	

#define INPUT_MGREEN_INT_1								GPIO_INT_PIN_4
#define INPUT_MGREEN_INT_2								GPIO_INT_PIN_5
#define INPUT_MGREEN_INT_3								GPIO_INT_PIN_6
#define INPUT_MGREEN_INT_4								GPIO_INT_PIN_7
#endif
	
/*!
* @public functions prototype
*/
void v_input_mgreen_init (void);
#ifdef __cplusplus
}
#endif

#endif /* _INPUT_MGREEN_H */

