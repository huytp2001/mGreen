/*! @file gsm_cmd.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
#ifndef GSM_CMD_H
#define GSM_CMD_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_uart.h"
#include "inc/hw_types.h"
#include "driverlib.h"
#include "uartstdio.h"
#include "board_config.h"
/*!
* @data types, constants and macro defintions
*/
#define AT_CMD_BUFFER_SIZE						768
#define AT_RESPONE_BUFFER_SIZE				256
#define MAX_AT_RESP_STATUS_TABLE			5
#define MAX_HANDLE_PDP_CONTEXT_ID			5
	
#define	AT_RESP_OK										"OK"
#define	AT_RESP_ERROR									"ERROR"
#define	AT_RESP_CME_ERROR							"+CME ERROR:"
#define AT_RESP_PWR_DOWN							"POWERED DOWN"
// "+CPIN: READY"
#define AT_RESP_CPIN									"+CPIN: "
#define AT_RESP_CPIN_READY						"+CPIN: READY"
// "+CREG: 0,1" or "+CREG: 0,5"
#define AT_RESP_REGISTERED_NET				"+CREG: "
// "+CGATT: 1"
#define AT_RESP_PS_ATTACHED						"+CGATT: "
// "+CGREG: 0,1" or "+CGREG: 0,5"
#define AT_RESP_REGISTERED_PS					"+CGREG: "
// Start a socket service successfully in transparent mode
#define	AT_RESP_CONNECT_OK						"CONNECT"


typedef enum
{
	GSM_CMD_NONE = 0,
	GSM_CMD_DISABLE_AT_ECHO = 1,
	GSM_CMD_POWER_DOWN,
	GSM_CMD_CHECK_SIM_READY,
	GSM_CMD_CHECK_REGISTERED_NET,
	GSM_CMD_GET_RSSI,
	GSM_CMD_CHECK_PS_ATTACHED,
	GSM_CMD_CHECK_REGISTERED_PS,
	GSM_CMD_CFG_PDP,
	GSM_CMD_CFG_QUALITY_SP_REQ,
	GSM_CMD_CFG_QUALITY_SP_MIN,
	GSM_CMD_CFG_3G_QUALITY_SP_REQ,
	GSM_CMD_CFG_3G_QUALITY_SP_MIN,
	GSM_CMD_ACTIVE_PDP_CONTEXT,
	GSM_CMD_CHECK_PDP_CONTEXT,
	GSM_CMD_DEACTIVE_PDP_CONTEXT,
	GSM_CMD_START_SOCKET_SERVICE,
	GSM_CMD_CLOSE_SOCKET_SERVICE,
	GSM_CMD_QUERY_SOCKET_SERVICE_STATUS,
	GSM_CMD_SEND_SOCKET_DATA,
	GSM_CMD_PING_SERVER,
	GSM_CMD_CFG_MQTT_PROTOCOL_VER,
	GSM_CMD_CFG_MQTT_PDP,
	GSM_CMD_CFG_MQTT_WILL,
	GSM_CMD_CFG_MQTT_MSG_DELIVERY_TIMEOUT,
	GSM_CMD_CFG_MQTT_SESSION_TYPE,
	GSM_CMD_CFG_MQTT_KEEP_ALIVE_TIME,
	GSM_CMD_CFG_MQTT_SSL,
	GSM_CMD_CFG_MQTT_RECV_MODE,
	GSM_CMD_CFG_MQTT_PING_INTERVAL,
	GSM_CMD_OPEN_NETWORK_MQTT,
	GSM_CMD_CLOSE_NETWORK_MQTT,
	GSM_CMD_MQTT_CONNECT,
	GSM_CMD_QUERY_MQTT_CONNECTION,
	GSM_CMD_MQTT_DISCONNECT,
	GSM_CMD_MQTT_SUBSCRIBE,
	GSM_CMD_MQTT_UNSUBSCRIBE,
	GSM_CMD_MQTT_PUBLISH,
	GSM_CMD_CFG_FTP_ACCOUNT,
	GSM_CMD_CFG_FTP_FILE_TYPE,	
	GSM_CMD_CFG_FTP_TRANSMODE,
	GSM_CMD_CFG_FTP_RSP_TIMEOUT,
	GSM_CMD_CFG_FTP_SSL_TYPE,
	GSM_CMD_CFG_FTP_SSL_CONTEXT_ID,
	GSM_CMD_FTP_OPEN,
	GSM_CMD_FTP_SET_CURR_DIR,
	GSM_CMD_FTP_DOWNLOAD_TO_COM,
	GSM_CMD_FTP_DOWNLOAD_TO_RAM,
	GSM_CMD_FTP_DOWNLOAD_TO_UFS,
	GSM_CMD_FTP_LOGOUT,
	GSM_CMD_FILE_OPEN,
	GSM_CMD_FILE_READ,
	GSM_CMD_FILE_CLOSE,
	GSM_CMD_FILE_DELETE,
	GSM_CMD_CHECK_UE,
	NUM_OF_GSM_ACTIVE_CMD
}gsm_active_cmd_t;
	
