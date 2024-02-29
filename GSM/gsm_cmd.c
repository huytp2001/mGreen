/*! @file gsm_cmd.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2017.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdlib.h>
#include <stdint.h>
#include "gsm_cmd.h"
#include "gsm_hal.h"
#include "uartstdio.h"
#include "ustdlib.h"
#include "mqtt_task.h"
#include "setting.h"
#include "gsm_check_at.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define TOPIC_ID					(stru_neces_params_get().u32_mqtt_topic)
/*!
* static data declaration
*/
static gsm_active_cmd_t e_gsm_active_cmd = GSM_CMD_NONE;
static uint8_t au8_at_cmd_buf[AT_CMD_BUFFER_SIZE]; 
static bool b_is_mqtt_network_close = false;

static uint8_t au8_active_pdp_table[MAX_HANDLE_PDP_CONTEXT_ID] = {0};	/**<This array contains
																								ids of activated or activating pdp contexts */
static char c_file_handle[5] = {0}; /**<File handle returned from Module SIM when QFOPEN command is issued */
static char c_imei[7] = {'A','7','6','7','0','C','L'};
/*!
* private function prototype
*/
static bool b_check_at_resp_status_from_table (at_resp_status_t e_status,
																							 at_resp_status_t e_not_expected_status,
																							 at_resp_status_t e_status_2,
																							 at_resp_status_t e_not_expected_status_2);
static bool b_check_at_resp_status_from_table_timewait (at_resp_status_t e_status,
																							 at_resp_status_t e_not_expected_status,
																							 uint16_t u16_wait_in_sec,
																							 at_resp_status_t e_status_2,
																							 at_resp_status_t e_not_expected_status_2,
																							 uint16_t u16_wait_in_sec_2);
static bool b_pdp_context_id_add(uint8_t u8_context_id);
static void v_pdp_context_id_remove(uint8_t u8_context_id);
static int8_t i8_check_at_resp_status_from_table_timewait (at_resp_status_t e_status,
																				 at_resp_status_t e_not_expected_status,
																				 uint16_t u16_wait_time,
																				 at_resp_status_t e_status_2,
																				 at_resp_status_t e_not_expected_status_2,
																				 uint16_t u16_wait_time_2);
/*!
* public function bodies
*/

/*!
 * @fn bool b_gsm_cmd_check_rdy(void)
 * @brief This function is used for checking module SIM is ready or not.
 * @param[in] 
 * @see
 * @return true if module SIM report RDY
 */
bool b_gsm_cmd_check_rdy(void)
{	
	e_gsm_active_cmd = GSM_CMD_NONE;
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_RDY, AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, AT_RESP_STATUS_NONE);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_disable_at_echo(void)
 * @brief This function is used for configurating SIM
 * Disable feature echo command of module sim.
 * @param[in] 
 * @see
 * @return true if that configuration sucess.
 */
