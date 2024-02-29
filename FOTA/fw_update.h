/*-------------------------------------------------------------------------*
 * File:  FOTA.h
 *-------------------------------------------------------------------------*
 * Description:
 *-------------------------------------------------------------------------*/
#ifndef _FOTA_H_
#define _FOTA_H_

#include <stdint.h>
#include <stdbool.h>

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

typedef enum
{
	FOTA_NO_NEW_UPDATE = 0,
	FOTA_NEW_UPDATE,
	FOTA_UPDATE_ERROR,
	FOTA_APP_IS_VALID,
	FOTA_APP_ERROR_USR,
	FOTA_FTP_CLOSED
} ENM_FOTA_UPDATE_STATUS;


typedef enum
{
	NEW_FIRMWARE_IS_CHECKING		= 0x00,
	NEW_FIRMWARE_IS_FALSE 			= 0x01,
	NEW_FIRMWARE_IS_TRUE 			= 0x02
}FOTA_FIRMWARE_STATUS;

void     fw_update_init(void);
uint32_t fw_update_get_ext_mem_ptr(void);
uint32_t fw_update_get_ext_mem_ptr_limit(void);
bool     fw_update_process(uint32_t fw_len);
void HostComm_FOTA_UpdateStatus(ENM_FOTA_UPDATE_STATUS status, uint32_t u32_CRC, uint32_t u32_firmware_length);

#endif
