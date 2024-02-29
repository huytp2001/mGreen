/*! @file mqtt_task.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef MQTT_TASK_H
#define MQTT_TASK_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib.h"
#include "board_config.h"
#include "download.h"
/*!
* @data types, constants and macro defintions
*/
typedef enum
{
	GSM_POWER_ON = 0,
	GSM_POWER_OFF,
	GSM_DISABLE_AT_ECHO,
	GSM_QUERY_SIM_CARD,
	GSM_QUERY_CS_SERVICE,
	GSM_GET_RSSI,
	GSM_GET_IMEI,
	GSM_QUERY_PS_SERVICE,
	GSM_QUERY_UE_SYSTEM,
	GSM_CFG_PDP_CONTEXT,
	GSM_ACTIVE_PDP_CONTEXT,
	GSM_CHECKING_PDP_CONTEXT,
	GSM_DEACTIVE_PDP_CONTEXT,
	MQTT_START,
	MQTT_ACQUIRE_CLIENT,
	MQTT_CFG_PARAM,
	MQTT_OPEN,
	MQTT_CONNECTING,
	MQTT_CONNECTED,
	MQTT_DISCONNECTING,
	MQTT_DISCONNECTED,
	NUM_OF_GSM_MQTT_STATE
}gsm_mqtt_state_t;

/*!
* @enum e_ftp_state_t
* State of file download process via FTP 
*/
typedef enum
{
	FTP_CFG_PDP_CONTEXT,		/**<Configure params of pdp context */
	FTP_ACTIVE_PDP_CONTEXT,	/**<Active the pdp context */
	FTP_CHECKING_PDP_CONTEXT, /**<Check status of activated the pdp context */
	FTP_CFG_USER_PARAMS,	/**<Configure username and password to logtin ftp server */
	FTP_CFG_FILE_TYPE,	/**<Configure file type using */
	FTP_CFG_TRANSFER_MODE,	/**<Configure mode of transfer process */
	FTP_CFG_TIMEOUT,	/**<Configure response timeout value */
	FTP_SERVER_LOGIN,	/**<Login to ftp server */
	FTP_DIRECTORY_SET,	/**<Go to the directory containing file in ftp server */
	FTP_DOWNLOAD,	/**<Download file */
	FTP_DOWNLOAD_COMPLETED, /**<Download process has done */
	FTP_SERVER_LOGOUT,	/**<Logout from server */
	FTP_DEACTIVE_PDP_CONTEXT,	/**<Deactive the activated pdp context */
	FTP_QUERY_ME,
	FTP_QUERY_CS_SERVICE,
	FTP_QUERY_PS_SERVICE,
	FTP_QUERY_UE_SYSTEM,	
	FTP_START, /**<Start service*/
	FTP_SET_TRANSFER_TYPE,
	FTP_GET_FILE_SIZE,
	FTP_STOP,
}e_ftp_state_t;

/*!
* @enum e_ftp_download_state_t
* State of file transfer process which get file from RAM of module SIM to storage of MCU
*/
typedef enum
{
	FTP_FILE_OPEN, 	/**<Open file and get session id */
	FTP_FILE_READ,	/**<Read and copy content of file to MCU storage */
	FTP_FILE_COMPLETE,	/**<Check if file has been tranfered completed */
	FTP_FILE_CLOSE,	/**<Close file */
	FTP_FILE_DELETE,	/**<Delete file to save memory space */
}e_ftp_download_state_t;

#define GSM_RETRY_DISABLE_AT_ECHO							10
#define GSM_QUERY_SIM_CARD_TIMES							10
#define GSM_QUERY_CS_SERVICE_TIMES						45
#define GSM_QUERY_PS_SERVICE_TIMES						30
#define GSM_CHECK_PDP_CONTEXT_TIMES						75
#define GSM_WAIT_SIM_SETUP										15
#define GSM_MAX_SIMCOM_RESPONE_TIMES					5