bool b_gsm_cmd_disable_at_echo(void)
{	
	v_delete_all_resp_status_on_table();	
	e_gsm_active_cmd = GSM_CMD_DISABLE_AT_ECHO;
	//Disable echo mode
	UARTprintf("ATE0");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_DISABLE_ECHO, AT_RESP_STATUS_NONE,
																										 TIME_TO_WAIT_AT_RESP,
																										 AT_RESP_STATUS_OK, AT_RESP_STATUS_NONE,
																										 TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_power_down(void)
 * @brief This function is used to turn of module sim by AT command AT+QPOWD
 * @param[in] 
 * @see
 * @return true if turn of successfuly
 */
bool b_gsm_cmd_power_down(void)
{	
	v_delete_all_resp_status_on_table();	
	e_gsm_active_cmd = GSM_CMD_POWER_DOWN;
	//Power off module sim by cmd AT+QPOWD
	UARTprintf("AT+QPOWD");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_PWR_OFF,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP_PWR_DOWN);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_check_sim_ready(void)
 * @brief This function is used for checking SIM CPIN ready or not?
 * @param[in] 
 * @see
 * @return true if SIM Ready
 */
bool b_gsm_cmd_check_sim_ready(void)
{	
	v_delete_all_resp_status_on_table();	
	e_gsm_active_cmd = GSM_CMD_CHECK_SIM_READY;
	//Check CPIN
	UARTprintf("AT+CPIN?");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_CPIN_READY,
																						AT_RESP_STATUS_CPIN_NOT_READY,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}
/*!
 * @fn bool b_gsm_cmd_check_registered_network(void)
 * @brief This function is used for checking SIM is registered to network or not?
 * @param[in] 
 * @see
 * @return true if sim registered network
 */
bool b_gsm_cmd_check_registered_network(void)
{	
	v_delete_all_resp_status_on_table();	
	e_gsm_active_cmd = GSM_CMD_CHECK_REGISTERED_NET;
	//Check Network Registered
	UARTprintf("AT+CREG?");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_REGISTERED_NET,
																						AT_RESP_STATUS_NOT_REGISTERED_NET,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}
/*!
 * @fn bool b_gsm_cmd_get_rssi(void)
 * @brief This function is used get sim RSSI
 * @param[in] 
 * @see
 * @return true if sucess
 */
bool b_gsm_cmd_get_rssi(void)
{	
	e_gsm_active_cmd = GSM_CMD_GET_RSSI;
	//Get RSSI
	UARTprintf("AT+CSQ");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_CSQ_OK,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_check_ps_attached(void)
 * @brief This function is used to check is MT attached to Packet Domain service
 * @param[in] none
 * @see
 * @return true if sucess MT is attached to Packet Domain service
 */
bool b_gsm_cmd_check_ps_attached(void)
{	
	e_gsm_active_cmd = GSM_CMD_CHECK_PS_ATTACHED;
	//Check MT is attached to Packet Domain service
	UARTprintf("AT+CGATT?");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_PS_ATTACHED,
																						AT_RESP_STATUS_PS_NOT_ATTACHED,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_check_registered_ps(void)
 * @brief This function is used for checking SIM
 * is registered to PS or not?
 * @param[in] 
 * @see
 * @return true if sim registered network
 */
bool b_gsm_cmd_check_registered_ps(void)
{	
	e_gsm_active_cmd = GSM_CMD_CHECK_REGISTERED_PS;
	v_delete_all_resp_status_on_table();	
	//Check PS Registered
	UARTprintf("AT+CGREG?");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_REGISTERED_PS,
																						AT_RESP_STATUS_NOT_REGISTERED_PS,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_pdp(uint8_t u8_context_id, uint8_t u8_context_type,
 *																network_operator_t e_network_operator)
 * @brief Configure the <APN>, <username>, <password> and other contexts by AT+QICSGP.
 * The QoS of the context can be configured by AT+CGQMIN, AT+CGEQMIN, AT+CGQREQ and
 * AT+CGEQREQ.
 * AT+QICSGP=<contextID>,<context_type>,[<APN>[,<username>,<password>)[,<authentication>]]]
 * @param[in] u8_context_id Integer type, context ID, range is 1-16
 * @param[in] u8_context_type Integer type, protocol type
 * 						1 IPV4
 * @param[in] e_network_operator
 *						0 VIETTEL
 *						1 VINAPHONE
 *						2 MOBIPHONE
 * @see
 * @return true if sim registered network
 */
bool b_gsm_cmd_cfg_pdp(uint8_t u8_context_id, uint8_t u8_context_type,
																network_operator_t e_network_operator)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_PDP;
	v_delete_all_resp_status_on_table();	
	// Configure PDP context
	// AT+QICSGP=<contextID>,<context_type>,[<APN>[,<username>,<password>)[,<authentication>]]]
	// <contextID> Integer type, context ID, range is 1-16
	// <context_type> Integer type, protocol type
	// 1 IPV4
	// <APN> String type, access point name
	// <username> String type, user name
	// <password> String type, password
	// <authentication> Integer type, the authentication methods
	// 0 NONE
	// 1 PAP
	// 2 CHAP
	// 3 PAH_OR_CHAP
	if(e_network_operator == VIETTEL)
	{
		//APN: v-internet, for D-COM is: e-connect
		//Username:
		//Password:
		UARTprintf("AT+QICSGP=%d,%d,\"v-internet\",\"\",\"\",0", u8_context_id, u8_context_type);
	}
	else if(e_network_operator == VINAPHONE)
	{
		//APN: m3-world
		//Username: mms
		//Password: mms
		UARTprintf("AT+QICSGP=%d,&d,\"m3-world\",\"mms\",\"mms\",0",u8_context_id,u8_context_type);		
	}
	else if(e_network_operator == MOBIPHONE)
	{
		//APN: m-wap
		//Username: mms
		//Password: mms
		UARTprintf("AT+QICSGP=%d,%d,\"m-wap\",\"mms\",\"mms\",0",u8_context_id, u8_context_type);				
	}
	
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
		
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_quality_SP_req(uint8_t u8_context_id, uint8_t u8_precedence,
 *			uint8_t u8_delay, uint8_t u8_reliability, uint8_t u8_peak, uint8_t u8_mean)
 * @brief AT+CGQREQ allows the TE to specify a quality of service profile that is
 * used when the MT activates a PDP context.
 * The write command specifies a profile for the context <cid>
 * AT+CGQREQ=<cid> causes the requested profile for context number <cid> to become 
 * undefined. The read command returns the current settings for each defined context.
 * AT+CGQREQ=<cid>[,<precedence>[,<delay>[,<reliability>[,<peak>[,<mean>]]]]]
 * @param[in] u8_context_id A numeric parameter which specifies a particular PDP context
 *						definition (see +CGDCONT command)
 * @param[in] u8_precedence A numeric parameter which specifies the precedence class
 *						0 Network subscribed value
 *						1 High priority mitments shall be maintained ahead of precedence
 *							classes 2 and 3
 *						2 Normal priority. Service commitments shall be maintained ahead of
 *							precedence class 3
 *						3 Low priority. Service commitments shall be maintained.
 * @param[in] u8_delay A numeric parameter which specifies the delay class. This
 * parameter defines the end-to-end transfer delay incurred in the transmission of SDUs
 * through the network. For the detail please refer to Table 5: Delay Class on "Quectel
 * UC15 AT Commands Manual V1.1.pdf"
 *						0 Network subscribed value
 * @param[in] u8_reliability A numeric parameter which specifies the reliability class
 *						0 Network subscribed value
 *						1 Non real-time traffic, error-sensitive application that cannot cope with
 *							data loss.
 *						2 Non real-time traffic, error-sensitive application that can cope with 
 *							infrequent data loss.
 *						3 Non real-time traffic, error-sensitive application that can cope with 
 *							data loss,GMM/SM, and SMS.
 *						4 Real-time traffic, error-sensitive application that can cope with data 
 *							loss.
 *						5 Real-time traffic, error non-sensitive application that can cope with 
 *							data loss.
 * @param[in] u8_peak A numeric parameter which specifies the peak throughput class, 
 * in octets per second.
 *						0 Network subscribed value
 *						1 Up to 1 000 (8 kbit/s)
 *						2 Up to 2 000 (16 kbit/s)
 *						3 Up to 4 000 (32 kbit/s)
 *						4 Up to 8 000 (64 kbit/s)
 *						5 Up to 16 000 (128 kbit/s)
 *						6 Up to 32 000 (256 kbit/s)
 *						7 Up to 64 000 (512 kbit/s)
 *						8 Up to 128 000 (1024 kbit/s)
 *						9 Up to 256 000 (2048 kbit/s)
 * @param[in] u8_mean A numeric parameter which specifies the mean throughput class, 
 * in octets per hour
 *						0 Network subscribed value
 *						1 100 (~0.22 bit/s)
 *						2 200 (~0.44 bit/s)
 *						3 500 (~1.11 bit/s)
 *						4 1 000 (~2.2 bit/s)
 *						5 2 000 (~4.4 bit/s)
 *						6 5 000 (~11.1 bit/s)
 *						7 10 000 (~22 bit/s)
 *						8 20 000 (~44 bit/s)
 *						9 50 000 (~111 bit/s)
 *						10 100 000 (~0.22 kbit/s)
 *						11 200 000 (~0.44 kbit/s)
 *						12 500 000(~1.11 kbit/s)
 *						13 1 000 000 (~2.2 kbit/s)
 *						14 2 000 000 (~4.4 kbit/s)
 * @see
 * @return true if configure successfully
 */
bool b_gsm_cmd_cfg_quality_SP_req(uint8_t u8_context_id, uint8_t u8_precedence,
			uint8_t u8_delay, uint8_t u8_reliability, uint8_t u8_peak, uint8_t u8_mean)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_QUALITY_SP_REQ;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CGQREQ=%u,%u,%u,%u,%u,%u", u8_context_id, u8_precedence,
						u8_delay, u8_reliability, u8_peak, u8_mean);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_CME_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_quality_SP_min(uint8_t u8_context_id, uint8_t u8_precedence,
 *			uint8_t u8_delay, uint8_t u8_reliability, uint8_t u8_peak, uint8_t u8_mean)
 * @brief AT+CGQMIN allows the TE to specify a minimum acceptable profile which is
 * checked by the MT against the negotiated profile when the PDP context is activated. 
 * The write command specifies a profile for the context identified by the context 
 * identification parameter <cid>.
 * A special form of the write command, AT+CGQMIN=<cid> causes the minimum acceptable 
 * profile for context number <cid> to become undefined. In this case no check is made 
 * against the negotiated profile.
 * The read command returns the current settings for each defined context. 
 * AT+CGQMIN=<cid>[,<precedence>[,<delay>[,<reliability>[,<peak>[,<mean>]]]]]
 * @param[in] u8_context_id A numeric parameter which specifies a particular PDP context
 *						definition (see +CGDCONT command)
 * @param[in] u8_precedence A numeric parameter which specifies the precedence class
 *						0 Network subscribed value
 *						1 High priority mitments shall be maintained ahead of precedence
 *							classes 2 and 3
 *						2 Normal priority. Service commitments shall be maintained ahead of
 *							precedence class 3
 *						3 Low priority. Service commitments shall be maintained.
 * @param[in] u8_delay A numeric parameter which specifies the delay class. This
 * parameter defines the end-to-end transfer delay incurred in the transmission of SDUs
 * through the network. For the detail please refer to Table 5: Delay Class on "Quectel
 * UC15 AT Commands Manual V1.1.pdf"
 *						0 Network subscribed value
 * @param[in] u8_reliability A numeric parameter which specifies the reliability class
 *						0 Network subscribed value
 *						1 Non real-time traffic, error-sensitive application that cannot cope with
 *							data loss.
 *						2 Non real-time traffic, error-sensitive application that can cope with 
 *							infrequent data loss.
 *						3 Non real-time traffic, error-sensitive application that can cope with 
 *							data loss,GMM/SM, and SMS.
 *						4 Real-time traffic, error-sensitive application that can cope with data 
 *							loss.
 *						5 Real-time traffic, error non-sensitive application that can cope with 
 *							data loss.
 * @param[in] u8_peak A numeric parameter which specifies the peak throughput class, 
 * in octets per second.
 *						0 Network subscribed value
 *						1 Up to 1 000 (8 kbit/s)
 *						2 Up to 2 000 (16 kbit/s)
 *						3 Up to 4 000 (32 kbit/s)
 *						4 Up to 8 000 (64 kbit/s)
 *						5 Up to 16 000 (128 kbit/s)
 *						6 Up to 32 000 (256 kbit/s)
 *						7 Up to 64 000 (512 kbit/s)
 *						8 Up to 128 000 (1024 kbit/s)
 *						9 Up to 256 000 (2048 kbit/s)
 * @param[in] u8_mean A numeric parameter which specifies the mean throughput class, 
 * in octets per hour
 *						0 Network subscribed value
 *						1 100 (~0.22 bit/s)
 *						2 200 (~0.44 bit/s)
 *						3 500 (~1.11 bit/s)
 *						4 1 000 (~2.2 bit/s)
 *						5 2 000 (~4.4 bit/s)
 *						6 5 000 (~11.1 bit/s)
 *						7 10 000 (~22 bit/s)
 *						8 20 000 (~44 bit/s)
 *						9 50 000 (~111 bit/s)
 *						10 100 000 (~0.22 kbit/s)
 *						11 200 000 (~0.44 kbit/s)
 *						12 500 000(~1.11 kbit/s)
 *						13 1 000 000 (~2.2 kbit/s)
 *						14 2 000 000 (~4.4 kbit/s)
 * @see
 * @return true if configure successfully
 */
bool b_gsm_cmd_cfg_quality_SP_min(uint8_t u8_context_id, uint8_t u8_precedence,
			uint8_t u8_delay, uint8_t u8_reliability, uint8_t u8_peak, uint8_t u8_mean)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_QUALITY_SP_MIN;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CGQMIN=%u,%u,%u,%u,%u,%u", u8_context_id, u8_precedence,
						u8_delay, u8_reliability, u8_peak, u8_mean);			
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_CME_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_3g_quality_SP_req(uint8_t u8_context_id, uint8_t u8_traffic_class,
 *			uint16_t u16_max_bit_rate_ul, uint16_t u16_max_bit_rate_dl,
 *			uint16_t u16_guaranteed_bit_rate_ul, uint16_t u16_guaranteed_bit_rate_dl,
 *			uint8_t u8_delivery_order, uint16_t u16_max_sdu_size, 
 *			const char* str_sdu_error_ratio, const char* str_residual_bit_error_ratio,
 *			uint8_t u8_delivery_of_erroneous_sdus, uint16_t u16_transfer_delay,
 *			uint8_t u8_traffic_handling_priority, uint8_t u8_source_statistics_descriptor,
 *			uint8_t u8_signaling_indication)
 * @brief AT+CGEQREQ allows the TE to specify a UMTS Quality of Service Profile that is
 * used when the MT activates a PDP context.
 * AT+CGEQREQ=[<cid>[,<Trafficclass>[,<Maximum bitrate UL>
 *									[,<Maximum bitrate DL>[,<Guaranteed bitrate UL>
 *									[,<Guaranteed bitrate DL>[,<Delivery order>
 *									[,<Maximum SDU size> [,<SDU error ratio>
 *									[,<Residual bit error ratio> [,<Delivery of erroneous SDUs>
 *									[,<Transfer delay> [,<Traffic handling priority>
 *									[,<Source statistics descriptor> 
 *									[,<Signalling indication>]]]]]]]]]]]]]]]
 * @param[in] u8_context_id PDP context identifier, a numeric parameter which specifies a
 * particular PDP context definition. The parameter is local to the TE-MT interface and is
 * used in other PDP context-related commands. The range of permitted values (minimum 
 * value=1) is returned by the test form of the command.
 * @param[in] u8_traffic_class  Integer type, indicates the type of application for which 
 * the UMTS bearer service is optimized (refer 3GPP TS 24.008 subclause 10.5.6.5). If the
 * Traffic class is specified as conversational or streaming, then the Guaranteed and
 * Maximum bitrate parameters should also be provided.
 *								0 Conversational 
 *								1 Streaming
 *								2 Interactive
 *								3 Background
 *								4 Subscribed value
 * @param[in] u16_max_bit_rate_ul Integer type, indicates the maximum number of kbits/s 
 * delivered to UMTS (up-link traffic) at a SAP. As an example a bit rate of 32kbit/s 
 * would be specified as '32' (e.g. AT+CGEQREQ=…,32, …)
 * 								0 Subscribed value
 *								1~5760 
 * @param[in] u16_max_bit_rate_dl Integer type, indicates the maximum number of kbits/s 
 * delivered by UMTS (down-link traffic) at a SAP. As an example a bitrate of 32kbit/s 
 * would be specified as '32' (e.g. AT+CGEQREQ=…,32, …)
 *								0 Subscribed value
 *								1~21600
 * @param[in] u16_guaranteed_bit_rate_ul Integer type, indicates the guaranteed number of
 * kbits/s delivered to UMTS (up-link traffic) at a SAP (provided that there is data to 
 * deliver). As an example a bitrate of 32kbit/s would be specified as '32' 
 * (e.g.T+CGEQREQ=…,32, …)
 *								0 Subscribed value
 *								1~5760
 * @param[in] u16_guaranteed_bit_rate_dl Integer type, indicates the guaranteed number of
 * kbits/s delivered by UMTS (down-link traffic) at a SAP (provided that there is data to 
 * deliver). As an example a bitrate of 32kbit/s would be specified as '32' 
 * (e.g.AT+CGEQREQ=…,32, …)
 *								0 Subscribed value
 *								1~21600
 * @param[in] u8_delivery_order Integer type, indicates whether the UMTS bearer shall 
 * provide in-sequence SDU delivery or not (refer 3GPP TS 24.008 subclause 10.5.6.5)
 *								0 No
 *								1 Yes
 *								2 Subscribed value
 * @param[in] u16_max_sdu_size Integer type, (1,2,3,…) indicates the maximum allowed SDU 
 * size in octets. If the parameter is set to '0' the subscribed value will be requested 
 * (refer 3GPP TS 24.008 subclause 10.5.6.5)
 *								0 Subscribed value
 *								10...1520 (value needs to be divisible by 10 without remainder)
 *								1502
 * @param[in] str_sdu_error_ratio String type, indicates the target value for the fraction 
 * of SDUs lost or detected as erroneous. SDU error ratio is defined only for conforming 
 * traffic. The value is specified as 'mEe'. As an example a target SDU error ratio of
 * 5*10-3 would be specified as "5E3" (e.g.AT+CGEQREQ=…,"5E3",…)
 *								"0E0" Subscribed value
 *								"1E2"
 *								"7E3"
 *								"1E3"
 *								"1E4"
 *								"1E5"
 *								"1E6"
 *								"1E1"
 * @param[in] str_residual_bit_error_ratio String type, indicates the target value for 
 * the undetected bit error ratio in the delivered SDUs. If no error detection is 
 * requested, Residual bit error ratio indicates the bit error ratio in the delivered SDUs. The value is specified as
 * "mEe". As an example a target residual bit error ratio of 5•10-3 would be specified as
 * "5E3" (e.g. AT+CGEQREQ=…,"5E3",…)
 *								"0E0" Subscribed value
 *								"5E2"
 * 								"1E2"
 *								"5E3"
 *								"4E3"
 *								"1E3"
 *								"1E4"
 *								"1E5"
 *								"1E6"
 *								"6E8"
 * @param[in]	u8_delivery_of_erroneous_sdus Integer type, indicates whether SDUs detected
 * as erroneous shall be delivered or not (refer to 3GPP TS 24.008 [8] subclause 10.5.6.5)
 *								0 No
 *								1 Yes
 *								2 No detect
 *								3 Subscribed value
 * @param[in] u16_transfer_delay Integer type, (0,1,2,…) indicates the targeted time 
 * between request to transfer an SDU at one SAP to its delivery at the other SAP, in
 * milliseconds. If the parameter is set to '0' the subscribed value will be requested 
 * (refer to 3GPP TS 24.008 subclause 10.5.6.5)
 *								0 Subscribed value
 *								100~150 (value needs to be divisible by 10 without remainder)
 *								200~950 (value needs to be divisible by 50 without remainder)
 *								1000~4000 (value needs to be divisible by 100 without remainder)
 * @param[in] u8_traffic_handling_priority Integer type, (1,2,3,…) specifies the relative 
 * importance for handling of all SDUs belonging to the UMTS bearer compared to the SDUs 
 * of other bearers. If the parameter is set to '0' the subscribed value will be requested 
 * (refer to 3GPP TS 24.008 [8] subclause 10.5.6.5)
 *								0 Subscribed
 *								1
 *								2
 * 								3
 * @param[in] u8_source_statistics_descriptor Integer type, specifies characteristics of 
 * the source of the submitted SDUs for a PDP context.
 *								0 Characteristics of SDUs is unknown
 *								1 Characteristics of SDUs corresponds to a speech source
 * @param[in] u8_signaling_indication Integer type, indicates signaling content of 
 * submitted SDUs for a PDP context
 *								0 PDP context is not optimized for signaling
 *								1 PDP context is optimized for signaling <PDP_type>
 * @see
 * @return true if configure successfully
 */
bool b_gsm_cmd_cfg_3g_quality_SP_req(uint8_t u8_context_id, uint8_t u8_traffic_class,
			uint16_t u16_max_bit_rate_ul, uint16_t u16_max_bit_rate_dl,
			uint16_t u16_guaranteed_bit_rate_ul, uint16_t u16_guaranteed_bit_rate_dl,
			uint8_t u8_delivery_order, uint16_t u16_max_sdu_size, 
			const char* str_sdu_error_ratio, const char* str_residual_bit_error_ratio,
			uint8_t u8_delivery_of_erroneous_sdus, uint16_t u16_transfer_delay,
			uint8_t u8_traffic_handling_priority, uint8_t u8_source_statistics_descriptor,
			uint8_t u8_signaling_indication)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_3G_QUALITY_SP_REQ;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CGEQREQ=%u,%u,%u,%u,%u,%u,%u,%u,\"%s\",\"%s\",%u,%u,%u,%u,%u",
		u8_context_id, u8_traffic_class, u16_max_bit_rate_ul, u16_max_bit_rate_dl,
		u16_guaranteed_bit_rate_ul, u16_guaranteed_bit_rate_dl, u8_delivery_order,
		u16_max_sdu_size, str_sdu_error_ratio, str_residual_bit_error_ratio,
		u8_delivery_of_erroneous_sdus, u16_transfer_delay, u8_traffic_handling_priority,
		u8_source_statistics_descriptor, u8_signaling_indication);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_CME_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_3g_quality_SP_min(uint8_t u8_context_id, uint8_t u8_traffic_class,
 *			uint16_t u16_max_bit_rate_ul, uint16_t u16_max_bit_rate_dl,
 *			uint16_t u16_guaranteed_bit_rate_ul, uint16_t u16_guaranteed_bit_rate_dl,
 *			uint8_t u8_delivery_order, uint16_t u16_max_sdu_size, 
 *			const char* str_sdu_error_ratio, const char* str_residual_bit_error_ratio,
 *			uint8_t u8_delivery_of_erroneous_sdus, uint16_t u16_transfer_delay,
 *			uint8_t u8_traffic_handling_priority, uint8_t u8_source_statistics_descriptor,
 *			uint8_t u8_signaling_indication)
 * @brief AT+CGEQREQ allows the TE to specify a UMTS Quality of Service Profile that is
 * used when the MT activates a PDP context.
 * AT+CGEQMIN=[<cid>[,<Trafficclass>[,<Maximum bitrate UL>
 *									[,<Maximum bitrate DL>[,<Guaranteed bitrate UL>
 *									[,<Guaranteed bitrate DL>[,<Delivery order>
 *									[,<Maximum SDU size> [,<SDU error ratio>
 *									[,<Residual bit error ratio> [,<Delivery of erroneous SDUs>
 *									[,<Transfer delay> [,<Traffic handling priority>
 *									[,<Source statistics descriptor> 
 *									[,<Signalling indication>]]]]]]]]]]]]]]]
 * @param[in] u8_context_id PDP context identifier, a numeric parameter which specifies a
 * particular PDP context definition. The parameter is local to the TE-MT interface and is
 * used in other PDP context-related commands. The range of permitted values (minimum 
 * value=1) is returned by the test form of the command.
 * @param[in] u8_traffic_class  Integer type, indicates the type of application for which 
 * the UMTS bearer service is optimized (refer 3GPP TS 24.008 subclause 10.5.6.5). If the
 * Traffic class is specified as conversational or streaming, then the Guaranteed and
 * Maximum bitrate parameters should also be provided.
 *								0 Conversational 
 *								1 Streaming
 *								2 Interactive
 *								3 Background
 *								4 Subscribed value
 * @param[in] u16_max_bit_rate_ul Integer type, indicates the maximum number of kbits/s 
 * delivered to UMTS (up-link traffic) at a SAP. As an example a bit rate of 32kbit/s 
 * would be specified as '32' (e.g. AT+CGEQREQ=…,32, …)
 * 								0 Subscribed value
 *								1~5760 
 * @param[in] u16_max_bit_rate_dl Integer type, indicates the maximum number of kbits/s 
 * delivered by UMTS (down-link traffic) at a SAP. As an example a bitrate of 32kbit/s 
 * would be specified as '32' (e.g. AT+CGEQREQ=…,32, …)
 *								0 Subscribed value
 *								1~21600
 * @param[in] u16_guaranteed_bit_rate_ul Integer type, indicates the guaranteed number of
 * kbits/s delivered to UMTS (up-link traffic) at a SAP (provided that there is data to 
 * deliver). As an example a bitrate of 32kbit/s would be specified as '32' 
 * (e.g.T+CGEQREQ=…,32, …)
 *								0 Subscribed value
 *								1~5760
 * @param[in] u16_guaranteed_bit_rate_dl Integer type, indicates the guaranteed number of
 * kbits/s delivered by UMTS (down-link traffic) at a SAP (provided that there is data to 
 * deliver). As an example a bitrate of 32kbit/s would be specified as '32' 
 * (e.g.AT+CGEQREQ=…,32, …)
 *								0 Subscribed value
 *								1~21600
 * @param[in] u8_delivery_order Integer type, indicates whether the UMTS bearer shall 
 * provide in-sequence SDU delivery or not (refer 3GPP TS 24.008 subclause 10.5.6.5)
 *								0 No
 *								1 Yes
 *								2 Subscribed value
 * @param[in] u16_max_sdu_size Integer type, (1,2,3,…) indicates the maximum allowed SDU 
 * size in octets. If the parameter is set to '0' the subscribed value will be requested 
 * (refer 3GPP TS 24.008 subclause 10.5.6.5)
 *								0 Subscribed value
 *								10...1520 (value needs to be divisible by 10 without remainder)
 *								1502
 * @param[in] str_sdu_error_ratio String type, indicates the target value for the fraction 
 * of SDUs lost or detected as erroneous. SDU error ratio is defined only for conforming 
 * traffic. The value is specified as 'mEe'. As an example a target SDU error ratio of
 * 5*10-3 would be specified as "5E3" (e.g.AT+CGEQREQ=…,"5E3",…)
 *								"0E0" Subscribed value
 *								"1E2"
 *								"7E3"
 *								"1E3"
 *								"1E4"
 *								"1E5"
 *								"1E6"
 *								"1E1"
 * @param[in] str_residual_bit_error_ratio String type, indicates the target value for 
 * the undetected bit error ratio in the delivered SDUs. If no error detection is 
 * requested, Residual bit error ratio indicates the bit error ratio in the delivered SDUs. The value is specified as
 * "mEe". As an example a target residual bit error ratio of 5•10-3 would be specified as
 * "5E3" (e.g. AT+CGEQREQ=…,"5E3",…)
 *								"0E0" Subscribed value
 *								"5E2"
 * 								"1E2"
 *								"5E3"
 *								"4E3"
 *								"1E3"
 *								"1E4"
 *								"1E5"
 *								"1E6"
 *								"6E8"
 * @param[in]	u8_delivery_of_erroneous_sdus Integer type, indicates whether SDUs detected
 * as erroneous shall be delivered or not (refer to 3GPP TS 24.008 [8] subclause 10.5.6.5)
 *								0 No
 *								1 Yes
 *								2 No detect
 *								3 Subscribed value
 * @param[in] u16_transfer_delay Integer type, (0,1,2,…) indicates the targeted time 
 * between request to transfer an SDU at one SAP to its delivery at the other SAP, in
 * milliseconds. If the parameter is set to '0' the subscribed value will be requested 
 * (refer to 3GPP TS 24.008 subclause 10.5.6.5)
 *								0 Subscribed value
 *								100~150 (value needs to be divisible by 10 without remainder)
 *								200~950 (value needs to be divisible by 50 without remainder)
 *								1000~4000 (value needs to be divisible by 100 without remainder)
 * @param[in] u8_traffic_handling_priority Integer type, (1,2,3,…) specifies the relative 
 * importance for handling of all SDUs belonging to the UMTS bearer compared to the SDUs 
 * of other bearers. If the parameter is set to '0' the subscribed value will be requested 
 * (refer to 3GPP TS 24.008 [8] subclause 10.5.6.5)
 *								0 Subscribed
 *								1
 *								2
 * 								3
 * @param[in] u8_source_statistics_descriptor Integer type, specifies characteristics of 
 * the source of the submitted SDUs for a PDP context.
 *								0 Characteristics of SDUs is unknown
 *								1 Characteristics of SDUs corresponds to a speech source
 * @param[in] u8_signaling_indication Integer type, indicates signaling content of 
 * submitted SDUs for a PDP context
 *								0 PDP context is not optimized for signaling
 *								1 PDP context is optimized for signaling <PDP_type>
 * @see
 * @return true if configure successfully
 */
