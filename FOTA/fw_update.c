#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "fw_update.h"
#include "params.h"
#include "eeprom_manage.h"

#include "sdram.h"
#include "loader.h"
#include "hex_processing.h"



#define FW_UPDATE_HEX_CHUNK_SIZE        			(256)
#define FW_UPDATE_COUNT_DOWN_TO_RESET_PERIOD		(5000UL)	

static uint8_t pu8_line[FW_UPDATE_HEX_CHUNK_SIZE];

static uint8_t 	au8_download_buffer[SDRAM_DOWNLOAD_BUFFER_USED] __attribute__((at(SDRAM_DOWNLOAD_BUFFER_MAPPING_ADDRESS)));
static bool 	b_fota_new_update = false;
static uint32_t u32_fota_fw_length = 0;
static uint32_t u32_fw_recv_length = 0;


bool fw_update_process(uint32_t u32_len)
{
	uint8_t *pu8_line = malloc(256);
	uint32_t i;
	uint32_t u32_current_pointer = 0;
	int8_t i8_return;
	for(i = 0; i < u32_len; i++)
	{
		if(au8_download_buffer[i] == '\n')
		{
			memcpy(pu8_line, &au8_download_buffer[u32_current_pointer], i + 1 - u32_current_pointer);		
			i8_return = s32_LOADER_Process(pu8_line, i + 1 - u32_current_pointer);
			switch(i8_return)
			{
				case LOADER_NO_ERR:
					break;
				case LOADER_SUCCESS:
					return true;
				default: return false;
			}
			u32_current_pointer = i+1;
		}
	}
	return false;
}


static void __fw_update_countdown_tmr_expired(void* arg)
{
    HWREG(NVIC_APINT) = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}

/* Public function */
void fw_update_init(void)
{
	/*Create count down timer */
	//IotClock_TimerCreate( &fw_update_timer, __fw_update_countdown_tmr_expired, NULL);
}

uint32_t fw_update_get_ext_mem_ptr(void)
{
	return (uint32_t)(&au8_download_buffer);
}


void HostComm_FOTA_UpdateStatus(ENM_FOTA_UPDATE_STATUS status, uint32_t u32_CRC, uint32_t u32_firmware_length)
{
	volatile Num32 num32;
	static uint8_t u8_retry = 0;
	
	switch (status)
	{
		case FOTA_NEW_UPDATE:
		{	
			//Calculate CRC, check received firmware
			num32.n32 = u32_hex_crc_get();
			//Write FOTA CRC to parameter
            while (s32_PARAMS_Set (PARAMS_ID_FOTA_FW_CRC, 4, (uint8_t *)&num32.n32) != 0)
            {
                vTaskDelay (2);
            }

			//Write FIRMWARE Length
			num32.n32 = u32_get_firmware_length();
            while (s32_PARAMS_Set (PARAMS_ID_FOTA_FW_LENGTH, 4, (uint8_t *)&num32.n32) != 0)
            {
                vTaskDelay (2);
            }

			//Write FIRMWARE status
			num32.n8.n[0] = NEW_FIRMWARE_IS_TRUE;
            while (s32_PARAMS_Set (PARAMS_ID_FOTA_STATUS, 1, (uint8_t *)&num32.n8.n[0]) != 0)
            {
                vTaskDelay (2);
            }
			
			b_fota_new_update = true;
			u8_retry = 0;
            uint32_t u32_temp = 0;
			taskENTER_CRITICAL();
			EEPROMProgram((uint32_t *)&u32_temp, EEPROM_CHECK_FOTA, sizeof(uint32_t));		//clear epprom
			taskEXIT_CRITICAL();
			//IotClock_TimerArm( &fw_update_timer, FW_UPDATE_COUNT_DOWN_TO_RESET_PERIOD, 0 );
		}
		break;
		case FOTA_NO_NEW_UPDATE:
			//Write FIRMWARE status
			num32.n8.n[0] = NEW_FIRMWARE_IS_FALSE;
            while (s32_PARAMS_Set (PARAMS_ID_FOTA_STATUS, 1, (uint8_t *)&num32.n8.n[0]) != 0)
            {
                vTaskDelay (2);
            }

			u8_retry = 0;
			break;
		case FOTA_APP_ERROR_USR:
			//Write FOTA status
			//Write FIRMWARE status
			return;
		default: break;
	}
}