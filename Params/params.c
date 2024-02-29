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

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <queue.h>

#include "driverlib.h"
#include "params.h"
#include "crc32.h"
#include "wdt.h"


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
	
    1, 	//PARAMS_ID_PORTS_CTR_STATE										 , // 1
		
    /* System parameters - 128 bytes*/
		2, // PARAMS_ID_MQTT_MESSAGE_ID, /**<Message ID for MQTT (2 bytes) */
		126, // PARAMS_ID_SYSTEM_RESERVED, /**<Reserver for System params (126 bytes) */
		
		4, //PARAMS_ID_SCHEDULE_CSV_CRC, /**<CRC of schedule csv file */
		4, //PARAMS_ID_THRESHOLD_CSV_CRC, /**<CRC of threshold csv file */
		1, //PARAMS_ID_MACHINE_STATE,		/**<State of machine */		
		1, //PARAMS_ID_SETTING_VALID,	/**<Seting is avalid (1) or not (otherwise) */
		4, //PARAMS_ID_TOPIC,					/**<MQTT Topic (4bytes) */
		8, //PARAMS_ID_EC_CALIB_VALUES, /**<Contain calibration values of EC sensor 
																		/*(8bytes - 2 check valid bytes + 3 x2bytes points)*/
		8, //PARAMS_ID_PH_CALIB_VALUES, /**<Contain calibration values of pH sensor 
																		/*(8bytes - 2 check valid bytes + 3 x2bytes points)*/
		1, //PARAMS_ID_USE_RF,		/**<Use RF (1) or not (0) */
		1, //PARAMS_ID_LORA, 		/**<Use LoRa (1) or short range (0) */
		1, //PARAMS_ID_FERTI_FLOW_TYPE,	/**<Type of fertilizer flowmeter */
		1, //PARAMS_ID_OPERATING_MODE
		1, //PARAMS_ID_FLOW_RATE_DEFAULT, /** L/m*10 */
		1, //PARAMS_ID_NORMAL_FLOWMTER_TYPE, /**(0): FS300A - (1):FS400A - (2)OF05ZAT*/
		1, //PARAMS_ID_TANK1_DIVISION,
		1, //PARAMS_ID_TANK2_DIVISION,
		1, //		PARAMS_ID_USE_MAIN_FLOW, /**<Use main flow (1) or not (0) */
		1, //		PARAMS_ID_FLOWMETER_COUNT, /**<Number of pulses to calculates flow rate*/
		4, //		PARAMS_ID_FLOWMETER_FACTOR, /**<Factor of main flowmeter (*1000) */
		1, //		PARAMS_ID_PRESSURE_TIME,
		1, //	PARAMS_ID_FLOWMETER_TIME,
   /* Add PARAMS_ID here. */
};

static uint8_t au8_params_buf[PARAMS_BUF_SIZE]; // BUFFER_SIZE = 3KB
static xQueueHandle x_params_msg_queue;
static uint32_t u32_current_fw_version;
static uint32_t u32_current_hw_version;

static bool b_params_is_processing = false;



StaticQueue_t xParamMsgQueueBuffer;
uint8_t ucParamMsgQueueStorage[PARAMS_QUEUE_LEN*PARAMS_MSG_SIZE];

