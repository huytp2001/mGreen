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

#ifndef __CAN_API_H__
#define __CAN_API_H__


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           INCLUDES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#include "stdint.h"


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
#define CANAPI_RESULT_TIMEOUT    (-1)
#define CANAPI_RESULT_OK         0
#define CANAPI_RESULT_OTHER_ER   1


typedef void (*CANAPI_Evt_CB) (int32_t s32_status, uint8_t *pu8_data, uint8_t u8_data_len);

typedef struct
{
   uint8_t u8_sensor_addr;
   uint8_t u8_data_len;
   uint8_t au8_data[8];
   CANAPI_Evt_CB f_res_cb;
} STRU_CANAPI_T;


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           VARIABLES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_CANAPI_Evt_Init (void);

void v_CAN_Reg_LPicked_Evt_CB (CANAPI_Evt_CB v_callback_function);

int32_t s32_CANAPI_Request_Time (STRU_CANAPI_T *pstru_canapi);

int32_t s32_CANAPI_Query_Discovery (uint8_t u8_dest_addr);

int32_t s32_CANAPI_Set_Addr (STRU_CANAPI_T *pstru_canapi);

int32_t s32_CANAPI_Set_Date_Time (STRU_CANAPI_T *pstru_canapi);

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/



#endif /* __CAN_API_H__ */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
