/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2014 PIF Lab Ltd.
**   All Rights Reserved.
**
**   The information contained herein is copyrighted by and
**   is the sole property of PIF Lab Ltd.
**   Any unauthorized use, copying, transmission, distribution
**   or disclosure of such information is strictly prohibited.
**
**   This Copyright notice shall not be removed or modified
**   without prior written consent of PIF Lab Ltd.
**
**   PIF Lab Ltd. reserves the right to modify this software
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

#ifndef __CAN_PROCESS_H__
#define __CAN_PROCESS_H__


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#include "can_parser.h"
#include "can_api.h"


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
typedef struct
{
   uint8_t u8_evt_id;
   uint8_t u8_sensor_addr;
   uint8_t u8_data_len;
   uint8_t au8_data[8];
} STRU_CANPROC_T;

typedef struct
{
   uint32_t u32_timeout;
   STRU_CANPROC_T stru_canproc;
   CANAPI_Evt_CB f_evt_cb;
} STRU_CANPROC_REQUEST_T;


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
void v_CANPROC_Init (void);

void v_CANPROC_LPicked_Evt (STRU_CANPSR_MSG_T *pstru_canpsr_msg);

int32_t s32_CANPROC_Send_Request (STRU_CANPROC_REQUEST_T *pstru_canproc_request);
int32_t s32_CANPROC_Send_Response (STRU_CANPROC_T *pstru_canproc_res);

int32_t s32_CANPROC_Send_Request_To_CANPSR (STRU_CANPROC_REQUEST_T *pstru_canproc_request);


#endif /* __CAN_PROCESS_H__ */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
