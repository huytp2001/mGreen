/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**� Copyright Camlin Technologies Limited, 2012. All rights reserved.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : loader.h
**   Project     : IMT - main CPU - bootloader.
**   Author      : Nguyen Anh Huy
**   Revision    : 1.0.0.1
**   Date        : 2012/01/30
**   Description : Header file for loader module. 
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   INCLUDE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#define LOADER_NO_ERR                       (0)
#define LOADER_SUCCESS                      (1)

#define LOADER_OPEN_FILE_ERR                (-1)
#define LOADER_HEX_DATA_ERR									(-2)

#define BOOTLOADER_ADDRESS									0x8000000
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   PROTOTYPE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

int32_t s32_LOADER_Process (uint8_t *line_data, uint8_t line_len);

extern uint32_t Jump2Bootloader(void);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   FUNCTION SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   END
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/







