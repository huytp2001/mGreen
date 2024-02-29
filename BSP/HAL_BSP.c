#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "HAL_BSP.h"
#include "sd_app.h"

void v_LED_init(void)
{	
	SysCtlPeripheralEnable(LED_STT_PER);
	while(!(SysCtlPeripheralReady(LED_STT_PER)));	
	GPIOPinTypeGPIOOutput(LED_STT_PORT, LED_STT_PIN);
	
	SysCtlPeripheralEnable(LED_DB_1_PER);
	while(!(SysCtlPeripheralReady(LED_DB_1_PER)));	
	GPIOPinTypeGPIOOutput(LED_DB_1_PORT, LED_DB_1_PIN);
	
	SysCtlPeripheralEnable(LED_DB_2_PER);
	while(!(SysCtlPeripheralReady(LED_DB_2_PER)));	
	GPIOPinTypeGPIOOutput(LED_DB_1_PORT, LED_DB_2_PIN); 
	
	SysCtlPeripheralEnable(LED_ERR_PER);
	while(!(SysCtlPeripheralReady(LED_ERR_PER)));	
	GPIOPinTypeGPIOOutput(LED_ERR_PORT, LED_ERR_PIN);
}
void v_HAL_MSP_Init(void)
{
	PER_3V3_GPIO_CLK_ENABLE();
	SD_3V3_GPIO_CLK_ENABLE();
	
	GPIOPinTypeGPIOOutput(PER_3V3_GPIO_PORT, PER_3V3_PIN);
	GPIOPinTypeGPIOOutput(SD_3V3_GPIO_PORT, SD_3V3_PIN);
	
	v_power_3v3_peripheral_control(true);
	//enable sd card
	v_power_3v3_out_control(true);
	
	v_LED_init();
}

void v_power_3v3_peripheral_control(bool enable)
{
	if (enable)
		PER_3V3_SET();
	else
		PER_3V3_RESET();
}

// power for sd_card
void v_power_3v3_out_control(bool enable)
{
	if (enable)
		SD_3V3_SET();
	else
		SD_3V3_RESET();
}
