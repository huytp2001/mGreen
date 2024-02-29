/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file sd_app.c
 * @author Danh Pham
 * @date 11 Nov 2020
 * @version: 2.0.0
 * @brief sd_card functions for application
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <string.h>
 #include "sd_manage_lib.h"
 #include "sd_app.h"
 #include "crc32.h"
 #include "error_handler.h"
 #include "Storage.h"
 #include "mqtt_publish.h"
 
 #include "FreeRTOS.h"
 #include "task.h"



/*!
*	Variables declare
*/
static uint8_t u8_sdcard_err_id = 0; /**<Id of sdcard error handler */
/*!
*	Private functions prototype
*/
static void v_sdcard_error_handler(void);
/*!
*	Public functions
*/

/*!
* @fn v_sdcard_init(void)
* @brief Init SD card
* @param None
* @return None
*/
void v_sdcard_init(void)
{
	vStorageHALInit();
	if(b_Storage_Is_Ready() == false)
	{
		b_error_handler_reg("sdcard_error", 20, v_sdcard_error_handler, &u8_sdcard_err_id);
	}
}

/*!
* @fn b_sdcard_ready(void)
* @brief Return status of SD card
* @param None
* @return true: SD card ok
*					false: SD card not ready
*/
bool b_sdcard_ready(void)
{
	return b_Storage_Is_Ready();
}

/*!
*	@fn i8_handle_csv_file_callback (char *txtname, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, bool b_check_crc
* @brief process a csv file
* @param[in] txtname name of file
* @param[in] cb1 function to parse row of file
* @param[in] cb2 function to parse feild in row
* @param[in] c buffer of file
* @param[in] b_check_crc Check CRC before read file or not
* @return 0: read success
*					-1: can not init file
*					-2: file is not exist
*					-3: fail to parse file
*					-4: CRC error
*/
int8_t i8_handle_csv_file_callback (char *txtname, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, bool b_check_crc)
{
	int8_t i8_result = 0;
	char str[CSV_BUFFER_LENGTH_READ_LINE] = {0};
	struct csv_parser p;
	uint32_t u32_crc = 0xFFFFFFFF;
	uint32_t u32_saved_crc = 0;
	if(b_check_crc)
	{
		if(i8_sdcard_read_value(SD_INFO_FILE, SD_SCHEDULE_CRC_ADDR, &u32_saved_crc) != SD_OK)
		{
			i8_result = -1;
		}
	}
	if(SD_OK == i8_result)
	{
		if(csv_init(&p, CSV_APPEND_NULL) != 0)
		{
			i8_result = -1;
		}
		else
		{
			while(i16_sd_take_semaphore(SD_TAKE_SEMAPHORE_TIMEOUT))
			{
				vTaskDelay(2);
			}
			if(i16_sd_open_file(txtname, READ_SD) != SD_OK)
			{
				i16_sd_give_semaphore();
				i8_result = -2;
			}
			else
			{
				for(int i = 0; i < CSV_MAX_ROW; i++)
				{
					if(sd_read_one_line(str, CSV_BUFFER_LENGTH_READ_LINE) != NULL)
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
					else							//no data read (eof or error)
					{
						break;
					}
				}
				if(SD_OK == i8_result)
				{
					u32_crc ^= 0xFFFFFFFF;
					if(b_check_crc && (u32_crc != u32_saved_crc))
					{
						i16_sd_close_file();
						i16_sd_give_semaphore();
						i8_result = -4;
					}
					else
					{
						csv_fini(&p, cb1, cb2, &c);
						csv_free(&p);
					}
				}
				i16_sd_close_file();
				i16_sd_give_semaphore();
			}
		}
	}
	return i8_result;					
}

/*!
*	@fn i8_sdcard_save_schedule_version(char* version)
* @brief save version of schedule file
* @param[in] version version of schedule file
* @return 0: read success
*					-2: can not write
*/
int8_t i8_sdcard_save_schedule_version (char* version)
{
	return (i8_sdcard_write_text(SD_INFO_FILE, SD_SCHEDULE_VER_ADDR, version));
}

