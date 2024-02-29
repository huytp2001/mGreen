/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file sd_manage_lib.c
 * @author Danh Pham
 * @date 11 Nov 2020
 * @version: 2.0.0
 * @brief Contain functions to manage read and write sd card action.
 * 
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 
 #include "sd_manage_lib.h"
 #include "ff.h"
 #include "HAL_BSP.h"
 
 #include "FreeRTOS.h"
 #include "task.h"
/*!
*	Variables declare
*/

/*!
*	Private functions prototype
*/

/*!
*	Public functions
*/
/*!
*	@fn int8_t i8_sdcard_check_exist_file (char *filename)
* @brief Check a file is exist or not
* @param[in] filename name of file
* @return 0: file is exist
*					-1: file is not exist
*/
int8_t i8_sdcard_check_exist_file (char *filename)
{
	int8_t i8_result = 0;
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, READ_SD) != SD_OK)
	{
		i8_result = -1; /**<file is not exist */
	}
	else
	{
		i16_sd_close_file();
	}
	i16_sd_give_semaphore();
	return i8_result;
}
/*!
*	@fn int8_t i8_sdcard_delete_file(char *filename)
* @brief Delete a file
* @param[in] filename name of file
* @return 0: delete success or file is not exist
*					-2: can not delete
*/
int8_t i8_sdcard_delete_file(char *filename)
{
	int8_t i8_result = 0;
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, READ_SD) != SD_OK)
	{
		i8_result = 0;	/**<file is not exist */
	}
	else
	{
		i16_sd_close_file();
		if(sd_remove_storage_file(filename) != SD_OK)
		{
			i8_result = -2;
		}
	}
	i16_sd_give_semaphore();
	return i8_result;
}

