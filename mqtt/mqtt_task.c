/*! @file mqtt_task.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdlib.h>
#include <stdint.h>
#include "mqtt_task.h"
#include "mqtt_publish.h"
#include "frame_parser.h"
#include "gsm_hal.h"
#include "gsm_cmd.h"
#include "wdt.h"
#include "rtc.h"
#include "config.h"
#include "mqtt_callbacks.h"
#include "ports_control.h"
#include "schedule_parser.h"
#include "schedule_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "setting.h"
#include "sd_card.h"
#include "digital_input.h"
#include "irrigation.h"
/*!
*@def MQTT_PUB_TO_SERVER_QUEUE_LEN
*@def MQTT_PUB_FROM_SERVER_QUEUE_LEN	
*@def MQTT_PUB_TO_SERVER_QUEUE_ITEM_SIZE				
*@def MQTT_TASK_SIZE
*@def MQTT_TASK_PRIORITY
*@def MQTT_TASK_DELAY
*@def MQTT_PUB_SERVER_KEEP_ALIVE_INTERVAL
*/
#define MQTT_PUB_TO_SERVER_QUEUE_LEN						15
#define MQTT_PUB_FROM_SERVER_QUEUE_LEN					10
#define MQTT_PUB_TO_SERVER_QUEUE_ITEM_SIZE			sizeof(mqtt_t)
#define MQTT_PUB_FROM_SERVER_QUEUE_ITEM_SIZE		sizeof(mqtt_t)
#define MQTT_TASK_SIZE													(configMINIMAL_STACK_SIZE * 8)
#define MQTT_TASK_PRIORITY											(tskIDLE_PRIORITY + 2)
#define MQTT_TASK_DELAY													(portTickType)(500 / portTICK_RATE_MS)
#define MQTT_PUB_SERVER_KEEP_ALIVE_INTERVAL			60000/MQTT_TASK_DELAY
#define MQTT_FROM_SERVER_PING_INTERVAL					90000/MQTT_TASK_DELAY
#define MQTT_FROM_SERVER_SYNC_TIME_INTERVAL			3600000/MQTT_TASK_DELAY

#define MQTT_MAX_NUM_CALLBACKS									10

#define TYPE_OF_MODULE_INTERNET 								SW5_READ() //MODULE_3G

/*!
* static data declaration
*/
static xQueueHandle q_mqtt_pub_to_server;
static xQueueHandle q_mqtt_pub_from_server;
static StaticQueue_t x_mqtt_pub_to_server_buff; 
static StaticQueue_t x_mqtt_pub_from_server_buff;
static uint8_t au8_mqtt_pub_to_server_mem[MQTT_PUB_TO_SERVER_QUEUE_LEN * 
																		MQTT_PUB_TO_SERVER_QUEUE_ITEM_SIZE];
static uint8_t au8_mqtt_pub_from_server_mem[MQTT_PUB_FROM_SERVER_QUEUE_LEN * 
																		MQTT_PUB_FROM_SERVER_QUEUE_ITEM_SIZE];

static StaticTask_t x_mqtt_task_buffer;
static StackType_t  x_mqtt_stack[MQTT_TASK_SIZE];
static uint8_t u8_mqtt_task_id = 0;	/**<ID which watchdog allocated for mqtt task */
static gsm_mqtt_state_t e_gsm_mqtt_state = GSM_POWER_OFF;
static e_ftp_state_t e_ftp_state = FTP_CFG_PDP_CONTEXT;
static e_ftp_download_state_t e_ftp_download_state = FTP_FILE_OPEN;
static mqtt_connect_code_t e_mqtt_connect_code = CONNECT_FROM_POWER_ON;

static mqtt_t stru_mqtt_pub_to_server;
static mqtt_t stru_mqtt_pub_from_server;
static mqtt_data_frame_t stru_mqtt_data_frame_from_server;
static uint16_t u16_mqtt_msg_id = 0;

static mqtt_action_t e_mqtt_control_topic = MQTT_REQ_SUBSCRIBE;
static mqtt_action_t e_mqtt_ping_topic = MQTT_REQ_SUBSCRIBE;

static uint32_t u32_count_to_pub_server_keep_alive = 
																MQTT_PUB_SERVER_KEEP_ALIVE_INTERVAL;
static uint32_t u32_count_from_server_ping = 
																MQTT_FROM_SERVER_PING_INTERVAL;
static uint32_t u32_count_from_server_sync_time = 0;
static bool b_req_sync_time = true;

static mqtt_call_back cb_mqtt_function_list[MQTT_MAX_NUM_CALLBACKS];
static stru_ftp_request_t stru_ftp_request;	/**<Contains params or a download request */
static bool b_file_available = false;		/**<Identify is file available on RAM or not */
static bool b_is_ftp_enable = false;	/**<Identify ftp is runing or not */
static bool b_send_ftp_feedback = false;
static bool b_reset_request = false;

/*!
* private function prototype
*/
static void v_mqtt_task(void *pvParameters);
static void v_mqtt_process_on_connected(void);
static void v_mqtt_process_publish(void);
static void v_mqtt_process_subscribe(void);
static void v_mqtt_process_frame_from_server(void);
static void v_mqtt_maintain_connection(void);
static void v_mqtt_process_pub_keep_alive(void);
static void v_mqtt_process_sync_time(void);
static void v_mqtt_process_check_connection(void);
static uint16_t u16_generate_mqtt_msg_id(void);
static void v_mqtt_process_callback(void);
static void v_mqtt_process_on_disconnected(void);
static void v_file_available_set(void);
static void v_file_unavailable_set(void);
static void v_ftp_process(void);
static void v_ftp_enable(void);
static void v_ftp_disable(void);
static void v_file_download_process(void);

/*!
* public function bodies
*/

/*!
* @fn v_mqtt_task_init(void)
* @brief Init mqtt task, must be called in main function.
*/
void v_mqtt_task_init(void)
{
	memset(cb_mqtt_function_list, NULL, MQTT_MAX_NUM_CALLBACKS);
	q_mqtt_pub_to_server = xQueueCreateStatic(MQTT_PUB_TO_SERVER_QUEUE_LEN, 
																		 MQTT_PUB_TO_SERVER_QUEUE_ITEM_SIZE,
																		 au8_mqtt_pub_to_server_mem, 
																		 &x_mqtt_pub_to_server_buff);
	
	q_mqtt_pub_from_server = xQueueCreateStatic(MQTT_PUB_FROM_SERVER_QUEUE_LEN, 
																		 MQTT_PUB_FROM_SERVER_QUEUE_ITEM_SIZE,
																		 au8_mqtt_pub_from_server_mem, 
																		 &x_mqtt_pub_from_server_buff);		
	
	xTaskCreateStatic(v_mqtt_task, "MQTT_TASK", 
										MQTT_TASK_SIZE, NULL, 
										MQTT_TASK_PRIORITY, 
										x_mqtt_stack, 
										&x_mqtt_task_buffer);
}
/*!
* @fn b_write_mqtt_pub_to_server(mqtt_t* pstru_mqtt_pub_to_server)
* @brief
*/
bool b_write_mqtt_pub_to_server(mqtt_t* pstru_mqtt_pub_to_server)
{
	if (NULL == q_mqtt_pub_to_server)
	{
		return false;
	}
	if(xQueueSend(q_mqtt_pub_to_server, pstru_mqtt_pub_to_server, MQTT_TASK_DELAY) == pdTRUE)
	{
			return true;
	}
	//TODO: save mqtt pub data to sd card
	return false;
}

/*!
* @fn b_write_mqtt_pub_from_server(mqtt_t* pstru_mqtt_pub_from_server)
* @brief
*/
bool b_write_mqtt_pub_from_server(mqtt_t* pstru_mqtt_pub_from_server)
{
	if (NULL == q_mqtt_pub_from_server)
	{
		return false;
	}
	if(xQueueSend(q_mqtt_pub_from_server, pstru_mqtt_pub_from_server, MQTT_TASK_DELAY) == pdTRUE)
	{
			return true;
	}
	//TODO: save mqtt pub data to sd card
	return false;
}

/*!
* @fn b_mqtt_register_callback(mqtt_call_back callback)
* @brief 
*/
bool b_mqtt_register_callback(mqtt_call_back callback)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < MQTT_MAX_NUM_CALLBACKS; u8_i++)
	{
		if(cb_mqtt_function_list[u8_i] == NULL) 
		{
			cb_mqtt_function_list[u8_i] = callback;
			return true;
		}
	}
	return false;
}

/*!
* @fn void v_mqtt_unregister_callback(mqtt_call_back callback)
* @brief
*/
void v_mqtt_unregister_callback(mqtt_call_back callback)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < MQTT_MAX_NUM_CALLBACKS; u8_i++)
	{
		if(cb_mqtt_function_list[u8_i] == callback) 
		{
			cb_mqtt_function_list[u8_i] = NULL;
			return;
		}
	}
}

/*!
* @fn v_file_download_request(stru_ftp_request_t stru_request)
* @brief request to enable ftp and download a file
* @param[in] stru_request Contains params of request
* @return None
*/
void v_file_download_request(stru_ftp_request_t stru_request)
{
	stru_ftp_request = stru_request;
	b_send_ftp_feedback = true;
}

static bool b_connection = false;
static bool b_update_rtc = false;

