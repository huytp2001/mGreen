/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2013 - 2014.
**   All Rights Reserved.
**   Developed by: PIF Lab Ltd.
**
**   The information contained herein is copyrighted by and
**   is the sole property of PIF.
**   Any unauthorized use, copying, transmission, distribution
**   or disclosure of such information is strictly prohibited.
**
**   This Copyright notice shall not be removed or modified
**   without prior written consent of PIF.
**
**   PIF reserves the right to modify this software
**   without notice.
**
**   This software is developed by PIF Lab Ltd.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : params.c
**   Project     : MGreen
**   Description : Read/Write a parameter value to flash.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#include <stdint.h>
#include <string.h>

#include "params.h"
#include "crc.h"
#include "rom.h"
#include "sysctl.h"
#include "eeprom.h"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#define MAX_NUM_QUEUE_DATA       			32
#define PARAMS_BUF_SIZE               3072

typedef struct
{
   ENM_PARAMS_ITEMS_T   enm_params_id;
   uint8_t              u8_params_len;
   uint8_t              au8_params_data[MAX_NUM_QUEUE_DATA];
} STRU_PARAMS_MSG_T;


#define PARAMS_QUEUE_WAIT              10//1000
/* Maximum items in received PARAMS queue. */
#define PARAMS_QUEUE_LEN               (25)
/* Size in bytes of each PARAMS queue item. */
#define PARAMS_MSG_SIZE                (sizeof(STRU_PARAMS_MSG_T))

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

const static uint8_t au8_params_id_len[PARAMS_ID_MAX_LEN] = {
   /* Params checking: 16 bytes */
   /* FW parameters. */
   4, // PARAMS_ID_CHECKSUM                        = 0, // 4
   /* Other reserved parameters. */
   12, // PARAMS_ID_CHECKING_RESERVED                  , // 12
   
   /* Version: 16 bytes */
   4, // PARAMS_ID_VERSION_FW                         , // 4
   4, // PARAMS_ID_VERSION_HW                         , // 4
   8, // PARAMS_ID_VERSION_RESERVED                   , // 8

	/* Sensor Data Storage: 64 bytes */
	4,	//PARAMS_ID_DATA_STORAGE_START_ADDR            , // 4
	60,	//PARAMS_ID_STORAGE_RESERVED                   , // 60

    /* FOTA parameters - 32 bytes*/
    1,  //PARAMS_ID_FOTA_STATUS                        , // 1
    4,  //PARAMS_ID_FOTA_FW_LENGTH                     , // 4
    4,  //PARAMS_ID_FOTA_FW_CRC                        , // 4
    23, //PARAMS_ID_FOTA_RESERVED                      , // 23
    
   /* Add PARAMS_ID here. */
};

static uint8_t au8_params_buf[PARAMS_BUF_SIZE]; // BUFFER_SIZE = 3KB
static uint32_t u32_current_fw_version;
static uint32_t u32_current_hw_version;

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/* Check CRC32 of main and backup params pages. */
static int32_t s32_Check_CRC_Params_Page (void);
static int32_t s32_Cal_CRC32_Buffer (uint32_t *pu32_crc32, uint8_t u8_write_action);
static uint32_t u32_Get_Pos_Of_Params_ID_On_Buf (ENM_PARAMS_ITEMS_T enm_params_id);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           FUNCTIONS SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