static StaticTask_t xParams_TaskBuffer;
static StackType_t  xParams_Stack[configMINIMAL_STACK_SIZE * 16];

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           PROTOTYPES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/* Check CRC32 of main and backup params pages. */
static int32_t s32_Check_CRC_Params_Page (void);
static int32_t s32_Cal_CRC32_Buffer (uint32_t *pu32_crc32, uint8_t u8_write_action);
static void v_params_Task (void *pvParameters);
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
int32_t s32_params_init (void)
{
	/* */
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);	//Enable EEPROM
	ROM_EEPROMInit();

	/* Check CRC32 of params page. */
	if (s32_Check_CRC_Params_Page() != 0)
	{
		v_params_reset();
	}


	x_params_msg_queue = xQueueCreateStatic(PARAMS_QUEUE_LEN, 
																					PARAMS_MSG_SIZE, 
																					&ucParamMsgQueueStorage[0],
																					&xParamMsgQueueBuffer);

	/* Create params task. */
	xTaskCreateStatic(v_params_Task,
							"PARAMS",
							(configMINIMAL_STACK_SIZE * 16),
							NULL,
							(tskIDLE_PRIORITY + 3),
							xParams_Stack,
							&xParams_TaskBuffer);
	// vTaskSuspend(paramTaskHandle);

	/* Get version. */
	/*
	while (s32_params_get (PARAMS_ID_VERSION_FW, (uint8_t *)&u32_current_fw_version) != 0)
	{
		vTaskDelay (2);
	}
	while (s32_params_get (PARAMS_ID_VERSION_HW, (uint8_t *)&u32_current_hw_version) != 0)
	{
		vTaskDelay (2);
	}

	// Update main application. 
	switch (u32_current_fw_version)
	{
		case 0:
		case 0xffffffff:
		break;
		case (PARM_MAINAPP_VERSION_DEFAULT):
		break;
		default:
			u32_current_fw_version = PARM_MAINAPP_VERSION_DEFAULT;
			// Get version. 
			while (s32_params_set (PARAMS_ID_VERSION_FW, 4, (uint8_t *)&u32_current_fw_version) != 0)
			{
				vTaskDelay (2);
			}
		break;
	}
	*/
	return (0);
}

