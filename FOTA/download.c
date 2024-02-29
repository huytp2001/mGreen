/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file download.c
 * @author Danh Pham
 * @date 10 Nov 2020
 * @version: draft 2.0.0
 * @brief Functions to manage file download process 
 */ 

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "download.h"
#include "crc32.h"
#include "sd_manage_lib.h"
#include "sd_app.h"
#include "config.h"
#include "loader.h"
#include "hex_processing.h"
#include "params.h"
#include "board_config.h"
#include "mqtt_task.h"
#include "wdt.h"
#include "Storage.h"

#include "FreeRTOS.h"
#include "task.h"

#ifdef BOARD_FF_CONTROLLER_V4_3
#include "sdram_utils.h"
#endif

#define FW_UPDATE_HEX_CHUNK_SIZE        			(256)
#define FW_UPDATE_COUNT_DOWN_TO_RESET_PERIOD		(5000UL)	

/*
* Private variables
*/
static uint8_t pu8_line[FW_UPDATE_HEX_CHUNK_SIZE];		/** Use for firmware parser */
static uint32_t u32_file_len = 0;
static uint32_t u32_sum_sub_len = 0;
static uint32_t u32_crc;
#ifdef BOARD_MGREEN_NG_V5
static char c_file_name[25] = {0};
#endif

static bool b_fw_update_process(char *filename, uint32_t u32_len);
#ifdef BOARD_FF_CONTROLLER_V4_3		//use sdram to save download data
static uint8_t au8_download_data[SDRAM_DOWNLOAD_BUFFER_USED] __attribute__((at(SDRAM_DOWNLOAD_BUFFER_MAPPING_ADDRESS)));
#endif

/*!
* @fn b_prepare_for_download(void)
* @brief Prepare params before starting the downloading process.
* Must be called at begining of download process 
* @param c_file_name File name using to save to sdcard
* @return None
*/
bool b_prepare_for_download(char *c_file_name)
{
	bool b_ret = true;
	u32_file_len = 0;
	u32_sum_sub_len = 0;
	u32_crc = 0xFFFFFFFF;
	strcpy(c_file_name, c_file_name);
	#ifdef BOARD_MGREEN_NG_V5
	if(i8_sdcard_delete_file(c_file_name) != 0)
	{
		b_ret = false;
	}
	#endif
	return b_ret;
}

/*!
* @fn v_download_file_len_set(uint16_t u16_len)
* @brief Set the total len of requesting file. 
* This length will be compared with sum of sub length in download process to identify when download process is complete.
* @param u16_len Lengh of file
* @return None
*/
void v_download_file_len_set(uint32_t u32_len)
{
	u32_file_len = u32_len;
}

uint32_t u32_download_file_len_get(void)
{
	return u32_file_len;
}

uint32_t u32_sum_sub_len_get(void)
{
	return u32_sum_sub_len;
}

uint32_t u32_download_size_request(void)
{
	uint32_t u32_size_ret = 0;
	u32_size_ret = u32_file_len - u32_sum_sub_len;
	if (u32_size_ret > 512)
	{
		u32_size_ret = 512;
	}
	return u32_size_ret;
}

/*!
* @fn b_is_download_completed(void)
* @brief Answer the download process has completed or not
* @param None
* @return None
*/
bool b_is_download_completed(void)
{
	return (u32_file_len == u32_sum_sub_len);
}

/*!
* @fn u32_file_download_crc_get(void)
* @brief Get the CRC32 of downloaded file
* @param None
* @return None
*/
uint32_t u32_file_download_crc_get(void)
{
	return u32_crc ^= 0xFFFFFFFF;
}