/*!
* @fn i8_sdcard_save_info(uint32_t u32_data, uint16_t u16_addr)
* @brief Save a value to INFO file at an identified address
* @param[in] u32_data Data value
* @param[in] u16_addr Address 
* @return 0: read success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_save_info(uint32_t u32_data, uint16_t u16_addr)
{
	return (i8_sdcard_write_value(SD_INFO_FILE, u16_addr, u32_data));
}

/*!
* @fn i8_sdcard_read_info(uint32_t *u32_data, uint16_t u16_addr)
* @brief Read a value from INFO file at an identified address
* @param[in] u32_data Data value
* @param[in] u16_addr Address 
* @return 0: read success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_read_info(uint32_t *u32_data, uint16_t u16_addr)
{
	return (i8_sdcard_read_value(SD_INFO_FILE, u16_addr, u32_data));
}
/*!
*	@fn i8_sdcard_get_schedule_version(char* version)
* @brief get version of schedule file
* @param[out] version version of schedule file
* @return 0: read success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_get_schedule_version (char* version)
{
	return (i8_sdcard_read_text(SD_INFO_FILE, SD_SCHEDULE_VER_ADDR, version));
}

/*!
*	@fn i8_sdcard_save_mainflow(uint8_t u8_location_id, uint32_t u32_main_flow)
* @brief save flow rate of main flow meter for a given location
* @param[in] u8_location_id Id of given location
* @param[in] u32_main_flow flow rate (real value * 10) Ex: the real value is 10.5m3/h => save value: 105
* @return 0: read success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_save_mainflow (uint8_t u8_location_id, uint32_t u32_main_flow)
{
	int8_t i8_result = 0;
	i8_result = i8_sdcard_write_value(SD_FLOW_FILE, 32 * u8_location_id + SD_MAIN_FLOW_VALID_ADDR, 1);
	if(SD_OK == i8_result)
	{
		i8_result = (i8_sdcard_write_value(SD_FLOW_FILE, 32 * u8_location_id + SD_MAIN_FLOW_1_ADDR, u32_main_flow));
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_get_mainflow(uint8_t u8_location_id, uint32_t* pu32_main_flow)
* @brief get flow rate of main flow meter for a given location
* @param[in] u8_location_id Id of given location
* @param[out] pu32_main_flow flow rate (real value * 10) Ex: the real value is 10.5m3/h => save value: 105
* @return 0: read success
*					-1: file is not exist
*					-2: can not read
*/
int8_t i8_sdcard_get_mainflow(uint8_t u8_location_id, uint32_t* pu32_main_flow)
{
	int8_t i8_result = 0;
	static uint32_t u32_valid = 0;
	i8_result = i8_sdcard_read_value(SD_FLOW_FILE, 32 * u8_location_id + SD_MAIN_FLOW_VALID_ADDR, &u32_valid);
	if(SD_OK == i8_result)
	{
		if(1 == u32_valid)
		{
			i8_result = i8_sdcard_read_value(SD_FLOW_FILE, 32 * u8_location_id + SD_MAIN_FLOW_1_ADDR, pu32_main_flow);
		}
		else
		{
			i8_result = -2;
		}
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_save_channels_flow(uint8_t u8_location_id, double *d_channels_flow)
* @brief save flow rate of 5 fertilizer channels for a given location
* @param[in] Id of given location
* @param[in] pu16_channels_flow flow rate
* @return 0: read success
*					-1: file is not exist
*					-2: can not write
*/
int8_t i8_sdcard_save_channels_flow(uint8_t u8_location_id, uint32_t *u32_channels_flow)
{
	int8_t i8_result = 0;
	i8_result = i8_sdcard_write_value(SD_FLOW_FILE, 32 * u8_location_id + SD_CH_FLOW_VALID_ADDR, 1);
	if(SD_OK == i8_result)
	{
		i8_result = (i8_sdcard_write_data(SD_FLOW_FILE, 32 * u8_location_id + SD_CH1_FLOW_1_ADDR, u32_channels_flow, 5));
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_get_channels_flow(uint8_t u8_location_id, uint16_t *pu16_channels_flow)
* @brief get flow rate of 5 fertilizer channels flow meter for a given location
* @param[in] u8_location_id Id of given location
* @param[out] pu16_channels_flow flow rate
* @return 0: read success
*					-1: file is not exist
*					-2: can not read
*/
int8_t i8_sdcard_get_channels_flow(uint8_t u8_location_id, uint32_t *pu32_channels_flow)
{
	int8_t i8_result = 0;
	static uint32_t u32_valid = 0;
	static uint32_t au32_flow[5];
	i8_result = i8_sdcard_read_value(SD_FLOW_FILE, 32 * u8_location_id + SD_CH_FLOW_VALID_ADDR, &u32_valid);
	if(SD_OK == i8_result)
	{
		if(1 == u32_valid)
		{
			i8_result = i8_sdcard_read_data(SD_FLOW_FILE, 32 * u8_location_id + SD_CH1_FLOW_1_ADDR, au32_flow, 5);
		}
		else
		{
			i8_result = -2;
		}
	}
	if(SD_OK == i8_result)
	{
		for(uint8_t i = 0; i < 5; i++)
		{
			pu32_channels_flow[i] = au32_flow[i];
		}
	}
	return i8_result;
}

/*!
*	@fn i8_sdcard_save_schedule_order(uint16_t u16_order)
* @brief saves the order of the irrigated event
* @param[in] u16_order  
* 				0: No irrigated event yet  
*					from 1: irrigation event  
* @return 0: read success
*					-2: can not write
*/
int8_t i8_sdcard_save_schedule_order(uint16_t u16_order)
{
	return (i8_sdcard_write_value(SD_INFO_FILE, SD_SCHEDULE_ORDER_ADDR, u16_order));
}

/*!
*	@fn i8_sdcard_get_schedule_order(uint16_t* u16_order)
* @brief get the order of the irrigated event
* @param[out] u16_order id of last irrigated event:  
*								0: No irrigated event yet
*								form 1: last irrigated event
* @return 0: read success
*					-1: file is not exist
*					-2: can not read
*/
int8_t i8_sdcard_get_schedule_order(uint16_t* u16_order)
{
	int8_t i8_result = 0;
	i8_result = i8_sdcard_read_value(SD_INFO_FILE, 
									SD_SCHEDULE_ORDER_ADDR, (uint32_t *)u16_order);
	return i8_result;
}

int8_t i8_sdcard_save_fertigation_day(uint16_t u16_day)
{
	return (i8_sdcard_write_value(SD_INFO_FILE, SD_FERTIGATION_DAY_ADDR, u16_day));
}

int8_t i8_sdcard_get_fertigation_day(uint16_t* u16_day)
{
	int8_t i8_result = 0;
	i8_result = i8_sdcard_read_value(SD_INFO_FILE, 
									SD_FERTIGATION_DAY_ADDR, (uint32_t *)u16_day);
	return i8_result;
}

/*!
*	Private functions
*/

/*!
* @fn v_sdcard_error_handler(void)
* @brief Handle sdcard error
* @param None
* @return None
*/
static void v_sdcard_error_handler(void)
{
	if(b_Storage_Is_Ready())
	{
		b_error_unreg(u8_sdcard_err_id);
	}
	else
	{
		v_mqtt_noti_pub(DATAID_SD_LOST, TYPE_NOTI_SENSOR_LOST);
	}
}