bool b_connection_get(void)
{
	return b_connection;
}

void v_connection_set(bool b_state_connect)
{
	b_connection = b_state_connect;
}

bool b_ping_get(void)
{
	return b_update_rtc;
}

E_DOWNLOAD_FILE_TYPE e_download_file_type_get(void)
{
	return stru_ftp_request.e_file_type;
}

bool b_is_publish_free(void)
{
	bool b_result = false;
	if(!uxQueueMessagesWaiting(q_mqtt_pub_to_server))
	{
		b_result = true;
	}
	return b_result;
}
void v_download_file_fail(void)
{
	char *pc_file_name;
	/* Strip file type extension from file name */
	pc_file_name = (char *)calloc(strlen(stru_ftp_request.c_file_name), sizeof(uint8_t));
	strncpy(pc_file_name, stru_ftp_request.c_file_name, strlen(stru_ftp_request.c_file_name) - 4);
	if (b_is_ftp_enable)
	{
		e_ftp_state = FTP_SERVER_LOGOUT;
		v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_SAVE_TO_SD_FAIL, pc_file_name);
		b_reset_request = true;
	}
	if(b_file_available)
	{
		e_ftp_download_state = FTP_FILE_CLOSE;
		v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_SAVE_TO_SD_FAIL, pc_file_name);
		b_reset_request = true;
	}
}
/**
* private function bodies
*/
/*!
* @fn static void v_mqtt_task(void *pvParameters)
* @brief 
*/
static void v_mqtt_task(void *pvParameters)
{
	static bool b_is_reconnect = false;
	static uint8_t u8_timeout_state = MAX_TIMEOUT_4G_STATE;
	uint8_t u8_tmp_cnt = 0;
	//register task with watchdog timer
	while(b_wdt_reg_new_task("mqtt_task", &u8_mqtt_task_id) != true){}
	b_mqtt_register_callback(mqtt_control_callback);
	v_fota_status_set(FOTA_NO_NEW_UPDATE, 0);
	for (;;)
	{
		//reset wdt flag
		b_wdt_task_reload_counter(u8_mqtt_task_id);
		if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
		{
			switch (e_gsm_mqtt_state)
			{
				case GSM_POWER_OFF:
				{
					b_gsm_cmd_power_down();
					v_gsm_hal_power_off();
					e_gsm_mqtt_state = GSM_POWER_ON;
				}
				break;			
				case GSM_POWER_ON:
				{
					v_gsm_hal_power_on();
					b_gsm_cmd_check_rdy();
					e_gsm_mqtt_state = GSM_DISABLE_AT_ECHO;		
				}
				break;
				case GSM_DISABLE_AT_ECHO:
				{
					// If failed to disable AT echo in 10 times (mean 20s), reboot module
					u8_tmp_cnt = GSM_RETRY_DISABLE_AT_ECHO;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_disable_at_echo() == true)
						{
							e_gsm_mqtt_state = GSM_QUERY_SIM_CARD;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
					}
				}
				break;
				case GSM_QUERY_SIM_CARD:
				{
					// If failed to find SIM card in 10 times (mean 20s), reboot module
					u8_tmp_cnt = GSM_QUERY_SIM_CARD_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_sim_ready() == true)
						{
							e_gsm_mqtt_state = GSM_QUERY_CS_SERVICE;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
					}
				}
				break;
				case GSM_QUERY_CS_SERVICE:
				{
					/*
					If failed to register to CS domain service in 45 times (90s), then reboot the module.
					*/
					u8_tmp_cnt = GSM_QUERY_CS_SERVICE_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_registered_network() == true)
						{
							e_gsm_mqtt_state = GSM_GET_RSSI;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
						vTaskDelay(2000);
					}			
				}
				break;
				case GSM_GET_RSSI:
				{
					if (b_gsm_cmd_get_rssi() == true)
					{
						e_gsm_mqtt_state = GSM_QUERY_PS_SERVICE;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;		
					}		
				}
				break;			
				case GSM_QUERY_PS_SERVICE:
				{
					/*
					Register to PS domain service in 60s
					Go to next step no matter register to PS domain service or not in 60
					*/
					u8_tmp_cnt = GSM_QUERY_PS_SERVICE_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_registered_ps() == true)
						{
							break;
						}
						u8_tmp_cnt--;
						vTaskDelay(2000);
					}
					e_gsm_mqtt_state = GSM_CFG_PDP_CONTEXT;				
				}
				break;
				case GSM_CFG_PDP_CONTEXT:
				{
					//Configure PDP context
					b_gsm_cmd_cfg_pdp(MQTT_PDP_CONTEXT_ID, PDP_CONTEXT_TYPE_IPV4, NETWORK_OPERATOR);
					//check at respone
					b_gsm_cmd_cfg_quality_SP_req(MQTT_PDP_CONTEXT_ID, SP_REQ_PRECEDENCE, SP_REQ_DELAY,
																				SP_REQ_RELIABILITY, SP_REQ_PEAK, SP_REQ_MEAN);
					b_gsm_cmd_cfg_quality_SP_min(MQTT_PDP_CONTEXT_ID, SP_MIN_PRECEDENCE, SP_MIN_DELAY,
																				SP_MIN_RELIABILITY, SP_MIN_PEAK, SP_MIN_MEAN);
					b_gsm_cmd_cfg_3g_quality_SP_req(MQTT_PDP_CONTEXT_ID, SP_3G_REQ_TRAFFIC_CLASS,
											SP_3G_REQ_MAX_BITRATE_UL, SP_3G_REQ_MAX_BITRATE_DL,
											SP_3G_REQ_GUARANTEED_BITRATE_UL, SP_3G_REQ_GUARANTEED_BITRATE_DL,
											SP_3G_REQ_DELIVERY_ORDER, SP_3G_REQ_MAX_SDU_SIZE,
											SP_3G_REQ_SDU_ERROR_RATIO, SP_3G_REQ_RESIDUAL_BIT_ERROR_RATIO,
											SP_3G_REQ_DELI_ERR_SUDS, SP_3G_REQ_TRANSFER_DELAY,
											SP_3G_REQ_TRAFFIC_HANDLING_PRIO, SP_3G_REQ_SOURCE_STATISTICS_DESC,
											SP_3G_REQ_SIGNALLING_INDICATION);
					b_gsm_cmd_cfg_3g_quality_SP_min(MQTT_PDP_CONTEXT_ID, SP_3G_MIN_TRAFFIC_CLASS,
											SP_3G_MIN_MAX_BITRATE_UL, SP_3G_MIN_MAX_BITRATE_DL,
											SP_3G_MIN_GUARANTEED_BITRATE_UL, SP_3G_MIN_GUARANTEED_BITRATE_DL,
											SP_3G_MIN_DELIVERY_ORDER, SP_3G_MIN_MAX_SDU_SIZE,
											SP_3G_MIN_SDU_ERROR_RATIO, SP_3G_MIN_RESIDUAL_BIT_ERROR_RATIO,
											SP_3G_MIN_DELI_ERR_SUDS, SP_3G_MIN_TRANSFER_DELAY,
											SP_3G_MIN_TRAFFIC_HANDLING_PRIO, SP_3G_MIN_SOURCE_STATISTICS_DESC,
											SP_3G_MIN_SIGNALLING_INDICATION);														
					e_gsm_mqtt_state = GSM_ACTIVE_PDP_CONTEXT;
				}
				break;
				case GSM_ACTIVE_PDP_CONTEXT:
				{
					//Activate PDP context
					b_gsm_cmd_activate_pdp_context(MQTT_PDP_CONTEXT_ID);
					e_gsm_mqtt_state = GSM_CHECKING_PDP_CONTEXT;
				}
				break;		
				case GSM_CHECKING_PDP_CONTEXT:
				{
					/*
					If failed to active PDP context in 75 times (mean 150s), go deactive PDP context
					*/
					u8_tmp_cnt = GSM_CHECK_PDP_CONTEXT_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_pdp_context() == true)
						{
							e_gsm_mqtt_state = MQTT_CFG_PARAM;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_DEACTIVE_PDP_CONTEXT;
							break;
						}					
					}		
				}
				break;		
				case GSM_DEACTIVE_PDP_CONTEXT:
				{
					//Deactive PDP context
					if (b_gsm_cmd_deactivate_pdp_context(MQTT_PDP_CONTEXT_ID) == true)
					{
						e_gsm_mqtt_state = GSM_QUERY_SIM_CARD;		
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;			
					}
				}
				break;
				case MQTT_CFG_PARAM:
				{
					//Configure MQTT param
					b_gsm_cmd_cfg_mqtt_protocol_ver(MQTT_CLIENT_ID, MQTT_VERSION);
					b_gsm_cmd_cfg_mqtt_pdp(MQTT_CLIENT_ID, MQTT_PDP_CONTEXT_ID);
					b_gsm_cmd_cfg_mqtt_msg_delivery_timeout(MQTT_CLIENT_ID, MQTT_PKT_TIMEOUT,
																MQTT_RETRY_TIMES, MQTT_TIMEOUT_NOTICE_TYPE);
					b_gsm_cmd_cfg_mqtt_session_type(MQTT_CLIENT_ID, MQTT_SESSION_TYPE);
					b_gsm_cmd_cfg_mqtt_keep_alive_time(MQTT_CLIENT_ID, MQTT_KEEPALIVE_TIME);
					b_gsm_cmd_cfg_mqtt_recv_mode(MQTT_CLIENT_ID, MQTT_RECV_MODE);
					b_gsm_cmd_cfg_mqtt_ping_interval(MQTT_CLIENT_ID, MQTT_PING_INTERVAL_TIME);
					
					e_gsm_mqtt_state = MQTT_OPEN;			
				}
				break;
				case MQTT_OPEN:
				{
					//Open a Network for MQTT Client
					e_mqtt_control_topic = MQTT_REQ_SUBSCRIBE;
					e_mqtt_ping_topic = MQTT_REQ_SUBSCRIBE;
					if( b_gsm_cmd_open_network_mqtt(MQTT_CLIENT_ID, MQTT_HOST_NAME, MQTT_PORT, 
																			MQTT_TIME_TO_WAIT_OPEN_NETWORK_MQTT))
					{
						v_gsm_mqtt_network_close(false);
						e_gsm_mqtt_state = MQTT_CONNECTING;					
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;
					}			
				}
				break;
				case MQTT_CONNECTING:
				{
					//Connect a Client to MQTT Server
					if( b_gsm_cmd_mqtt_connect(MQTT_CLIENT_ID, MQTT_CLIENT_ID_1, 
								MQTT_USER_NAME, MQTT_PASSWORD, MQTT_TIME_TO_WAIT_CONNECTING))
					{
						// Publish init frame to say hello to cloud
						e_gsm_mqtt_state = MQTT_CONNECTED;	
						if(!b_is_reconnect)
						{
							v_mqtt_pub_hello(HW_VERSION, FW_VERSION, e_mqtt_connect_code);
							v_mqtt_data_pub(0xD0, TANK_1_DIVISION);
							v_mqtt_data_pub(0xD1, TANK_2_DIVISION);
							b_connection = true;
						}
						else
						{
							b_is_reconnect = false;
						}
					}
					else
					{
						e_gsm_mqtt_state = MQTT_OPEN;
					}			
				}
				break;
				case MQTT_DISCONNECTING:
				{
					//Disconnect a Client to MQTT Server
					if (b_gsm_cmd_mqtt_disconnect(MQTT_CLIENT_ID, MQTT_TIME_TO_WAIT_DISCONNECT))
					{
						e_gsm_mqtt_state = MQTT_DISCONNECTED;					
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;
					}			
				}
				break;	
				case MQTT_CONNECTED:
				{
					// write MQTT connetecd process
					if(b_is_ftp_enable)
					{
						v_ftp_process();
					}
					else if(b_file_available)
					{
						// Do nothing, wait transfer complete
					}
					else
					{
						if(b_gsm_is_mqtt_network_closed())
						{
							e_gsm_mqtt_state = MQTT_OPEN;
							e_mqtt_connect_code = RE_OPEN_MQTT_CONNECTION;
							b_is_reconnect = true;
						}
						else
						{
							v_mqtt_process_on_connected();
						}
						//send all frame in queue before switch to FTP
						if(b_send_ftp_feedback)
						{
							if(b_is_publish_free())
							{
								b_send_ftp_feedback = false;
								v_ftp_enable();
							}
						}
						//send all frame in queue before reset
						if(b_reset_request)
						{
							if(b_is_publish_free())
							{
								b_reset_request = false;
								ROM_SysCtlReset();
							}
						}
						
					}
				}
				break;
				case MQTT_DISCONNECTED:
				{
					e_gsm_mqtt_state = MQTT_OPEN;
					e_mqtt_connect_code = RE_OPEN_MQTT_CONNECTION;
					v_mqtt_process_on_disconnected();
				}
				break;			
				default: break;
			}
			if(b_file_available)
			{
				/* TODO: download file from RAM to local memory */
				v_file_download_process();
			}
		}
		else if (MODULE_WIFI == TYPE_OF_MODULE_INTERNET)
		{
			switch (e_gsm_mqtt_state)
			{
				case GSM_POWER_OFF:
				{
					//reset module wifi
					v_wifi_reset();					
					e_gsm_mqtt_state = MQTT_OPEN;
				}
				break;
				case MQTT_OPEN:
				{
					v_mqtt_process_on_disconnected();
					e_gsm_mqtt_state = MQTT_CONNECTING;
				}
				case MQTT_CONNECTING:
				{				
					if (b_connection)
					{
						v_mqtt_pub_hello(HW_VERSION, FW_VERSION, e_mqtt_connect_code);
						v_mqtt_data_pub(0xD0, TANK_1_DIVISION);
						v_mqtt_data_pub(0xD1, TANK_2_DIVISION);
						e_gsm_mqtt_state = MQTT_CONNECTED;
					}
					else
					{
						// Coding... waiting connect MQTT, timeout reset module
						static uint16_t u16_t_wait = 0;
						static uint8_t u8_retry = 0;
						u16_t_wait++;
						// 60s request 1 lan
						if (u16_t_wait > (60000/MQTT_TASK_DELAY))
						{ 
							u16_t_wait = 0;
							u8_retry++;
							if (u8_retry > 5)
							{
								u8_retry = 0;
								e_gsm_mqtt_state = GSM_POWER_OFF;
								break;
							}
							i8_gsm_cmd_esp_check_mqtt_state();
						}				
					}
				}
				break;
				case MQTT_CONNECTED:
				{
					// write MQTT connetecd process
					if(b_is_ftp_enable)
					{
						v_ftp_process();
					}
					else 
					{
						if(b_file_available)
						{
							// Do nothing, wait transfer complete
						}
						else
						{
							if(!b_connection)
							{
								e_gsm_mqtt_state = MQTT_OPEN;
							}
							else
							{
								v_mqtt_process_on_connected();
							}
							//send all frame in queue before switch to FTP
							if(b_send_ftp_feedback)
							{
								if(b_is_publish_free())
								{
									b_send_ftp_feedback = false;
									v_ftp_enable();
								}
							}
							//send all frame in queue before reset
							if(b_reset_request)
							{
								if(b_is_publish_free())
								{
									b_reset_request = false;
									ROM_SysCtlReset();
								}
							}
						}
					}
				}
				break;
				case MQTT_DISCONNECTING:
				{
					//if time out
					if (i8_gsm_cmd_esp_check_mqtt_state() == -1)
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;
					}
					else
					{
						e_gsm_mqtt_state = MQTT_OPEN;
					}
				}
				break;
				default: 
				{
					e_gsm_mqtt_state = GSM_POWER_OFF;
				}
				break;
			}
			if(b_file_available)
			{
				/* TODO: download file from RAM to local memory */
				v_file_download_process();
			}
		}
		else
		{
			//module 4G
			switch (e_gsm_mqtt_state)
			{
				case GSM_POWER_OFF:
				{
					//b_gsm_cmd_power_down();
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					v_module_4g_hal_power_off();
					e_gsm_mqtt_state = GSM_POWER_ON;
				}
				break;			
				case GSM_POWER_ON:
				{
					v_module_4g_hal_power_on();
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = 3;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_rdy() == true)
						{
							e_gsm_mqtt_state = GSM_DISABLE_AT_ECHO;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
					}		
				}
				break;
				case GSM_DISABLE_AT_ECHO:
				{
					// If failed to disable AT echo in 10 times (mean 20s), reboot module
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = GSM_RETRY_DISABLE_AT_ECHO;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_disable_at_echo() == true)
						{
							e_gsm_mqtt_state = GSM_QUERY_SIM_CARD;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
					}
				}
				break;
				case GSM_QUERY_SIM_CARD:
				{
					// If failed to find SIM card in 10 times (mean 20s), reboot module
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = GSM_QUERY_SIM_CARD_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_sim_ready() == true)
						{
							e_gsm_mqtt_state = GSM_GET_RSSI;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
					}
				}
				break;
				case GSM_GET_RSSI:
				{
					// If failed to disable AT echo in 5 times (mean 10s), reboot module
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = GSM_MAX_SIMCOM_RESPONE_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_get_rssi() == true)
						{
							e_gsm_mqtt_state = GSM_GET_IMEI;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;		
							break;
						}		
					}
				}
				break;				
				case GSM_GET_IMEI:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					if (b_gsm_cmd_get_imei() == true)
					{
						e_gsm_mqtt_state = GSM_QUERY_CS_SERVICE;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;		
					}
				}
				break;
				case GSM_QUERY_CS_SERVICE:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = 5;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_registered_network() == true)
						{
							e_gsm_mqtt_state = GSM_QUERY_PS_SERVICE;
							break;
						}
						u8_tmp_cnt--;
						if (0 == u8_tmp_cnt)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}	
					}						
				}
				break;
				case GSM_QUERY_PS_SERVICE:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = GSM_QUERY_PS_SERVICE_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_registered_ps() == true)
						{
							e_gsm_mqtt_state = GSM_QUERY_UE_SYSTEM;	
							break;
						}
						u8_tmp_cnt--;
						if (0 == u8_tmp_cnt)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}	
					}												
				}
				break;			
				case GSM_QUERY_UE_SYSTEM:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					if (b_gsm_cmd_check_ue_system() == true)
					{
						e_gsm_mqtt_state = GSM_CFG_PDP_CONTEXT;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;
					}		
				}
				break;			
				case GSM_CFG_PDP_CONTEXT:
				{
					//Configure PDP context
					//b_gsm_cmd_cfg_pdp_4g(MQTT_PDP_CONTEXT_ID, NETWORK_OPERATOR);
					//dont send cfg pdp, module auto read pdp
					//e_gsm_mqtt_state = GSM_ACTIVE_PDP_CONTEXT;
					e_gsm_mqtt_state = MQTT_START;
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
				}
				break;
				case GSM_ACTIVE_PDP_CONTEXT:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					//Activate PDP context
					if (b_gsm_cmd_activate_pdp_context_4g(true, MQTT_PDP_CONTEXT_ID))
					{
						e_gsm_mqtt_state = GSM_CHECKING_PDP_CONTEXT;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;
					}		
				}
				break;
				case GSM_CHECKING_PDP_CONTEXT:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					u8_tmp_cnt = GSM_CHECK_PDP_CONTEXT_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if (b_gsm_cmd_check_pdp_context_4g() == true)
						{
							e_gsm_mqtt_state = MQTT_START;
							break;
						}
						u8_tmp_cnt--;
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_DEACTIVE_PDP_CONTEXT;
							break;
						}					
					}		
				}
				break;		
				case GSM_DEACTIVE_PDP_CONTEXT:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					//Deactive PDP context
					if (b_gsm_cmd_activate_pdp_context_4g(false, MQTT_PDP_CONTEXT_ID) == true)
					{
						e_gsm_mqtt_state = GSM_QUERY_SIM_CARD;		
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;			
					}
				}
				break;			
				case MQTT_START:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					e_mqtt_control_topic = MQTT_REQ_SUBSCRIBE;
					e_mqtt_ping_topic = MQTT_REQ_SUBSCRIBE;
					if (b_gsm_mqtt_start_4g())
					{
						e_gsm_mqtt_state = MQTT_ACQUIRE_CLIENT;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;	
					}
				}
				break;
				case MQTT_ACQUIRE_CLIENT:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					if (b_gsm_mqtt_acquire_client(MQTT_CLIENT_ID))
					{
						e_gsm_mqtt_state = MQTT_CFG_PARAM;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;	
					}
				}
				break;
				case MQTT_CFG_PARAM:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					if(b_gsm_cmd_config_utf8_4g(MQTT_CLIENT_ID, 0, 20))
					{
						e_gsm_mqtt_state = MQTT_CONNECTING;
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;		
					}
				}
				break;
				case MQTT_CONNECTING:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					//Connect a Client to MQTT Server
					u8_tmp_cnt = GSM_MAX_SIMCOM_RESPONE_TIMES;
					while (u8_tmp_cnt > 0)
					{
						if(b_gsm_cmd_mqtt_connect_4g(MQTT_CLIENT_ID, MQTT_HOST_NAME_4G, MQTT_KEEPALIVE_TIME,
																			 MQTT_SESSION_TYPE, MQTT_USER_NAME, MQTT_PASSWORD))
						{
							// Publish init frame to say hello to cloud
							e_gsm_mqtt_state = MQTT_CONNECTED;	
							v_gsm_mqtt_network_close(false);
							if(!b_is_reconnect)
							{
								v_mqtt_pub_hello(HW_VERSION, FW_VERSION, e_mqtt_connect_code);
								b_connection = true;
							}
							else
							{
								b_is_reconnect = false;
							}
							break;
						}
						u8_tmp_cnt--;		
						if (u8_tmp_cnt == 0)
						{
							e_gsm_mqtt_state = GSM_POWER_OFF;
							break;
						}
					}											
				}
				break;
				case MQTT_CONNECTED:
				{
					// write MQTT connetecd process
					if(b_is_ftp_enable)
					{
						v_ftp_process();
					}
					else if(b_file_available)
					{
						// Do nothing, wait transfer complete
					}
					else
					{
						if(b_gsm_is_mqtt_network_closed())
						{
							e_gsm_mqtt_state = MQTT_START;
							e_mqtt_connect_code = RE_OPEN_MQTT_CONNECTION;
							b_is_reconnect = true;
						}
						else
						{
							v_mqtt_process_on_connected();
						}
						//send all frame in queue before switch to FTP
						if(b_send_ftp_feedback)
						{
							if(b_is_publish_free())
							{
								b_send_ftp_feedback = false;
								v_ftp_enable();
							}
						}
						//send all frame in queue before reset
						if(b_reset_request)
						{
							if(b_is_publish_free())
							{
								b_reset_request = false;
								ROM_SysCtlReset();
							}
						}
						
					}
				}
				break;
				case MQTT_DISCONNECTING:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					//Disconnect a Client to MQTT Server
					if (b_gsm_cmd_mqtt_disconnect_4g(MQTT_CLIENT_ID, MQTT_TIME_TO_WAIT_DISCONNECT))
					{
						e_gsm_mqtt_state = MQTT_DISCONNECTED;					
					}
					else
					{
						e_gsm_mqtt_state = GSM_POWER_OFF;
					}			
				}
				break;
				case MQTT_DISCONNECTED:
				{
					u8_timeout_state = MAX_TIMEOUT_4G_STATE;
					e_gsm_mqtt_state = MQTT_START;
					e_mqtt_connect_code = RE_OPEN_MQTT_CONNECTION;
					v_mqtt_process_on_disconnected();
				}
				break;
				default:
				{
					e_gsm_mqtt_state = GSM_POWER_OFF;
				}
				break;
			}
			if(b_file_available)
			{
				/* TODO: download file from RAM to local memory */
				v_file_download_process();
			}
			if (e_gsm_mqtt_state != MQTT_CONNECTED)
			{
				u8_timeout_state--;
				if (u8_timeout_state == 0)
				{
					e_gsm_mqtt_state = GSM_POWER_OFF;
				}
			}
		}
		vTaskDelay(MQTT_TASK_DELAY);
	}
}