/*!
* @fn v_download_data_append(uint8_t *pu8_data, uint16_t u16_len)
* @brief Append data to download memmory
* @param None
* @return None
*/
void v_download_data_append(uint8_t *pu8_data, uint16_t u16_len)
{
	#ifdef BOARD_FF_CONTROLLER_V4_3
	memcpy(au8_download_data + u32_sum_sub_len, pu8_data, u16_len);
	#endif
	#ifdef BOARD_MGREEN_NG_V5
	if (e_download_file_type_get() == FILE_TYPE_SCHEDULE)
	{
		if (i8_sdcard_save_file(CONFIG_SCHEDULE_FILENAME, pu8_data, u16_len) != 0)
		{
			v_download_file_fail();
		}
	}
	else if (e_download_file_type_get() == FILE_TYPE_THRESHOLD)
	{
		if (i8_sdcard_save_file(CONFIG_THRESHOLD_FILENAME, pu8_data, u16_len) != 0)
		{
			v_download_file_fail();
		}
	}
	else if (e_download_file_type_get() == FILE_TYPE_FIRMWARE)
	{
		if (i8_sdcard_save_file(CONFIG_FIRMWARE_FILENAME, pu8_data, u16_len) != 0)
		{
			v_download_file_fail();
		}
	}
	#endif
	u32_sum_sub_len += u16_len;
	u32_crc = u32_CRC32(u32_crc, pu8_data, u16_len);
}

/*!
* @fnbool b_save_file_process(E_DOWNLOAD_FILE_TYPE e_file_type)
* @brief Processing the data of downloaded file
* @param[in] e_file_type Type of file (schedule/threshold/firmware 
* @return True: process OK  
*					False: process false
*/
bool b_save_file_process(E_DOWNLOAD_FILE_TYPE e_file_type)
{
	bool b_result = true;
	switch(e_file_type)
	{
		case FILE_TYPE_SCHEDULE:
		{
//			if( b_sdcard_ready())
//			{
//				/* Remove old file */
//				i8_sdcard_delete_file(CONFIG_SCHEDULE_FILENAME);
//				/* Check CRC32 */
//				uint32_t u32_file_crc = 0xFFFFFFFF;
//				u32_file_crc = u32_CRC32(u32_file_crc, au8_download_data, u32_file_len);
//				u32_file_crc ^= 0xFFFFFFFF;
//				if(u32_file_crc == u32_crc)
//				{
//					if( i8_sdcard_save_file(CONFIG_SCHEDULE_FILENAME, au8_download_data, u32_file_len) != SD_OK)
//					{
//						b_result = false;
//					}
//					if( i8_sdcard_save_info(u32_crc, SD_SCHEDULE_CRC_ADDR) != SD_OK)
//					{
//						b_result = false;
//					}
//				}
//				else
//				{
//					b_result = false;
//				}
//			}
//			else
//			{
//				//todo: process when save sd card fail
//				//v_sdram_save_schedule_csv(au8_download_data, u32_file_len);
//				while (s32_params_set (PARAMS_ID_SCHEDULE_CSV_CRC, 4, (uint8_t *)&u32_crc) != 0)
//				{
//					vTaskDelay (2);
//				}
//			}
		}
		break;
		case FILE_TYPE_THRESHOLD:
		{
//			if( b_sdcard_ready())
//			{
//				/* Remove old file */
//				i8_sdcard_delete_file(CONFIG_THRESHOLD_FILENAME);
//				/* Check CRC32 */
//				uint32_t u32_CRC = 0xFFFFFFFF;
//				u32_CRC = u32_CRC32(u32_CRC, au8_download_data, u32_file_len);
//				u32_CRC ^= 0xFFFFFFFF;
//				if(u32_CRC == u32_crc)
//				{
//					if(i8_sdcard_save_file(CONFIG_THRESHOLD_FILENAME, au8_download_data, u32_file_len) != SD_OK)
//					{
//						b_result = false;
//					}
//					if( i8_sdcard_save_info(u32_crc, SD_THRESHOLD_CRC_ADDR) != SD_OK)
//					{
//						b_result = false;
//					}
//				}
//				else
//				{
//					b_result = false;
//				}
//			}
//			else
//			{
//				v_sdram_save_threshold_csv (au8_download_data, u32_file_len);
//				while (s32_params_set (PARAMS_ID_THRESHOLD_CSV_CRC, 4, (uint8_t *)&u32_crc) != 0)
//				{
//					vTaskDelay (2);
//				}
//			}
		}
		break;
		case FILE_TYPE_FIRMWARE:
		{
			b_result = b_fw_update_process( CONFIG_FIRMWARE_FILENAME, u32_file_len);
			if(b_result)
			{
				v_fota_status_set(FOTA_NEW_UPDATE, u32_file_len);
			}
		}
		break;
		default: break;
	}
	return b_result;
}
//#endif
//#ifdef BOARD_MGREEN_NG_V5
//bool b_save_file_process(E_DOWNLOAD_FILE_TYPE e_file_type)
//{
//	bool b_result = false;
//	/*TODO: write the body of function to process file */
//	return b_result;
//}
//#endif

