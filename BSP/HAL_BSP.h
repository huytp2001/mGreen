#ifndef HAL_BSP_H
#define HAL_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "sdram.h"
//#define COREBOARD_4_0				
/* Definition for 3V3 PERIPHERAL ENABLE Pin */
#define PER_3V3_GPIO_CLK_ENABLE()							ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ)
#define PER_3V3_GPIO_PORT											GPIO_PORTQ_BASE
#define PER_3V3_PIN														GPIO_PIN_2
#define PER_3V3_SET()													ROM_GPIOPinWrite(PER_3V3_GPIO_PORT, PER_3V3_PIN, PER_3V3_PIN)
#define PER_3V3_RESET()												ROM_GPIOPinWrite(PER_3V3_GPIO_PORT, PER_3V3_PIN, 0x00)

/* Definition for 3V3 SD CARD ENABLE Pin */
#define SD_3V3_GPIO_CLK_ENABLE()							ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK)
#define SD_3V3_GPIO_PORT											GPIO_PORTK_BASE
#define SD_3V3_PIN														GPIO_PIN_4
#define SD_3V3_SET()													ROM_GPIOPinWrite(SD_3V3_GPIO_PORT, SD_3V3_PIN, SD_3V3_PIN)
#define SD_3V3_RESET()												ROM_GPIOPinWrite(SD_3V3_GPIO_PORT, SD_3V3_PIN, 0x00)

#define BTSW_PIN															GPIO_PIN_2

#define LED_DB_1_PER													SYSCTL_PERIPH_GPION
#define LED_DB_1_PORT													GPIO_PORTN_BASE
#define LED_DB_1_PIN													GPIO_PIN_0
#define LED_DB_1_ON()													ROM_GPIOPinWrite(LED_DB_1_PORT, LED_DB_1_PIN, LED_DB_1_PIN)
#define LED_DB_1_OFF()												ROM_GPIOPinWrite(LED_DB_1_PORT, LED_DB_1_PIN, 0x00)

#define LED_DB_2_PER													SYSCTL_PERIPH_GPION
#define LED_DB_2_PORT													GPIO_PORTN_BASE
#define LED_DB_2_PIN													GPIO_PIN_1
#define LED_DB_2_ON()													ROM_GPIOPinWrite(LED_DB_2_PORT, LED_DB_2_PIN, LED_DB_2_PIN)
#define LED_DB_2_OFF()												ROM_GPIOPinWrite(LED_DB_2_PORT, LED_DB_2_PIN, 0x00)

#define LED_ERR_PER														SYSCTL_PERIPH_GPION
#define LED_ERR_PORT													GPIO_PORTN_BASE
#define LED_ERR_PIN														GPIO_PIN_2
#define LED_ERR_ON()													ROM_GPIOPinWrite(LED_ERR_PORT, LED_ERR_PIN, LED_ERR_PIN)
#define LED_ERR_OFF()													ROM_GPIOPinWrite(LED_ERR_PORT, LED_ERR_PIN, 0x00)

#define LED_STT_PER														SYSCTL_PERIPH_GPION
#define LED_STT_PORT													GPIO_PORTN_BASE
#define LED_STT_PIN														GPIO_PIN_3
#define LED_STT_ON()													ROM_GPIOPinWrite(LED_STT_PORT, LED_STT_PIN, LED_STT_PIN)
#define LED_STT_OFF()													ROM_GPIOPinWrite(LED_STT_PORT, LED_STT_PIN, 0x00)

extern void v_HAL_MSP_Init(void);
extern void v_power_3v3_peripheral_control(bool enable);
extern void v_power_3v3_out_control(bool enable);
	
#endif