bool b_gsm_cmd_cfg_3g_quality_SP_min(uint8_t u8_context_id, uint8_t u8_traffic_class,
			uint16_t u16_max_bit_rate_ul, uint16_t u16_max_bit_rate_dl,
			uint16_t u16_guaranteed_bit_rate_ul, uint16_t u16_guaranteed_bit_rate_dl,
			uint8_t u8_delivery_order, uint16_t u16_max_sdu_size, 
			const char* str_sdu_error_ratio, const char* str_residual_bit_error_ratio,
			uint8_t u8_delivery_of_erroneous_sdus, uint16_t u16_transfer_delay,
			uint8_t u8_traffic_handling_priority, uint8_t u8_source_statistics_descriptor,
			uint8_t u8_signaling_indication)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_3G_QUALITY_SP_REQ;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CGEQMIN=%u,%u,%u,%u,%u,%u,%u,%u,\"%s\",\"%s\",%u,%u,%u,%u,%u",
		u8_context_id, u8_traffic_class, u16_max_bit_rate_ul, u16_max_bit_rate_dl,
		u16_guaranteed_bit_rate_ul, u16_guaranteed_bit_rate_dl, u8_delivery_order,
		u16_max_sdu_size, str_sdu_error_ratio, str_residual_bit_error_ratio,
		u8_delivery_of_erroneous_sdus, u16_transfer_delay, u8_traffic_handling_priority,
		u8_source_statistics_descriptor, u8_signaling_indication);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_CME_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_activate_pdp_context(void)
 * @brief Before activating context by AT+QIACT, host should configure the context by 
 * AT+QICSGP. After activation, the IP address can be queried by AT+QIACT?. 
 * The range of <contextID> is 1-16, but the maximum number of context which can be 
 * activated at the same time is 3. Depending on the Network, it may take at most 150
 * seconds to return OK or ERROR after executing AT+QIACT. Before the response is returned,
 * other AT commands cannot be executed
 * AT+QIACT=<contextID>
 * @param[in] 
 * @see
 * @return true if Activate the context OK
 */
bool b_gsm_cmd_activate_pdp_context(uint8_t u8_context_id)
{	
	e_gsm_active_cmd = GSM_CMD_ACTIVE_PDP_CONTEXT;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+QIACT=%d", u8_context_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						1000,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE,
																						0);	
	if(b_ret)
	{
		while((b_pdp_context_id_add(u8_context_id)) != true){}	//hold the process untill reset
	}
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_check_pdp_context(void)
 * @brief check PDP context is active?
 * AT+QIACT?
 * Response
 * Return the list of the current activated context and its IP address:
 * +QIACT:1,<context_state>,<context_type>[,<IP_address>]
 * [.....
 * +QIACT:16,<context_state>,<context_type>[,<IP_address>]]
 * OK
 * <contextID> Integer type, context ID, range is 1-16
 * <context_state> Integer type, context state
 *									0 Deactivated
 *									1 Activated
 * <contex_type> Integer type, protocol type
 *									1 IPV4
 * <IP_address> The local IP address after context is activated
 * @param[in] 
 * @see
 * @return true if sim registered network
 */