/*!
* @fn static void v_mqtt_process_on_conneted(void)
* @brief 
*/
static void v_mqtt_process_on_connected(void)
{
	v_mqtt_process_publish();
	v_mqtt_process_subscribe();
	v_mqtt_process_frame_from_server();
	v_mqtt_maintain_connection();
	//v_ui_set_connection_status(true);
}

/*!
* @fn v_mqtt_process_on_disconnected(void)
* @brief 
*/
static void v_mqtt_process_on_disconnected(void)
{
	//v_ui_set_connection_status(false);
}

/*!
* @fn static void v_mqtt_process_publish(void)
* @brief 
*/
static void v_mqtt_process_publish(void)
{
	//TODO: Get and publish data
	if(uxQueueMessagesWaiting(q_mqtt_pub_to_server))
	{
		if(xQueueReceive(q_mqtt_pub_to_server,&stru_mqtt_pub_to_server, 
												(portTickType)(MQTT_TASK_DELAY)) == pdPASS)
		{
			if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
			{
				b_gsm_cmd_mqtt_publish_msg(MQTT_CLIENT_ID, u16_generate_mqtt_msg_id(), 
						stru_mqtt_pub_to_server.u8_qos, stru_mqtt_pub_to_server.u8_retain,
						(const char*)stru_mqtt_pub_to_server.au8_topic, 
						stru_mqtt_pub_to_server.u8_payload_len, stru_mqtt_pub_to_server.au8_payload, 
						MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES);
			}
			else if (MODULE_WIFI == TYPE_OF_MODULE_INTERNET)
			{
				if(!b_gsm_cmd_esp_mqtt_publish(stru_mqtt_pub_to_server.u8_payload_len, 
																			stru_mqtt_pub_to_server.au8_payload))
				{
					xQueueSendToFront(q_mqtt_pub_to_server,&stru_mqtt_pub_to_server, 
													(portTickType)(MQTT_TASK_DELAY));
					e_gsm_mqtt_state = MQTT_DISCONNECTING; // check connect
				}
			}
			else
			{
				if(!b_gsm_cmd_mqtt_publish_msg_4g(MQTT_CLIENT_ID, stru_mqtt_pub_to_server.u8_qos,
						stru_mqtt_pub_to_server.u8_retain, (const char*)stru_mqtt_pub_to_server.au8_topic, 
						stru_mqtt_pub_to_server.u8_payload_len, stru_mqtt_pub_to_server.au8_payload, 
						MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES))
				{
					xQueueSendToFront(q_mqtt_pub_to_server,&stru_mqtt_pub_to_server, 
													(portTickType)(MQTT_TASK_DELAY));
					e_gsm_mqtt_state = MQTT_DISCONNECTING; // check connect
				}
			}
		}
	}
}


