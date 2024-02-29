/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file download.h
 * @author Danh Pham
 * @date 23 Nov 2020
 * @version: 1.0.0
 * @brief Params to manage file download process 
 */
 
#ifndef DOWNLOAD_H
#define DOWNLOAD_H
#include "stdint.h"
#include "stdbool.h"

typedef union
{
  uint32_t n32;
  struct 
  {
    uint8_t n[4]; 
  }n8;
  struct 
  {
    uint16_t n[2]; 
  }n16;
  struct 
  {
    uint32_t bit0:1;
    uint32_t bit1:1;
    uint32_t bit2:1;
    uint32_t bit3:1;
    uint32_t bit4:1;
    uint32_t bit5:1;
    uint32_t bit6:1;
    uint32_t bit7:1;
    uint32_t bit8:1;
    uint32_t bit9:1;
    uint32_t bit10:1;
    uint32_t bit11:1;
    uint32_t bit12:1;
    uint32_t bit13:1;
    uint32_t bit14:1;
    uint32_t bit15:1;
    uint32_t bit16:1;
    uint32_t bit17:1;
    uint32_t bit18:1;
    uint32_t bit19:1;
    uint32_t bit20:1;
    uint32_t bit21:1;
    uint32_t bit22:1;
    uint32_t bit23:1;
    uint32_t bit24:1;
    uint32_t bit25:1;
    uint32_t bit26:1;
    uint32_t bit27:1;
    uint32_t bit28:1;
    uint32_t bit29:1;
    uint32_t bit30:1;
    uint32_t bit31:1;
  }bits;
}Num32;

/*!
 * @enum E_DOWNLOAD_FILE_TYPE
 * Types of download file
 */
 typedef enum
 {
	 FILE_TYPE_SCHEDULE = 0, /**<CSV file for schedule */
	 FILE_TYPE_THRESHOLD = 1, /**<CSV file for threshold */
	 FILE_TYPE_FIRMWARE = 2, /**<Hex file for firmware update */
	 FILE_TYPE_MAX,	/**<Maximum of file type */ 
 }E_DOWNLOAD_FILE_TYPE;
 
 /*!
 * @enum E_FOTA_UPDATE_STATUS
 * Status of firmware update request
 */
 typedef enum
{
	FOTA_NO_NEW_UPDATE = 0,		/**<No new firmware available */
	FOTA_NEW_UPDATE						/**<New firmware available */
} E_FOTA_UPDATE_STATUS;

 /*!
 * @enum E_FOTA_FIRMWARE_STATUS
 * Status of firmware
 */
typedef enum
{
	NEW_FIRMWARE_IS_CHECKING		= 0x00,		/**<Checking firmware */
	NEW_FIRMWARE_IS_FALSE 			= 0x01,		/**<Checking false */
	NEW_FIRMWARE_IS_TRUE 			= 0x02			/**<Check done without error */
}E_FOTA_FIRMWARE_STATUS;


extern void v_fota_status_set(E_FOTA_UPDATE_STATUS e_status, uint32_t u32_firmware_length);
extern bool b_save_file_process(E_DOWNLOAD_FILE_TYPE e_file_type);
extern bool b_prepare_for_download(char *c_file_name);
extern void v_download_file_len_set(uint32_t u32_len);
extern bool b_is_download_completed(void);
extern uint32_t u32_file_download_crc_get(void);
extern void v_download_data_append(uint8_t *pu8_data, uint16_t u16_len);
uint32_t u32_download_file_len_get(void);
uint32_t u32_sum_sub_len_get(void);
uint32_t u32_download_size_request(void);
#endif