typedef enum
{
	AT_RESP_STATUS_NONE = 0,
	AT_RESP_STATUS_OK = 1,
	AT_RESP_STATUS_PWR_OFF,
	AT_RESP_STATUS_ERROR,
	AT_RESP_STATUS_CME_ERROR,
	AT_RESP_STATUS_PROMOTING_MARK,
	AT_RESP_STATUS_RDY,
	AT_RESP_STATUS_DISABLE_ECHO,
	AT_RESP_STATUS_CPIN_READY,
	AT_RESP_STATUS_CPIN_NOT_READY,
	AT_RESP_STATUS_CSQ_OK,
	AT_RESP_STATUS_IMEI_OK,
	AT_RESP_STATUS_REGISTERED_NET,
	AT_RESP_STATUS_NOT_REGISTERED_NET,
	AT_RESP_STATUS_PS_ATTACHED,
	AT_RESP_STATUS_PS_NOT_ATTACHED,	
	AT_RESP_STATUS_REGISTERED_PS,
	AT_RESP_STATUS_NOT_REGISTERED_PS,
	AT_RESP_STATUS_PDP_ACTIVE,
	AT_RESP_STATUS_PDP_DEACTIVE,
	AT_RESP_STATUS_CONNECT_OK,
	AT_RESP_STATUS_CONNECT_FAIL,	
	AT_RESP_STATUS_READY_TO_SEND,
	AT_RESP_STATUS_SEND_OK,
	AT_RESP_STATUS_SEND_FAIL,	
	AT_RESP_STATUS_SEND_ACK,	
	AT_RESP_STATUS_SOCKET_CLOSED,
	AT_RESP_STATUS_CLOSE_OK,	
	AT_RESP_STATUS_CLOSED,	
	AT_RESP_STATUS_INCOMING_DATA,
	AT_RESP_STATUS_RECEIVED_DATA,
	AT_RESP_STATUS_PING_OK,
	AT_RESP_STATUS_PING_FAIL,
	AT_RESP_STATUS_OPEN_NETWORK_MQTT_OK,
	AT_RESP_STATUS_OPEN_NETWORK_MQTT_FAIL,
	AT_RESP_STATUS_CLOSE_NETWORK_MQTT_OK,
	AT_RESP_STATUS_CLOSE_NETWORK_MQTT_FAIL,
	AT_RESP_STATUS_MQTT_START_OK,
	AT_RESP_STATUS_MQTT_START_FAIL,
	AT_RESP_STATUS_MQTT_CONNECT_OK,
	AT_RESP_STATUS_MQTT_CONNECT_FAIL,
	AT_RESP_STATUS_MQTT_DISCONNECT_OK,
	AT_RESP_STATUS_MQTT_DISCONNECT_FAIL,
	AT_RESP_STATUS_MQTT_SUBSCRIBE_OK,
	AT_RESP_STATUS_MQTT_SUBSCRIBE_FAIL,
	AT_RESP_STATUS_MQTT_UNSUBSCRIBE_OK,
	AT_RESP_STATUS_MQTT_UNSUBSCRIBE_FAIL,
	AT_RESP_STATUS_MQTT_PUBLISH_OK,
	AT_RESP_STATUS_MQTT_PUBLISH_FAIL,
	AT_RESP_STATUS_MQTT_INCOMING_DATA,
	AT_RESP_STATUS_MQTT_RECV_DATA_OK,
	AT_RESP_STATUS_MQTT_RECV_DATA_FAIL,
	AT_RESP_STATUS_FTP_OPEN_OK,
	AT_RESP_STATUS_FTP_OPEN_FAIL,
	AT_RESP_STATUS_FTP_SET_CURR_DIR_OK,
	AT_RESP_STATUS_FTP_SET_CURR_DIR_FAIL,	
	AT_RESP_STATUS_FTP_DOWNLOAD_TO_COM_OK,
	AT_RESP_STATUS_FTP_DOWNLOAD_TO_COM_FAIL,
	AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK,
	AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL,
	AT_RESP_STATUS_FTP_DOWNLOAD_TO_UFS_OK,
	AT_RESP_STATUS_FTP_DOWNLOAD_TO_UFS_FAIL,
	AT_RESP_STATUS_FTP_LOGOUT_OK, 
	AT_RESP_STATUS_FTP_LOGOUT_FAIL,
	AT_RESP_STATUS_FILE_OPEN_OK,
	AT_RESP_STATUS_FILE_OPEN_FAIL,
	AT_RESP_STATUS_FILE_READ_OK,
	AT_RESP_STATUS_FILE_READ_FAIL,
	AT_RESP_STATUS_MQTT_NETWORK_CLOSE,
	AT_RESP_STATUS_SYSTEM_OK,
	AT_RESP_STATUS_SYSTEM_NOSERVICE,
	AT_RESP_STATUS_FTP_GET_SIZE_OK,
	AT_RESP_STATUS_FTP_GET_SIZE_FAIL,
	AT_RESP_STATUS_FTP_CHECK_FUN_OK,
	AT_RESP_STATUS_FTP_CHECK_FUN_FAIL,
	AT_RESP_STATUS_FTP_START_OK,
	AT_RESP_STATUS_FTP_START_FAIL,
	AT_RESP_STATUS_FTP_STOP_OK,
	AT_RESP_STATUS_FTP_STOP_FAIL,
	AT_RESP_STATUS_FTP_SET_TYPE_OK,
	AT_RESP_STATUS_FTP_SET_TYPE_FAIL,
	NUM_OF_AT_RESP_STATUS
}at_resp_status_t;