/*!
* @fn static void v_mqtt_process_subscribe(void)
* @brief 
*/
static void v_mqtt_process_subscribe(void)
{
	//TODO: Subcribe or un-Subcribe
	switch(e_mqtt_control_topic)
	{
		case MQTT_REQ_SUBSCRIBE:
		{
			if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
			{
				if (b_gsm_cmd_mqtt_subscibe(MQTT_CLIENT_ID, u16_generate_mqtt_msg_id(),
					TOPIC_CONTROL, TOPIC_CONTROL_QOS, MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES))
				{
					e_mqtt_control_topic = MQTT_ACT_NONE;
				}
			}
			else if (MODULE_4G == TYPE_OF_MODULE_INTERNET)
			{	
				if (b_gsm_cmd_mqtt_subscibe_4g(MQTT_CLIENT_ID, TOPIC_CONTROL, TOPIC_CONTROL_QOS, MQTT_RETRY_TIMES))
				{
					e_mqtt_control_topic = MQTT_ACT_NONE;
				}
			}
		}
		break;
		case MQTT_REQ_UNSUBSCRIBE:
		{
			if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
			{
				if (b_gsm_cmd_mqtt_unsubscibe(MQTT_CLIENT_ID, u16_generate_mqtt_msg_id(),
					TOPIC_CONTROL, MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES))
				{
					e_mqtt_control_topic = MQTT_ACT_NONE;
				}
			}
			else if (MODULE_4G == TYPE_OF_MODULE_INTERNET)
			{	
				if (b_gsm_cmd_mqtt_unsubscibe_4g(MQTT_CLIENT_ID, TOPIC_CONTROL))
				{
					e_mqtt_control_topic = MQTT_REQ_SUBSCRIBE;
				}
			}
		}
		default: break;
	}

#if MQTT_SUBSCRIBE_PING_TOPIC
	switch(e_mqtt_ping_topic)
	{
		case MQTT_REQ_SUBSCRIBE:
		{
			if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
			{
				if (b_gsm_cmd_mqtt_subscibe(MQTT_CLIENT_ID, u16_generate_mqtt_msg_id(),
					TOPIC_PING, TOPIC_PING_QOS, MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES))
				{
					e_mqtt_ping_topic = MQTT_ACT_NONE;
				}
			}
			else if (MODULE_4G == TYPE_OF_MODULE_INTERNET)
			{
				if (b_gsm_cmd_mqtt_subscibe_4g(MQTT_CLIENT_ID, TOPIC_PING, TOPIC_PING_QOS, MQTT_RETRY_TIMES))
				{
					e_mqtt_ping_topic = MQTT_ACT_NONE;
				}
			}
		}
		break;
		case MQTT_REQ_UNSUBSCRIBE:
		{
			if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
			{
				if (b_gsm_cmd_mqtt_unsubscibe(MQTT_CLIENT_ID, u16_generate_mqtt_msg_id(),
					TOPIC_PING, MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES))
				{
					e_mqtt_ping_topic = MQTT_ACT_NONE;
				}
			}
			else if (MODULE_4G == TYPE_OF_MODULE_INTERNET)
			{
				if (b_gsm_cmd_mqtt_unsubscibe_4g(MQTT_CLIENT_ID, TOPIC_PING))
				{
					e_mqtt_ping_topic = MQTT_ACT_NONE;
				}
			}
		}
		default: break;
	}
#endif	
}