static void v_params_Task (void *pvParameters)
{
	static STRU_PARAMS_MSG_T stru_params_msg;
	static uint32_t u32_byte_index, u32_idx;
	static uint32_t u32_crc;
	static uint8_t u8_params_task_id = 0;
	
	/* Get wdt task id */
	while(b_wdt_reg_new_task("params_task", &u8_params_task_id) != true){}
	for (;;)
	{
		/* Reload watchdog*/
		b_wdt_task_reload_counter(u8_params_task_id);
		/* Get params queue. */
		if (xQueueReceive (x_params_msg_queue, &stru_params_msg, PARAMS_QUEUE_WAIT))
		{
			b_params_is_processing = true;

			s32_Check_CRC_Params_Page();
			
			/*
			** Main param page.
			*/
			u32_byte_index = u32_Get_Pos_Of_Params_ID_On_Buf (stru_params_msg.enm_params_id);
         
			/* Modify params buffer. */
			for (u32_idx = 0; u32_idx < stru_params_msg.u8_params_len; u32_idx++)
			{
				au8_params_buf[u32_byte_index + u32_idx] = stru_params_msg.au8_params_data[u32_idx];
			}

			/* Calculate CRC32. */
			u32_crc = 0xffffffff;
			s32_Cal_CRC32_Buffer (&u32_crc, 1);
			au8_params_buf[0] = (uint8_t)(u32_crc);
			au8_params_buf[1] = (uint8_t)(u32_crc >> 8);
			au8_params_buf[2] = (uint8_t)(u32_crc >> 16);
			au8_params_buf[3] = (uint8_t)(u32_crc >> 24);

			EEPROMProgram((uint32_t *)au8_params_buf, 0, PARAMS_BUF_SIZE);
			EEPROMProgram((uint32_t *)au8_params_buf, PARAMS_BUF_SIZE, PARAMS_BUF_SIZE);
			
			b_params_is_processing = false;
		}
	}
}


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: int32_t s32_params_set (ENM_PARAMS_ITEMS_T enm_params_id,
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
int32_t s32_params_set (ENM_PARAMS_ITEMS_T enm_params_id,
                        uint8_t u8_params_len,
                        uint8_t *pu8_params_data)
{
   static STRU_PARAMS_MSG_T stru_params_msg;
   
   if ((enm_params_id < PARAMS_NUM_OF_ITEMS) && (u8_params_len <= au8_params_id_len[enm_params_id]))
   {
      /* Copy. */
      stru_params_msg.enm_params_id = enm_params_id;
      stru_params_msg.u8_params_len = u8_params_len;
      memcpy (stru_params_msg.au8_params_data, pu8_params_data, u8_params_len);
      /* Send params queue. */
      if (xQueueSend (x_params_msg_queue, &stru_params_msg, PARAMS_QUEUE_WAIT))
      {
         return (0);
      }
      else
      {
         return (-1);
      }
   }
   else
   {
      return (-1);
   }
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function: int32_t s32_params_get (ENM_PARAMS_ITEMS_T enm_params_id,
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
int32_t s32_params_get (ENM_PARAMS_ITEMS_T enm_params_id,
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
**   Function    : void v_PARAMS_Reset (void)
**
**   Arguments   : n/a
**
**   Return      : n/a
**
**   Description : Reset parameters to default values.
**
**   Notes       :
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
void v_params_reset (void)
{
	uint32_t u32_crc;
	static uint32_t u32_byte_index, u32_value;
	
	/* Modify params buffer. */
	u32_byte_index = u32_Get_Pos_Of_Params_ID_On_Buf (PARAMS_ID_VERSION_FW);
	u32_value = PARM_MAINAPP_VERSION_DEFAULT;
	au8_params_buf[u32_byte_index + 0] = u32_value;
	au8_params_buf[u32_byte_index + 1] = u32_value >> 8;
	au8_params_buf[u32_byte_index + 2] = u32_value >> 16;
	au8_params_buf[u32_byte_index + 3] = u32_value >> 24;
	
	
	u32_byte_index = u32_Get_Pos_Of_Params_ID_On_Buf (PARAMS_ID_VERSION_HW);
	u32_value = HARDWARE_VERSION_DEFAULT;
	au8_params_buf[u32_byte_index + 0] = u32_value;
	au8_params_buf[u32_byte_index + 1] = u32_value >> 8;
	au8_params_buf[u32_byte_index + 2] = u32_value >> 16;
	au8_params_buf[u32_byte_index + 3] = u32_value >> 24;

	/* Calculate CRC32. */
	u32_crc = 0xffffffff;
	s32_Cal_CRC32_Buffer (&u32_crc, 1);
	au8_params_buf[0] = (uint8_t)(u32_crc);
	au8_params_buf[1] = (uint8_t)(u32_crc >> 8);
	au8_params_buf[2] = (uint8_t)(u32_crc >> 16);
	au8_params_buf[3] = (uint8_t)(u32_crc >> 24);

	EEPROMProgram((uint32_t *)au8_params_buf, 0, PARAMS_BUF_SIZE);
	EEPROMProgram((uint32_t *)au8_params_buf, PARAMS_BUF_SIZE, PARAMS_BUF_SIZE);
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

	while (s32_params_get (PARAMS_ID_CHECKSUM, (uint8_t *)&u32_store_crc) != 0)
	{
		vTaskDelay (2);
	}
	
	if (u32_crc != u32_store_crc)
	{
		EEPROMRead((uint32_t *)au8_params_buf, PARAMS_BUF_SIZE, PARAMS_BUF_SIZE);

		u32_crc = 0xFFFFFFFF;
		s32_Cal_CRC32_Buffer(&u32_crc, 1);

		while (s32_params_get (PARAMS_ID_CHECKSUM, (uint8_t *)&u32_store_crc) != 0)
		{
			vTaskDelay (2);
		}
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

uint32_t u32_params_queue_wait(void)
{
	volatile uint32_t u32_queue_wait = 0;
	
	u32_queue_wait = (uint32_t)uxQueueMessagesWaiting(x_params_msg_queue);
	
	if ((b_params_is_processing == true) && (u32_queue_wait == 0))
	{
		u32_queue_wait = 1;
	}
	
	return (u32_queue_wait);
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