bool b_gsm_cmd_check_pdp_context(void)
{	
	e_gsm_active_cmd = GSM_CMD_CHECK_PDP_CONTEXT;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+QIACT?");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_PDP_ACTIVE,
																						AT_RESP_STATUS_PDP_DEACTIVE,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_deactivate_pdp_context(void)
 * @brief This function is used for checking SIM
 * is registered to PS or not?
 * @param[in] 
 * @see
 * @return true if sim registered network
 */
bool b_gsm_cmd_deactivate_pdp_context(uint8_t u8_context_id)
{	
	e_gsm_active_cmd = GSM_CMD_DEACTIVE_PDP_CONTEXT;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+QIDEACT=%d", u8_context_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP_PDP_DEACT_MAX,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	if(b_ret)
	{
		v_pdp_context_id_remove(u8_context_id);
	}
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_start_socket_service(uint8_t u8_contex_id, uint8_t u8_connect_id,
 *																const char* str_service_type, const char* str_domain_name,
 *																uint16_t u16_remote_port, uint16_t u16_local_port,
 *															socket_data_access_mode_t e_socket_data_access_mode)
 *
 * @brief Start a socket service by AT+QIOPEN
 * @param[in] u8_contex_id Integer type, context ID, range is 1-16
 * @param[in] u8_connect_id Integer type, socket service index, range is 0-11
 * @param[in] str_service_type String type, socket service type
 * 						“TCP” Start a TCP connection as a client
 *						“UDP” Start a UDP connection as a client
 *            “TCP LISTENER” Start a TCP server to listen to TCP connection
 * 						“UDP SERVICE” Start a UDP service
 * @param[in] str_domain_name String type, the domain name address of the remote server
 * @param[in] u16_remote_port The port of the remote server, only valid when 
 *						<service_type> is “TCP” or “UDP”, range is 0-65535
 * @param[in] u16_local_port The local port, range is 0-65535
 *						If <service_type> is “TCP LISTENER” or “UDP SERVICE”, this parameter must be
 *						specified
 *						If <service_type> is “TCP” or “UDP”, if <local_port> is 0, then the local port
 *						will be assigned automatically, else the local port is assigned as specified
 * @param[in] e_socket_data_access_mode Integer type, the data access mode of the socket services
 *						0 Buffer access mode
 *						1 Direct push mode
 *						2 Transparent access mode
 * @see
 * @return true if start socket service successfully
 */
bool b_gsm_cmd_start_socket_service (uint8_t u8_contex_id, uint8_t u8_connect_id,
											const char* str_service_type, const char* str_domain_name,
											uint16_t u16_remote_port, uint16_t u16_local_port,
											socket_data_access_mode_t e_socket_data_access_mode)
{	
	e_gsm_active_cmd = GSM_CMD_START_SOCKET_SERVICE;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_contex_id must be 1 - 16
	if ((u8_contex_id < 1) || (u8_contex_id > 16))
	{
		return false;
	}
	// u8_connect_id must be 0 - 11
	if (u8_connect_id > 11)
	{
		return false;
	}
	UARTprintf("AT+QIDEACT=%u,%u,\"%s\",\"%s\",%u,%u,%u",u8_contex_id,u8_connect_id,
							str_service_type, str_domain_name, u16_remote_port, u16_local_port,
							e_socket_data_access_mode);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	/*
	If the <e_socket_data_access_mode> is transparent access mode and it is
	successful to start the service, response:
		CONNECT
		Else, response:
		ERROR
		Error description can be got via AT+QIGETERROR
	Maximum Response Time is 150 seconds, determined by network
	*/
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP_QIOPEN,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_close_socket_service (uint8_t u8_connect_id, uint16_t u16_timeout_s)
 *
 * @brief Close the specified socket service by AT+QICLOSE
 * Depending on the Network, it will take at most 10
 * seconds (default value, host could modify the time with <timeout>) to return OK or ERROR after
 * executing AT+QICLOSE. Before the response is returned, other AT commands cannot be executed.
 * @param[in] u8_connect_id Integer type, socket service index, range is 0-11
 * @param[in] u16_timeout_s Integer type. If the FIN ACK of the other peers is still not received
 * until <timeout> expires, the module will force to close the socket. Range is 0-65535. 
 * Default: 10. Unit: second
 * @see
 * @return true if close socket service successfully
 */
bool b_gsm_cmd_close_socket_service (uint8_t u8_connect_id, uint16_t u16_timeout_s)
{	
	e_gsm_active_cmd = GSM_CMD_CLOSE_SOCKET_SERVICE;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_connect_id must be 0 - 11
	if (u8_connect_id > 11)
	{
		return false;
	}
	UARTprintf("AT+QICLOSE=%u,%u",u8_connect_id,u16_timeout_s);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP_QIOPEN,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_query_socket_service_status (uint8_t u8_query_type, uint8_t u8_connect_id)
 *
 * @brief AT+QISTATE can be used to query the socket service status. If the <u8_query_type> is 0, 
 * it will return the status of all existing socket services in the context of specified <contextID>.
 * If the <u8_query_type> is 1, it will return the status of specified <u8_connect_id> socket service
 * @param[in] u8_query_type Integer type, the query type
 *						0 Query connection status by <contextID>
 *						1 Query connection status by <connectID>
 * @param[in] u8_connect_id Integer type, context ID, range is 1-16
 * @see
 * @return true if socket state is "Connected"
 */
bool b_gsm_cmd_query_socket_service_status (uint8_t u8_query_type, uint8_t u8_connect_id)
{	
	e_gsm_active_cmd = GSM_CMD_QUERY_SOCKET_SERVICE_STATUS;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_query_type must be 0 or 1
	if (u8_query_type > 1)
	{
		return false;
	}
	// u8_connect_id must be 0 - 11
	if (u8_connect_id > 11)
	{
		return false;
	}
	UARTprintf("AT+QISTATE=%u,%u", u8_query_type, u8_connect_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_CONNECT_OK,
																						AT_RESP_STATUS_CONNECT_FAIL,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_send_socket_data (uint8_t u8_connect_id, uint8_t* pu8_data, uint16_t u16_len)
 * @brief If <access_mode> of the specified socket service is buffer access mode or direct push mode, send data
 * by AT+QISEND. If data is sent to the module successfully, return “SEND OK”, else return “SEND FAIL” or
 * “ERROR”. “SEND FAIL” indicates the send buffer is full and host can try to resend the data. “ERROR”
 * indicates it encounters error in the process of sending data. The host should delay some time for sending
 * data. The maximum length of sending data is 1460. “SEND OK” does not mean the data has been sent to
 * the server successfully. Through the “AT+QISEND=<connectID>, 0” command, host can query whether
 * the data has reached the server.
 * @param[in] u8_connect_id Integer type, socket service index, range is 0-11
 * @param[in] pu8_data pointer type, data pointer
 * @param[in] u16_len Integer type. The length of data to be sent, which cannot exceed 1460
 * @see
 * @return true if the data has been sent to the server successfully.
 */
bool b_gsm_cmd_send_socket_data (uint8_t u8_connect_id, uint8_t* pu8_data, uint16_t u16_len)
{	
	e_gsm_active_cmd = GSM_CMD_SEND_SOCKET_DATA;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_connect_id must be 0 - 11
	if (u8_connect_id > 11)
	{
		return false;
	}
	// pu8_data must be valid
	if (NULL == pu8_data)
	{
		return false;
	}
	// u16_len must be 0 - 1460
	if (u16_len > 1460)
	{
		return false;
	}
	//Request to send u16_len data byte
	UARTprintf("AT+QISEND=%u,%u", u8_connect_id, u16_len);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_PROMOTING_MARK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	//write data
	if (b_ret == true)
	{
		UARTwriteData(pu8_data, u16_len);
	}
	//check respone
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_SEND_OK,
																						AT_RESP_STATUS_SEND_FAIL,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	//check ACK
	/*
		Through the “AT+QISEND=<connectID>, 0” command, host can query whether
		the data has reached the server
	*/
	if (b_ret == true)
	{	
		//check respone
		uint8_t u8_cnt_tmp = 24;
		while (u8_cnt_tmp > 0)
		{
			UARTprintf("AT+QISEND=%u,0", u8_connect_id);				
			au8_at_cmd_buf[0] = CR;
			au8_at_cmd_buf[1] = LF;
			UARTwriteData(au8_at_cmd_buf, 2);
			b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_SEND_ACK,
																								AT_RESP_STATUS_NONE,
																								TIME_TO_WAIT_AT_RESP_QISEND_ACK,
																								AT_RESP_STATUS_OK,
																								AT_RESP_STATUS_NONE,
																								AT_RESP_STATUS_ERROR);		
			if (b_ret == true)
			{
				break;
			}
		}
	}
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_ping_server (uint8_t u8_context_id, const char* str_domain_name,
 *														uint8_t	u8_timeout, uint8_t u8_pingnum)
 *
 * @brief AT+QPING is used to test the Internet Protocol reachability of a host. 
 * Before using ping tools, host should activate the context corresponding to
 * <contextID> via AT+QIACT first. It will return the result during <timeout> and 
 * the default value of <timeout> is 1 second
 * @param[in] u8_context_id Integer type, the context ID, the range is 1-16
 * @param[in] str_domain_name The host address in string type, format is a domain name or
 * a dotted decimal IP address
 * @param[in] u8_timeout Integer type, set the maximum time to wait for the response
 * of each ping request Unit: second, range: 1-255, default: 1
 * @param[in] u8_pingnum Integer type, set the maximum time of ping request. Range: 1-10. Default: 4
 * @see
 * @return true if ping is finished normally
 */
bool b_gsm_cmd_ping_server (uint8_t u8_context_id, const char* str_domain_name,
														uint8_t	u8_timeout, uint8_t u8_pingnum)
{	
	e_gsm_active_cmd = GSM_CMD_PING_SERVER;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_context_id must be 1 - 16
	if ((u8_context_id < 1) || (u8_context_id > 16))
	{
		return false;
	}
	// u8_timeout must be 1 - 255
	if (u8_timeout == 0)
	{
		return false;
	}
	// u8_pingnum must be 1 - 10
	if (u8_pingnum == 0)
	{
		return false;
	}	
	UARTprintf("AT+QPING=%u,\"%u\",%u,%u", u8_context_id, str_domain_name,
							u8_timeout, u8_pingnum);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						u8_timeout * 1000/INTERVAL_TIME_TO_CHECK_AT_RESP,
																						AT_RESP_STATUS_PING_OK,
																						AT_RESP_STATUS_PING_FAIL,
																						u8_timeout * (u8_pingnum + 1) * 1000/
																						INTERVAL_TIME_TO_CHECK_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_protocol_ver (uint8_t u8_client_id, uint8_t u8_version)
 *
 * @brief Configure the MQTT protocol version
 * AT+QMTCFG=“version”,<client_idx> [,<vsn>]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_version Integer type. MQTT protocol version
 *											3 MQTT protocol v3.1
 *											4 MQTT protocol v3.1.1
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_protocol_ver (uint8_t u8_client_id, uint8_t u8_version)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_PROTOCOL_VER;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_version must be 3 or 4
	if ((u8_version != 3) || (u8_version != 4))
	{
		return false;
	}
	UARTprintf("AT+QMTCFG=\"version\",%u,%u", u8_client_id, u8_version);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_pdp (uint8_t u8_client_id, uint8_t u8_cid)
 *
 * @brief Configure the PDP to be used by the MQTT client
 *				AT+QMTCFG=“pdpcid”,<client_idx>[,<cid>]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_cid Integer type. The PDP to be used by the MQTT client. 
 * 						The range is 1-16. The default value is 1
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_pdp (uint8_t u8_client_id, uint8_t u8_cid)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_PDP;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_cid must be 1 - 16
	if ((u8_cid < 1) || (u8_cid > 16))
	{
		return false;
	}
	UARTprintf("AT+QMTCFG=\"pdpcid\",%u,%u", u8_client_id, u8_cid);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool bool b_gsm_cmd_cfg_mqtt_will (uint8_t u8_client_id, uint8_t u8_will_flag, uint8_t u8_will_qos,
 *															uint8_t u8_will_retain, const char* str_will_topic,
 *															const char* str_will_msg)
 *
 * @brief Configure Will information
 * AT+QMTCFG=“will”,<client_idx>[,<will_fg>[,<will_qos>,<will_retain>,
 * 						“<will_topic>”,“<will_msg>”]]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_will_flag Integer type. Configure the Will flag
 *												 0 Ignore the Will flag configuration
 *												 1 Require the Will flag configuration
 * @param[in] u8_will_qos Integer type. Quality of service for message delivery
 *												0 At most once
 *												1 At least once
 *												2 Exactly once
 * @param[in] u8_will_retain Integer type. The Will retain flag is only used on PUBLISH messages.
 *						0 When a client sends a PUBLISH message to a server, the server will not hold
 *							on to the message after it has been delivered to the current subscribers
 *						1 When a client sends a PUBLISH message to a server, the server should hold
 *							on to the message after it has been delivered to the current subscribers
 * @param[in] str_will_topic String type. Will topic string
 * @param[in] str_will_msg String type. The Will message defines the content of the message that is
 *						published to the will topic if the client is unexpectedly disconnected. It can be a
 *						zero-length message.
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_will (uint8_t u8_client_id, uint8_t u8_will_flag, uint8_t u8_will_qos,
															uint8_t u8_will_retain, const char* str_will_topic,
															const char* str_will_msg)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_WILL;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_will_flag must be 0 or 1
	if (u8_will_flag > 1)
	{
		return false;
	}
	// u8_will_qos must be 0 - 2
	if (u8_will_qos > 2)
	{
		return false;
	}
	// u8_will_retain must be 0 or 1
	if (u8_will_retain > 1)
	{
		return false;
	}	
	UARTprintf("AT+QMTCFG=\"will\",%u,%u,%u,%u,\"%s\",\"%s\"", u8_client_id, u8_will_flag,
												u8_will_qos, u8_will_retain, str_will_topic, str_will_msg);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_msg_delivery_timeout (uint8_t u8_client_id, uint8_t u8_pkt_timeout,
 *																							uint8_t u8_retry_times, uint8_t u8_timeout_notice)
 *
 * @brief Configure timeout of message delivery
 * AT+QMTCFG=“timeout”,<client_idx>[,<pkt_timeout>[,<retry_times>][,<timeout_notice>]]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_pkt_timeout Integer type. Timeout of the packet delivery. The range is 1-60.
 *						The default value is 5. Unit: second.
 * @param[in] u8_retry_times Integer type. Retry times when packet delivery times out. 
 *						The range is 1-10. The default value is 3.
 * @param[in] u8_timeout_notice Integer type.
 *						0 Not report timeout message when transmitting packet
 *						1 Report timeout message when transmitting packet
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_msg_delivery_timeout (uint8_t u8_client_id, uint8_t u8_pkt_timeout,
																							uint8_t u8_retry_times, uint8_t u8_timeout_notice)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_MSG_DELIVERY_TIMEOUT;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_pkt_timeout must be 1 - 60
	if ((u8_pkt_timeout < 1) || (u8_pkt_timeout > 60))
	{
		return false;
	}
	// u8_retry_times must be 1 - 10
	if ((u8_retry_times < 1) || (u8_retry_times > 10))
	{
		return false;
	}
	// u8_timeout_notice must be 0 or 1
	if (u8_timeout_notice > 1)
	{
		return false;
	}	
	UARTprintf("AT+QMTCFG=\"timeout\",%u,%u,%u,%u", u8_client_id, u8_pkt_timeout,
												u8_retry_times, u8_timeout_notice);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}


/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_session_type (uint8_t u8_client_id, uint8_t u8_clean_session)
 *
 * @brief Configure the session type
 * AT+QMTCFG=“session”,<client_idx>[,<clean_session>]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_clean_session Integer type. Configure the session type
 *						0 The server must store the subscriptions of the client after it disconnects.
 *						1 The server must discard any previously maintained information about the
 *							client and treat the connection as “clean”.
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_session_type (uint8_t u8_client_id, uint8_t u8_clean_session)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_SESSION_TYPE;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_clean_session must be 0 or 1
	if (u8_clean_session > 1)
	{
		return false;
	}	
	UARTprintf("AT+QMTCFG=\"session\",%u,%u", u8_client_id, u8_clean_session);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_keep_alive_time (uint8_t u8_client_id, uint16_t u16_keep_alive_time_s)
 *
 * @brief Configure the keep-alive time
 * AT+QMTCFG=“keepalive”,<client_idx>[,<keep-alive time>]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u16_keep_alive_time_s nteger type. Keep-alive time. The range is 0-3600. 
 *						The default value is 120. Unit: second. It defines the maximum time interval
 *						between messages received from a client. If the server does not receive a message
 *						from the client within 1.5 times of the keep alive time period, it disconnects
 *						the client as if the client has sent a DISCONNECT message.
 *						0 The client is not disconnected
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_keep_alive_time (uint8_t u8_client_id, uint16_t u16_keep_alive_time_s)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_KEEP_ALIVE_TIME;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u16_keep_alive_time_s must be 0 - 3600
	if (u16_keep_alive_time_s > 3600)
	{
		return false;
	}	
	UARTprintf("AT+QMTCFG=\"keepalive\",%u,%u", u8_client_id, u16_keep_alive_time_s);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_ssl (uint8_t u8_client_id, uint8_t u8_ssl_enable, uint8_t u8_ssl_ctx_id)
 *
 * @brief Configure the MQTT SSL mode and SSL context index
 * AT+QMTCFG=“ssl”,<client_idx>[,<sslenable>[,<sslctx_idx>]]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_ssl_enable Integer type. Configure the MQTT SSL mode
 *						0 Use normal TCP connection for MQTT
 *						1 Use SSL TCP secure connection for MQTT
 * @param[in] u8_ssl_ctx_id Integer type. SSL context index. The range is 0-5
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_ssl (uint8_t u8_client_id, uint8_t u8_ssl_enable, uint8_t u8_ssl_ctx_id)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_SSL;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_ssl_enable must be 0 or 1
	if (u8_ssl_enable > 2)
	{
		return false;
	}	
	// u8_ssl_ctx_id must be 0 - 5
	if (u8_ssl_ctx_id > 5)
	{
		return false;
	}	
	UARTprintf("AT+QMTCFG=\"ssl\",%u,%u,%u", u8_client_id, u8_ssl_enable, u8_ssl_ctx_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_recv_mode (uint8_t u8_client_id, uint8_t u8_recv_mode)
 *
 * @brief Configure receiving mode when data is received from server
 * AT+QMTCFG=“recv/mode”,<client_idx>[,<msg_recv_mode>]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u8_recv_mode Integer type. Configure the MQTT message receiving mode.
 *						0 MQTT message received from server will be contained in URC.
 *						1 MQTT message received from server will not be contained in URC.
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_recv_mode (uint8_t u8_client_id, uint8_t u8_recv_mode)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_RECV_MODE;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_recv_mode must be 0 or 1
	if (u8_recv_mode > 2)
	{
		return false;
	}	
	UARTprintf("AT+QMTCFG=\"recv/mode\",%u,%u", u8_client_id, u8_recv_mode);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_mqtt_ping_interval (uint8_t u8_client_id, uint16_t u16_interval_s)
 *
 * @brief Configure the <qmtping_interval>
 * AT+QMTCFG="qmtping",<client_idx>[,<qmtping_interval>]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u16_interval_s Integer type, the longest time for +QMTSTAT report after +QMTPING.
 * 						The default value is 5. Unit: second.
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_mqtt_ping_interval (uint8_t u8_client_id, uint16_t u16_interval_s)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_PING_INTERVAL;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	UARTprintf("AT+QMTCFG=\"qmtping\",%u,%u", u8_client_id, u16_interval_s);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_open_network_mqtt (uint8_t u8_client_id, const char* str_host_name,
 *																	uint16_t u16_port, uint16_t u16_timewait)
 *
 * @brief The command is used to open a network for MQTT client.
 * AT+QMTOPEN=<client_idx>,“<host_name>”,<port>
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] str_host_name String type. The address of the server. It could be an IP 
 *						address or a domain name. The maximum size is 100 bytes
 * @param[in] u16_port Integer type. The port of the server. The range is 1-65535.
 * @see
 * @return Return true if network opened successfully
 */
bool b_gsm_cmd_open_network_mqtt (uint8_t u8_client_id, const char* str_host_name,
																	uint16_t u16_port, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_OPEN_NETWORK_MQTT;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u16_port must be 1-65535
	if (u16_port == 0)
	{
		return false;
	}
	UARTprintf("AT+QMTOPEN=%u,\"%s\",%u", u8_client_id, str_host_name, u16_port);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_OPEN_NETWORK_MQTT_OK,
																						AT_RESP_STATUS_OPEN_NETWORK_MQTT_FAIL,
																						u16_timewait * 100);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_close_network_mqtt (uint8_t u8_client_id, uint16_t u16_timewait)
 *
 * @brief The command is used to close a network for MQTT client.
 * AT+QMTCLOSE=<client_idx>
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @see
 * @return Return true if network closed successfully
 */
bool b_gsm_cmd_close_network_mqtt (uint8_t u8_client_id, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_CLOSE_NETWORK_MQTT;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	UARTprintf("AT+QMTCLOSE=%u", u8_client_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_CLOSE_NETWORK_MQTT_OK,
																						AT_RESP_STATUS_CLOSE_NETWORK_MQTT_FAIL,
																						u16_timewait * 100);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_mqtt_connect (uint8_t u8_client_id, const char* str_client_id,
 *														const char* str_username, const char* str_password,
 *														uint16_t u16_timewait)
 *
 * @brief The command is used when a client requests a connection to MQTT server. 
 * When a TCP/IP socket connection is established from a client to a server, 
 * a protocol level session must be created using a CONNECT flow
 * AT+QMTCONN=<client_idx>,“<clientID>”[,“<username>”[,“<password>”]]
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] str_client_id String type. The client identifier string
 * @param[in] str_username String type. User name of the client. It can be used
 * for authentication.
 * @param[in] str_password String type. Password corresponding to the user name
 * of the client. It can be used for authentication.
 * @see
 * @return Return true if MQTT is connected
 */
bool b_gsm_cmd_mqtt_connect (uint8_t u8_client_id, const char* str_client_id,
														const char* str_username, const char* str_password,
														uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_CONNECT;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	UARTprintf("AT+QMTCONN=%u,\"%s/%u/%u\",\"%s\",\"%s\"", u8_client_id, 
												str_client_id, TOPIC_ID, DEVICE_SUB_ID,
												str_username, str_password);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_MQTT_CONNECT_OK,
																						AT_RESP_STATUS_MQTT_CONNECT_FAIL,
																						u16_timewait * 100);
	return b_ret;
} 	

/*!
 * @fn bool b_gsm_cmd_mqtt_query_connection (void)
 *
 * @brief Query MQTT connection status
 * @param[in] none
 * @see
 * @return Return true if MQTT is connected
 */
bool b_gsm_cmd_mqtt_query_connection (void)
{	
	e_gsm_active_cmd = GSM_CMD_QUERY_MQTT_CONNECTION;
	v_delete_all_resp_status_on_table();

	UARTprintf("AT+QMTCONN?");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_MQTT_CONNECT_OK,
																						AT_RESP_STATUS_MQTT_CONNECT_FAIL,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
} 	

/*!
 * @fn bool b_gsm_cmd_mqtt_disconnect (uint8_t u8_client_id, uint16_t u16_timewait)
 *
 * @brief The command is used when a client requests a disconnection 
 * from MQTT server. A DISCONNECT message is sent from the client to 
 * the server to indicate that it is about to close its TCP/IP connection.
 * AT+QMTDISC=<client_idx>
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @see
 * @return Return true if connection closed successfully
 */
bool b_gsm_cmd_mqtt_disconnect (uint8_t u8_client_id, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_DISCONNECT;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	UARTprintf("AT+QMTDISC=%u", u8_client_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						u16_timewait * 100,
																						AT_RESP_STATUS_MQTT_DISCONNECT_OK,
																						AT_RESP_STATUS_MQTT_DISCONNECT_FAIL,
																						u16_timewait * 100);
	return b_ret;
} 

/*!
 * @fn bool b_gsm_cmd_mqtt_subscibe (uint8_t u8_client_id, uint16_t u16_msg_id,
 *															const char* str_topic, uint8_t u8_qos,
 *															uint16_t u16_pkt_timeout, uint8_t u8_retry_time)
 *
 * @brief The command is used to subscribe to one topic. A SUBSCRIBE message is sent by
 * a client to register an interest in one topic names with the server. 
 * Messages published to topic is delivered from the server to the client as PUBLISH messages.
 * AT+QMTSUB=<client_idx>,<msgID>,“<topic>”,<qos>
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u16_msg_id Integer type. Message identifier of packet. The range is 1-65535.
 * @param[in] str_topic String type. Topic that the client wants to subscribe to or unsubscribe from.
 * @param[in] u8_qos Integer type. The QoS level at which the client wants to publish the messages.
 *						0 At most once
 *						1 At least once
 *						2 Exactly once
 * @see
 * @return Return true if sent packet successfully and received ACK from server
 */
bool b_gsm_cmd_mqtt_subscibe (uint8_t u8_client_id, uint16_t u16_msg_id,
															const char* str_topic, uint8_t u8_qos,
															uint16_t u16_pkt_timeout, uint8_t u8_retry_time)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_SUBSCRIBE;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	if (u8_qos == 0)
	{
		u16_msg_id = 0;
	}
	if(ustrcmp((const char*)(str_topic), (const char*)TOPIC_PING) == 0)
	{
		UARTprintf("AT+QMTSUB=%u,%u,\"%s/%u/%u\",%u", u8_client_id, u16_msg_id, 
								str_topic, 1, 1, u8_qos);				
	}
	else
	{
		UARTprintf("AT+QMTSUB=%u,%u,\"%s/%u/%u\",%u", u8_client_id, u16_msg_id, 
								str_topic, TOPIC_ID, DEVICE_SUB_ID, u8_qos);				
	}
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
				AT_RESP_STATUS_ERROR,
				u16_pkt_timeout * u8_retry_time * 100,
				AT_RESP_STATUS_MQTT_SUBSCRIBE_OK,
				AT_RESP_STATUS_MQTT_SUBSCRIBE_FAIL,
				u16_pkt_timeout * u8_retry_time * 100);
	return b_ret;
} 

/*!
 * @fn bool b_gsm_cmd_mqtt_unsubscibe (uint8_t u8_client_id, uint16_t u16_msg_id,
 *																const char* str_topic, uint16_t u16_pkt_timeout,
 *																uint8_t u8_retry_time)
 *
 * @brief The command is used to unsubscribe from a topic. An UNSUBSCRIBE message 
 * is sent by the client to the server to unsubscribe from named topics.
 * AT+QMTUNS=<client_idx>,<msgID>,“<topic>”
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u16_msg_id Integer type. Message identifier of packet. The range is 1-65535.
 * @param[in] str_topic String type. Topic that the client wants to subscribe to or unsubscribe from.
 * @see
 * @return Return true if sent packet successfully and received ACK from server
 */
bool b_gsm_cmd_mqtt_unsubscibe (uint8_t u8_client_id, uint16_t u16_msg_id,
																const char* str_topic, uint16_t u16_pkt_timeout,
																uint8_t u8_retry_time)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_UNSUBSCRIBE;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	UARTprintf("AT+QMTUNS=%u,%u,\"%s\"", u8_client_id, u16_msg_id, str_topic);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
				AT_RESP_STATUS_ERROR,
				u16_pkt_timeout * u8_retry_time * 100,
				AT_RESP_STATUS_MQTT_UNSUBSCRIBE_OK,
				AT_RESP_STATUS_MQTT_UNSUBSCRIBE_FAIL,
				u16_pkt_timeout * u8_retry_time * 100);
	return b_ret;
} 

/*!
 * @fn bool b_gsm_cmd_mqtt_publish_msg (uint8_t u8_client_id, uint16_t u16_msg_id, uint8_t u8_qos, 
 *						uint8_t u8_retain, const char* str_topic, uint16_t u16_len, uint8_t* pu8_data,
 *						uint16_t u16_pkt_timeout, uint8_t u8_retry_time)
 * @brief The command is used to publish messages with fixed length by a client to a server for
 * distribution to interested subscribers. Each PUBLISH message is associated with a topic name. 
 * If a client subscribes to one or more topics, any message published to those topics are sent 
 * by the server to the client as a PUBLISH message.
 * AT+QMTPUB=<client_idx>,<msgID>,<qos>,<retain>,“<topic>”,<msg_length>
 * After “>” is responded, input the data to be sent. When the actual size of data is
 * larger than <msg_length>, only the first part of data within <msg_length> byte(s)
 * will be sent out.
 * @param[in] u8_client_id Integer type. MQTT client identifier. The range is 0-5.
 * @param[in] u16_msg_id Integer type. Message identifier of packet. The range is 0-65535. 
 * It will be 0 only when <qos>=0.
 * @param[in] u8_qos Integer type. The QoS level at which the client wants to publish the messages.
 *						0 At most once
 *						1 At least once
 *						2 Exactly once
 * @param[in] u8_retain Integer type. Whether to enable the server to retain the message after 
 * it has been delivered to the current subscribers.
 *						0 Disable the server to retain the message after it has been delivered to the
 *							current subscribers
 *						1 Enable the server to retain the message after it has been delivered to the
 *							current subscribers
 * @param[in] str_topic String type. Topic that needs to be published
 * @param[in] u16_len Integer type. Length of message to be published: 1 - 1500  bytes
 * @param[in] pu8_data Pointer type. Payload to be published.
 * @see
 * @return true if the data has been sent to the server successfully.
 */
bool b_gsm_cmd_mqtt_publish_msg (uint8_t u8_client_id, uint16_t u16_msg_id, uint8_t u8_qos, 
						uint8_t u8_retain, const char* str_topic, uint16_t u16_len, uint8_t* pu8_data,
						uint16_t u16_pkt_timeout, uint8_t u8_retry_time)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_PUBLISH;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_connect_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	// u8_qos must be 0 - 2
	if (u8_qos > 2)
	{
		return false;
	}	
	// u8_retain must be 0 - 1
	if (u8_retain > 1)
	{
		return false;
	}		
	// u16_len must be 1 - 1500
	if ((u16_len < 1) || (u16_len > 1500))
	{
		return false;
	}	
	// pu8_data must be valid
	if (NULL == pu8_data)
	{
		return false;
	}
	//Request to send u16_len data byte
	if (u8_qos == 0)
	{
		u16_msg_id = 0;			
	}
	UARTprintf("AT+QMTPUB=%u,%u,%u,%u,\"%s/%u/%u\",%u", u8_client_id, u16_msg_id, 
									u8_qos,u8_retain, str_topic, TOPIC_ID, DEVICE_SUB_ID, u16_len);		
	
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_PROMOTING_MARK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	//write data
	if (b_ret == true)
	{
		UARTwriteData(pu8_data, u16_len);
	}
	//check respone
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
												AT_RESP_STATUS_SEND_FAIL,
												AT_RESP_STATUS_ERROR,
												AT_RESP_STATUS_MQTT_PUBLISH_OK,
												AT_RESP_STATUS_MQTT_PUBLISH_FAIL,
												u16_pkt_timeout * u8_retry_time * 100);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_account (const char* str_user_name, const char* str_password)
 * @brief Configure FTP account: user name and password
 * AT+QFTPCFG=“account”[,<username>,<password>]
 * @param[in] str_user_name String type, the user name for the authentication. The maximum 
 *						size of the parameter is 50 bytes.
 * @param[in] str_password String type, the password for the authentication. The maximum 
 *						size of the parameter is 50 bytes.
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_account (const char* str_user_name, const char* str_password)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_FTP_ACCOUNT;
	v_delete_all_resp_status_on_table();
	
	UARTprintf("AT+QFTPCFG=\"account\",\"%s\",\"%s\"", str_user_name, str_password);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_file_type (uint8_t u8_file_type)
 * @brief Configure FTP file type.
 * AT+QFTPCFG=“filetype”[,<file_type>]
 * @param[in] u8_file_type Integer type, indicates the type of transferred data is ASCII
 *or binary data.
 *						0 Binary
 *						1 ASCII
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_file_type (uint8_t u8_file_type)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_FTP_FILE_TYPE;
	v_delete_all_resp_status_on_table();
	
	UARTprintf("AT+QFTPCFG=\"filetype\",%u",u8_file_type);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_transmode (uint8_t u8_transmode)
 * @brief Configure FTP transmode.
 * AT+QFTPCFG=“transmode”[,<transmode>]
 * @param[in] u8_transmode Integer type, indicates whether the FTP server or client listens 
 * to data connection
 *						0 Active mode, module will listen to data connection
 *						1 Passive mode, FTP server will listen to data connection
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_transmode (uint8_t u8_transmode)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_FTP_TRANSMODE;
	v_delete_all_resp_status_on_table();
	
	UARTprintf("AT+QFTPCFG=\"transmode\",%u",u8_transmode);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_context_id (uint8_t u8_cid)
 * @brief Configure the FTP context ID
 *				AT+QFTPCFG=“contextid”[,<contextID>]
 * @param[in] u8_cid Integer type, the PDP context ID, the range is 1-16. It should be
 * activated by AT+QIACT before QFTPOPEN. For details, please refer to 
 * Quectel_UC15_TCPIP_AT_Commands_Manual
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_context_id (uint8_t u8_cid)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_MQTT_PDP;
	v_delete_all_resp_status_on_table();
	// u8_cid must be 1 - 16
	if ((u8_cid < 1) || (u8_cid > 16))
	{
		return false;
	}
	UARTprintf("AT+QFTPCFG=\"contextid\",%u", u8_cid);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_rsp_timeout (uint8_t u8_rsp_timeout)
 * @brief Configure FTP respone time out.
 * AT+QFTPCFG=“rsptimeout”[,<timeout>]
 * @param[in] u8_rsp_timeout Integer type, the range is 20-180, the unit is second, 
 * and the default value is 90s. Generally, it is the timeout value for most 
 * +QFTPXXX: xx,xx after the result code OK is returned, except the commands 
 * QFTPPUT/QFTPGET/QFTPLST/QFTPNLST. The rules for these four commands are shown as below:
 *				a) When the command has been sent, but “CONNECT” has not been output yet,
 *				this parameter indicates the maximum interval time for “CONNECT” outputting
 *				after the command has been sent.
 *				b) When the module has entered into data mode, this parameter indicates the
 *				maximum interval time between two packets of received/transmitted data
 *				c) When the <local_name> is not “COM:”, it indicates the maximum interval time
 *				between two packets of received/transmitted data
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_rsp_timeout (uint8_t u8_rsp_timeout)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_FTP_RSP_TIMEOUT;
	v_delete_all_resp_status_on_table();
	// u8_rsp_timeout must be 20 - 180
	if ((u8_rsp_timeout < 20) || (u8_rsp_timeout > 180))
	{
		return false;
	}	
	UARTprintf("AT+QFTPCFG=\"rsptimeout\",%u",u8_rsp_timeout);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_ssl_type (uint8_t u8_rsp_ssl_type)
 * @brief Configure FTP SSL type.
 * AT+QFTPCFG=“ssltype”[,<ssltype>]
 * @param[in] u8_rsp_ssl_type Integer type, indicates the module plays as FTP client or FTPS client
 *						0 FTP client
 *						1 FTPS client
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_ssl_type (uint8_t u8_rsp_ssl_type)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_FTP_SSL_TYPE;
	v_delete_all_resp_status_on_table();
	// u8_rsp_ssl_type must be 0 or 1
	if (u8_rsp_ssl_type > 1)
	{
		return false;
	}	
	UARTprintf("AT+QFTPCFG=\"ssltype\",%u",u8_rsp_ssl_type);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_cfg_ftp_ssl_context_id (uint8_t u8_rsp_ssl_context_id)
 * @brief Configure FTP SSL context id.
 * AT+QFTPCFG=“sslctxid”[,<sslctxid>]
 * @param[in] u8_rsp_ssl_context_id Integer type, indicates the SSL context ID. 
 * The range is 0-5. It should be configured by AT+QSSLCFG, please refer to 
 * Quectel_UC15_SSL_AT_Commands_Manual.
 * @see
 * @return true if configure sucessfully
 */