/*!
* @fn static void v_mqtt_process_frame_from_server(void)
* @brief
*/
static void v_mqtt_process_frame_from_server(void)
{
	//TODO: Receive and process data from server
	if(uxQueueMessagesWaiting(q_mqtt_pub_from_server))
	{
		if(xQueueReceive(q_mqtt_pub_from_server,&stru_mqtt_pub_from_server, 
												(portTickType)(MQTT_TASK_DELAY)) == pdPASS)
		{
#if MQTT_ECHO_FRAME_EN
			//Echo the received frame for testing
			b_gsm_cmd_mqtt_publish_msg(MQTT_CLIENT_ID, u16_generate_mqtt_msg_id(), 
					stru_mqtt_pub_from_server.u8_qos, stru_mqtt_pub_from_server.u8_retain,
					(const char*)TOPIC_MONITOR, 
					stru_mqtt_pub_from_server.u8_payload_len, stru_mqtt_pub_from_server.au8_payload, 
					MQTT_PKT_TIMEOUT, MQTT_RETRY_TIMES);
			b_gsm_cmd_esp_mqtt_publish(stru_mqtt_pub_from_server.u8_payload_len, stru_mqtt_pub_from_server.au8_payload);
#endif
			// Parse mqtt payload to frame
			v_frame_mqtt_parse (&stru_mqtt_pub_from_server, &stru_mqtt_data_frame_from_server);
			// Handle incomming frame from server
			switch (stru_mqtt_data_frame_from_server.u8_frame_type)
			{
				case FRAME_TYPE_PING:
				{
					//Preset u32_count_from_server_ping
					u32_count_from_server_ping = MQTT_FROM_SERVER_PING_INTERVAL;
					// Check and synctime
					if (true == b_req_sync_time)
					{
						b_req_sync_time = false;
						u32_count_from_server_sync_time = MQTT_FROM_SERVER_SYNC_TIME_INTERVAL;
						//Check unix time in frame,then call function to set rtc time
						T_DATETIME t_time;
						v_long_2_datetime(&t_time,
															stru_mqtt_data_frame_from_server.stru_data_field[0].u32_value + 
															(u16_get_time_zone() * 60));
						vTaskDelay(1000);
						if (t_time.u16_year > 2020)
						{
							if (RTC_AUTO_SET)
							{
								b_rtc_set_time(&t_time);
							}
							//v_ui_display_date_time();
							b_update_rtc = true;
						}
					}
				}
				break;
				case FRAME_TYPE_QUERY:
				{
					switch(stru_mqtt_data_frame_from_server.stru_data_field[0].u16_id)
					{
						case DATAID_QUERY_VERSION_CODE:
						{
							v_mqtt_fw_version_pub(FW_VERSION);
							v_mqtt_data_pub(DATAID_MACHINE_STATE, u8_irrigation_prog_state_get());
							if (FERTILIZER_FLOWMETER_TYPE == FERTI_FLOW_NORMAL)
							{
								if (NORMAL_FLOWMETER_TYPE == OF05ZAT)
								{
									v_mqtt_data_pub(0x00, 0x0A);
								}
								else if (NORMAL_FLOWMETER_TYPE == FS300A)
								{
									v_mqtt_data_pub(0x00, 0xF3);
								}
								else if (NORMAL_FLOWMETER_TYPE == FS400A)
								{
									v_mqtt_data_pub(0x00, 0xF4);	
								}
							}
							else if (FERTILIZER_FLOWMETER_TYPE == FERTI_FLOW_HUBA)
							{
								v_mqtt_data_pub(0x00, 0xBA);	
							}
							else
							{
								v_mqtt_data_pub(0x00, 0x00);	
							}
						}
						break;
						case DATAID_SYNC_TIME:
						{
							T_DATETIME t_time;
							v_long_2_datetime(&t_time,
																stru_mqtt_data_frame_from_server.stru_data_field[0].u32_value + 
																(u16_get_time_zone() * 60));
							b_rtc_set_time(&t_time);
							vTaskDelay(1000);
							v_mqtt_pub_feedback(&stru_mqtt_data_frame_from_server, (uint32_t)true);
						}
						break;
						case DATAID_RESET:
						{
							b_reset_request = true;
							//v_mqtt_pub_feedback(&stru_mqtt_data_frame_from_server, (uint32_t)true);
						}
						break;
						case DATAID_FORMAT_SD_CARD:
						{
							if(i16_sd_format() == 0)
							{
								v_mqtt_pub_feedback(&stru_mqtt_data_frame_from_server, (uint32_t)true);
							}
							else
							{
								v_mqtt_pub_feedback(&stru_mqtt_data_frame_from_server, (uint32_t)false);
							}
						}
						break;
						default: break;
					}
				}
				break;
				default: break;
			}
			// Call back funtions
			v_mqtt_process_callback();
		}
	}	
}

/*!
* @fn v_mqtt_maintain_connection(void)
* @brief
*/
static void v_mqtt_maintain_connection(void)
{
#if MQTT_PUBLISH_KEEP_ALIVE
	v_mqtt_process_pub_keep_alive();
#endif
#if MQTT_SUBSCRIBE_PING_TOPIC
	v_mqtt_process_sync_time();
	v_mqtt_process_check_connection();
#endif
}

/*!
* @fn static void v_mqtt_process_pub_keep_alive(void)
* @brief 
*/
static void v_mqtt_process_pub_keep_alive(void)
{
	if (u32_count_to_pub_server_keep_alive > 0)
	{
		u32_count_to_pub_server_keep_alive--;
	}
	//Publish frame 70 to keep alive with server
	if(u32_count_to_pub_server_keep_alive == 0)
	{
		u32_count_to_pub_server_keep_alive = MQTT_PUB_SERVER_KEEP_ALIVE_INTERVAL;
		v_mqtt_pub_keep_alive(0);
	}
}

/*!
* @fn static void v_mqtt_process_sync_time(void)
* @brief 
*/
static void v_mqtt_process_sync_time(void)
{
	if (u32_count_from_server_sync_time > 0)
	{
		u32_count_from_server_sync_time--;
	}	
	// Request to sync time
	if(u32_count_from_server_sync_time == 0)
	{
		u32_count_from_server_sync_time = MQTT_FROM_SERVER_SYNC_TIME_INTERVAL;
		b_req_sync_time = true;		
	}
}

/*!
* @fn static void v_mqtt_process_check_connection(void)
* @brief 
*/
static void v_mqtt_process_check_connection(void)
{
	if (u32_count_from_server_ping > 0)
	{
		u32_count_from_server_ping--;
	}
	if (u32_count_from_server_ping == 0)
	{
		e_gsm_mqtt_state = MQTT_DISCONNECTING;
		u32_count_from_server_ping = MQTT_FROM_SERVER_PING_INTERVAL;
	}
}