#define MQTT_PDP_CONTEXT_ID										1
#define FTP_PDP_CONTEXT_ID										2
#define PDP_CONTEXT_TYPE_IPV4									1
#define NETWORK_OPERATOR										  VIETTEL //(network_operator_t)(stru_neces_params_get().e_network_operator)
#define SP_REQ_PRECEDENCE											1
#define SP_MIN_PRECEDENCE											0
#define SP_REQ_DELAY													0
#define SP_MIN_DELAY													0
#define SP_REQ_RELIABILITY										1
#define SP_MIN_RELIABILITY										0
#define SP_REQ_PEAK														0
#define SP_MIN_PEAK														0
#define SP_REQ_MEAN														0
#define SP_MIN_MEAN														0

#define SP_3G_REQ_TRAFFIC_CLASS								2
#define SP_3G_MIN_TRAFFIC_CLASS								4
#define SP_3G_REQ_MAX_BITRATE_UL							0
#define SP_3G_MIN_MAX_BITRATE_UL							0
#define SP_3G_REQ_MAX_BITRATE_DL							0
#define SP_3G_MIN_MAX_BITRATE_DL							0
#define SP_3G_REQ_GUARANTEED_BITRATE_UL				0
#define SP_3G_MIN_GUARANTEED_BITRATE_UL				0
#define SP_3G_REQ_GUARANTEED_BITRATE_DL				0
#define SP_3G_MIN_GUARANTEED_BITRATE_DL				0
#define SP_3G_REQ_DELIVERY_ORDER							2
#define SP_3G_MIN_DELIVERY_ORDER							2
#define SP_3G_REQ_MAX_SDU_SIZE								0
#define SP_3G_MIN_MAX_SDU_SIZE								0
#define SP_3G_REQ_SDU_ERROR_RATIO							"0E0"
#define SP_3G_MIN_SDU_ERROR_RATIO							"0E0"
#define SP_3G_REQ_RESIDUAL_BIT_ERROR_RATIO		"0E0"
#define SP_3G_MIN_RESIDUAL_BIT_ERROR_RATIO		"0E0"
#define SP_3G_REQ_DELI_ERR_SUDS								3
#define SP_3G_MIN_DELI_ERR_SUDS								3
#define SP_3G_REQ_TRANSFER_DELAY							0
#define SP_3G_MIN_TRANSFER_DELAY							0
#define SP_3G_REQ_TRAFFIC_HANDLING_PRIO				0
#define SP_3G_MIN_TRAFFIC_HANDLING_PRIO				0
#define SP_3G_REQ_SOURCE_STATISTICS_DESC			0
#define SP_3G_MIN_SOURCE_STATISTICS_DESC			0
#define SP_3G_REQ_SIGNALLING_INDICATION				0
#define SP_3G_MIN_SIGNALLING_INDICATION				0

#define MQTT_CLIENT_ID												0
#define MQTT_VERSION													4
#define MQTT_PDP_CONTEXT_ID										1
#define MQTT_PKT_TIMEOUT											5
#define MQTT_RETRY_TIMES											3
#define MQTT_TIMEOUT_NOTICE_TYPE							0
#define MQTT_SESSION_TYPE											1
#define MQTT_KEEPALIVE_TIME										10
#define MQTT_RECV_MODE												0
#define MQTT_PING_INTERVAL_TIME								10
#define MAX_TIMEOUT_4G_STATE									20000/MQTT_TASK_DELAY

#ifdef BROKER_DEMO
#define MQTT_HOST_NAME									"demo.mimosatek.com"
#define MQTT_HOST_NAME_4G								"tcp://demo.mimosatek.com"
#define MQTT_USER_NAME 									"admin"
#define MQTT_PASSWORD 									"public"
#endif

#ifdef BROKER_APP
#define MQTT_HOST_NAME									"app.mimosatek.com"
#define MQTT_HOST_NAME_4G								"tcp://app.mimosatek.com"
#define MQTT_USER_NAME 									"eclientmtek"
#define MQTT_PASSWORD 									"AvSu2NGrfRZHua3dpHbNf3L6vBk"
#endif

#define FTP_HOST												"data.mimosatek.com"
#define FTP_PORT												21
#define FTP_USER_NAME										"controller"
#define FTP_PASSWORD										"Dzg8bumCC5hnjkzt"
#define FTP_DIRECTORY										"/home/controller"
#define FTP_FILE_TYPE_BINARY						0
#define FTP_FILE_TYPE_ASCII							1
#define FTP_TRANSFER_MODE_ACTIVE				0
#define FTP_TRANSFER_MODE_PASSIVE				1
#define FTP_TIMEOUT											90
#define FTP_OPEN_FILE_RO_MODE						2