bool b_gsm_cmd_cfg_ftp_ssl_context_id (uint8_t u8_rsp_ssl_context_id)
{	
	e_gsm_active_cmd = GSM_CMD_CFG_FTP_SSL_CONTEXT_ID;
	v_delete_all_resp_status_on_table();
	// u8_rsp_ssl_context_id must be 0 - 5
	if (u8_rsp_ssl_context_id > 5)
	{
		return false;
	}	
	UARTprintf("AT+QFTPCFG=\"sslctxid\",%u",u8_rsp_ssl_context_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_ftp_open (const char* str_host_name, uint16_t u16_port, uint16_t u16_timewait)
 * @brief This command is used to login to FTP server. You should activate the PDP context by 
 * AT+QIACT first. “+QFTPOPEN: <err>,<protocol_error>” indicates the result of QFTPOPEN and it 
 * should be output in <timeout> set by AT+QFTPCFG.
 * AT+QFTPOPEN=<hostname>[,<port>]
 * @param[in] str_host_name String type, the IP address or domain name of the FTP server. 
 * The maximum size of the parameter is 50 bytes.
 * @param[in] u16_port Integer type, the port of the FTP server. The default value is 21.
 * @param[in] u16_timewait Integer type, the range is 20-180, the unit is second, 
 * and the default value is 90s. Generally, it is the timeout value for most 
 * +QFTPXXX: xx,xx after the result code OK is returned, except the commands 
 * QFTPPUT/QFTPGET/QFTPLST/QFTPNLST. The rules for these four commands are shown as below:
 *				a) When the command has been sent, but “CONNECT” has not been output yet,
 *				this parameter indicates the maximum interval time for “CONNECT” outputting
 *				after the command has been sent.
 *				b) When the module has entered into data mode, this parameter indicates the
 *				maximum interval time between two packets of received/transmitted data
 *				c) When the <local_name> is not “COM:”, it indicates the maximum interval time
 *				between two packets of received/transmitted data.
 * @see
 * @return Return true if login to FTP server sucessfully
 */