typedef enum
{
	VIETTEL = 0,
	VINAPHONE = 1,
	MOBIPHONE = 2,
	NUM_OF_NETWORK_OPERATOR
}network_operator_t;

#define TCP_CLIENT											"TCP"
#define UDP_CLIENT											"UDP"
#define TCP_SERVER											"TCP LISTENER"
#define UDP_SERVICE											"UDP SERVICE"

typedef enum
{
	BUFFER_ACCESS = 0,
	DIRECT_PUSH = 1,
	TRANSPARENT_ACCESS = 2,
	NUM_OF_SOCKET_DATA_ACCESS_MODE
}socket_data_access_mode_t;

#define INTERVAL_TIME_TO_CHECK_AT_RESP			10 		//ms
#define TIME_TO_WAIT_AT_RESP								900		//x10ms
#define TIME_TO_WAIT_AT_RESP_PWR_DOWN				6000	//x10ms
#define TIME_TO_WAIT_AT_RESP_PDP_DEACT_MAX	4000	//x10ms
#define TIME_TO_WAIT_AT_RESP_QIOPEN					15000	//x10ms
#define TIME_TO_WAIT_AT_RESP_QICLOSE				1000	//x10ms
#define TIME_TO_WAIT_AT_RESP_QISEND_ACK			500	//x10ms
#define TIME_TO_WAIT_TCP_INCOMING_DATA			200 //x10ms

