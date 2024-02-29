/*! @file digital_input.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef _DIGITAL_INPUT_H
#define _DIGITAL_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern “C” {
#endif
/*!
* @data types, constants and macro defintions
*/
	
#define MAX_INPUT_SETTING				4

#define INPUT_12_COUNTER_CLK_EN() ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3)
#define INPUT_34_COUNTER_CLK_EN() ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4)

#define INPUT_12_TIMER_PORT				TIMER3_BASE
#define INPUT_34_TIMER_PORT				TIMER4_BASE
	
#define INPUT_1_CLK_ENABLE()		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD)
#define INPUT_1_GPIO_PORT				GPIO_PORTD_BASE
#define INPUT_1_PIN							GPIO_PIN_4
#define INPUT_1_READ() 					(ROM_GPIOPinRead(INPUT_1_GPIO_PORT, INPUT_1_PIN)?0:1)

#define INPUT_2_CLK_ENABLE()		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD)
#define INPUT_2_GPIO_PORT				GPIO_PORTD_BASE
#define INPUT_2_PIN							GPIO_PIN_5
#define INPUT_2_READ() 					(ROM_GPIOPinRead(INPUT_2_GPIO_PORT, INPUT_2_PIN)?0:1)

#define INPUT_3_CLK_ENABLE()		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD)
#define INPUT_3_GPIO_PORT				GPIO_PORTD_BASE
#define INPUT_3_PIN							GPIO_PIN_6
#define INPUT_3_READ() 					(ROM_GPIOPinRead(INPUT_3_GPIO_PORT, INPUT_3_PIN)?0:1)

#define INPUT_4_CLK_ENABLE()		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD)
#define INPUT_4_GPIO_PORT				GPIO_PORTD_BASE
#define INPUT_4_PIN							GPIO_PIN_7
#define INPUT_4_READ() 					(ROM_GPIOPinRead(INPUT_4_GPIO_PORT, INPUT_4_PIN)?0:1)

#define SW1_CLK_ENABLE()				ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL)
#define SW1_GPIO_PORT						GPIO_PORTL_BASE
#define SW1_PIN									GPIO_PIN_7
#define SW1_READ() 							(ROM_GPIOPinRead(SW1_GPIO_PORT, SW1_PIN)?1:0)

#define SW2_CLK_ENABLE()				ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL)
#define SW2_GPIO_PORT						GPIO_PORTL_BASE
#define SW2_PIN									GPIO_PIN_6
#define SW2_READ() 							(ROM_GPIOPinRead(SW2_GPIO_PORT, SW2_PIN)?1:0)

#define SW3_CLK_ENABLE()				ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ)
#define SW3_GPIO_PORT						GPIO_PORTQ_BASE
#define SW3_PIN									GPIO_PIN_4
#define SW3_READ() 							(ROM_GPIOPinRead(SW3_GPIO_PORT, SW3_PIN)?1:0)

#define SW4_CLK_ENABLE()				ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP)
#define SW4_GPIO_PORT						GPIO_PORTP_BASE
#define SW4_PIN									GPIO_PIN_2
#define SW4_READ() 							(ROM_GPIOPinRead(SW4_GPIO_PORT, SW4_PIN)?1:0)

#define SW5_CLK_ENABLE()				ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP)
#define SW5_GPIO_PORT						GPIO_PORTP_BASE
#define SW5_PIN									GPIO_PIN_3
#define SW5_READ() 							(ROM_GPIOPinRead(SW5_GPIO_PORT, SW5_PIN)?1:0)


#define PUMP_1_PRESSURE_OK			(u32_get_di1_status())
#define PUMP_2_PRESSURE_OK			(u32_get_di2_status())


typedef enum
{
	INPUT_PIN_NOT_USE = 0x00,
	INPUT_PIN_1 = 0x01,
	INPUT_PIN_2 = 0x02,
	INPUT_PIN_3 = 0x04,
	INPUT_PIN_4 = 0x08,
}DIO_INPUT_PIN;
/*!
* @public functions prototype
*/
void v_digital_input_task_init (void);
double d_get_flow_meter(uint8_t u8_output_port);
void v_di_get_value(uint32_t *u32_return_value);
uint32_t u32_get_di1_status(void);
uint32_t u32_get_di2_status(void);
uint32_t u32_get_di3_status(void);
uint32_t u32_get_di4_status(void);
void v_get_all_di_status(uint32_t *status);
void v_clear_flow_meter(uint8_t u8_output_port);
void v_digital_input_set_type_counter(DIO_INPUT_PIN e_pin);
#ifdef __cplusplus
}
#endif

#endif /* _DIGITAL_INPUT_H */

