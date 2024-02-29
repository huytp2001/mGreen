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
#include "driverlib.h"

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           DEFINES SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Main application version. */
#define MAINCPU_MAINAPP_MAJOR_VERSION                 1
#define MAINCPU_MAINAPP_MINOR_VERSION                 1
#define MAINCPU_MAINAPP_BUILD_VERSION                 3
#define MAINCPU_MAINAPP_SVN_REVISION                  17

#define PARM_MAINAPP_VERSION_DEFAULT                  (((uint32_t)MAINCPU_MAINAPP_MAJOR_VERSION << 24) | \
                                                       ((uint32_t)MAINCPU_MAINAPP_MINOR_VERSION << 16) | \
                                                       ((uint32_t)MAINCPU_MAINAPP_BUILD_VERSION << 8)  | \
                                                       ((uint32_t)MAINCPU_MAINAPP_SVN_REVISION))

#define HARDWARE_MAJOR_VERSION                 				2
#define HARDWARE_MINOR_VERSION                 				3
#define HARDWARE_BUILD_VERSION                 				4
#define HARDWARE_SVN_REVISION                  				1

#define HARDWARE_VERSION_DEFAULT                  			(((uint32_t)HARDWARE_MAJOR_VERSION << 24) | \
                                                       ((uint32_t)HARDWARE_MINOR_VERSION << 16) | \
                                                       ((uint32_t)HARDWARE_BUILD_VERSION << 8)  | \
                                                       ((uint32_t)HARDWARE_SVN_REVISION))
typedef enum
{
	/* Params checking: 16 bytes */
	/* FW parameters. */
	PARAMS_ID_CHECKSUM = 0,  /**<Check sum of params's memmory (4 bytes) */ 
	/* Other reserved parameters. */
	PARAMS_ID_CHECKING_RESERVED, /**< Reserver for checking (12 bytes) */

	/* Version: 16 bytes */
	PARAMS_ID_VERSION_FW, /**<Firmware version (not used) (4 bytes) */
	PARAMS_ID_VERSION_HW, /**<Hardware version (not used) (4 bytes) */
	PARAMS_ID_VERSION_RESERVED, /**<Reserver for version (8 bytes)*/

	/* Sensor Data Storage: 64 bytes */
	PARAMS_ID_DATA_STORAGE_START_ADDR, /**<Start address of storage (4 bytes) */
	PARAMS_ID_STORAGE_RESERVED, /**<Reserver for storage (60 bytes) */

	/* FOTA parameters - 32 bytes*/
	PARAMS_ID_FOTA_STATUS, /**<FOTA status (1 byte) */
	PARAMS_ID_FOTA_FW_LENGTH, /**<Lenght of firmware (4 bytes)*/
	PARAMS_ID_FOTA_FW_CRC, /**<Firmware CRC (4 bytes)*/
	PARAMS_ID_FOTA_RESERVED, /**<Reserver for FOTA params (23 bytes) */
	
	PARAMS_ID_PORTS_CTR_STATE, /**< State of fertigation event(1 byte)*/
/*-----------------------------------------------------------------------------*/	
	/* System parameters - 128 bytes*/
	PARAMS_ID_MQTT_MESSAGE_ID, /**<Message ID for MQTT (2 bytes) */
	PARAMS_ID_SYSTEM_RESERVED, /**<Reserver for System params (126 bytes) */

	/* Add PARAMS_ID here. */
	PARAMS_ID_SCHEDULE_CSV_CRC, /**<CRC of schedule csv file */
	PARAMS_ID_THRESHOLD_CSV_CRC, /**<CRC of threshold csv file */
	PARAMS_ID_MACHINE_STATE,		/**<State of machine */
	PARAMS_ID_SETTING_VALID,	/**<Seting is avalid (1) or not (otherwise) */
	PARAMS_ID_TOPIC,					/**<MQTT Topic (4bytes) */
	PARAMS_ID_EC_CALIB_VALUES, /**<Contain calibration values of EC sensor (8bytes - 2 check valid bytes + 3 x2bytes points)*/
	PARAMS_ID_PH_CALIB_VALUES, /**<Contain calibration values of pH sensor (8bytes - 2 check valid bytes + 3 x2bytes points)*/
	PARAMS_ID_USE_RF,		/**<Use RF (1) or not (0) */
	PARAMS_ID_LORA, 		/**<Use LoRa (1) or short range (0) */
	PARAMS_ID_FERTI_FLOW_TYPE,	/**<Type of fertilizer flowmeter */
	PARAMS_ID_OPERATING_MODE,
	PARAMS_ID_FLOW_RATE_DEFAULT, //L/m*10
	PARAMS_ID_NORMAL_FLOWMTER_TYPE, /**(0): FS300A - (1):FS400A - (2)OF05ZAT*/
	PARAMS_ID_TANK1_DIVISION,
	PARAMS_ID_TANK2_DIVISION,
	PARAMS_ID_USE_MAIN_FLOW, /**<Use main flow (1) or not (0) */
	PARAMS_ID_FLOWMETER_COUNT, /**<Number of pulses to calculates flow rate*/
	PARAMS_ID_FLOWMETER_FACTOR, /**<Factor of main flowmeter (*1000) */
	PARAMS_ID_PRESSURE_TIME,
	PARAMS_ID_FLOWMETER_TIME,
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

extern int32_t s32_params_init (void);

extern int32_t s32_params_set (ENM_PARAMS_ITEMS_T enm_params_id,
                               uint8_t u8_params_len,
                               uint8_t *pu8_params_data);

extern int32_t s32_params_get (ENM_PARAMS_ITEMS_T enm_params_id,
                               uint8_t *pu8_params_data);

extern void v_params_reset (void);
extern uint32_t u32_Get_FW_Version (void);
extern uint32_t u32_Get_HW_Version (void);

extern uint8_t u8_Data_Length_Of_One_Parameter (ENM_PARAMS_ITEMS_T enm_params_id);

extern uint32_t u32_params_queue_wait(void);

#endif /* __PARAMS_H__ */

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
/*                           END OF FILE
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/
