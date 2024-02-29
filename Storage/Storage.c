/* Standard includes. */
#include <stdio.h>
#include <stdint.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "Storage.h"
#include "ff.h"
#include "diskio.h"
#include "sd_card.h"
#include "HAL_BSP.h"
//#include "params.h"
#include "sd_app.h"

/* STORAGE task stack size. */
#define STORAGE_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE * 16)
/* STORAGE task priority (lowest priority except Idle Task). */
#define STORAGE_TASK_PRIORITY             (tskIDLE_PRIORITY + 1)
/* STORAGE task delay. */
#define STORAGE_TASK_DELAY                (portTickType)(5000 / portTICK_RATE_MS)

#define STORAGE_QUEUE_LEN                 30
#define STORAGE_MSG_SIZE               		(sizeof(T_STORAGE_SENSOR_TYPE))

/*
 * Storage Task.
 */


static bool b_sdcard_is_ready = false;

void vStorageTaskInit(void)
{
  vStorageHALInit();
}

void vStorageHALInit(void)
{
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)));
	
	ROM_GPIOPinConfigure(GPIO_PF3_SSI3CLK);
	ROM_GPIOPinConfigure(GPIO_PF1_SSI3XDAT0);
//	ROM_GPIOPinConfigure(GPIO_PF3_SSI3FSS);
	ROM_GPIOPinConfigure(GPIO_PF0_SSI3XDAT1);
	ROM_GPIOPinTypeSSI(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
	ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
		
	//v_power_3v3_peripheral_control(true);
	
	v_ff_init();
	
	if (i16_sd_init() == 0)
	{
		b_sdcard_is_ready = true;
		LED_ERR_OFF();
	}	
	else
	{
		b_sdcard_is_ready = false;
		LED_ERR_ON();
	}
}

int32_t i32_Storage_Write_Firmware_File(char *p_data, uint32_t u32_data_len)
{
	while (i16_sd_take_semaphore(10))
	{
		vTaskDelay(2);
	}
	
	//Open file for writing
	if (i16_sd_open_file(STORAGE_FIMWARE_FILE, 0) != 0)
	{
		i16_sd_give_semaphore();
        return -1;
	}
	
	//write data to log file
	i16_sd_write_bytes((char *)p_data, u32_data_len);
	
	//Close file to save data
	i16_sd_close_file();
	
	//Release semaphore for another tasks
	i16_sd_give_semaphore();
    
    return 0;
}

bool b_Storage_Is_Ready(void)
{
    return b_sdcard_is_ready;
}

void v_firmware_file_errase(void)
{
	sd_remove_storage_file(STORAGE_FIMWARE_FILE);
}