bool b_gsm_cmd_ftp_open (const char* str_host_name, uint16_t u16_port, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_FTP_OPEN;
	v_delete_all_resp_status_on_table();
	// u16_timewait must be 20 - 180
	if ((u16_timewait < 20) || (u16_timewait > 180))
	{
		return false;
	}
	UARTprintf("AT+QFTPOPEN=\"%s\",%u", str_host_name, u16_port);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_FTP_OPEN_OK,
																						AT_RESP_STATUS_FTP_OPEN_FAIL,
																						u16_timewait * 100);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_ftp_set_curr_dir (const char* str_path_name, uint16_t u16_timewait)
 * @brief Set the current directory on FTP server. If OK is returned, 
 * “+QFTPCWD: <err>,<protocol_error>” should be output in <timeout> set by AT+QFTPCFG. All 
 * the files and directory operation will be set in the current directory.
 * AT+QFTPCWD=<path_name>
 * @param[in] str_path_name String type, a directory path on FTP server. The maximum size 
 * of the parameter is 100 bytes. The root path of FTP server is “/”
 * @param[in] u16_timewait Integer type, the range is 20-180, the unit is second, 
 * and the default value is 90s. Generally, it is the timeout value for most 
 * +QFTPXXX: xx,xx after the result code OK is returned, except the commands 
 * QFTPPUT/QFTPGET/QFTPLST/QFTPNLST. The rules for these four commands are shown as below:
 *				a) When the command has been sent, but “CONNECT” has not been output yet,
 *				this parameter indicates the maximum interval time for “CONNECT” outputting
 *				after the command has been sent.
 *				b) When the module has entered into data mode, this parameter indicates the
 *				maximum interval time between two packets of received/transmitted data
 *				c) When the <local_name> is not “COM:”, it indicates the maximum interval time
 *				between two packets of received/transmitted data.
 * @see
 * @return Return true if login to FTP server sucessfully
 */
bool b_gsm_cmd_ftp_set_curr_dir (const char* str_path_name, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_FTP_SET_CURR_DIR;
	v_delete_all_resp_status_on_table();
	// u16_timewait must be 20 - 180
	if ((u16_timewait < 20) || (u16_timewait > 180))
	{
		return false;
	}
	UARTprintf("AT+QFTPCWD=\"%s\"", str_path_name);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_FTP_SET_CURR_DIR_OK,
																						AT_RESP_STATUS_FTP_SET_CURR_DIR_FAIL,
																						u16_timewait * 100);
	return b_ret;
}

/*!
 * @fn bool b_gsm_cmd_ftp_download (const char* str_file_name, const char* str_local_name,
 *														uint32_t u32_start_pos, uint32_t u32_download_len, 
 *														uint16_t u16_timewait)
 * @brief Download a file from FTP server. You can output the file to COM port by
 * AT+QFTPGET=“filename”,“COM:”. The module will enter data mode on receiving data from server. 
 * After data is transferred completely, the module will exit from data mode automatically and 
 * output “QFTPGET: 0,<transferlen>”. You can save the file to RAM by 
 * AT+QFTPGET=“filename”,“RAM:localname” or to UFS by AT+QFTPGET=“filename”,“localname”. 
 * After file has been transferred completely, the module will output “+QFTPGET: 0,<transferlen>”.
 * If the <local_name> is “COM:”, “CONNECT” should be output in <timeout> set by AT+QFTPCFG. If the
 * <local_name> is not “COM:”, “OK” will be output first, “+QFTPGET: 0,<transferlen>” will be output
 * after data being transferred completely.
 * If the module has entered data mode or the <local_name> is not “COM:”, the <timeout> set by
 * AT+QFTPCFG indicates the maximum interval time between two packets of received/transmitted data.
 *
 * AT+QFTPGET=<file_name>,“COM:”[,<startpos>[,<downloadlen>]]
 * AT+QFTPGET=<file_name>,<local_name>[,<startpos >]
 * @param[in] str_file_name String type, the file name in FTP server. The maximum size of the 
 * parameter is 50 bytes
 * @param[in] str_local_name String type, the local file name. The maximum size of the parameter
 * is 60 bytes. If it is “COM:”, the file data will be output to COM port. If it starts with “RAM:”,
 * the file will be saved to RAM, else it will be saved to UFS. It is strongly recommended to save 
 * the file in RAM. Then you can read the file by AT+QFREAD (For details, please refer to
 * Quectel_UC15_FILE_AT_Commands_Manual)
 * @param[in] u32_start_pos Integer type, the start position of file to get. The default value is 0.  
 * @param[in] u32_download_len Integer type, the data length to download. It is valid only if 
 * <local_name> is “COM:”. If this parameter is specified, module will output <downloadlen> bytes to 
 * COM and exit from data mode. You can continue to get data from <startpos> by the same AT command 
 * if there is data lef.
 * @param[in] u16_timewait Integer type, the range is 20-180, the unit is second, 
 * and the default value is 90s. Generally, it is the timeout value for most 
 * +QFTPXXX: xx,xx after the result code OK is returned, except the commands 
 * QFTPPUT/QFTPGET/QFTPLST/QFTPNLST. The rules for these four commands are shown as below:
 *				a) When the command has been sent, but “CONNECT” has not been output yet,
 *				this parameter indicates the maximum interval time for “CONNECT” outputting
 *				after the command has been sent.
 *				b) When the module has entered into data mode, this parameter indicates the
 *				maximum interval time between two packets of received/transmitted data
 *				c) When the <local_name> is not “COM:”, it indicates the maximum interval time
 *				between two packets of received/transmitted data.
 * @see
 * @return Return true if login to FTP server sucessfully
 */
bool b_gsm_cmd_ftp_download (const char* str_file_name, const char* str_local_name,
														uint32_t u32_start_pos, uint32_t u32_download_len, 
														uint16_t u16_timewait)
{	
	v_delete_all_resp_status_on_table();
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	bool b_ret = false;
	// u16_timewait must be 20 - 180
	if ((u16_timewait < 20) || (u16_timewait > 180))
	{
		return false;
	}
	
	if(ustrcmp((const char*)(str_local_name), (const char*)FTP_DOWNLOAD_TO_COM) == 0)
	{
		e_gsm_active_cmd = GSM_CMD_FTP_DOWNLOAD_TO_COM;
		if(u32_start_pos != 0 && u32_download_len != 0)
		{
			//AT+QFTPGET=<file_name>,<local_name>[,<startpos >]
			UARTprintf("AT+QFTPGET=\"%s\",\"%s\",%u,%u", str_file_name, str_local_name, 
									u32_start_pos, u32_download_len);
		}
		UARTwriteData(au8_at_cmd_buf, 2);
		//respone
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_CONNECT_OK,
																							AT_RESP_STATUS_CME_ERROR,
																							u16_timewait * 100,
																							AT_RESP_STATUS_NONE,
																							AT_RESP_STATUS_NONE,
																							u16_timewait * 100);
		if (true == b_ret)
		{
			b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																								AT_RESP_STATUS_NONE,
																								u16_timewait * 100,
																								AT_RESP_STATUS_FTP_DOWNLOAD_TO_COM_OK,
																								AT_RESP_STATUS_FTP_DOWNLOAD_TO_COM_FAIL,
																								u16_timewait * 100);				
		}
	}
	else if(ustrcmp((const char*)(str_local_name), (const char*)FTP_DOWNLOAD_TO_RAM) == 0)
	{
		e_gsm_active_cmd = GSM_CMD_FTP_DOWNLOAD_TO_RAM;
		//AT+QFTPGET=<file_name>,“COM:”[,<startpos>[,<downloadlen>]]
		UARTprintf("AT+QFTPGET=\"%s\",\"%s\",0", str_file_name, str_local_name);
		UARTwriteData(au8_at_cmd_buf, 2);
		//respone
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																							AT_RESP_STATUS_CME_ERROR,
																							u16_timewait * 100,
																							AT_RESP_STATUS_NONE,
																							AT_RESP_STATUS_NONE,
																							0);
	}	
	else
	{
		e_gsm_active_cmd = GSM_CMD_FTP_DOWNLOAD_TO_UFS;
		//AT+QFTPGET=<file_name>,“COM:”[,<startpos>[,<downloadlen>]]
		UARTprintf("AT+QFTPGET=\"%s\",\"%s\",%u", str_file_name, str_local_name, 
								u32_start_pos);
		UARTwriteData(au8_at_cmd_buf, 2);
		//respone
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																							AT_RESP_STATUS_CME_ERROR,
																							u16_timewait * 100,
																							AT_RESP_STATUS_FTP_DOWNLOAD_TO_UFS_OK,
																							AT_RESP_STATUS_FTP_DOWNLOAD_TO_UFS_FAIL,
																							u16_timewait * 100);	
	}
	return b_ret;
}


bool b_gsm_cmd_ftp_wait_download_done(uint16_t u16_timewait)
{
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK,
																							AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL,
																							u16_timewait * 100,
																							AT_RESP_STATUS_NONE,
																							AT_RESP_STATUS_NONE,
																							0);
	return b_ret;
}
bool b_gsm_cmd_ftp_logout(void)
{
	// AT+QFTPCLOSE //Logout from FTP server.
	e_gsm_active_cmd = GSM_CMD_FTP_LOGOUT;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+QFTPCLOSE");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_FTP_LOGOUT_OK,
																						AT_RESP_STATUS_FTP_SET_CURR_DIR_FAIL,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

/*!
* @fn b_gsm_cmd_file_open(const char* c_filename, uint8_t u8_open_mode)
* @brief open file in module
* @param[in] c_filename Name of file
* @param[in] u8_open_mode Mode to open
*  0 If the file does not exist, it will be created; if the file exists, it will be directly
* opened. And both of them can be read and written.
*  1 If the file does not exist, it will be created; If the file exists, the file will be
* overwritten and cleared. And both of them can be read and written.
*  2 If the file exists, open it and can be read only. When the file does not exist, it
* will respond the error.
* @return True: open file sucessfully
*/
bool b_gsm_cmd_file_open(const char* c_filename, uint8_t u8_open_mode)
{
	e_gsm_active_cmd = GSM_CMD_FILE_OPEN;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+QFOPEN=\"%s\",%u", c_filename, u8_open_mode);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FILE_OPEN_OK,
																						AT_RESP_STATUS_CME_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
	
}

/*!
* @fn v_gsm_file_handle_set(uint32_t u32_handle)
* @brief Set the file handle using to read/write file in module SIM.
* This function must be called after recieved respond of open file command.
* @param[in] u32_handle File handle return from module SIM
* @return None
*/
void v_gsm_file_handle_set(char *pc_handle)
{
	memcpy(c_file_handle, pc_handle, strlen(pc_handle));
}