/*!
* @fn uint16_t u16_generate_mqtt_msg_id(void)
* @brief 
*/
static uint16_t u16_generate_mqtt_msg_id(void)
{
	u16_mqtt_msg_id++;
	//Do not use message ID = 0, it is reserved as an invalid message ID
	if(u16_mqtt_msg_id == 0)
	{
		u16_mqtt_msg_id = 1;
	}
	return u16_mqtt_msg_id;
}

/*!
* @fn void v_mqtt_process_callback(void)
* @brief
*/
static void v_mqtt_process_callback(void)
{
	uint8_t u8_i = 0;
	for(u8_i = 0; u8_i < MQTT_MAX_NUM_CALLBACKS; u8_i++)
	{
		if(cb_mqtt_function_list[u8_i] != NULL) 
		{
			cb_mqtt_function_list[u8_i](stru_mqtt_data_frame_from_server);
		}
	}
}

/*!
* @fn v_ftp_process(void)
* @brief Process to download a file from FTP server to RAM of module SIM
* @param None
* @return None
*/
static void v_ftp_process(void)
{
	static uint8_t u8_tmp_cnt = 0;
	char *pc_file_name;
	static bool b_download_success = false;
	/* Strip file type extension from file name */
	pc_file_name = (char *)calloc(strlen(stru_ftp_request.c_file_name), sizeof(uint8_t));
	strncpy(pc_file_name, stru_ftp_request.c_file_name, strlen(stru_ftp_request.c_file_name) - 4);
	if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
	{
		switch(e_ftp_state)
		{
			case 	FTP_CFG_PDP_CONTEXT:		/**<Configure params of pdp context */
			{
				//Configure PDP context for fpt
				b_gsm_cmd_cfg_pdp(FTP_PDP_CONTEXT_ID, PDP_CONTEXT_TYPE_IPV4, NETWORK_OPERATOR);
				//Quality of Service Profile (Requested)
				b_gsm_cmd_cfg_quality_SP_req(MQTT_PDP_CONTEXT_ID, SP_REQ_PRECEDENCE, SP_REQ_DELAY,
																			SP_REQ_RELIABILITY, SP_REQ_PEAK, SP_REQ_MEAN);
				//Quality of Service Profile (Minimum Acceptable) 
				b_gsm_cmd_cfg_quality_SP_min(MQTT_PDP_CONTEXT_ID, SP_MIN_PRECEDENCE, SP_MIN_DELAY,
																			SP_MIN_RELIABILITY, SP_MIN_PEAK, SP_MIN_MEAN);
				//3G Quality of Service Profile (Requested) 
				b_gsm_cmd_cfg_3g_quality_SP_req(MQTT_PDP_CONTEXT_ID, SP_3G_REQ_TRAFFIC_CLASS,
										SP_3G_REQ_MAX_BITRATE_UL, SP_3G_REQ_MAX_BITRATE_DL,
										SP_3G_REQ_GUARANTEED_BITRATE_UL, SP_3G_REQ_GUARANTEED_BITRATE_DL,
										SP_3G_REQ_DELIVERY_ORDER, SP_3G_REQ_MAX_SDU_SIZE,
										SP_3G_REQ_SDU_ERROR_RATIO, SP_3G_REQ_RESIDUAL_BIT_ERROR_RATIO,
										SP_3G_REQ_DELI_ERR_SUDS, SP_3G_REQ_TRANSFER_DELAY,
										SP_3G_REQ_TRAFFIC_HANDLING_PRIO, SP_3G_REQ_SOURCE_STATISTICS_DESC,
										SP_3G_REQ_SIGNALLING_INDICATION);
				//3G Quality of Service Profile (Minimum Acceptable) 
				b_gsm_cmd_cfg_3g_quality_SP_min(MQTT_PDP_CONTEXT_ID, SP_3G_MIN_TRAFFIC_CLASS,
										SP_3G_MIN_MAX_BITRATE_UL, SP_3G_MIN_MAX_BITRATE_DL,
										SP_3G_MIN_GUARANTEED_BITRATE_UL, SP_3G_MIN_GUARANTEED_BITRATE_DL,
										SP_3G_MIN_DELIVERY_ORDER, SP_3G_MIN_MAX_SDU_SIZE,
										SP_3G_MIN_SDU_ERROR_RATIO, SP_3G_MIN_RESIDUAL_BIT_ERROR_RATIO,
										SP_3G_MIN_DELI_ERR_SUDS, SP_3G_MIN_TRANSFER_DELAY,
										SP_3G_MIN_TRAFFIC_HANDLING_PRIO, SP_3G_MIN_SOURCE_STATISTICS_DESC,
										SP_3G_MIN_SIGNALLING_INDICATION);
				e_ftp_state = FTP_ACTIVE_PDP_CONTEXT;
			}
			break;
			case FTP_ACTIVE_PDP_CONTEXT:	/**<Active the pdp context */
			{
				//Activate PDP context
				b_gsm_cmd_activate_pdp_context(FTP_PDP_CONTEXT_ID);
				e_ftp_state = FTP_CHECKING_PDP_CONTEXT;
			}
			break;
			case FTP_CHECKING_PDP_CONTEXT: /**<Check status of activated the pdp context */
			{
				/*
				If failed to active PDP context in 75 times (mean 150s), go deactive PDP context
				*/
				u8_tmp_cnt = GSM_CHECK_PDP_CONTEXT_TIMES;
				while (u8_tmp_cnt > 0)
				{
					if (b_gsm_cmd_check_pdp_context() == true)
					{
						u8_tmp_cnt = FTP_CONFIG_RETRY;
						e_ftp_state = FTP_CFG_USER_PARAMS;
						break;
					}
					u8_tmp_cnt--;
					if (u8_tmp_cnt == 0)
					{
						e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
						break;
					}					
				}
			}
			break;
			case FTP_CFG_USER_PARAMS:	/**<Configure username and password to logtin ftp server */
			{
				if(b_gsm_cmd_cfg_ftp_account (FTP_USER_NAME, FTP_PASSWORD))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state = FTP_CFG_FILE_TYPE;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
					}
				}
			}
			break;
			case FTP_CFG_FILE_TYPE:	/**<Configure file type using */
			{
				if(b_gsm_cmd_cfg_ftp_file_type(FTP_FILE_TYPE_ASCII))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state = FTP_CFG_TRANSFER_MODE;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
					}
				}
			}
			break;
			case FTP_CFG_TRANSFER_MODE:	/**<Configure mode of transfer process */
			{
				if(b_gsm_cmd_cfg_ftp_transmode(FTP_TRANSFER_MODE_PASSIVE))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state = FTP_CFG_TIMEOUT;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
					}
				}
			}
			break;
			case FTP_CFG_TIMEOUT:	/**<Configure response timeout value */
			{
				if(b_gsm_cmd_cfg_ftp_rsp_timeout(FTP_TIMEOUT))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state = FTP_SERVER_LOGIN;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
					}
				}
			}
			break;
			case FTP_SERVER_LOGIN:	/**<Login to ftp server */
			{
				if(b_gsm_cmd_ftp_open (FTP_HOST, FTP_PORT, 60))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state  = FTP_DIRECTORY_SET;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
					}
				}
			}
			break;
			case FTP_DIRECTORY_SET:	/**<Go to the directory containing file in ftp server */
			{
				if(b_gsm_cmd_ftp_set_curr_dir(FTP_DIRECTORY, 60))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state = FTP_DOWNLOAD;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
				}
				
			}
			break;
			case FTP_DOWNLOAD:	/**<Download file */
			{
				char c_filename_on_sdcard[25] = {0};
				switch (stru_ftp_request.e_file_type)
				{
					case FILE_TYPE_SCHEDULE:
					{
						strcpy(c_filename_on_sdcard, CONFIG_SCHEDULE_FILENAME);
					}
					break;
					case FILE_TYPE_THRESHOLD:
					{
						strcpy(c_filename_on_sdcard, CONFIG_THRESHOLD_FILENAME);
					}
					break;
					case FILE_TYPE_FIRMWARE:
					{
						strcpy(c_filename_on_sdcard, CONFIG_FIRMWARE_FILENAME);
					}
					break;
					default: break;
				}
				if(b_prepare_for_download(c_filename_on_sdcard))
				{
					if(stru_ftp_request.e_file_type < FILE_TYPE_MAX)
					{
						if(b_gsm_cmd_ftp_download(stru_ftp_request.c_file_name, "RAM:file.bin" , 0, 0, FTP_TIMEOUT) == true)
						{
							e_ftp_state = FTP_DOWNLOAD_COMPLETED;
						}
						else
						{
							u8_tmp_cnt--;
							if(0 == u8_tmp_cnt)	//fail to retry
							{
								v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
								e_ftp_state = FTP_SERVER_LOGOUT;
							}
						}
					}
					else
					{
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
				}
				else
				{
					v_download_file_fail();
				}
			}
			break;
			case FTP_DOWNLOAD_COMPLETED:
			{
				if(b_gsm_cmd_ftp_wait_download_done(FTP_MAX_DOWNLOAD_TIME))
				{
					v_file_available_set();
				}
				else
				{
					v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
				}
				e_ftp_state = FTP_SERVER_LOGOUT;
			}
			case FTP_SERVER_LOGOUT:	/**<Logout from server */
			{
				b_gsm_cmd_ftp_logout();
				e_ftp_state = FTP_DEACTIVE_PDP_CONTEXT;
			}
			break;
			case FTP_DEACTIVE_PDP_CONTEXT:	/**<Deactive the activated pdp context */
			{
				b_gsm_cmd_deactivate_pdp_context(FTP_PDP_CONTEXT_ID);
				v_ftp_disable();
				e_ftp_state = FTP_CFG_PDP_CONTEXT;
			}
			break;
		}
	}
	else if (MODULE_WIFI == TYPE_OF_MODULE_INTERNET)
	{
		switch(e_ftp_state)
		{
			case FTP_DOWNLOAD:
			{
				char c_filename_on_sdcard[25] = {0};
				switch (stru_ftp_request.e_file_type)
				{
					case FILE_TYPE_SCHEDULE:
					{
						strcpy(c_filename_on_sdcard, CONFIG_SCHEDULE_FILENAME);
					}
					break;
					case FILE_TYPE_THRESHOLD:
					{
						strcpy(c_filename_on_sdcard, CONFIG_THRESHOLD_FILENAME);
					}
					break;
					case FILE_TYPE_FIRMWARE:
					{
						strcpy(c_filename_on_sdcard, CONFIG_FIRMWARE_FILENAME);
					}
					break;
					default: break;
				}
				if (b_prepare_for_download(c_filename_on_sdcard))
				{
					if(stru_ftp_request.e_file_type < FILE_TYPE_MAX)
					{
						if(b_gsm_cmd_esp_ftp_download(stru_ftp_request.c_file_name, stru_ftp_request.u32_crc, FTP_TIMEOUT + FTP_MAX_DOWNLOAD_TIME))
						{
							e_ftp_state = FTP_DOWNLOAD_COMPLETED;
						}
						else
						{
							v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
							e_ftp_state = FTP_SERVER_LOGOUT;
							//reset module wifi
							e_gsm_mqtt_state = GSM_POWER_OFF;
						}
					}
					else
					{
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
				}
				else
				{
					v_download_file_fail();
				}
			}
			break;
			case FTP_DOWNLOAD_COMPLETED:
			{
				v_file_available_set();
				e_ftp_state = FTP_SERVER_LOGOUT;
			}
			break;
			case FTP_SERVER_LOGOUT:
			{
				v_ftp_disable();
				e_ftp_state = FTP_DOWNLOAD;
			}
			break;
			default: 
			{
				e_ftp_state = FTP_DOWNLOAD;
			}
			break;
		}
	}
	else
	{
		//module 4G
		switch(e_ftp_state)
		{
			case FTP_QUERY_ME:
			{
				b_download_success = false;
				if (b_gsm_cmd_query_me())
				{
					e_ftp_state = FTP_QUERY_CS_SERVICE;
				}
				else
				{	
					//reboot module
					v_ftp_disable();
					e_ftp_state = FTP_QUERY_ME;
				}
			}
			break;
			case FTP_QUERY_CS_SERVICE:
			{
				u8_tmp_cnt = 5;
				while (u8_tmp_cnt > 0)
				{
					if (b_gsm_cmd_check_registered_network() == true)
					{
						e_ftp_state = FTP_QUERY_PS_SERVICE;
						break;
					}
					u8_tmp_cnt--;
					vTaskDelay(2000);
				}
				if (0 == u8_tmp_cnt)
				{
					v_ftp_disable();
					e_ftp_state = FTP_QUERY_ME;
					break;
				}		
			}
			break;
			case FTP_QUERY_PS_SERVICE:
			{
				u8_tmp_cnt = GSM_QUERY_PS_SERVICE_TIMES;
				while (u8_tmp_cnt > 0)
				{
					if (b_gsm_cmd_check_registered_ps() == true)
					{
						e_ftp_state = FTP_QUERY_UE_SYSTEM;	
						break;
					}
					u8_tmp_cnt--;
					vTaskDelay(2000);
				}
				if (0 == u8_tmp_cnt)
				{
					//reboot module
					v_ftp_disable();
					e_ftp_state = FTP_QUERY_ME;
					break;
				}								
			}
			break;			
			case FTP_QUERY_UE_SYSTEM:
			{
				if (b_gsm_cmd_check_ue_system() == true)
				{
					e_ftp_state = FTP_START;
					break;
				}
				else
				{
					//reboot module
					v_ftp_disable();
					e_ftp_state = FTP_QUERY_ME;
					break;
				}		
			}
			break;
			case FTP_CHECKING_PDP_CONTEXT:
			{
				u8_tmp_cnt = GSM_CHECK_PDP_CONTEXT_TIMES;
				while (u8_tmp_cnt > 0)
				{
					if (b_gsm_cmd_check_pdp_context_4g() == true)
					{
						e_ftp_state = FTP_START;
						break;
					}
					u8_tmp_cnt--;
					if (u8_tmp_cnt == 0)
					{
						//reboot module
						v_ftp_disable();
						e_ftp_state = FTP_QUERY_ME;
						break;
					}					
				}		
			}
			break;
			case FTP_START:
			{
				if (b_gsm_cmd_ftp_start_4g())
				{
					e_ftp_state = FTP_SERVER_LOGIN;
				}
				else
				{
					v_ftp_disable();
					e_ftp_state = FTP_SERVER_LOGOUT;
					v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
					//check connnect:
				}
			}
			break;
			case FTP_SERVER_LOGIN:
			{
				if(b_gsm_cmd_ftp_login_4g (FTP_HOST, FTP_PORT, FTP_USER_NAME, FTP_PASSWORD, 0, 9))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state  = FTP_SET_TRANSFER_TYPE;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						u8_tmp_cnt = FTP_CONFIG_RETRY;
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
				}
			}
			break;
			case FTP_SET_TRANSFER_TYPE:
			{
				if(b_gsm_cmd_ftp_set_transfer_type())
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state  = FTP_DIRECTORY_SET;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						u8_tmp_cnt = FTP_CONFIG_RETRY;
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
				}
			}
			break;
			case FTP_DIRECTORY_SET:
			{
				if(b_gsm_cmd_ftp_set_dir_4g(FTP_DIRECTORY, 9))
				{
					u8_tmp_cnt = FTP_CONFIG_RETRY;
					e_ftp_state = FTP_GET_FILE_SIZE;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						u8_tmp_cnt = FTP_CONFIG_RETRY;
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
				}
			}
			break;
			case FTP_GET_FILE_SIZE:
			{
				char c_filename_on_sdcard[25] = {0};
				switch (stru_ftp_request.e_file_type)
				{
					case FILE_TYPE_SCHEDULE:
					{
						strcpy(c_filename_on_sdcard, CONFIG_SCHEDULE_FILENAME);
					}
					break;
					case FILE_TYPE_THRESHOLD:
					{
						strcpy(c_filename_on_sdcard, CONFIG_THRESHOLD_FILENAME);
					}
					break;
					case FILE_TYPE_FIRMWARE:
					{
						strcpy(c_filename_on_sdcard, CONFIG_FIRMWARE_FILENAME);
					}
					break;
					default: break;
				}
				if (b_prepare_for_download(c_filename_on_sdcard))
				{
					if (b_gsm_cmd_ftp_get_file_size_4g(stru_ftp_request.c_file_name))
					{
						u8_tmp_cnt = FTP_TRANSFER_RETRY;
						e_ftp_state = FTP_DOWNLOAD;
					}
					else
					{
						u8_tmp_cnt--;
						if(0 == u8_tmp_cnt)	//fail to retry
						{
							u8_tmp_cnt = FTP_CONFIG_RETRY;
							v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
							e_ftp_state = FTP_SERVER_LOGOUT;
						}
					}
				}
				else
				{
					v_download_file_fail();
				}
			}
			break;
			case FTP_DOWNLOAD:
			{			
				if(stru_ftp_request.e_file_type < FILE_TYPE_MAX)
				{
					if(b_gsm_cmd_ftp_get_file_serial_4g(stru_ftp_request.c_file_name))
					{
						u8_tmp_cnt = FTP_TRANSFER_RETRY;
						b_download_success = true;
						e_ftp_state = FTP_SERVER_LOGOUT;
					}
					else
					{					
						u8_tmp_cnt--;
						if(0 == u8_tmp_cnt)	//fail to retry
						{
							u8_tmp_cnt = FTP_CONFIG_RETRY;
							v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
							e_ftp_state = FTP_SERVER_LOGOUT;
						}
					}
				}
				
			}
			break;
			case FTP_DOWNLOAD_COMPLETED:
			{
				//check done, sum size
				//check size, csv, feed back
				if(b_gsm_cmd_ftp_wait_download_done(FTP_MAX_DOWNLOAD_TIME))
				{
					v_file_available_set();
					e_ftp_state = FTP_SERVER_LOGOUT;
				}
				else
				{
					v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_FAIL, pc_file_name);
				}
				e_ftp_state = FTP_SERVER_LOGOUT;
			}
			break;			
			case FTP_SERVER_LOGOUT:
			{
				if (b_gsm_cmd_ftp_logout_4g())
				{
					u8_tmp_cnt = FTP_TRANSFER_RETRY;
					e_ftp_state = FTP_STOP;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						u8_tmp_cnt = FTP_CONFIG_RETRY;
						e_ftp_state = FTP_STOP;
					}
				}			
			}
			break;
			case FTP_STOP:
			{
				b_gsm_cmd_ftp_stop_4g();
				v_ftp_disable();
				e_ftp_state = FTP_QUERY_ME;
				if (b_download_success)
				{
					v_file_available_set();
				}
			}
			break;
			default:
			{
				e_ftp_state = FTP_QUERY_ME;
			}
			break;
		}
	}
	free(pc_file_name);
}