#define TIME_TO_WAIT_ATE_RESP								12000		//x10ms
#define TIME_TO_WAIT_MQTTSTART_RESP					1200		//x10ms

#define MQTT_URC_QMTRECV										15

#define FTP_DOWNLOAD_TO_COM					"COM:"
#define FTP_DOWNLOAD_TO_RAM					"RAM:"

/*!
* @public functions prototype
*/
void v_gsm_check_at_respone (void);
bool b_gsm_cmd_check_rdy(void);
bool b_gsm_cmd_disable_at_echo(void);
bool b_gsm_cmd_power_down(void);
bool b_gsm_cmd_check_sim_ready(void);
bool b_gsm_cmd_check_registered_network(void);
bool b_gsm_cmd_get_rssi(void);
bool b_gsm_cmd_check_ps_attached(void);
bool b_gsm_cmd_check_registered_ps(void);
gsm_active_cmd_t e_gsm_active_cmd_get(void);
void v_gsm_parse_mqtt_mess(uint8_t *pu8_data);
bool b_is_activating(char c_pdp_context_id);
bool b_gsm_cmd_cfg_pdp(uint8_t u8_context_id, uint8_t u8_context_type,
																network_operator_t e_network_operator);
bool b_gsm_cmd_cfg_quality_SP_req(uint8_t u8_context_id, uint8_t u8_precedence,
			uint8_t u8_delay, uint8_t u8_reliability, uint8_t u8_peak, uint8_t u8_mean);
bool b_gsm_cmd_cfg_quality_SP_min(uint8_t u8_context_id, uint8_t u8_precedence,
			uint8_t u8_delay, uint8_t u8_reliability, uint8_t u8_peak, uint8_t u8_mean);
bool b_gsm_cmd_cfg_3g_quality_SP_req(uint8_t u8_context_id, uint8_t u8_traffic_class,
			uint16_t u16_max_bit_rate_ul, uint16_t u16_max_bit_rate_dl,
			uint16_t u16_guaranteed_bit_rate_ul, uint16_t u16_guaranteed_bit_rate_dl,
			uint8_t u8_delivery_order, uint16_t u16_max_sdu_size, 
			const char* str_sdu_error_ratio, const char* str_residual_bit_error_ratio,
			uint8_t u8_delivery_of_erroneous_sdus, uint16_t u16_transfer_delay,
			uint8_t u8_traffic_handling_priority, uint8_t u8_source_statistics_descriptor,
			uint8_t u8_signaling_indication);
bool b_gsm_cmd_cfg_3g_quality_SP_min(uint8_t u8_context_id, uint8_t u8_traffic_class,
			uint16_t u16_max_bit_rate_ul, uint16_t u16_max_bit_rate_dl,
			uint16_t u16_guaranteed_bit_rate_ul, uint16_t u16_guaranteed_bit_rate_dl,
			uint8_t u8_delivery_order, uint16_t u16_max_sdu_size, 
			const char* str_sdu_error_ratio, const char* str_residual_bit_error_ratio,
			uint8_t u8_delivery_of_erroneous_sdus, uint16_t u16_transfer_delay,
			uint8_t u8_traffic_handling_priority, uint8_t u8_source_statistics_descriptor,
			uint8_t u8_signaling_indication);
bool b_gsm_cmd_activate_pdp_context(uint8_t u8_context_id);
bool b_gsm_cmd_check_pdp_context(void);
bool b_gsm_cmd_deactivate_pdp_context(uint8_t u8_context_id);
bool b_gsm_cmd_start_socket_service (uint8_t u8_contex_id, uint8_t u8_connect_id,
											const char* str_service_type, const char* str_domain_name,
											uint16_t u16_remote_port, uint16_t u16_local_port,
											socket_data_access_mode_t e_socket_data_access_mode);