static uint32_t u32_Get_Pos_Of_Params_ID_On_Buf (ENM_PARAMS_ITEMS_T enm_params_id)
{
   static uint16_t u8_idx;
   static uint32_t u32_pos_of_params_id;
   
   u32_pos_of_params_id  = 0;
   for (u8_idx = 0; u8_idx < enm_params_id; u8_idx++)
   {
      u32_pos_of_params_id += au8_params_id_len[u8_idx];
   }
   
   return (u32_pos_of_params_id);
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: uint8_t u8_Data_Length_Of_One_Parameter (ENM_PARAMS_ITEMS_T enm_params_id)
**      
**   Arguments: (in) enm_params_id: params id
**
**   Return: unsigned char value: data length of one parameter.
**
**   Description: 
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
uint8_t u8_Data_Length_Of_One_Parameter (ENM_PARAMS_ITEMS_T enm_params_id)
{
   return (au8_params_id_len[enm_params_id]);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: void v_PARAMS_Init (void)
**      
**   Arguments: n/a
**
**   Return: n/a
**
**   Description: Check CRC of params pages.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_PARAMS_Init (void)
{
	/* TODO: */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);	//Enable EEPROM
	ROM_EEPROMInit();

	/* Check CRC32 of params page. */
	if (s32_Check_CRC_Params_Page() != 0)
	{
	}

	return (0);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: int32_t s32_PARAMS_Set (ENM_PARAMS_ITEMS_T enm_params_id,
**                                     uint32_t u8_params_len,
**                                     uint8_t *pu8_params_data)
**      
**   Arguments:
**             (in) enm_params_id      : param index
**             (in) u8_params_len      : data length of params
**             (in) *pu8_params_data   : data pointer of params
**
**   Return: integer value
**
**   Description: Set a parameter value to flash.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_PARAMS_Set (ENM_PARAMS_ITEMS_T enm_params_id,
                        uint8_t u8_params_len,
                        uint8_t *pu8_params_data)
{
    return (-1);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: int32_t s32_PARAMS_Get (ENM_PARAMS_ITEMS_T enm_params_id,
**                                     uint8_t *pu8_params_data)
**      
**   Arguments:
**             (in) enm_params_id      : param index
**             (in) *pu8_params_data   : data pointer of params
**
**   Return: integer value
**
**   Description: Get a parameter value from flash.
**
**   Notes:
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_PARAMS_Get (ENM_PARAMS_ITEMS_T enm_params_id,
                        uint8_t *pu8_params_data)
{
	static uint32_t u32_byte_addr = 0;
	static uint32_t u8_params_len = 0;

	if (enm_params_id < PARAMS_NUM_OF_ITEMS)
	{
		u32_byte_addr = u32_Get_Pos_Of_Params_ID_On_Buf (enm_params_id);

		u8_params_len = au8_params_id_len[enm_params_id];

		memcpy(pu8_params_data, &au8_params_buf[u32_byte_addr], u8_params_len);
		return (0);
	}
	else
	{
		return (-1);
	}
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function    : int32_t s32_Cal_CRC32_Buffer (uint32_t *pu32_crc32,
**                                               uint8_t u8_write_action)
**
**   Arguments   : (out) *pu32_crc32      : pointer of calculated crc32 value.
**                 (in)  u8_write_action  : buffer to write or be read
**
**   Return      : crc32 value
**
**   Description : Calculate crc32 for all params page.
**
**   Notes       :
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static int32_t s32_Cal_CRC32_Buffer (uint32_t *pu32_crc32, uint8_t u8_write_action)
{
   uint32_t u32_byte_idx_in_buf = 0;
   
   *pu32_crc32 = 0xffffffff;
   
   switch (u8_write_action)
   {
   case 0:
      {
         *pu32_crc32 = u32_CRC32 (*pu32_crc32, (const uint8_t *)(au8_params_buf + 4), (PARAMS_BUF_SIZE - 4));
      }
      break;
      
   case 1:
      {
         for (u32_byte_idx_in_buf = 4; u32_byte_idx_in_buf < PARAMS_BUF_SIZE;)
         {
            *pu32_crc32 = u32_CRC32 (*pu32_crc32, (const uint8_t*)(au8_params_buf + u32_byte_idx_in_buf), 4);
            u32_byte_idx_in_buf = (u32_byte_idx_in_buf + 4);
         }
      }
      break;
      
   default:
      break;
   }
   
   *pu32_crc32 = *pu32_crc32 ^ 0xffffffff;
   return (0);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function    : void v_Check_CRC_Params_Page (void)
**
**   Arguments   : n/a
**
**   Return      : n/a
**
**   Description : Check CRC32 of main and backup params pages.
**
**   Notes       :
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
int32_t s32_Check_CRC_Params_Page (void)
{
	static uint32_t u32_crc;
	static uint32_t u32_store_crc;
	
	EEPROMRead((uint32_t *)au8_params_buf, 0, PARAMS_BUF_SIZE);

	u32_crc = 0xFFFFFFFF;
	s32_Cal_CRC32_Buffer(&u32_crc, 1);

	s32_PARAMS_Get (PARAMS_ID_CHECKSUM, (uint8_t *)&u32_store_crc);
	
	if (u32_crc != u32_store_crc)
	{
		EEPROMRead((uint32_t *)au8_params_buf, PARAMS_BUF_SIZE, PARAMS_BUF_SIZE);

		u32_crc = 0xFFFFFFFF;
		s32_Cal_CRC32_Buffer(&u32_crc, 1);

		s32_PARAMS_Get (PARAMS_ID_CHECKSUM, (uint8_t *)&u32_store_crc);
        
		if (u32_crc != u32_store_crc)
			return -1;
	}

	return (0);
}

uint32_t u32_Get_FW_Version (void)
{
   return (u32_current_fw_version);
}

uint32_t u32_Get_HW_Version (void)
{
   return (u32_current_hw_version);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
