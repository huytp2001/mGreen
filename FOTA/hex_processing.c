/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**� Copyright Camlin Technologies Limited, 2012. All rights reserved.
**
** *  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : hex_processing.c
**   Project     : IMT - MainCPU - Main Application
**   Author      : Nguyen Anh Huy
**   Version     : 1.0.0.1
**   Date        : 2012/11/22.
**   Description : Hex Processing - this module contains functions to process the
**                 Intel/Keil hex record and write to flash memory.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   INCLUDE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Standard includes. */
#include <string.h>
#include <stdint.h>

/* firmware data includes. */
#include "hex_processing.h"
#include "crc32.h"
#include "sdram.h"
#include "Storage.h"



//*****************************************************************************
//
// The size of the flash for this microcontroller.
//
//*****************************************************************************
#define CRC_ADDRESS											0x0FFFF4
#define FLASH_SIZE 							(1 * 1024 * 1024)
#define FLASH_START_ADDRESS            	    0x000000
#define BOOT_LOADER_ADDRESS            	    FLASH_START_ADDRESS
#define APPLICATION_ADDRESS            	    0x010000
#define MAX_APP_ADDRESS            			0x0FFFF0

/* Hex record offset. */
#define HEX_REC_DATA_LEN_OFFSET        1
#define HEX_REC_HIGH_ADDR_OFFSET       2
#define HEX_REC_LOW_ADDR_OFFSET        3
#define HEX_REC_TYPE_OFFSET            4
#define HEX_REC_DATA_OFFSET            5

/* Total bytes of hex record header. */
#define HEX_REC_HEADER_SIZE            6
                        
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   PROTOTYPE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static uint32_t s32_Cal_CRC32(uint32_t u32_crc, uint8_t *buff, uint32_t length_data);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   VARIABLE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
static uint8_t au8_fw_bin[FLASH_SIZE-APPLICATION_ADDRESS] 			__attribute__((at(SDRAM_FOTA_DATA_MAPPING_ADDRESS)));
/* Firmware Binary data */

/* Address to write firmware data to. */
static uint32_t u32_hex_rec_full_addr = 0;

/*
** Extended flash address.
*/
static uint32_t u32_hex_rec_extd_addr = 0;

/* First hex record data. */
static uint8_t au8_end_data[HEX_REC_DATA_LEN_MAX];

/* First hex record data length. */
//static uint8_t u8_first_data_len;

/* First adress. */
static uint32_t u32_first_addr;

/* Hex data buffer. */
static uint8_t u8_hex_rec_data[HEX_REC_DATA_LEN_MAX];

/* Hex data buffer to write to external flash */
static uint8_t au8_hex_rec_ex[256];
static uint32_t u32_hex_rec_ex_count = 0;

static uint32_t u32_crc = 0xFFFFFFFF;
static uint32_t u32_crc_hex = 0;

static uint32_t u32_hex_total_len = 0;

static uint8_t b_is_recv_crc = 0;
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   FUNCTION SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/**
  * @brief  This function calculates CRC
  * @retval crc32 value
  */