bool b_gsm_cmd_close_socket_service (uint8_t u8_connect_id, uint16_t u16_timeout_s);
bool b_gsm_cmd_query_socket_service_status (uint8_t u8_query_type, uint8_t u8_connect_id);
bool b_gsm_cmd_send_socket_data (uint8_t u8_connect_id, uint8_t* pu8_data, uint16_t u16_len);
bool b_gsm_cmd_ping_server (uint8_t u8_context_id, const char* str_domain_name,
														uint8_t	u8_timeout, uint8_t u8_pingnum);
bool b_gsm_cmd_cfg_mqtt_protocol_ver (uint8_t u8_client_id, uint8_t u8_version);
bool b_gsm_cmd_cfg_mqtt_pdp (uint8_t u8_client_id, uint8_t u8_cid);
bool b_gsm_cmd_cfg_mqtt_will (uint8_t u8_client_id, uint8_t u8_will_flag, uint8_t u8_will_qos,
															uint8_t u8_will_retain, const char* str_will_topic,
															const char* str_will_msg);
bool b_gsm_cmd_cfg_mqtt_msg_delivery_timeout (uint8_t u8_client_id, uint8_t u8_pkt_timeout,
																							uint8_t u8_retry_times, uint8_t u8_timeout_notice);
bool b_gsm_cmd_cfg_mqtt_session_type (uint8_t u8_client_id, uint8_t u8_clean_session);
bool b_gsm_cmd_cfg_mqtt_keep_alive_time (uint8_t u8_client_id, uint16_t u16_keep_alive_time_s);
bool b_gsm_cmd_cfg_mqtt_ssl (uint8_t u8_client_id, uint8_t u8_ssl_enable, uint8_t u8_ssl_ctx_id);
bool b_gsm_cmd_cfg_mqtt_recv_mode (uint8_t u8_client_id, uint8_t u8_recv_mode);
bool b_gsm_cmd_cfg_mqtt_ping_interval (uint8_t u8_client_id, uint16_t u16_interval_s);
bool b_gsm_cmd_open_network_mqtt (uint8_t u8_client_id, const char* str_host_name,
																	uint16_t u16_port, uint16_t u16_timewait);
bool b_gsm_cmd_close_network_mqtt (uint8_t u8_client_id, uint16_t u16_timewait);
bool b_gsm_cmd_mqtt_connect (uint8_t u8_client_id, const char* str_client_id,
														const char* str_username, const char* str_password,
														uint16_t u16_timewait);
bool b_gsm_cmd_mqtt_query_connection (void);
bool b_gsm_cmd_mqtt_disconnect (uint8_t u8_client_id, uint16_t u16_timewait);
bool b_gsm_cmd_mqtt_subscibe (uint8_t u8_client_id, uint16_t u16_msg_id,
															const char* str_topic, uint8_t u8_qos,
															uint16_t u16_pkt_timeout, uint8_t u8_retry_time);
bool b_gsm_cmd_mqtt_unsubscibe (uint8_t u8_client_id, uint16_t u16_msg_id,
																const char* str_topic, uint16_t u16_pkt_timeout,
																uint8_t u8_retry_time);
bool b_gsm_cmd_mqtt_publish_msg (uint8_t u8_client_id, uint16_t u16_msg_id, uint8_t u8_qos, 
						uint8_t u8_retain, const char* str_topic, uint16_t u16_len, uint8_t* pu8_data,
						uint16_t u16_pkt_timeout, uint8_t u8_retry_time);
bool b_gsm_cmd_cfg_ftp_account (const char* str_user_name, const char* str_password);						
bool b_gsm_cmd_cfg_ftp_file_type (uint8_t u8_file_type);
bool b_gsm_cmd_cfg_ftp_transmode (uint8_t u8_transmode);		
bool b_gsm_cmd_cfg_ftp_context_id (uint8_t u8_cid);				
bool b_gsm_cmd_cfg_ftp_rsp_timeout (uint8_t u8_rsp_timeout);	
bool b_gsm_cmd_cfg_ftp_ssl_type (uint8_t u8_rsp_ssl_type);	
bool b_gsm_cmd_cfg_ftp_ssl_context_id (uint8_t u8_rsp_ssl_context_id);						
bool b_gsm_cmd_ftp_open (const char* str_host_name, uint16_t u16_port, uint16_t u16_timewait);
bool b_gsm_cmd_ftp_set_curr_dir (const char* str_path_name, uint16_t u16_timewait);
bool b_gsm_cmd_ftp_download (const char* str_file_name, const char* str_local_name,
														uint32_t u32_start_pos, uint32_t u32_download_len, 
														uint16_t u16_timewait);