#define MQTT_PORT												1883
#define MQTT_CLIENT_ID_1								"mgreen1"
#define MQTT_CLIENT_ID_2								"mgreen2"
#define TOPIC_CONTROL										"mgreen/control"
#define TOPIC_MONITOR										"mgreen/monitor"
#define TOPIC_SMS												"mgreen/sms"
#define TOPIC_PING											"mgreen/ping"

#define TOPIC_CONTROL_QOS											2
#define TOPIC_MONITOR_QOS											1
#define TOPIC_PING_QOS												1

#define MQTT_TIME_TO_WAIT_OPEN_NETWORK_MQTT		120
#define MQTT_TIME_TO_WAIT_CLOSE_NETWORK_MQTT	30
#define MQTT_TIME_TO_WAIT_CONNECTING					120
#define MQTT_TIME_TO_WAIT_DISCONNECT					30

#define MQTT_TOPIC_LEN_MAX										32
#define MQTT_PAYLOAD_LEN_MAX									256

#define MQTT_ECHO_FRAME_EN										(0)
#define MQTT_PUBLISH_KEEP_ALIVE								(1)
#define MQTT_SUBSCRIBE_PING_TOPIC							(1)

#define SCHEDULE_FILE_NAME_ON_RAM				"RAM:sche.csv"
#define THRESHOLD_FILE_NAME_ON_RAM			"RAM:thres.csv"
#define FIRMWARE_FILE_NAME_ON_RAM				"RAM:firm.hex"
#define FTP_MAX_DOWNLOAD_TIME						(60) /**<Maximum time to wait for a download process, in second(s) */
#define FTP_READ_PACKAGE_TIMEOUT				(90)	/**<second(s) */
#define FPT_MAX_PACKAGE_LEN							(1024)	/**<Bytes per reading time */
#define FTP_CONFIG_RETRY								(3)		/**<Maximum retry on config state */
#define FTP_TRANSFER_RETRY							(3)		/**<Maximum retry on file transfer state */


typedef struct
{
	uint8_t u8_client_id;
	uint16_t u16_msg_id;
	uint8_t u8_qos;
	uint8_t u8_retain;
	uint8_t au8_topic[MQTT_TOPIC_LEN_MAX];
	uint8_t u8_payload_len;
	uint8_t au8_payload[MQTT_PAYLOAD_LEN_MAX];
}mqtt_t;

/*!
* @struct stru_ftp_request_t
* Struct contains params of a download request
*/
typedef struct
{
	E_DOWNLOAD_FILE_TYPE e_file_type;	/**<Type of file */
	uint32_t u32_crc;		/**<CRC32 of file neet to download */
	char c_file_name[25];	/**<File name on ftp server */
}stru_ftp_request_t;

typedef enum
{
	MQTT_ACT_NONE = 0,
	MQTT_REQ_SUBSCRIBE,
	MQTT_REQ_UNSUBSCRIBE,
	NUM_OF_MQTT_ACTTION
}mqtt_action_t;

typedef enum
{
	CONNECT_FROM_POWER_ON = 0,
	RE_OPEN_MQTT_CONNECTION,
	RE_ATACH_PDP_CONTEXT,
	RE_POWER_ON_GSM
}mqtt_connect_code_t;

/*!
* @public functions prototype
*/
void v_mqtt_task_init(void);
bool b_write_mqtt_pub_to_server(mqtt_t* pstru_mqtt_pub_to_server);
bool b_write_mqtt_pub_from_server(mqtt_t* pstru_mqtt_pub_from_server);
void v_file_download_request(stru_ftp_request_t stru_request);
bool b_connection_get(void);
void v_connection_set(bool b_state_connect);
bool b_ping_get(void);
E_DOWNLOAD_FILE_TYPE e_download_file_type_get(void);
bool b_is_publish_free(void);
void v_download_file_fail(void);
#ifdef __cplusplus
}
#endif

#endif /* MQTT_TASK_H */