/*!
* @fn v_fota_status_set(E_FOTA_UPDATE_STATUS e_status, uint32_t u32_CRC, uint32_t u32_firmware_length)
* @brief Set status of Firmware in storage.
* @param[in] e_status Status of firmware (FOTA_NEW_UPDATE/FOTA_NO_NEW_UPDATE
* @param[in] u32_CRC CRC32 from server
* @param[in] u32_firmware_length Length of file
* @return None
*/
void v_fota_status_set(E_FOTA_UPDATE_STATUS e_status, uint32_t u32_firmware_length)
{
	volatile Num32 num32;
	
	switch (e_status)
	{
		case FOTA_NEW_UPDATE:
		{	
			//Calculate CRC, check received firmware
			num32.n32 = u32_hex_crc_get();
			//Write FOTA CRC to parameter
			while (s32_params_set (PARAMS_ID_FOTA_FW_CRC, 4, (uint8_t *)&num32.n32) != 0)
			{
					vTaskDelay (2);
			}

			//Write FIRMWARE Length
			num32.n32 = u32_get_firmware_length();
			while (s32_params_set (PARAMS_ID_FOTA_FW_LENGTH, 4, (uint8_t *)&num32.n32) != 0)
			{
					vTaskDelay (2);
			}

			//Write FIRMWARE status
			num32.n8.n[0] = NEW_FIRMWARE_IS_TRUE;
			while (s32_params_set (PARAMS_ID_FOTA_STATUS, 1, (uint8_t *)&num32.n8.n[0]) != 0)
			{
					vTaskDelay (2);
			}
		}
		break;
		case FOTA_NO_NEW_UPDATE:
		{
			//Write FIRMWARE status
			num32.n8.n[0] = NEW_FIRMWARE_IS_FALSE;
			while (s32_params_set (PARAMS_ID_FOTA_STATUS, 1, (uint8_t *)&num32.n8.n[0]) != 0)
			{
					vTaskDelay (2);
			}
			break;
		}
		default: break;
	}
}

/*!
* @fn b_fw_update_process(uint8_t *pu8_data, uint32_t u32_len)
* @brief Parse .hex file to .bin file
* @param[in] pu8_data Data of file
* @param[in] u32_len Length of file
* @return True: parse OK  
*					False: parse false
*/
static bool b_fw_update_process(char *filename, uint32_t u32_len)
{
	bool b_result = false;
	uint32_t u32_current_pointer = 0;
	int8_t i8_return;
	/* Cleanr HEX record buffer */
	memset(pu8_line, 0x00, FW_UPDATE_HEX_CHUNK_SIZE);
	
	v_firmware_file_errase();
	
	for(uint32_t i = 0; i < u32_len; i++)	
	{	
		i16_read_one_line_sd_card(filename, i, pu8_line, 256);
		for (uint16_t j = 0; j < 256; j++)
		{
			if (pu8_line[j] == '\n')
			{
				i8_return = s32_LOADER_Process(pu8_line, j + 1);		
				switch(i8_return)
				{
					case LOADER_NO_ERR:
						break;
					case LOADER_SUCCESS:
						return true;
					default: 
						return false;
				}
				i += j;				
				break;
			}
		}
		v_watchdog_reload_counter();
	}
	return b_result;
}