static uint32_t s32_Cal_CRC32(uint32_t u32_crc, uint8_t *buff, uint32_t length_data)
{
	return (u32_CRC32(u32_crc, (const uint8_t *) buff, length_data));
}	


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function    :
**      ENUM_HEX_PROC_RES enum_Hex_Processing (ENUM_FLASH_TYPE enum_flash_type,
**                                             uint8_t* pu8_rec_hex_buff,
**                                             uint8_t u8_hex_buff_len)
**
**   Arguments   :
**      (in) enum_flash_type    -   Flash type to write hex data.
**      (in) pu8_rec_hex_buff   -   Hex buffer to process.
**      (in) u8_hex_buff_len    -   Hex buffer length.
**
**   Return      :
**      HEX_PROC_OK          -   Process the hex buffer sucessully.
**      HEX_PROC_BAD         -   Hex buffer is corrupted or not supported.
**      HEX_PROC_COMPELETED  -   A end of file hex record has been received.
**
**   Description : This function called by the loader-process function to
**                 process a received hex buffer.
**
**   Notes       :
**
**   Author      : Nguyen Anh Huy.
**
**   Date        : 2012/11/24
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
ENUM_HEX_PROC_RES enum_Hex_Processing (ENUM_FLASH_TYPE enum_flash_type, uint8_t* pu8_rec_hex_buff,  uint8_t u8_hex_buff_len)
{
   /* Hex record type.*/
   uint8_t u8_hex_rec_type;
   /* Hex record address.*/
   uint16_t u16_hex_rec_addr;
   /* Hex record data length. */
   uint8_t u8_hex_rec_data_len;
   /* Hex record check sum. */
   uint8_t u8_hex_rec_checksum;
   /* Maximum flash address. */
   uint32_t u32_flash_addr_max;
   /* Check summation result. */
   uint8_t u8_checksum_res = 0;
   /* Check summation index. */
   uint8_t u8_idx = 0; 
   /* Local variables for internal processing.*/
   uint8_t *pu8_hex_buff = pu8_rec_hex_buff;
   uint8_t u8_hex_len = u8_hex_buff_len;
   /* Hex processing result. */
   ENUM_HEX_PROC_RES enm_res = HEX_PROC_OK;
   uint32_t u32_crc_hex_calc = 0;

   switch (enum_flash_type)
   {
       /* SPI flash. */
      case SPI_FLASH:
		   u32_first_addr = APPLICATION_ADDRESS;
		   u32_flash_addr_max = FLASH_START_ADDRESS + FLASH_SIZE;
      break;
	   /* internal flash. */
      case INTERNAL_FLASH:
		   u32_first_addr = APPLICATION_ADDRESS;
		   u32_flash_addr_max = FLASH_START_ADDRESS + FLASH_SIZE;
      break;
			case RAM_MEMORY:
		   u32_first_addr = APPLICATION_ADDRESS;
		   u32_flash_addr_max = FLASH_START_ADDRESS + FLASH_SIZE;
			break;
   }
   
    /* Get hex record type. */
   u8_hex_rec_type = *(pu8_hex_buff + HEX_REC_TYPE_OFFSET);

   /* Get hex record address. */
   u16_hex_rec_addr = ((uint16_t)(*(pu8_hex_buff + HEX_REC_HIGH_ADDR_OFFSET)) << 8) +
                                   (*(pu8_hex_buff + HEX_REC_LOW_ADDR_OFFSET));

   /* Get hex record data length. */
   u8_hex_rec_data_len = *(pu8_hex_buff + HEX_REC_DATA_LEN_OFFSET);

   /* Get hex record check sum. */
   u8_hex_rec_checksum = *(pu8_hex_buff + u8_hex_rec_data_len + HEX_REC_DATA_OFFSET);

   /* Get  hex record  data. */
   memcpy(u8_hex_rec_data, pu8_hex_buff + HEX_REC_DATA_OFFSET,
          u8_hex_rec_data_len);

   /* Validate hex record. */
   if ((*pu8_hex_buff) != 0x3A) 
   {
      return (HEX_PROC_BAD);
   }
   
   /* Validate hex buffer length. */
   if (u8_hex_buff_len < (HEX_REC_HEADER_SIZE + u8_hex_rec_data_len))
   {
      return (HEX_PROC_BAD);
   }
   
   /* Validate checksum. */
   for (u8_idx = 0; u8_idx < u8_hex_len - 2; u8_idx++)
   {
      u8_checksum_res += *(pu8_hex_buff + u8_idx + 1);
   }
   u8_checksum_res ^= 0xFF;
   u8_checksum_res += 0x01;
   if (u8_checksum_res !=  u8_hex_rec_checksum)
   {
      return (HEX_PROC_BAD);
   }

   /* Process each record in the hex buffer. */
   while (u8_hex_len > 0)
   {
      /* Hex record processing state machine. */
      switch (u8_hex_rec_type)
      {
         /*
         ** Record type = 4 - Extended linear address.
         ** Set the upper linear address to new value.
         */
         case HEX_REC_EXTD_LINEAR_ADDR:
            u32_hex_rec_extd_addr = ((uint32_t)(u8_hex_rec_data[0]) << 8) + (uint32_t)(u8_hex_rec_data[1]);

         break;

          /*
           ** Record type = 0 - Data.
           ** Save data to the flash page.
           */
         case HEX_REC_DATA:

            /* Get address of hex record data. */
            u32_hex_rec_full_addr = (u32_hex_rec_extd_addr << 16) + (uint32_t)(u16_hex_rec_addr);
            /*
            ** If the address is beyond the maximum flash size of device, return error.
            */
            if ((u32_hex_rec_full_addr >= u32_flash_addr_max) || (u32_hex_rec_full_addr < u32_first_addr))
            {
               enm_res = HEX_PROC_BAD;
            }
            else
            {  
                if (u32_hex_rec_full_addr == MAX_APP_ADDRESS)
                {
                    memset(au8_end_data, 0, sizeof(au8_end_data));
                    memcpy(au8_end_data, u8_hex_rec_data, u8_hex_rec_data_len);
                    u32_hex_total_len += 8;
                    b_is_recv_crc = 1;
                }
                else
                {
                    u32_crc = s32_Cal_CRC32(u32_crc, u8_hex_rec_data, u8_hex_rec_data_len);
                }
								switch (enum_flash_type)
								{
										 /*  SPI flash. */
										 case SPI_FLASH:
										 break;
										 /* internal flash. */
										 case INTERNAL_FLASH:
												 /* Write firmware data to flash. */
										 break;
										 case RAM_MEMORY:
											 i32_Storage_Write_Firmware_File((char *)u8_hex_rec_data, u8_hex_rec_data_len);											 
										 break;
								}      
            }
						
        break;

         /*
         ** Record type = 1 - End of file.
         ** Set the flag to finish firmware update.
         */
         case HEX_REC_END:
            /*
            ** We have transferred all the firmware data.
            ** Write the first hex record data to flash.
            */
            u32_hex_rec_full_addr = u32_first_addr;

            if (!b_is_recv_crc)
                return HEX_PROC_BAD;
            
            u32_crc_hex = ((uint32_t)(u8_hex_rec_data[7] << 24) | (uint32_t)(u8_hex_rec_data[6] << 16) | (uint32_t)(u8_hex_rec_data[5] << 8) | (uint32_t)u8_hex_rec_data[4]);

            u32_crc_hex_calc = s32_Cal_CRC32(u32_crc, au8_end_data, 4);
            u32_crc_hex_calc ^= 0xffffffff;
            
            if (u32_crc_hex_calc != u32_crc_hex)
            {
                //Error
                return HEX_PROC_BAD;
            }
            
						
            switch (enum_flash_type)
            {
                 /* Spi flash. */
                 case SPI_FLASH:
                 break;
                 /* internal flash. */
                 case INTERNAL_FLASH:
                        /* Write firmware data to flash. */
                 break;
                 case RAM_MEMORY:
                     break;
								 default:
									 break;
             } 
            /* Return hex transfer finished. */
            enm_res = HEX_PROC_COMPELETED;
         break;

         /*
         ** Record type = 5 - Start linear address.
         */
         case HEX_REC_EXTD_LINEAR_START:
            u32_hex_rec_full_addr = ((uint32_t)(u8_hex_rec_data[0]) << 24) +((uint32_t)(u8_hex_rec_data[1]) << 16) 
                                    +((uint32_t)(u8_hex_rec_data[2]) << 8) + (uint32_t)(u8_hex_rec_data[3]);
         break;
         default:
            enm_res = HEX_PROC_BAD;
         break;
      }

      /*
      ** Stop processing hex buffer if the result is bad
      ** or received end-of-file record.
      */
      if (enm_res != HEX_PROC_OK)
      {
         break;
      }

      /* If the hex record is processed correctly, move to the next one. */
      u8_hex_len = u8_hex_len - (u8_hex_rec_data_len + HEX_REC_HEADER_SIZE);
      pu8_hex_buff = pu8_hex_buff + (u8_hex_rec_data_len + HEX_REC_HEADER_SIZE);
   }

   /* Return the result. */
   return (enm_res);
}

uint32_t u32_hex_crc_get(void)
{
	return u32_crc;
}

uint32_t u32_get_firmware_length(void)
{
	return u32_hex_total_len;
}
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   END
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
