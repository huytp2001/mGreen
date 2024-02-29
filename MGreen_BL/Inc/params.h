/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2014 Inteliss Ltd.
**   All Rights Reserved.
**
**   The information contained herein is copyrighted by and
**   is the sole property of Inteliss Ltd.
**   Any unauthorized use, copying, transmission, distribution
**   or disclosure of such information is strictly prohibited.
**
**   This Copyright notice shall not be removed or modified
**   without prior written consent of Inteliss Ltd.
**
**   Inteliss Ltd. reserves the right to modify this software
**   without notice.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : 
**   Project     : 
**   Description :
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

#ifndef __PARAMS_H__
#define __PARAMS_H__


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#include <stdint.h>
#include <stdbool.h>

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Main application version. */


typedef enum
{
	/* Params checking: 16 bytes */
	/* FW parameters. */
	PARAMS_ID_CHECKSUM                        = 0, // 4
	/* Other reserved parameters. */
	PARAMS_ID_CHECKING_RESERVED                  , // 12

	/* Version: 16 bytes */
	PARAMS_ID_VERSION_FW                         , // 4
	PARAMS_ID_VERSION_HW                         , // 4
	PARAMS_ID_VERSION_RESERVED                   , // 8

	/* Sensor Data Storage: 64 bytes */
	PARAMS_ID_DATA_STORAGE_START_ADDR            , // 4
	PARAMS_ID_STORAGE_RESERVED                   , // 60

    /* FOTA parameters - 32 bytes*/
    PARAMS_ID_FOTA_STATUS                        , // 1
    PARAMS_ID_FOTA_FW_LENGTH                     , // 4
    PARAMS_ID_FOTA_FW_CRC                        , // 4
    PARAMS_ID_FOTA_RESERVED                      , // 23
    
	/* Add PARAMS_ID here. */
	/* Remaining:  */

	PARAMS_NUM_OF_ITEMS
} ENM_PARAMS_ITEMS_T;

#define PARAMS_ID_MAX_LEN                       PARAMS_NUM_OF_ITEMS
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

extern int32_t s32_PARAMS_Init (void);

extern int32_t s32_PARAMS_Get (ENM_PARAMS_ITEMS_T enm_params_id,
                               uint8_t *pu8_params_data);

extern uint32_t u32_Get_FW_Version (void);
extern uint32_t u32_Get_HW_Version (void);

extern uint8_t u8_Data_Length_Of_One_Parameter (ENM_PARAMS_ITEMS_T enm_params_id);

#endif /* __PARAMS_H__ */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/*                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
