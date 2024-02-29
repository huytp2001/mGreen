/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file sdram_utils.c
 * @author Danh Pham
 * @date 19 Nov 2020
 * @version: 1.0.0
 * @brief This file contains functions used to write and read file from sdram.
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <string.h>
 
 #include "sdram_utils.h"
 #include "csv.h"
 #include "crc32.h"
 
 #define SDRAM_CSV_MAX_ROW		(1000)
/*!
*	Variables declare
*/
static uint8_t* pu8_csv_schedule_data = (uint8_t *)SDRAM_CSV_SCHEDULE_ADDRESS; /**<Data contains csv of schedule */
static uint8_t* pu8_csv_threshold_data = (uint8_t *)SDRAM_CSV_THRESHOLD_ADDRESS; /**<Data contains csv of threshold */
/*!
*	Private functions prototype
*/
static bool b_sdram_csv_read_one_row(char *pc_buffer, uint16_t *pu16_start, uint8_t *pu8_data);
static int8_t i8_sdram_csv_parse_callback(uint8_t *pu8_data, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set);
/*!
*	Public functions
*/

/*!
*	@fn v_sdram_save_schedule_csv(uint8_t *pu8_data, uint32_t u32_len)
* @brief Save schedule csv file to sdram
* @param[in] pu8_data Data of file
* @param[in] u32_len Length of file
* @return None
*/
void v_sdram_save_schedule_csv(uint8_t *pu8_data, uint32_t u32_len)
{
	if(u32_len < (1024 * 1024))
	{
		memcpy(pu8_csv_schedule_data, pu8_data, u32_len * sizeof(uint8_t));
	}
}

/*!
*	@fn v_sdram_save_threshold_csv(uint8_t *pu8_data, uint32_t u32_len)
* @brief Save threshold csv file to sdram
* @param[in] pu8_data Data of file
* @param[in] u32_len Length of file
* @return None
*/
void v_sdram_save_threshold_csv(uint8_t *pu8_data, uint32_t u32_len)
{
	if(u32_len < (1024 * 1024))
	{
		memcpy(pu8_csv_threshold_data, pu8_data, u32_len * sizeof(uint8_t));
	}
}

/*!
* @fn i8_sdram_parse_schedule_csv(void (*cb1)(void *, size_t, void *),
*														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set)
* @brief Parse schedule csv data in sdram
* @param[in] cb1 Function to parse row of file
* @param[in] cb2 Function to parse feild in row
* @param[in] c Buffer of file
* @param[in] u32_crc_set CRC32 of data
* @return 0: read success
*					-1: can not init file
*					-2: file is not exist
*					-3: fail to parse file
*					-4: CRC error 
*/
int8_t i8_sdram_parse_schedule_csv(void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set)
{
	return (i8_sdram_csv_parse_callback(pu8_csv_schedule_data, cb1, cb2, c, u32_crc_set));
}

/*!
* @fn i8_sdram_parse_threshold_csv(void (*cb1)(void *, size_t, void *),
*														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set)
* @brief Parse threshold csv data in sdram
* @param[in] cb1 Function to parse row of file
* @param[in] cb2 Function to parse feild in row
* @param[in] c Buffer of file
* @param[in] u32_crc_set CRC32 of data
* @return 0: read success
*					-1: can not init file
*					-2: file is not exist
*					-3: fail to parse file
*					-4: CRC error 
*/
int8_t i8_sdram_parse_threshold_csv(void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set)
{
	return (i8_sdram_csv_parse_callback(pu8_csv_threshold_data, cb1, cb2, c, u32_crc_set));
}
/*!
*	Private functions
*/
/*!
*	@fn b_sdram_csv_read_one_row(char *pc_buffer, uint16_t *pu16_start, uint8_t *pu8_data)
* @brief Read one line csv from sdram (read until meet '\n' character
* @param[out] pc_buffer Buffer cotains return row
* @param[in][out] pu16_start Start point to read and return start point of next row
* @param[in] pu8_data Array contains csv data
* @return True: read ok
*					False: no row to read
*/
static bool b_sdram_csv_read_one_row(char *pc_buffer, uint16_t *pu16_start, uint8_t *pu8_data)
{
	bool b_result = true;
	uint16_t u16_len = 0;
	uint16_t u16_cursor = *pu16_start;
	/* find \n (0x0A) character */
	while(pu8_data[u16_cursor] != 0)
	{
		if(pu8_data[u16_cursor] != 0x0A)
		{
			u16_len++;
			u16_cursor++;
		}
		else
		{
			break;
		}
	}
	if(0 == u16_len)
	{
		b_result = false;
	}
	else
	{
		memcpy(pc_buffer, (pu8_data + *pu16_start) , u16_len);
		*pu16_start = u16_cursor + 1;
	}
	return b_result;
}

/*!
* @fn i8_sdram_csv_parse_callback(uint8_t *pu8_data, void (*cb1)(void *, size_t, void *),
*														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set)
* @brief Parse csv content of csv data in sdram
* @param[in] pu8_data CSV data
* @param[in] cb1 Function to parse row of file
* @param[in] cb2 Function to parse feild in row
* @param[in] c Buffer of file
* @param[in] u32_crc_set CRC32 of data
* @return 0: read success
*					-1: can not init file
*					-2: file is not exist
*					-3: fail to parse file
*					-4: CRC error 
*/
static int8_t i8_sdram_csv_parse_callback(uint8_t *pu8_data, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set)
{
	int8_t i8_result = 0;
	char str[CSV_BUFFER_LENGTH_READ_LINE] = {0};
	struct csv_parser p;
	uint32_t u32_crc = 0xFFFFFFFF;
	uint16_t u16_row_start_point = 0;
	
	if(csv_init(&p, CSV_APPEND_NULL) != 0)
	{
		i8_result = -1;
	}
	else
	{
		for(uint16_t i = 0; i < SDRAM_CSV_MAX_ROW; i++)
		{
			if(b_sdram_csv_read_one_row(str, &u16_row_start_point, pu8_data))
			{
				u32_crc = u32_CRC32(u32_crc, (const uint8_t *)str, strlen(str));
				if(str[0] != CSV_COMMENT_LINE_FIRST_CHAR && str[0] != CSV_QUOTE_CHAR)
				{
					if(csv_parse(&p, str, strlen(str), cb1, cb2, &c) != strlen(str))
					{
						i8_result = -3;
						break;
					}
					//clear buffer
					memset(str, 0, sizeof(str));
				}
			}
			else
			{
				break;
			}
		}
		if(0 == i8_result)
		{
			u32_crc ^= 0xFFFFFFFF;
			if(u32_crc != u32_crc_set)
			{
				i8_result = -4;
			}
			else
			{
				csv_fini(&p, cb1, cb2, &c);
				csv_free(&p);
			}
		}
	}
	return i8_result;
}
