/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**� Copyright Camlin Technologies Limited, 2012. All rights reserved.
**
** *  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : hex_processing.h
**   Project     : IMT - MainCPU - Main Application
**   Author      : Nguyen Anh Huy
**   Version     : 1.0.0.1
**   Date        : 2012/11/22.
**   Description : Header file of Hex Processing module.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifndef __HEX_PROCESSING_H__
#define __HEX_PROCESSING_H__


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   INCLUDE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   DEFINE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Maximum data length for each hex record. */
#define HEX_REC_DATA_LEN_MAX                256

/* Define Hex record types. */
#define HEX_REC_DATA                        0
#define HEX_REC_END                         1
#define HEX_REC_SEG_ADD                     2
#define HEX_REC_EXTD_SEG_START              3
#define HEX_REC_EXTD_LINEAR_ADDR            4
#define HEX_REC_EXTD_LINEAR_START           5


/* Flash type. */
typedef enum
{
   SPI_FLASH,
   INTERNAL_FLASH,
   RAM_MEMORY
}
ENUM_FLASH_TYPE;

/* Hex processing result. */
typedef enum
{
   HEX_PROC_OK         = 0,
   HEX_PROC_BAD        = 1,
   HEX_PROC_COMPELETED = 2
}
ENUM_HEX_PROC_RES;


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   PROTOTYPE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Function to process the hex buffer received via CAN communication. */
ENUM_HEX_PROC_RES enum_Hex_Processing (ENUM_FLASH_TYPE enum_flash_type, uint8_t* pu8_rec_hex_buff,  uint8_t u8_hex_buff_len);
uint32_t u32_hex_crc_get(void);
uint32_t u32_get_firmware_length(void);
extern void v_Init_Hex_Process(void);

extern int8_t FOTA_Read_App_Status(void);
extern int8_t FOTA_Write_App_Status(uint8_t status);
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   VARIABLE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


#endif

/* 
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   END
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