/*!
* @fn v_file_download_process(void)
* @brief Process to download file from RAM of module SIM and save to storage of MCU (SDRAM/SDcard)
* @param None
* @return None
*/
static void v_file_download_process(void)
{
	static uint8_t u8_tmp_cnt = FTP_TRANSFER_RETRY;
	char *pc_file_name;
	
	/* Strip file type extension from file name */
	pc_file_name = (char *)calloc(strlen(stru_ftp_request.c_file_name), sizeof(uint8_t));
	strncpy(pc_file_name, stru_ftp_request.c_file_name, strlen(stru_ftp_request.c_file_name) - 4);
	if (MODULE_3G == TYPE_OF_MODULE_INTERNET)
	{
		switch(e_ftp_download_state)
		{
			case FTP_FILE_OPEN: /**<Open file and get session id */
			{
				if(b_gsm_cmd_file_open("RAM:file.bin", FTP_OPEN_FILE_RO_MODE))
				{
					u8_tmp_cnt = FTP_TRANSFER_RETRY;
					e_ftp_download_state = FTP_FILE_READ;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_TRANSFER_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_OPEN;
						v_file_unavailable_set();
					}
				}
			}
			break;
			case FTP_FILE_READ:	/**<Read and copy content of file to MCU storage */
			{
				if(b_gsm_cmd_file_read(FPT_MAX_PACKAGE_LEN, FTP_READ_PACKAGE_TIMEOUT))
				{
					u8_tmp_cnt = FTP_TRANSFER_RETRY;
					if(b_is_download_completed() && (FTP_FILE_READ == e_ftp_download_state))
					{
						e_ftp_download_state = FTP_FILE_COMPLETE;
					}
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_TRANSFER_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
				}
			}
			break;
			case FTP_FILE_COMPLETE: /**<Check if file has been tranfered completed */
			{
				if(u32_file_download_crc_get() == stru_ftp_request.u32_crc)
				{
					if(b_save_file_process(stru_ftp_request.e_file_type))		//save file to sd card 
					{
						e_ftp_download_state = FTP_FILE_CLOSE;
						/* Send feedback */
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_SUCCESS, pc_file_name);
						v_mqtt_data_pub(DATAID_MACHINE_STATE, u8_irrigation_prog_state_get());
						switch(stru_ftp_request.e_file_type)
						{
							case FILE_TYPE_SCHEDULE:
							{
								v_new_schedule_available_set();
								//v_set_ports_control_state(STATE_REQ_RUN);	
							}
							break;
							case FILE_TYPE_THRESHOLD:
							{
							}
							break;
							case FILE_TYPE_FIRMWARE:
							{
								//reset
								b_reset_request = true;
							}
							break;
							default: break;
						}
					}
					else
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_SAVE_TO_SD_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
				}
				else
				{
					v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_CRC_FAIL, pc_file_name);
					e_ftp_download_state = FTP_FILE_CLOSE;
				}
			}
			break;
			case FTP_FILE_CLOSE:		/**<Close file */
			{
				if(b_gsm_cmd_file_close())
				{
					u8_tmp_cnt = FTP_TRANSFER_RETRY;
					e_ftp_download_state = FTP_FILE_DELETE;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						e_ftp_download_state = FTP_FILE_DELETE;
					}
				}
			}
			break;
			case FTP_FILE_DELETE:		/**<Delete file to save memory space */
			{
				if(b_gsm_cmd_file_delete("RAM:file.bin"))
				{
					v_file_unavailable_set();
					e_ftp_download_state = FTP_FILE_OPEN;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_file_unavailable_set();
						e_ftp_download_state = FTP_FILE_OPEN;
					}
				}
			}
			break;
		}
	}
	else if (MODULE_WIFI == TYPE_OF_MODULE_INTERNET)
	{
		switch(e_ftp_download_state)
		{
			case FTP_FILE_READ:	/**<Read and copy content of file to MCU storage */
			{	
				
				if(b_gsm_cmd_esp_file_pull(FTP_READ_PACKAGE_TIMEOUT))
				{
					u8_tmp_cnt = FTP_TRANSFER_RETRY;
					if(b_is_download_completed() && (FTP_FILE_READ == e_ftp_download_state))
					{
						e_ftp_download_state = FTP_FILE_COMPLETE;
					}
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_TRANSFER_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
				}
			}
			break;
			case FTP_FILE_COMPLETE: /**<Check if file has been tranfered completed */
			{
				if(u32_file_download_crc_get() == stru_ftp_request.u32_crc)
				{
					//doc lai file da luu duoc hay chua
					if(b_save_file_process(stru_ftp_request.e_file_type))		//save file to sd card 
					{
						// Send feedback 
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_SUCCESS, pc_file_name);
						v_mqtt_data_pub(DATAID_MACHINE_STATE, u8_irrigation_prog_state_get());
						switch(stru_ftp_request.e_file_type)
						{
							case FILE_TYPE_SCHEDULE:
							{
								//reload schedule
								//v_new_schedule_set();
								v_new_schedule_available_set();
							}
							break;
							case FILE_TYPE_THRESHOLD:
							{
							}
							break;
							case FILE_TYPE_FIRMWARE:
							{
								//reset
								b_reset_request = true;
							}
							break;
							default: break;
						}
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
					else
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_SAVE_TO_SD_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
					
				}
				else
				{
					v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_CRC_FAIL, pc_file_name);
					e_ftp_download_state = FTP_FILE_CLOSE;
				}
			}
			break;
			case FTP_FILE_CLOSE:
			{
				v_file_unavailable_set();
				e_ftp_download_state = FTP_FILE_READ;
			}
			break;
			default:
			{
				e_ftp_download_state = FTP_FILE_READ;
			}
			break;
		}
	}
	else
	{
		//module 4G
		switch(e_ftp_download_state)
		{
			case FTP_FILE_READ:	/**<Read and copy content of file to MCU storage */
			{			
				if(b_gsm_cmd_get_file_from_fs_4g(stru_ftp_request.c_file_name,u32_sum_sub_len_get(),u32_download_size_request()))
				{
					u8_tmp_cnt = FTP_TRANSFER_RETRY;
					if(b_is_download_completed() && (FTP_FILE_READ == e_ftp_download_state))
					{
						e_ftp_download_state = FTP_FILE_COMPLETE;
					}
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_TRANSFER_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
				}
			}
			break;
			case FTP_FILE_COMPLETE: /**<Check if file has been tranfered completed */
			{
				if(u32_file_download_crc_get() == stru_ftp_request.u32_crc)
				{
					if(b_save_file_process(stru_ftp_request.e_file_type))		//save file to sd card 
					{
						/* Send feedback */
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_DOWNLOAD_SUCCESS, pc_file_name);
						v_mqtt_data_pub(DATAID_MACHINE_STATE, u8_irrigation_prog_state_get());
						switch(stru_ftp_request.e_file_type)
						{
							case FILE_TYPE_SCHEDULE:
							{
								//reload schedule
								v_new_schedule_available_set();
							}
							break;
							case FILE_TYPE_THRESHOLD:
							{
							}
							break;
							case FILE_TYPE_FIRMWARE:
							{
								//reset
								b_reset_request = true;
							}
							break;
							default: break;
						}
						e_ftp_download_state = FTP_FILE_DELETE;
					}
					else
					{
						v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_SAVE_TO_SD_FAIL, pc_file_name);
						e_ftp_download_state = FTP_FILE_DELETE;
					}
				}
				else
				{
					v_mqtt_feedback_ftp(stru_ftp_request.e_file_type, FEEDBACK_CRC_FAIL, pc_file_name);
					e_ftp_download_state = FTP_FILE_DELETE;
				}
			}
			break;		
			case FTP_FILE_DELETE:		/**<Delete file to save memory space */
			{
				if(b_gsm_cmd_file_delete_4g(stru_ftp_request.c_file_name))
				{
					e_ftp_download_state = FTP_FILE_CLOSE;
				}
				else
				{
					u8_tmp_cnt--;
					if(0 == u8_tmp_cnt)	//fail to retry
					{
						e_ftp_download_state = FTP_FILE_CLOSE;
					}
				}
			}
			break;
			case FTP_FILE_CLOSE:
			{
				v_file_unavailable_set();
				e_ftp_download_state = FTP_FILE_READ;
			}
			break;
			default:
			{
				e_ftp_download_state = FTP_FILE_READ;
			}
			break;
		}
	}
	free(pc_file_name);
}

static void v_file_available_set(void)
{
	b_file_available = true;
}
static void v_file_unavailable_set(void)
{
	b_file_available = false;
}
static void v_ftp_enable(void)
{
	b_is_ftp_enable = true;
}
static void v_ftp_disable(void)
{
	b_is_ftp_enable = false;
}
