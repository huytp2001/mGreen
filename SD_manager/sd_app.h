/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.h
 * @author Danh Pham
 * @date 10 Nov 2020
 * @version: draft 2.0.0
 * @brief Template for doxygen automation document generate.
 * Add more detail desciption here
 */
 
 #ifndef _SD_APP_H_
 #define _SD_APP_H_
 
 #include <stdint.h>
 #include <stdio.h>
 #include <stdbool.h>

/*!
*	@def CSV_MAX_ROW			(1000)
* Maximum rows of csv file
*/
/*!
*	@def SD_INFO_FILE		"F1.BIN"
* File contain information of aplication
*/
/*!
*	@def SD_CSV_CRC_ADDR	(0)
* Address of CRC of shcedule file in SD card
*/
/*!
*	@def SD_SCHEDULE_VER_ADDR	(4)
* Address of version of shcedule file in SD card
*/
#define CSV_MAX_ROW			(1000)

#define SD_INFO_FILE		"F1.BIN"
#define SD_SCHEDULE_CRC_ADDR	(0)
#define SD_SCHEDULE_VER_ADDR	(4)
#define SD_SCHEDULE_ORDER_ADDR	(8)
#define SD_EC_THRESHOLD_ADDR		(12)
#define SD_PH_THRESHOLD_ADDR		(16)
#define SD_THRESHOLD_CRC_ADDR		(20)
#define SD_THRESHOLD_VER_ADDR		(24)
#define SD_FERTIGATION_DAY_ADDR (28)

#define SD_FLOW_FILE		"F2.BIN"
#define SD_MAIN_FLOW_VALID_ADDR	(0)
#define SD_CH_FLOW_VALID_ADDR		(4)
#define SD_MAIN_FLOW_1_ADDR			(8)
#define SD_CH1_FLOW_1_ADDR			(12)
#define SD_CH2_FLOW_1_ADDR			(16)
#define SD_CH3_FLOW_1_ADDR			(20)
#define SD_CH4_FLOW_1_ADDR			(24)
#define SD_CH5_FLOW_1_ADDR			(28)
 
/*! @struct STRU_CSV_COUNT
* Struct contain content of csv file
*/
typedef struct 
{
	uint16_t u16_fields;
	uint16_t u16_rows;
}STRU_CSV_COUNT;

/*!
*	Extern functions
*/
void v_sdcard_init(void);
extern bool b_sdcard_ready(void);
extern int8_t i8_handle_csv_file_callback (char *txtname, void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, bool b_check_crc);
extern int8_t i8_sdcard_save_schedule_version (char* version);
extern int8_t i8_sdcard_get_schedule_version (char* version);
extern int8_t i8_sdcard_save_mainflow (uint8_t u8_location_id, uint32_t u32_main_flow);
extern int8_t i8_sdcard_get_mainflow(uint8_t u8_location_id, uint32_t* pu32_main_flow);
extern int8_t i8_sdcard_save_channels_flow(uint8_t u8_location_id, uint32_t *u32_channels_flow);
extern int8_t i8_sdcard_get_channels_flow(uint8_t u8_location_id, uint32_t *pu32_channels_flow);
extern int8_t i8_sdcard_save_schedule_order(uint16_t u16_order);
extern int8_t i8_sdcard_get_schedule_order(uint16_t* u16_order);
extern int8_t i8_sdcard_save_info(uint32_t u32_data, uint16_t u16_addr);
extern int8_t i8_sdcard_read_info(uint32_t *u32_data, uint16_t u16_addr);
extern int8_t i8_sdcard_save_fertigation_day(uint16_t u16_day);
extern int8_t i8_sdcard_get_fertigation_day(uint16_t* u16_day);
#endif /* _SD_APP_H_ */

