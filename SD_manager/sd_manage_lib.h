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
 
 #ifndef _SD_MANAGE_LIB_
 #define _SD_MANAGE_LIB_
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #include "ff.h"
 #include "sd_card.h"
 #include "csv.h"

/*!
*	@def SD_TAKE_SEMAPHORE_TIMEOUT	(5)
* Time to wait semaphore
*/
/*!
*	@def WRITE_SD	(0)
* Macro to write SD card
*/
/*!
*	@def READ_SD	(1)
* Macro to read SD card
*/
/*!
*	@def SD_OK		(0)
* Result of read/write process
*/
/*!
*	@def MAX_SD_TEXT_LEN						(30)
* Maximum length of text to save/load from sd card
*/

#define SD_TAKE_SEMAPHORE_TIMEOUT	(5)
#define WRITE_SD									(0)
#define READ_SD										(1)
#define SD_OK											(0)
#define MAX_SD_TEXT_LEN						(30)

/*!
*	Extern functions
*/
extern int8_t i8_sdcard_check_exist_file (char *filename);
extern int8_t i8_sdcard_delete_file(char *filename);
extern int8_t i8_sdcard_write_value(char *filename, uint32_t u32_addr, int32_t i32_value);
extern int8_t i8_sdcard_write_data(char *filename, 
																	uint32_t u32_addr, uint32_t *u32_data, uint32_t u32_len);
extern int8_t i8_sdcard_write_text(char *filename, uint32_t u32_addr, char* c_text);
extern int8_t i8_sdcard_read_value(char *filename, uint32_t u32_addr, uint32_t* pu32_value);
extern int8_t i8_sdcard_read_data(char *filename, 
																	uint32_t u32_addr, uint32_t* pu32_data, uint32_t u32_len);
extern int8_t i8_sdcard_read_text(char *filename, uint32_t u32_addr, char* c_text);
extern int8_t i8_sdcard_save_file(char* filename, const uint8_t *pu8_data, uint32_t u32_len);
extern int8_t i8_sdcard_save_file_apart(char* filename, const uint8_t *pu8_data, uint32_t u32_len, uint32_t u32_address);
int i16_read_one_line_sd_card(char *filename, uint32_t u32_offset, uint8_t *buffer, int len);
#endif /* _SD_MANAGE_LIB_ */