/*!
* @fn
* @brief AT+QFREAD reads the data of the file related to the handle. The data starts from the current position of
* the file pointer which belongs to the file handle.
* Write Command
* AT+QFREAD=<filehandle>[,<length>]
*/
bool b_gsm_cmd_file_read(uint16_t u16_read_length, uint16_t u16_timeout)
{
	e_gsm_active_cmd = GSM_CMD_FILE_READ;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+QFREAD=%s,%u", c_file_handle, u16_read_length);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FILE_READ_OK,
																						AT_RESP_STATUS_FILE_READ_FAIL,
																						u16_timeout * 100,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						0);
	return b_ret;
}

bool b_gsm_cmd_file_close(void)
{
	e_gsm_active_cmd = GSM_CMD_FILE_CLOSE;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+QFCLOSE=%s", c_file_handle);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_CME_ERROR,
																						(5 * 100),
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return b_ret;
}

bool b_gsm_cmd_file_delete(const char* c_filename)
{
	e_gsm_active_cmd = GSM_CMD_FILE_DELETE;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+QFDEL=\"%s\"", c_filename);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_CME_ERROR, (5 * 100),
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return b_ret;
}
/**
* private function bodies
*/

gsm_active_cmd_t e_gsm_active_cmd_get(void)
{
	return e_gsm_active_cmd;
}

/*!
* @fn b_is_activating(char c_pdp_context_id)
* @brief Check if a given id in ASCII type is in pdp context handle table or not
* @param[in] c_pdp_context_id ID of pdp context, in ASCII type
* @return True: ID existed in pdp context table 
*					False: ID didn't exist in pdp context table 
*/
bool b_is_activating(char c_pdp_context_id)
{
	bool b_result = false;
	uint8_t u8_context_id = c_pdp_context_id - 0x30;	//convert char to number
	for(uint8_t i = 0; i < MAX_HANDLE_PDP_CONTEXT_ID; i++)
	{
		if(u8_context_id == au8_active_pdp_table[i])
		{
			b_result = true;
			break;
		}
	}
	return b_result;
}


/*!
* @fn v_mqtt_network_close(bool b_close)
* @brief Set state of mqtt network. This function must be called when recive QMTSTAT
* @param[in] b_close Is close or not
* @return None
*/
void v_gsm_mqtt_network_close(bool b_close)
{
	b_is_mqtt_network_close = b_close;
}

/*!
* @fn b_is_mqtt_network_closed(void)
* @brief Get state of mqtt network 
* @param None
* @return None
*/
bool b_gsm_is_mqtt_network_closed(void)
{
	return b_is_mqtt_network_close;
}

// coding... func send cmd to esp
int8_t i8_gsm_cmd_esp_check_mqtt_state()
{
	UARTprintf("AT+MCON:?\r\n");
	int8_t i8_ret = 0;
	i8_ret = i8_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_MQTT_PUBLISH_OK,
																						AT_RESP_STATUS_MQTT_PUBLISH_FAIL, (5 * 100),
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return i8_ret;
}

bool b_gsm_cmd_esp_mqtt_publish(uint16_t u16_len, uint8_t* pu8_data)
{
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+MPUT:");
	UARTwriteData(pu8_data, u16_len);
	//UARTprintf("\r\n");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_MQTT_PUBLISH_OK,
																						AT_RESP_STATUS_MQTT_PUBLISH_FAIL, (5 * 100),
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return b_ret;
}

bool b_gsm_cmd_esp_ftp_download(const char* str_file_name, uint32_t u32_crc, uint16_t u16_timewait)
{
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+FDOW:");
	au8_at_cmd_buf[0] = (uint8_t) (u32_crc >> 24) & 0xFF;
	au8_at_cmd_buf[1] = (uint8_t) (u32_crc >> 16) & 0xFF;
	au8_at_cmd_buf[2] = (uint8_t) (u32_crc >> 8) & 0xFF;
	au8_at_cmd_buf[3] = (uint8_t) u32_crc & 0xFF;
	UARTwriteData(au8_at_cmd_buf, 4);
	UARTprintf("%s\r\n", str_file_name);
//	au8_at_cmd_buf[0] = CR;
//	au8_at_cmd_buf[1] = LF;
//	UARTwriteData(au8_at_cmd_buf, 2);
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK,
																						AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL, (u16_timewait * 100),
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return b_ret;
}

bool b_gsm_cmd_esp_file_pull(uint16_t u16_timewait)
{
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+FSHA:?\r\n");
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FILE_READ_OK,
																						AT_RESP_STATUS_FILE_READ_FAIL, (u16_timewait * 100),
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return b_ret;
}

bool b_gsm_cmd_get_imei(void)
{	
	e_gsm_active_cmd = GSM_CMD_GET_RSSI;
	//Get RSSI
	UARTprintf("AT+SIMEI?");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_IMEI_OK,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

void v_gsm_imei_set(char *pc_handle)
{
	memcpy(c_imei, pc_handle, strlen(pc_handle));
}

/*!
 * @fn bool b_gsm_cmd_check_ue_system(void)
 * @brief This function is used for checking UE system information?
 * @param[in] 
 * @see
 * @return true if mode GSM/WCDMA/LTE
 */
bool b_gsm_cmd_check_ue_system(void)
{	
	v_delete_all_resp_status_on_table();	
	e_gsm_active_cmd = GSM_CMD_CHECK_UE;
	//Check CPIN
	UARTprintf("AT+CPSI?");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_SYSTEM_OK,
																						AT_RESP_STATUS_SYSTEM_NOSERVICE,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

bool b_gsm_cmd_activate_pdp_context_4g(bool b_active, uint8_t u8_context_id)
{	
	e_gsm_active_cmd = GSM_CMD_ACTIVE_PDP_CONTEXT;
	v_delete_all_resp_status_on_table();	
	bool b_ret = false;
	if (b_active)
	{
		UARTprintf("AT+CGACT=1,%d", u8_context_id);		
	}
	else
	{
		UARTprintf("AT+CGACT=0,%d", u8_context_id);
	}		
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	if(b_ret)
	{
		while((b_pdp_context_id_add(u8_context_id)) != true){}	//hold the process untill reset
	}
	return b_ret;
}

bool b_gsm_cmd_check_pdp_context_4g(void)
{	
	e_gsm_active_cmd = GSM_CMD_CHECK_PDP_CONTEXT;
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CGACT?");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_PDP_ACTIVE,
																						AT_RESP_STATUS_PDP_DEACTIVE,
																						AT_RESP_STATUS_OK, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

bool b_gsm_mqtt_start_4g(void)
{
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CMQTTSTART");
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_MQTT_START_OK,
																						AT_RESP_STATUS_MQTT_START_FAIL,
																						TIME_TO_WAIT_MQTTSTART_RESP,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);		
	return b_ret;
}

bool b_gsm_mqtt_acquire_client(uint8_t u8_client_index)
{
	v_delete_all_resp_status_on_table();	
	UARTprintf("AT+CMQTTACCQ=%d,\"A7670C%s\"", u8_client_index,c_imei);
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//get at resp on at resp table
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						AT_RESP_STATUS_NONE, 
																						AT_RESP_STATUS_NONE);	
	return b_ret;
}

bool b_gsm_cmd_config_utf8_4g(uint8_t u8_client_id, bool b_check_flag, uint8_t u8_timeout)
{
	v_delete_all_resp_status_on_table();
	bool b_ret = false;
	UARTprintf("AT+CMQTTCFG=\"checkUTF8\",%u,%u", u8_client_id, b_check_flag);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_NONE,
																										AT_RESP_STATUS_NONE,
																										TIME_TO_WAIT_AT_RESP);
	if (b_ret)
	{
		/*Configure the max timeout interval of the send or receive data operation */
		v_delete_all_resp_status_on_table();
		bool b_ret = false;
		UARTprintf("AT+CMQTTCFG=\"optimeout\",%u,%u", u8_client_id, u8_timeout);				
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);
		
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																											AT_RESP_STATUS_ERROR,
																											TIME_TO_WAIT_AT_RESP,
																											AT_RESP_STATUS_NONE,
																											AT_RESP_STATUS_NONE,
																											TIME_TO_WAIT_AT_RESP);
	}
	return b_ret;
}

bool b_gsm_cmd_mqtt_connect_4g(uint8_t u8_client_id, const char* str_host_name,
															 uint16_t u16_timewait, uint8_t u8_clean_session,
															 const char* str_username,const char* str_password)
{
	e_gsm_active_cmd = GSM_CMD_MQTT_CONNECT;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+CMQTTCONNECT=%u,\"%s\",%u,%u,\"%s\",\"%s\"",
	u8_client_id, str_host_name, u16_timewait,u8_clean_session, str_username, str_password);
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_MQTT_CONNECT_OK,
																						AT_RESP_STATUS_MQTT_CONNECT_FAIL,
																						TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_mqtt_disconnect_4g (uint8_t u8_client_id, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_DISCONNECT;
	v_delete_all_resp_status_on_table();
	// TODO: check input param
	// u8_client_id must be 0 - 5
	if (u8_client_id > 5)
	{
		return false;
	}
	UARTprintf("AT+CMQTTDISC=%u", u8_client_id);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_MQTT_DISCONNECT_OK,
																						AT_RESP_STATUS_MQTT_DISCONNECT_FAIL,
																						u16_timewait * 100,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						0);
	return b_ret;
} 

bool b_gsm_cmd_query_me(void)
{
	v_delete_all_resp_status_on_table();
	bool b_ret = false;
	UARTprintf("AT+CFUN?");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FTP_CHECK_FUN_OK,
																										AT_RESP_STATUS_FTP_CHECK_FUN_FAIL,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_NONE,
																										TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_ftp_start_4g(void)
{	
	v_delete_all_resp_status_on_table();
	bool b_ret = false;
	UARTprintf("AT+CFTPSSTART");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_FTP_START_OK,
																										AT_RESP_STATUS_FTP_START_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_ftp_login_4g (const char* str_host_name, uint16_t u16_port, const char* str_user_name,
														 const char* str_password, uint8_t u8_server_type ,uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_FTP_OPEN;
	v_delete_all_resp_status_on_table();

	UARTprintf("AT+CFTPSLOGIN=\"%s\",%u,\"%s\",\"%s\",%u", str_host_name, u16_port, str_user_name, str_password, u8_server_type);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_FTP_OPEN_OK,
																						AT_RESP_STATUS_FTP_OPEN_FAIL,
																						u16_timewait*100);
	return b_ret;
}

bool b_gsm_cmd_ftp_set_transfer_type(void)
{	
	v_delete_all_resp_status_on_table();
	bool b_ret = false;
	UARTprintf("AT+CFTPSTYPE=I");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_FTP_SET_TYPE_OK,
																										AT_RESP_STATUS_FTP_SET_TYPE_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_ftp_set_dir_4g (const char* str_path_name, uint16_t u16_timewait)
{	
	e_gsm_active_cmd = GSM_CMD_FTP_SET_CURR_DIR;
	v_delete_all_resp_status_on_table();

	UARTprintf("AT+CFTPSCWD=\"%s\"", str_path_name);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_FTP_SET_CURR_DIR_OK,
																						AT_RESP_STATUS_FTP_SET_CURR_DIR_FAIL,
																						u16_timewait * 100);
	return b_ret;
}

bool b_gsm_cmd_ftp_get_file_size_4g (const char* str_path_name)
{
	e_gsm_active_cmd = GSM_CMD_FTP_SET_CURR_DIR;
	v_delete_all_resp_status_on_table();
	
	UARTprintf("AT+CFTPSSIZE=\"%s\"", str_path_name);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_FTP_GET_SIZE_OK,
																										AT_RESP_STATUS_FTP_GET_SIZE_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_ftp_get_file_serial_4g(const char* str_path_name)
{
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+CFTPSGETFILE=\"%s\"", str_path_name);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK,
																										AT_RESP_STATUS_NONE,
																										TIME_TO_WAIT_AT_RESP*5,
																										AT_RESP_STATUS_NONE,
																										AT_RESP_STATUS_NONE,
																										TIME_TO_WAIT_AT_RESP);
																										
	return b_ret;
}

bool b_gsm_cmd_ftp_logout_4g(void)
{
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+CFTPSLOGOUT");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	//respone
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_FTP_LOGOUT_OK,
																										AT_RESP_STATUS_FTP_LOGOUT_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_get_file_from_fs_4g (const char* str_path_name, uint32_t u32_location, uint16_t u16_size)
{
	v_delete_all_resp_status_on_table();
	//respone
	bool b_ret = false;
	if (u16_size > 0)
	{
		UARTprintf("AT+CFTRANTX=\"c:/%s\",%u,%u",str_path_name, u32_location, u16_size);				
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);		
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_FILE_READ_OK,
																											AT_RESP_STATUS_ERROR,
																											TIME_TO_WAIT_AT_RESP,
																											AT_RESP_STATUS_NONE,
																											AT_RESP_STATUS_NONE,
																											TIME_TO_WAIT_AT_RESP);
	}
	return b_ret;
}

bool b_gsm_cmd_file_delete_4g(const char* c_filename)
{
	e_gsm_active_cmd = GSM_CMD_FILE_DELETE;
	v_delete_all_resp_status_on_table();
	UARTprintf("AT+FSDEL=%s", c_filename);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																						AT_RESP_STATUS_ERROR, 
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE, 0);
	return b_ret;
}

bool b_gsm_cmd_ftp_stop_4g(void)
{	
	v_delete_all_resp_status_on_table();
	bool b_ret = false;
	UARTprintf("AT+CFTPSSTOP");				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_FTP_STOP_OK,
																										AT_RESP_STATUS_FTP_STOP_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	return b_ret;
}