bool b_gsm_cmd_ftp_wait_download_done(uint16_t u16_timewait);
bool b_gsm_cmd_ftp_logout(void);
bool b_gsm_cmd_file_open(const char* c_filename, uint8_t u8_open_mode);
void v_gsm_file_handle_set(char *pc_handle);
bool b_gsm_cmd_file_read(uint16_t u16_read_length, uint16_t u16_timeout);
bool b_gsm_cmd_file_close(void);
bool b_gsm_cmd_file_delete(const char* c_filename);
int8_t i8_gsm_cmd_esp_check_mqtt_state(void);
bool b_gsm_cmd_esp_mqtt_publish(uint16_t u16_len, uint8_t* pu8_data);
bool b_gsm_cmd_esp_ftp_download(const char* str_file_name, uint32_t u32_crc, uint16_t u16_timewait);
bool b_gsm_cmd_esp_file_pull(uint16_t u16_timewait);
bool b_gsm_cmd_get_imei(void);
void v_gsm_imei_set(char *pc_handle);
bool b_gsm_cmd_check_ue_system(void);
bool b_gsm_cmd_activate_pdp_context_4g(bool b_active, uint8_t u8_context_id);
bool b_gsm_cmd_check_pdp_context_4g(void);
bool b_gsm_mqtt_start_4g(void);
bool b_gsm_mqtt_acquire_client(uint8_t u8_client_index);
bool b_gsm_cmd_config_utf8_4g(uint8_t u8_client_id,bool b_check_flag, uint8_t u8_timeout);
bool b_gsm_cmd_mqtt_connect_4g(uint8_t u8_client_id, const char* str_host_name,
															 uint16_t u16_timewait, uint8_t u8_clean_session,
															 const char* str_username,const char* str_password);
bool b_gsm_cmd_mqtt_disconnect_4g (uint8_t u8_client_id, uint16_t u16_timewait);
bool b_gsm_cmd_query_me(void);
bool b_gsm_cmd_ftp_start_4g(void);
bool b_gsm_cmd_ftp_login_4g (const char* str_host_name, uint16_t u16_port, const char* str_user_name,
														 const char* str_password, uint8_t u8_server_type ,uint16_t u16_timewait);
bool b_gsm_cmd_ftp_set_transfer_type(void);
bool b_gsm_cmd_ftp_set_dir_4g (const char* str_path_name, uint16_t u16_timewait);
bool b_gsm_cmd_ftp_get_file_size_4g (const char* str_path_name);
bool b_gsm_cmd_ftp_get_file_serial_4g(const char* str_path_name);
bool b_gsm_cmd_ftp_logout_4g(void);
bool b_gsm_cmd_get_file_from_fs_4g (const char* str_path_name, uint32_t u32_location, uint16_t u16_size);
bool b_gsm_cmd_file_delete_4g(const char* c_filename);
bool b_gsm_cmd_ftp_stop_4g(void);
bool b_gsm_cmd_mqtt_publish_msg_4g (uint8_t u8_client_id, uint8_t u8_qos, uint8_t u8_retain, 
																		const char* str_topic, uint16_t u16_len, uint8_t* pu8_payload,
																		uint16_t u16_pkt_timeout, uint8_t u8_retry_time);
bool b_gsm_cmd_mqtt_subscibe_4g(uint8_t u8_client_id, const char* str_topic, uint8_t u8_qos, uint8_t u8_retry_time);
bool b_gsm_cmd_mqtt_unsubscibe_4g(uint8_t u8_client_id, const char* str_topic);																		
#ifdef __cplusplus
}
#endif

#endif /* GSM_CMD_H_ */
