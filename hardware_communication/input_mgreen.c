/*! @file input_mgreen.c
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
#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "input_mgreen.h"

/*!
* static data declaration
*/

/*!
* private function prototype
*/
static void v_input_mgreen_isr (void);
/*!
* public function bodies
*/

void v_input_mgreen_init (void)
{
	HWREG(INPUT_MGREEN_GPIO_PORT+GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(INPUT_MGREEN_GPIO_PORT+GPIO_O_CR) |= INPUT_MGREEN_GPIO_PIN_1;
	
	SysCtlPeripheralEnable(INPUT_MGREEN_GPIO_PER);
	while(!(SysCtlPeripheralReady(INPUT_MGREEN_GPIO_PER)));	
	
	GPIOPinTypeGPIOInput(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_1 | INPUT_MGREEN_GPIO_PIN_2|
																							 INPUT_MGREEN_GPIO_PIN_3 | INPUT_MGREEN_GPIO_PIN_4);
	
	GPIOPadConfigSet(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_1 | INPUT_MGREEN_GPIO_PIN_2|
																					 INPUT_MGREEN_GPIO_PIN_3 | INPUT_MGREEN_GPIO_PIN_4, 
																					 GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);	
	
	GPIOIntTypeSet(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_1 | INPUT_MGREEN_GPIO_PIN_2|
																				 INPUT_MGREEN_GPIO_PIN_3 | INPUT_MGREEN_GPIO_PIN_4,
																				 GPIO_BOTH_EDGES);	
	
	SysCtlDelay(SYSTEM_CLOCK_GET()/300);	
	
	GPIOIntEnable(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_1 | INPUT_MGREEN_GPIO_PIN_2|
																				 INPUT_MGREEN_GPIO_PIN_3 | INPUT_MGREEN_GPIO_PIN_4);	
	GPIOIntRegister(INPUT_MGREEN_GPIO_PORT, &v_input_mgreen_isr);
}
/*!
* private function bodies
*/
static void v_input_mgreen_isr (void)
{
	//Get the current interrupt status
	uint32_t u32_interrupt_status = GPIOIntStatus(INPUT_MGREEN_GPIO_PORT, true);
	
	if((u32_interrupt_status & INPUT_MGREEN_INT_1) == INPUT_MGREEN_INT_1)
	{
		GPIOIntClear(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_INT_1);	
		if(GPIOPinRead(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_1))
		{
			//raising edge
		}
		else
		{
			//falling edge
		}
	}
	else if((u32_interrupt_status & INPUT_MGREEN_INT_2) == INPUT_MGREEN_INT_2)
	{
		GPIOIntClear(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_INT_2);	
		if(GPIOPinRead(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_2))
		{
			//raising edge
		}
		else
		{
			//falling edge
		}
	}
	else if((u32_interrupt_status & INPUT_MGREEN_INT_3) == INPUT_MGREEN_INT_3)
	{
		GPIOIntClear(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_INT_3);	
		if(GPIOPinRead(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_3))
		{
			//raising edge
		}
		else
		{
			//falling edge
		}
	}
	else if((u32_interrupt_status & INPUT_MGREEN_INT_4) == INPUT_MGREEN_INT_4)
	{
		GPIOIntClear(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_INT_4);	
		if(GPIOPinRead(INPUT_MGREEN_GPIO_PORT, INPUT_MGREEN_GPIO_PIN_4))
		{
			//raising edge
		}
		else
		{
			//falling edge
		}
	}	
}