bool b_gsm_cmd_mqtt_publish_msg_4g (uint8_t u8_client_id, uint8_t u8_qos, uint8_t u8_retain, 
																		const char* str_topic, uint16_t u16_len, uint8_t* pu8_payload,
																		uint16_t u16_pkt_timeout, uint8_t u8_retry_time)
{	
//	while(i16_gsm_take_semaphore(GSM_TAKE_SEMAPHORE_TIMEOUT))
//	{
//		vTaskDelay(1);
//	}
	bool b_ret = false;
	uint8_t u8_topic_len = 0;
	uint8_t u8_payload_len = 0;
	char c_topic_full[MQTT_TOPIC_LEN_MAX];
	e_gsm_active_cmd = GSM_CMD_MQTT_PUBLISH;
	v_delete_all_resp_status_on_table();
	
	// TODO: check input param
	// u8_connect_id must be 0 - 1
	if (u8_client_id > 1)
	{
		return false;
	}
	// u8_qos must be 0 - 2
	if (u8_qos > 2)
	{
		return false;
	}	
	// u8_retain must be 0 - 1
	if (u8_retain > 1)
	{
		return false;
	}		
	// u16_len must be 1 - 1500
	if ((u16_len < 1) || (u16_len > 1500))
	{
		return false;
	}	
	// pu8_data must be valid
	if (NULL == pu8_payload)
	{
		return false;
	}
	usprintf(c_topic_full, "%s/%u/%u", str_topic,TOPIC_ID,DEVICE_SUB_ID);
	u8_topic_len = strlen(c_topic_full); 
	/*input the topic*/
	UARTprintf("AT+CMQTTTOPIC=%u,%u", u8_client_id, u8_topic_len);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_PROMOTING_MARK,
																						AT_RESP_STATUS_ERROR,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);	
	if (b_ret)
	{
		v_delete_all_resp_status_on_table();
		UARTprintf("%s", c_topic_full);
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);		
		b_ret = false;
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_NONE,
																										AT_RESP_STATUS_NONE,
																										TIME_TO_WAIT_AT_RESP);
	}
	
	/*input the message*/
	if (b_ret)
	{
		v_delete_all_resp_status_on_table();
		UARTprintf("AT+CMQTTPAYLOAD=%u,%u", u8_client_id, u16_len);	
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);
		b_ret = false;
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_PROMOTING_MARK,
																							AT_RESP_STATUS_ERROR,
																							TIME_TO_WAIT_AT_RESP,
																							AT_RESP_STATUS_NONE,
																							AT_RESP_STATUS_NONE,
																							TIME_TO_WAIT_AT_RESP);
		if (b_ret)
		{
			v_delete_all_resp_status_on_table();
			UARTwriteData(pu8_payload, u16_len);
			au8_at_cmd_buf[0] = CR;
			au8_at_cmd_buf[1] = LF;
			UARTwriteData(au8_at_cmd_buf, 2);		
			b_ret = false;
			b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																											AT_RESP_STATUS_ERROR,
																											TIME_TO_WAIT_AT_RESP,
																											AT_RESP_STATUS_NONE,
																											AT_RESP_STATUS_NONE,
																											TIME_TO_WAIT_AT_RESP);
		}
	}
	/*publish a message*/
	if (b_ret)
	{
		v_delete_all_resp_status_on_table();
		UARTprintf("AT+CMQTTPUB=%u,%u,%u,%u", u8_client_id, u8_qos, 60, u8_retain);				
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);
		b_ret = false;
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																											AT_RESP_STATUS_ERROR,
																											TIME_TO_WAIT_AT_RESP,
																											AT_RESP_STATUS_MQTT_PUBLISH_OK,
																											AT_RESP_STATUS_MQTT_PUBLISH_FAIL,
																											TIME_TO_WAIT_AT_RESP);
	}
//	i16_gsm_give_semaphore();
	return b_ret;
}

bool b_gsm_cmd_mqtt_subscibe_4g(uint8_t u8_client_id, const char* str_topic, uint8_t u8_qos, uint8_t u8_retry_time)
{	
	e_gsm_active_cmd = GSM_CMD_MQTT_SUBSCRIBE;
	v_delete_all_resp_status_on_table();
	bool b_frame_ping = false;
	uint8_t u8_topic_len = 0;
	char c_topic_full[MQTT_TOPIC_LEN_MAX];
	// TODO: check input param
	// u8_client_id must be 0 - 1
	if (u8_client_id > 1)
	{
		return false;
	}
	
	if(ustrcmp((const char*)(str_topic), (const char*)TOPIC_PING) == 0)
	{
		b_frame_ping = true;
		usprintf(c_topic_full, "%s/%u/%u", str_topic,1,1);
		u8_topic_len = strlen(c_topic_full); 
	}
	else
	{
		b_frame_ping = false;
		usprintf(c_topic_full, "%s/%u/%u", str_topic,TOPIC_ID,DEVICE_SUB_ID);
		u8_topic_len = strlen(c_topic_full);
	}
	UARTprintf("AT+CMQTTSUB=%u,%u,%u", u8_client_id, u8_topic_len, u8_qos);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_PROMOTING_MARK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	
	if (b_ret)
	{
		UARTprintf("%s", c_topic_full);
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);
		
		b_ret = false;
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_MQTT_SUBSCRIBE_OK,
																										AT_RESP_STATUS_MQTT_SUBSCRIBE_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	}	
	return b_ret;
}

bool b_gsm_cmd_mqtt_unsubscibe_4g(uint8_t u8_client_id, const char* str_topic)
{
	e_gsm_active_cmd = GSM_CMD_MQTT_SUBSCRIBE;
	v_delete_all_resp_status_on_table();
	bool b_frame_ping = false;
	uint8_t u8_topic_len = 0;
	char c_topic_full[MQTT_TOPIC_LEN_MAX];
	// TODO: check input param
	// u8_client_id must be 0 - 1
	if (u8_client_id > 1)
	{
		return false;
	}
	
	if(ustrcmp((const char*)(str_topic), (const char*)TOPIC_PING) == 0)
	{
		b_frame_ping = true;
		usprintf(c_topic_full, "%s/%u/%u", str_topic,1,1);
		u8_topic_len = strlen(c_topic_full); 
	}
	else
	{
		b_frame_ping = false;
		usprintf(c_topic_full, "%s/%u/%u", str_topic,TOPIC_ID,DEVICE_SUB_ID);
		u8_topic_len = strlen(c_topic_full);
	}
	UARTprintf("AT+CMQTTUNSUB=%u,%u,%u", u8_client_id, u8_topic_len);				
	au8_at_cmd_buf[0] = CR;
	au8_at_cmd_buf[1] = LF;
	UARTwriteData(au8_at_cmd_buf, 2);
	bool b_ret = false;
	b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_PROMOTING_MARK,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP,
																						AT_RESP_STATUS_NONE,
																						AT_RESP_STATUS_NONE,
																						TIME_TO_WAIT_AT_RESP);
	if (b_ret)
	{
		UARTprintf("%s", c_topic_full);
		au8_at_cmd_buf[0] = CR;
		au8_at_cmd_buf[1] = LF;
		UARTwriteData(au8_at_cmd_buf, 2);
		
		b_ret = false;
		b_ret = b_check_at_resp_status_from_table_timewait(AT_RESP_STATUS_OK,
																										AT_RESP_STATUS_ERROR,
																										TIME_TO_WAIT_AT_RESP,
																										AT_RESP_STATUS_MQTT_UNSUBSCRIBE_OK,
																										AT_RESP_STATUS_MQTT_UNSUBSCRIBE_FAIL,
																										TIME_TO_WAIT_AT_RESP);
	}	
	return b_ret;
}

/*!
*	@fn b_check_at_resp_status_from_table (at_resp_status_t e_status,
*														 at_resp_status_t e_not_expected_status,
*														 at_resp_status_t e_status_2,
*														 at_resp_status_t e_not_expected_status_2)
* @brief Check AT respone status
* @param[in] e_status
* @param[in] e_not_expected_status
* @param[in] e_status_2
* @param[in] e_not_expected_status_2
* @param[out] none
* @return true if we found expected AT respone status before time out
*/
static bool b_check_at_resp_status_from_table (at_resp_status_t e_status,
																 at_resp_status_t e_not_expected_status,
																 at_resp_status_t e_status_2,
																 at_resp_status_t e_not_expected_status_2)
{
	//get at resp on at resp table
	bool b_ret = false;
	uint16_t u16_time_out = TIME_TO_WAIT_AT_RESP;
	while(u16_time_out > 0)
	{
		if(b_get_at_resp_status_from_table(e_status))
		{
			b_ret = true;
			break;
		}
		else if(b_get_at_resp_status_from_table(e_not_expected_status))
		{
			b_ret = false;
			break;			
		}
		vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
		u16_time_out--;
	}
	if((b_ret == true) && (e_status_2 != AT_RESP_STATUS_NONE))
	{
		b_ret = false;
		u16_time_out = TIME_TO_WAIT_AT_RESP;
		while(u16_time_out > 0)
		{
			if(b_get_at_resp_status_from_table(e_status_2))
			{
				b_ret = true;
				break;
			}
			else if(b_get_at_resp_status_from_table(e_not_expected_status_2))
			{
				b_ret = false;
				break;			
			}
			vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
			u16_time_out--;
		}		
	}
	else
	{
		b_get_at_resp_status_from_table(e_status_2);
		b_get_at_resp_status_from_table(e_not_expected_status_2);
	}	
	return b_ret;
}

/*!
*	@fn b_check_at_resp_status_from_table_timewait (at_resp_status_t e_status,
*																		 at_resp_status_t e_not_expected_status,
*																		 uint16_t u16_wait_time,
*																		 at_resp_status_t e_status_2,
*																		 at_resp_status_t e_not_expected_status_2,
*																		 uint16_t u16_wait_time_2)
* @brief Check AT respone status
* @param[in] e_status
* @param[in] e_not_expected_status
* @param[in] u16_wait_time
* @param[in] e_status_2
* @param[in] e_not_expected_status_2
* @param[in] u16_wait_time_2
* @param[out] none
* @return true if we found expected AT respone status before time out
*/
static bool b_check_at_resp_status_from_table_timewait (at_resp_status_t e_status,
																				 at_resp_status_t e_not_expected_status,
																				 uint16_t u16_wait_time,
																				 at_resp_status_t e_status_2,
																				 at_resp_status_t e_not_expected_status_2,
																				 uint16_t u16_wait_time_2)
{
	//get at resp on at resp table
	bool b_ret = false;
	uint16_t u16_time_out = u16_wait_time;
	while(u16_time_out > 0)
	{
		if(b_get_at_resp_status_from_table(e_status))
		{
			b_ret = true;
			break;
		}
		else if(b_get_at_resp_status_from_table(e_not_expected_status))
		{
			b_ret = false;
			break;			
		}
		vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
		u16_time_out--;
	}	
	if((b_ret == true) && (e_status_2 != AT_RESP_STATUS_NONE))
	{
		b_ret = false;
		u16_time_out = u16_wait_time_2;
		while(u16_time_out > 0)
		{
			if(b_get_at_resp_status_from_table(e_status_2))
			{
				b_ret = true;
				break;
			}
			else if(b_get_at_resp_status_from_table(e_not_expected_status_2))
			{
				b_ret = false;
				break;			
			}
			vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
			u16_time_out--;
		}		
	}
	else
	{
		b_get_at_resp_status_from_table(e_status_2);
		b_get_at_resp_status_from_table(e_not_expected_status_2);
	}	
	return b_ret;
}

/*!
* @fn bool b_pdp_context_id_add(uint8_t u8_context_id)
* @brief Add a new pdp context id to pdp context table.
* Must be call when activating a new pdp context.
* @param[in] u8_context_id ID of pdp context, in range [1-16]
* @return True: add successfully  
*					False: the ID is not in [1-16] or pdp context table was full
*/
static bool b_pdp_context_id_add(uint8_t u8_context_id)
{
	bool b_result = false;
	if((u8_context_id > 0) && (u8_context_id <= 16))
	{
		for(uint8_t i = 0; i < MAX_HANDLE_PDP_CONTEXT_ID; i++)
		{
			if(0 == au8_active_pdp_table[i])
			{
				au8_active_pdp_table[i] = u8_context_id;
				b_result = true;
				break;
			}
		}
	}
	return b_result;
}

/*!
* @fn v_pdp_context_id_remove(uint8_t u8_context_id)
* @brief Remove pdp context id in pdp context table. Must be called when deactive pdp context.
* @param[in] u8_context_id ID of pdp context, in range [1-16]
* @return None
*/
static void v_pdp_context_id_remove(uint8_t u8_context_id)
{
	for(uint8_t i; i < MAX_HANDLE_PDP_CONTEXT_ID; i++)
	{
		if(u8_context_id == au8_active_pdp_table[i])
		{
			au8_active_pdp_table[i] = 0;
		}
	}
}

/*!
*	@fn i8_check_at_resp_status_from_table (at_resp_status_t e_status,
*														 at_resp_status_t e_not_expected_status,
*														 at_resp_status_t e_status_2,
*														 at_resp_status_t e_not_expected_status_2)
* @brief Check AT respone status
* @param[in] e_status
* @param[in] e_not_expected_status
* @param[in] e_status_2
* @param[in] e_not_expected_status_2
* @param[out] none
* @return 1: if we found expected AT respone status before time out
					0: if not found
					-1: if timeout
*/
static int8_t i8_check_at_resp_status_from_table_timewait (at_resp_status_t e_status,
																				 at_resp_status_t e_not_expected_status,
																				 uint16_t u16_wait_time,
																				 at_resp_status_t e_status_2,
																				 at_resp_status_t e_not_expected_status_2,
																				 uint16_t u16_wait_time_2)
{
	//get at resp on at resp table
	int8_t i8_ret = -1;
	uint16_t u16_time_out = u16_wait_time;
	while(u16_time_out > 0)
	{
		if(b_get_at_resp_status_from_table(e_status))
		{
			i8_ret = 1;
			break;
		}
		else if(b_get_at_resp_status_from_table(e_not_expected_status))
		{
			i8_ret = 0;
			break;			
		}
		vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
		u16_time_out--;
	}	
	if((i8_ret == 1) && (e_status_2 != AT_RESP_STATUS_NONE))
	{
		i8_ret = 0;
		u16_time_out = u16_wait_time_2;
		while(u16_time_out > 0)
		{
			if(b_get_at_resp_status_from_table(e_status_2))
			{
				i8_ret = 1;
				break;
			}
			else if(b_get_at_resp_status_from_table(e_not_expected_status_2))
			{
				i8_ret = 0;
				break;			
			}
			vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
			u16_time_out--;
		}		
	}
	else
	{
		b_get_at_resp_status_from_table(e_status_2);
		b_get_at_resp_status_from_table(e_not_expected_status_2);
	}	
	return i8_ret;
}