/*!
*	@fn i8_sdcard_write_value(char *filename, uint32_t u32_addr, int32_t i32_value)
* @brief Write a value to the given address in file
* @param[in] filename name of file
* @param[in] u32_addr address to write, the address must divisible by 4
* @param[in] i32_value value to write
* @return 0: write success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_write_value(char *filename, uint32_t u32_addr, int32_t i32_value)
{
	int8_t i8_result = 0;
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, WRITE_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;	/**<file is not exist */
	}
	else
	{
		if(i16_sd_write_data((char*)&i32_value, sizeof(uint32_t), u32_addr) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_write_data(char *filename, uint32_t u32_addr, uint32_t *u32_data, uint32_t u32_len)
* @brief Write a array of data to the given address in file
* @param[in] filename name of file
* @param[in] u32_addr address to write, the address must divisible by 4
* @param[in] u32_data data array to write
* @param[in] u32_len lenght of array
* @return 0: write success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_write_data(char *filename, 
														uint32_t u32_addr, uint32_t *u32_data, uint32_t u32_len)
{
	int8_t i8_result = 0;
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, WRITE_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;	/**<file is not exist */
	}
	else
	{
		if(i16_sd_write_data((char *)u32_data, u32_len * sizeof(uint32_t), u32_addr) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	return i8_result;
}

/*!
*	@fn int8_t i8_sdcard_write_text(char *filename, uint32_t u32_addr, char* c_text)
* @brief Write a string data the given address in file
* @param[in] filename name of file
* @param[in] u32_addr address to write
* @param[in] c_text string write
* @return 0: write success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_write_text(char *filename, uint32_t u32_addr, char* c_text)
{
	int8_t i8_result = 0;
	uint8_t u8_len = strlen(c_text);
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, WRITE_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;
	}
	else
	{
		if(i16_sd_write_data((char *)c_text, u8_len + 1, u32_addr) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_read_value(char *filename, uint32_t u32_addr, uint32_t* pu32_value)
* @brief read a value from sd card
* @param[in] filename name of file
* @param[in] u32_addr address to read
* @param[out] pu32_value buffer of value
* @return 0: read success
*					-1: file is not exist
*					-2: can not read
*/
int8_t i8_sdcard_read_value(char *filename, uint32_t u32_addr, uint32_t* pu32_value)
{
	int8_t i8_result = 0;
	static uint8_t au8_data[4] = {0, 0, 0, 0};
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, READ_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;
	}
	else
	{
		if(i16_sd_read_bytes(u32_addr, au8_data, sizeof(uint32_t)) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	if(SD_OK == i8_result)
	{
		memcpy(pu32_value, &au8_data, sizeof(uint32_t));
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_read_data(char *filename, uint32_t u32_addr, uint32_t* pu32_data, uint32_t u32_len)
* @brief read an array of data from sd card
* @param[in] filename name of file
* @param[in] u32_addr address to read
* @param[out] pu32_data buffer of data
* @param[in] u32_len length of data
* @return 0: read success
*					-1: file is not exist
*					-2: can not read
*/
int8_t i8_sdcard_read_data(char *filename, 
													uint32_t u32_addr, uint32_t* pu32_data, uint32_t u32_len)
{
	int8_t i8_result = 0;
	static uint8_t *pu8_data;
	pu8_data = (uint8_t *)calloc(0, u32_len * sizeof(uint32_t));
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, READ_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;
	}
	else
	{
		if(i16_sd_read_bytes(u32_addr, pu8_data, u32_len * sizeof(uint32_t)) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	if(SD_OK == i8_result)
	{
		memcpy(pu32_data, pu8_data, u32_len * sizeof(uint32_t));
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_read_text(char *filename, uint32_t u32_addr, char* c_text)
* @brief read an array of data from sd card
* @param[in] filename name of file
* @param[in] u32_addr address to read
* @param[out] c_text buffer of text string
* @return 0: read success
*					-1: file is not exist
*					-2: can not read
*/
int8_t i8_sdcard_read_text(char *filename, uint32_t u32_addr, char* c_text)
{
	int8_t i8_result = 0;
	static char c_temptext[MAX_SD_TEXT_LEN];
	memset(c_temptext, 0, MAX_SD_TEXT_LEN * sizeof(char));
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, READ_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;
	}
	else
	{
		if(i16_sd_read_bytes(u32_addr, (uint8_t *)c_temptext, MAX_SD_TEXT_LEN) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	if(SD_OK == i8_result)
	{
		memcpy(c_text, &c_temptext, strlen(c_temptext) + 1);
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_save_file(char* filename, const uint8_t pu8_data, uint32_t u32_len)
* @brief save file to sd card
* @param[in] filename name of file
* @param[in] pu8_data data of file
* @param[in] u32_len lenght of file
* @return 0: write success
*					-2: can not write
*/
int8_t i8_sdcard_save_file(char* filename, const uint8_t *pu8_data, uint32_t u32_len)
{
	int8_t i8_result = 0;
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, WRITE_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;
	}
	else
	{
		if(i16_sd_write_bytes((char *)pu8_data, u32_len) != SD_OK)
		{
			i8_result = -2;
		}
		i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	return i8_result;
}

int8_t i8_sdcard_save_file_apart(char* filename, const uint8_t *pu8_data, uint32_t u32_len, uint32_t u32_address)
{
	int8_t i8_result = 0;
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, WRITE_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		i8_result = -1;
	}
	else
	{
		if(i16_sd_write_data((char *)pu8_data, u32_len, u32_address) != SD_OK)
		{
			i8_result = -2;
		}
		//i16_sd_close_file();
		i16_sd_give_semaphore();
	}
	return i8_result;
}

int i16_read_one_line_sd_card(char *filename, uint32_t u32_offset, uint8_t *buffer, int len)
{
	while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(2);
	}
	if(i16_sd_open_file(filename, READ_SD) != SD_OK)
	{
		i16_sd_give_semaphore();
		return false;
	}
	if(i16_sd_read_one_line(u32_offset,(char *)buffer, len) != 0)
	{
		i16_sd_close_file();
		i16_sd_give_semaphore();
		return false;
	}	
	i16_sd_close_file();
	i16_sd_give_semaphore();
	return true;	
}
/*!
*	Private functions
*/

