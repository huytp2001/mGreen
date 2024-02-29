/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file gsm_check_at.c
 * @author Danh Pham, Bach Pham
 * @date 15 Dec 2020
 * @version: 1.0.0
 * @brief This file handles and parses AT command from module SIM
 */ 
 /*!
 * Add more include here
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gsm_cmd.h"
#include "gsm_check_at.h"
#include "gsm_hal.h"
#include "uartstdio.h"
#include "ustdlib.h"
#include "frame_parser.h"

#include "FreeRTOS.h"
#include "task.h"
 
 /* Private variables */
 static uint8_t au8_at_res_buf[AT_RESPONE_BUFFER_SIZE] = {0};
 static int32_t i32_at_respone_data_len = -1;
 static at_resp_status_t ae_at_resp_status_table[MAX_AT_RESP_STATUS_TABLE];
 static uint8_t u8_at_resp_status_table_write_index = 0;
 static mqtt_t stru_mqtt;
 /* Private functions prototype */
 static bool b_compare_at_command(const char* c_command, uint8_t *pu8_data);
 static void v_add_at_resp_status_to_table (at_resp_status_t e_resp_status);
 
 /*!
*	@fn v_gsm_check_at_respone (void)
* @brief Check AT respone in uart rx buffer
* @param[in] none
* @param[out] none
* @return \b true if we found one AT respone
*/
void v_gsm_check_at_respone (void)
{
	static bool b_get_payload = false;
	static bool b_get_file = false;
	static uint16_t u16_payload_len = 0;
	static uint16_t u16_packet_size = 0;
	//Peek AT respone in UART RX buffer
	memset(au8_at_res_buf, 0, AT_RESPONE_BUFFER_SIZE);
	i32_at_respone_data_len = UARTPeekATResponePattern(au8_at_res_buf, AT_RESPONE_BUFFER_SIZE);
	/*remove <CR><LF> */
	uint16_t u16_data_len= strlen((const char*)au8_at_res_buf);
	if(0x0A == au8_at_res_buf[u16_data_len - 1])
	{
		au8_at_res_buf[u16_data_len - 1] = 0;
	}
	if(0x0D == au8_at_res_buf[u16_data_len - 2])
	{
		au8_at_res_buf[u16_data_len - 2] = 0;
	}
	
#ifdef	_DEBUG_GSM_CMD
	static uint32_t u32_cnt_at_resp = 0;
	static uint32_t u32_cnt_mqtt_revc = 0;
	static uint32_t u32_cnt_mqtt_revc_ok = 0;
	static uint32_t u32_cnt_at_resp_fail = 0;
#endif
	
	if(i32_at_respone_data_len >= AT_RESPONE_SIZE_MIN)
	{
#ifdef	_DEBUG_GSM_CMD		
		u32_cnt_at_resp++;
#endif
		
		// Check SIM READY
		// <CR><LF>RDY<CR><LF>
		if(b_compare_at_command("RDY", au8_at_res_buf) || (b_compare_at_command("PB DONE", au8_at_res_buf)))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_RDY);
		}		
		// Check AT respone of disable echo mode
		// ATE0<CR><LF><CR><LF>OK<CR><LF>
		else if(b_compare_at_command("ATE0", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_DISABLE_ECHO);
		}
		// Check AT respone OK
		// <CR><LF>OK<CR><LF>
		else if(b_compare_at_command("OK", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_OK);
		}
		// Check AT respone ERROR
		// <CR><LF>ERROR<CR><LF>
		else if(b_compare_at_command("ERROR", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_ERROR);
		}
		// Check AT respone +CME ERROR: 
		else if(b_compare_at_command("+CME ERROR:", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_CME_ERROR);
		}
		// Check AT respone POWERED DOWN: 
		else if(b_compare_at_command("POWERED DOWN", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_PWR_OFF);
		}
		// Check AT respone for promoting mark
		// <CR><LF>'>'
		else if(b_compare_at_command(">", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_PROMOTING_MARK);
		}
		// Check AT respone for CPIN READY
		// we expect to receive: "+CPIN: READY"
		else if(b_compare_at_command("+CPIN: READY", au8_at_res_buf))
		{
			if(ustrcmp((const char*)(au8_at_res_buf), (const char*)AT_RESP_CPIN_READY) == 0)
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CPIN_READY);							
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CPIN_NOT_READY);
			}
		}	
		// Check AT respone for REGISTERED NETWORK
		// "+CREG: 0,1" : Registered, home network
		// "+CREG: 0,5" : Registered, roaming
		// "+CREG: 0,6" : Registered "SMS only", home network
		else if(b_compare_at_command("+CREG:", au8_at_res_buf))
		{
			if((au8_at_res_buf[9] == '1') || (au8_at_res_buf[9] == '5') || (au8_at_res_buf[9] == '6'))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_REGISTERED_NET);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_NOT_REGISTERED_NET);
			}
		}
		// Check AT respone of get RSSI 
		// <CR><LF>+CSQ: xx,x<CR><LF>
		else if(b_compare_at_command("+CSQ:", au8_at_res_buf))
		{
			uint8_t u8_tmp = 0;
			u8_tmp = ustrtoul((const char*)(au8_at_res_buf+6),0,10);
			if (u8_tmp <= 31)
			{
				v_gsm_hal_set_sim_rssi(u8_tmp);
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CSQ_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_NONE);
			}
		}
		// Check AT respone for Packet Domain Service Attached
		// "+CGATT: 0" : PS not attached
		// "+CGATT: 1" : PS attached
		else if(b_compare_at_command("+CGATT:", au8_at_res_buf))
		{
			if(au8_at_res_buf[8] == '1')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_PS_ATTACHED);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_PS_NOT_ATTACHED);
			}
		}
		// Check AT respone for REGISTERED PS
		// "+CGREG: 0,1" : Registered, home network
		// "+CGREG: 0,5" : Registered, roaming
		// "+CGREG: 0,6" : Registered "SMS only", home network
		else if(b_compare_at_command("+CGREG:", au8_at_res_buf))
		{
			if((au8_at_res_buf[10] == '1') || (au8_at_res_buf[10] == '5') || (au8_at_res_buf[10] == '6'))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_REGISTERED_PS);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_NOT_REGISTERED_PS);
			}
		}
		// Check AT respone for reading current activated context.
		// "+QIACT: <contextID>,<context_state>,<IP_address>"
		// "+QIACT: 1,1"
		else if(b_compare_at_command("+QIACT:", au8_at_res_buf))
		{
			if(b_is_activating(au8_at_res_buf[8]))	//Check if return id value is in pdp context table
			{
				if(au8_at_res_buf[10] == '1')	//check if pdp type is ipv4
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_PDP_ACTIVE);					
				}
				else
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_PDP_DEACTIVE);
				}
			}
		}
		// Check AT respone start a socket successfully by AT+QIOPEN
		// <CR><LF>CONNECT<CR><LF>
//		else if(b_compare_at_command("CONNECT", au8_at_res_buf))
//		{
//			v_add_at_resp_status_to_table(AT_RESP_STATUS_CONNECT_OK);
//		}
		// Check AT respone query the socket service status by AT+QISTATE
		/* <CR><LF>+QISTATE:
			 <connectID>,<service_type>,<IP_address>,<remote_port>,<local_port>,
			 <socket_state>,<contextID>,<serverID>,<access_mode>,<AT_port><CR><LF>
		*/
		else if(b_compare_at_command("+QISTATE", au8_at_res_buf))
		{
			uint8_t u8_cnt_comma = 0;
			bool b_ret = false;
			for(uint16_t u16_i = 9; u16_i < AT_RESPONE_BUFFER_SIZE; u16_i++)
			{
				if (au8_at_res_buf[u16_i] == ',')
				{
					u8_cnt_comma++;
				}
				if (u8_cnt_comma == 5)
				{
					/*
					<socket_state> Integer type, socket service state
					0 “Initial?connection has not been established
					1 “Opening?client is connecting or server is trying to listen
					2 “Connected?client/incoming connection has been established
					3 “Listening?server is listening
					4 “Closing?connection is closing
					*/
					if (au8_at_res_buf[u16_i + 1] == '2')
					{
						b_ret = true;
					}
					break;
				}
			}
			if (b_ret == true)
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CONNECT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CONNECT_FAIL);
			}
		}
		// Check AT respone query the sent data by AT+QISEND
		/* <CR><LF>+QISEND:
			 <total_send_length>,<ackedbytes>,<unackedbytes><CR><LF>
		*/
		else if(b_compare_at_command("+QISEND", au8_at_res_buf))
		{
			uint16_t u16_tmp_unacked = 0;
			char* ptr;
			//Get send length in total
			ustrtoul((const char*)(au8_at_res_buf + 8), (const char**)&ptr, 10);
			//Get number of byte acked 
			ustrtoul(ptr + 1, (const char**)&ptr, 10);
			//Get number of byte unacked 
			u16_tmp_unacked = ustrtoul(ptr + 1, (const char**)&ptr, 10);
			if (u16_tmp_unacked == 0)
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_SEND_ACK);
			}
		}
		// Check AT respone AT+QPING Ping a Remote Server
		/* <CR><LF>[+QPING:
								<result>[,<IP_address>,<bytes>,<time>,<ttl>]<CR><LF>…]
								+QPING:
								<finresult>[,<sent>,<rcvd>,<lost>,<min>,<max>,<avg>]
		*/
		else if(b_compare_at_command("+QPING", au8_at_res_buf))
		{
			/*
			<result> The result of each ping request
								0 Received the ping response from the server. In this case, it is followed by
									?<IP_address>,<bytes>,<time>,<ttl>?
								1 Timeout for the ping request. In the case, no other information followed
			*/
			/*
			<finresult> The final result of the command
								2 It is finished normally. It is successful to activate the context and find the
									host. In this case, it is followed by ?<sent>,<rcvd>,<lost>,<min>,<max>,<avg>?
								3 The TCP/IP stack is busy now. In the case, no other information followed
								4 Not find the host. In the case, no other information followed
								5 Failed to activate PDP context. In this case, no other information followed
			*/
			if ((au8_at_res_buf[8] == '0') || (au8_at_res_buf[8] == '2'))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_PING_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_PING_FAIL);
			}
		}		
		// Check URC: +QIURC: 
		// <CR><LF>+QIURC:
		else if(b_compare_at_command("+QPING", au8_at_res_buf))
		{
			/*
			URC of Connection Closed
			When TCP socket service is closed by remote peer or network error, this report will be outputted. The
			<socket_state> of <connectID> will be “closing? Host must execute AT+QICLOSE=<connectID> to
			change the <socket_state> to “initial?
			*/
			//+QIURC: “closed?<connectID>
			if(b_compare_at_command("close", (au8_at_res_buf + 9)))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_SOCKET_CLOSED);
				//TODO request to execute AT+QICLOSE=<connectID> to
				//change the <socket_state> to “initial?
			}
			/*
			URC of Incoming Data
			In buffer access mode: +QIURC: “recv?<connectID>
			In direct push mode: +QIURC: “recv?<connectID>,<currentrecvlength><CR><LF><data>
			*/
			else if(b_compare_at_command("recv", (au8_at_res_buf + 9)))
			{
				uint16_t u16_tmp_data_len = 0;
				char* ptr;
				//Get ID of connection
				ustrtoul((const char*)(au8_at_res_buf + 15), (const char**)&ptr, 10);
				u16_tmp_data_len = ustrtoul(ptr + 1, (const char**)&ptr, 10);
				if(u16_tmp_data_len > 0)
				{
					UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
					//Do a while loop to get all of incoming data len
					uint8_t u8_tmp_cnt = TIME_TO_WAIT_TCP_INCOMING_DATA;
					while (u8_tmp_cnt > 0)
					{
						if(UARTPeekData(au8_at_res_buf, u16_tmp_data_len))
						{
							v_add_at_resp_status_to_table(AT_RESP_STATUS_RECEIVED_DATA);
							//TODO: process incoming TCP data
							break;
						}
						vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
						u8_tmp_cnt--;
					}
				}
				else
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_INCOMING_DATA);
				}
			}
			/*
			PDP context may be deactivated by the network. The module will report a URC to the host. Host must
			execute AT+QIDEACT to deactivate the context and reset all connections.
			*/
			//+QIURC: “pdpdeact?<contextID>
			if(b_compare_at_command("pdpdeact", (au8_at_res_buf + 9)))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_PDP_DEACTIVE);
				//TODO request to execute AT+QIDEACT to deactivate the context and reset all connections.
			}			
		}
		// Check AT respone AT+QMTOPEN open a network for MQTT client
		/* <CR><LF>+QMTOPEN: <client_idx>,<result><CR><LF>
																			-1 Failed to open network
																			0 Network opened successfully
																			1 Wrong parameter
																			2 MQTT identifier is occupied
																			3 Failed to activate PDP
																			4 Failed to parse domain name
																			5 Network connection error
		*/
		else if(b_compare_at_command("+QMTOPEN", au8_at_res_buf))
		{
			if (au8_at_res_buf[12] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_OPEN_NETWORK_MQTT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_OPEN_NETWORK_MQTT_FAIL);
			}
		}
		// Check AT respone AT+QMTCLOSE close a network for MQTT client
		/* <CR><LF>+QMTCLOSE: <client_idx>,<result><CR><LF>
												-1 Failed to close network
												0 Network closed successfully
		*/
		else if(b_compare_at_command("+QMTCLOSE", au8_at_res_buf))
		{
			if (au8_at_res_buf[12] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CLOSE_NETWORK_MQTT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_CLOSE_NETWORK_MQTT_FAIL);
			}
		}
		// Check AT respone AT+QMTCONN Connect a Client to MQTT Server
		/* <CR><LF>+QMTCONN: <client_idx>,<result>[,<ret_code>]<CR><LF>
						<result> Integer type. Result of the command execution
						0 Packet sent successfully and ACK received from server
						1 Packet retransmission
						2 Failed to send packet
						<ret_code> Integer type. Connection status return code
						0 Connection Accepted
						1 Connection Refused: Unacceptable Protocol Version
						2 Connection Refused: Identifier Rejected
						3 Connection Refused: Server Unavailable
						4 Connection Refused: Bad User Name or Password
						5 Connection Refused: Not Authorized
			<CR><LF>+QMTCONN: <client_idx>,<state><CR><LF>
						<state> Integer type. MQTT connection state
						1 MQTT is initializing
						2 MQTT is being connected
						3 MQTT is connected
						4 MQTT is being disconnected
		*/
		else if(b_compare_at_command("+QMTCONN", au8_at_res_buf))
		{
			if (e_gsm_active_cmd_get() == GSM_CMD_MQTT_CONNECT)
			{
				if ((au8_at_res_buf[12] == '0') && (au8_at_res_buf[14] == '0'))
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_CONNECT_OK);
				}
				else
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_CONNECT_FAIL);
				}
			}
			else if(e_gsm_active_cmd_get() == GSM_CMD_QUERY_MQTT_CONNECTION)
			{
				if (au8_at_res_buf[12] == '3')
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_CONNECT_OK);
				}
				else
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_CONNECT_FAIL);
				}				
			}
		}
		// Check AT respone AT+QMTDISC Disconnect a Client from MQTT Server
		/* <CR><LF>+QMTDISC: <client_idx>,<result><CR><LF>
				<result> Integer type. Result of the command execution
				-1 Failed to close connection
				0 Connection closed successfully
		*/
		else if(b_compare_at_command("+QMTDISC", au8_at_res_buf))
		{
			if (au8_at_res_buf[12] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_DISCONNECT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_DISCONNECT_FAIL);
			}
		}
		// Check AT respone AT+QMTSUB Subscribe to Topic
		/* <CR><LF>+QMTSUB: <client_idx>,<msgID>,<result>[,<value>]<CR><LF>
					<result> Integer type. Result of the command execution
							0 Sent packet successfully and received ACK from server
							1 Packet retransmission
							2 Failed to send packet
					<value> Integer type.
							If <result> is 0, it is a vector of granted QoS levels.
							If <result> is 1, it means the times of packet retransmission.
							If <result> is 2, it will not be presented.
		*/
		else if(b_compare_at_command("+QMTSUB", au8_at_res_buf))
		{
			uint8_t u8_cnt_comma = 0;
			uint16_t u16_i = 9;
			for (u16_i = 9; u16_i <  AT_RESPONE_BUFFER_SIZE; u16_i++)
			{
				if (au8_at_res_buf[u16_i] == ',')
				{
					u8_cnt_comma++;
				}
				if (u8_cnt_comma == 2)
				{
					if (au8_at_res_buf[u16_i + 1] == '0')
					{
						v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_SUBSCRIBE_OK);
						break;
					}
					else if (au8_at_res_buf[u16_i + 1] == '2')
					{
						v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_SUBSCRIBE_FAIL);
						break;
					}
				}
			}
			if (u16_i == AT_RESPONE_BUFFER_SIZE)
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_SUBSCRIBE_FAIL);
			}			
		}
		// Check AT respone AT+QMTUNS Unsubscribe to Topic
		/* <CR><LF>+QMTUNS: <client_idx>,<msgID>,<result>[,<value>]<CR><LF>
					<result> Integer type. Result of the command execution
							0 Sent packet successfully and received ACK from server
							1 Packet retransmission
							2 Failed to send packet
					<value> Integer type.
							If <result> is 0, it is a vector of granted QoS levels.
							If <result> is 1, it means the times of packet retransmission.
							If <result> is 2, it will not be presented.
		*/
		else if(b_compare_at_command("+QMTUNS", au8_at_res_buf))
		{
			uint8_t u8_cnt_comma = 0;
			uint16_t u16_i = 9;
			for (u16_i = 9; u16_i <  AT_RESPONE_BUFFER_SIZE; u16_i++)
			{
				if (au8_at_res_buf[u16_i] == ',')
				{
					u8_cnt_comma++;
				}
				if (u8_cnt_comma == 2)
				{
					if (au8_at_res_buf[u16_i + 1] == '0')
					{
						v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_UNSUBSCRIBE_OK);
						break;
					}
					else if (au8_at_res_buf[u16_i + 1] == '2')
					{
						v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_UNSUBSCRIBE_FAIL);
						break;
					}
				}
			}
			if (u16_i == AT_RESPONE_BUFFER_SIZE)
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_UNSUBSCRIBE_FAIL);
			}						
		}
		// Check AT respone AT+QMTPUB Publish Messages
		/* <CR><LF>+QMTPUB: <client_idx>,<msgID>,<result>[,<value>]<CR><LF>
						<result> Integer type. Result of the command execution
							0 Packet sent successfully and ACK received from server (message that
								published when <qos>=0 does not require ACK)
							1 Packet retransmission
							2 Failed to send packet
						<value> Integer type.
							If <result> is 1, it means the times of packet retransmission.
							If <result> is 0 or 2, it will not be presented.
		*/
		else if(b_compare_at_command("+QMTPUB", au8_at_res_buf))
		{
			uint8_t u8_cnt_comma = 0;
			uint16_t u16_i = 9;
			for (u16_i = 9; u16_i <  AT_RESPONE_BUFFER_SIZE; u16_i++)
			{
				if (au8_at_res_buf[u16_i] == ',')
				{
					u8_cnt_comma++;
				}
				if (u8_cnt_comma == 2)
				{
					if (au8_at_res_buf[u16_i + 1] == '0')
					{
						v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_OK);
						break;
					}
					else if (au8_at_res_buf[u16_i + 1] == '2')
					{
						v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_FAIL);
						break;
					}
				}
			}
			if (u16_i == AT_RESPONE_BUFFER_SIZE)
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_FAIL);
			}
		}
		// Check MQTT URCs: +QMTRECV:
		// <CR><LF>+QMTRECV:
		else if(b_compare_at_command("+QMTRECV", au8_at_res_buf))
		{
#ifdef	_DEBUG_GSM_CMD			
			u32_cnt_mqtt_revc++;
#endif			
			/*
			The URC begins with ?QMTRECV:? It is mainly used to notify the host to read
			the received MQTT packet data that is reported from MQTT server.
			
			+QMTRECV: <client_idx>,<msgID>,<topic>,<payload_len>,?payload>?
			
			+QMTRECV: <client_idx>,<recv_id>
			*/
			if(i32_at_respone_data_len < MQTT_URC_QMTRECV)
			{
				char* ptr;
				//Get client ID
				ustrtoul((const char*)(au8_at_res_buf + 12), (const char**)&ptr, 10);
				//Get recv ID
				ustrtoul(ptr + 1, (const char**)&ptr, 10);
				//TODO: Reported when the message that received from MQTT server
				//has been stored in buffer.
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_INCOMING_DATA);
			}
			else
			{
				v_gsm_parse_mqtt_mess(au8_at_res_buf);
			}
		}
		// Check AT respone AT+QFTPOPEN Login to FTP Server
		/* <CR><LF>+QFTPOPEN: <err>,<protocol_error><CR><LF>
						<err> Integer type, indicates the operation error code. 
		It is the type of error (Please refer to the Chapter 4 FTP_AT_Commands_Manual)
						<protocol_error> Integer type, for reference only, indicates the original 
		error code from FTP server which is defined in FTP protocol (Please refer to 
		Chapter 5 FTP_AT_Commands_Manual). If it is 0, it is meaningless.
		*/
		else if(b_compare_at_command("+QFTPOPEN", au8_at_res_buf))
		{
			if ((au8_at_res_buf[11] == '0') && (au8_at_res_buf[13] == '0'))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_OPEN_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_OPEN_FAIL);
			}
		}
		// Check AT respone AT+QFTPCWD Set the Current Directory on FTP Server
		/* <CR><LF>+QFTPCWD: <err>,<protocol_error><CR><LF>
						<err> Integer type, indicates the operation error code. 
		It is the type of error (Please refer to the Chapter 4 FTP_AT_Commands_Manual)
						<protocol_error> Integer type, for reference only, indicates the original 
		error code from FTP server which is defined in FTP protocol (Please refer to 
		Chapter 5 FTP_AT_Commands_Manual). If it is 0, it is meaningless.
		*/
		else if(b_compare_at_command("+QFTPCWD:", au8_at_res_buf))
		{
			if ((au8_at_res_buf[10] == '0') && (au8_at_res_buf[12] == '0'))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_SET_CURR_DIR_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_SET_CURR_DIR_FAIL);
			}
		}	
		/* Check AT response AT+QFTPGET Download file from FTP server
		* +QFTPGET: 0,<transferlen>
		*/
		else if(b_compare_at_command("+QFTPGET", au8_at_res_buf))
		{
			if(au8_at_res_buf[10] == '0')
			{
				uint32_t u32_file_len = atoi((const char *)(au8_at_res_buf + strlen("+QFTPGET: 0,")));
				v_download_file_len_set(u32_file_len);
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL);
			}
		}
		// Check AT respone AT+QFTPCLOSE Logout the FTP server
		/* <CR><LF>+QFTPCLOSE: <err>,<protocol_error><CR><LF>
						<err> Integer type, indicates the operation error code. 
		It is the type of error (Please refer to the Chapter 4 FTP_AT_Commands_Manual)
						<protocol_error> Integer type, for reference only, indicates the original 
		error code from FTP server which is defined in FTP protocol (Please refer to 
		Chapter 5 FTP_AT_Commands_Manual). If it is 0, it is meaningless.
		*/
		else if(b_compare_at_command("+QFTPCLOSE:", au8_at_res_buf))
		{
			if ((au8_at_res_buf[12] == '0') && (au8_at_res_buf[14] == '0'))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_LOGOUT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_LOGOUT_FAIL);
			}
		}		
		/* Get the file handle by executing the ¡°AT+QFOPEN¡± which is used in other commands, such as
		¡°AT+QFWRITE¡±, ¡°AT+QFREAD¡±, ¡°AT+QFSEEK¡±, ¡°AT+QFCLOSE¡±, ¡°AT+QFPOSITION¡±, ¡°AT+QFFLUSH¡±
		and ¡°AT+QFTUCAT¡±. 
		Response
		+QFOPEN: <filehandle>
		OK
		+CME ERROR: <errcode>
		<filename> The file to be operated. The max length is 80 bytes.
		¡°<filename>¡± The operated file in the UFS.
		¡°RAM: <filename>¡± The operated file in the RAM.
		<filehandle> The handle of the file. The data type is 4 bytes.
		<mode> The open mode of the file. Default is 0.
		*/
		else if(b_compare_at_command("+QFOPEN", au8_at_res_buf))
		{
			uint8_t u8_handle_len = strlen((const char *)au8_at_res_buf) - strlen("+QFOPEN: ");
			char *pc_handle;
			pc_handle = (char *)calloc(u8_handle_len + 1, sizeof(uint8_t));
			memcpy(pc_handle, au8_at_res_buf + strlen("+QFOPEN: "), u8_handle_len * sizeof(uint8_t));
			v_gsm_file_handle_set(pc_handle);
			free(pc_handle);
			v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_OPEN_OK);
		}
		else if(b_compare_at_command("CONNECT", au8_at_res_buf))
		{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_CONNECT_OK);
			uint16_t u16_data_len = atoi((const char *)(au8_at_res_buf + strlen("CONNECT ")));
			int16_t u16_tmp_cnt = FTP_READ_PACKAGE_TIMEOUT * 100;
			while(u16_tmp_cnt > 0)
			{
				if(UARTPeekData(au8_at_res_buf, u16_data_len))
				{
					UARTFlushRx();
					i32_at_respone_data_len = 0;	
					u16_tmp_cnt = 0;
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_OK);
					v_download_data_append(au8_at_res_buf, u16_data_len);
					break;
				}
				vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
				u16_tmp_cnt--;
				if (u16_tmp_cnt == 0)
				{
					UARTFlushRx();
					i32_at_respone_data_len = 0;							
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_FAIL);		
				}
			}
		}
		/* QMTSTAT
		* +QMTSTAT: <client_idx>,<err_code> 
		*/
		else if(b_compare_at_command("+QMTSTAT", au8_at_res_buf))
		{
			if('1' == au8_at_res_buf[12])
			{
				v_gsm_mqtt_network_close(true);
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_NETWORK_CLOSE);
			}
		}
		else if(b_compare_at_command("MCON:", au8_at_res_buf))
		{
			if('1' == au8_at_res_buf[strlen("MCON:")])
			{
				// coding... mqtt connect = true
				v_connection_set(true);
				v_gsm_hal_set_sim_rssi(au8_at_res_buf[strlen("MCON:") + 2]);
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_OK);
			}
			else
			{
				// coding... mqtt connect = fail
				v_connection_set(false);
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_FAIL);
			}
		}
		else if(b_compare_at_command("MPUT:", au8_at_res_buf))
		{
			if('1' == au8_at_res_buf[strlen("MPUT:")])
			{
				// coding... pub success
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_OK);
			}
			else
			{
				// coding... pub fail
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_FAIL);
			}
		}
		else if(b_compare_at_command("MMES:", au8_at_res_buf))
		{
			// coding... add message to q_mqtt_pub_from_server
			memset(&stru_mqtt, 0, sizeof(mqtt_t));
			stru_mqtt.u8_payload_len = i32_at_respone_data_len - strlen("MMES:") - 2;
			//stru_mqtt.u8_payload_len = len_buf - len_key;
			memcpy(stru_mqtt.au8_payload, au8_at_res_buf + strlen("MESS:"), stru_mqtt.u8_payload_len);
			b_write_mqtt_pub_from_server(&stru_mqtt);
			if (stru_mqtt.au8_payload[0] == 0x90)
			{
				int a = 0;
				a++;
				UARTFlushRx();
					i32_at_respone_data_len = 0;
			}
			if(i32_at_respone_data_len > 0)
			{
				UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
			}
		}
		else if(b_compare_at_command("FDOW:", au8_at_res_buf))
		{
			switch(au8_at_res_buf[strlen("FDOW:")])
			{
				case '1':
				{
					// coding... for download success
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK);
					uint32_t u16_data_len = atoi((const char *)(au8_at_res_buf + strlen("FDOW:1,")));
					v_download_file_len_set(u16_data_len);
				}
				break;
				case '2':
				{
					// coding... for connect FTP sever fail
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL);
				}
				break;
				case '3':
				{
					// coding... for check CRC32 file fail
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL);
				}
				break;
				case '4':
				{
					// coding... for used file fail
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_FAIL);
				}
				break;
				default: break;
			}
		}
		else if(b_compare_at_command("FSHA:", au8_at_res_buf))
		{
			// coding... for receiving file			
			uint16_t u16_packet_size = (au8_at_res_buf[5]<<8)|au8_at_res_buf[6];
			int32_t i16_tmp_cnt = FTP_READ_PACKAGE_TIMEOUT * 100;		
			while(i16_tmp_cnt > 0)
			{
				if(UARTPeekData(au8_at_res_buf, u16_packet_size+9))
				{ 
					i16_tmp_cnt = 0;									
					v_download_data_append(au8_at_res_buf+7, u16_packet_size);
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_OK);
					break;
				}
				vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
				i16_tmp_cnt--;
				if (i16_tmp_cnt == 0)
				{							
					UARTFlushRx();
					i32_at_respone_data_len = 0;
					v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_FAIL);		
				}
			}				
			UARTFlushRx();
					i32_at_respone_data_len = 0;
		}
		/*--------- 4G ---------*/
		else if (b_compare_at_command("+SIMEI:", au8_at_res_buf))
		{
			uint8_t u8_handle_len = strlen((const char *)au8_at_res_buf) - strlen("+SIMEI: ");
			char *pc_handle;
			pc_handle = (char *)calloc(u8_handle_len + 1, sizeof(uint8_t));
			memcpy(pc_handle, au8_at_res_buf + strlen("+SIMEI: ") + 8, u8_handle_len * sizeof(uint8_t)); //bo 8 ky tu dau
			v_gsm_imei_set(pc_handle);
			free(pc_handle);
			v_add_at_resp_status_to_table(AT_RESP_STATUS_IMEI_OK);
		}
		// Check AT respone for UE system information
		else if(b_compare_at_command("+CPSI:", au8_at_res_buf))
		{
			if (b_compare_at_command("NO SERVICE", au8_at_res_buf))
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_SYSTEM_NOSERVICE);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_SYSTEM_OK);
			}
		}
		else if (b_compare_at_command("+CGACT:", au8_at_res_buf))
		{
			if(b_is_activating(au8_at_res_buf[10]))	//Check if return id value is in pdp context table
			{
				if(au8_at_res_buf[8] == '1')	//check if pdp type is ipv4
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_PDP_ACTIVE);					
				}
				else
				{
					v_add_at_resp_status_to_table(AT_RESP_STATUS_PDP_DEACTIVE);
				}
			}
		}
		else if(b_compare_at_command("+CMQTTSTART", au8_at_res_buf))
		{
			if (au8_at_res_buf[13] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_START_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_START_FAIL);
			}			
		}
		else if(b_compare_at_command("+CMQTTCONNECT", au8_at_res_buf))
		{
			if (au8_at_res_buf[17] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_CONNECT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_CONNECT_FAIL);
			}			
		}
		else if(b_compare_at_command("+CMQTTSUB", au8_at_res_buf))
		{
			if (au8_at_res_buf[13] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_SUBSCRIBE_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_SUBSCRIBE_FAIL);				
			}
		}
		else if(b_compare_at_command("+CMQTTUNSUB", au8_at_res_buf))
		{
			if (au8_at_res_buf[15] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_UNSUBSCRIBE_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_UNSUBSCRIBE_FAIL);				
			}
		}
		else if (b_compare_at_command("+CMQTTRXPAYLOAD", au8_at_res_buf))
		{	
			char *index;
			index = strstr((const char *)au8_at_res_buf, ",");
			u16_payload_len = atoi(index + 1);
			if (u16_payload_len < MQTT_PAYLOAD_LEN_MAX)
			{
				b_get_payload = true;
			}
			if(i32_at_respone_data_len > 0)
			{
				UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
			}
		}
		else if (b_get_payload)
		{
			b_get_payload = false;				
			
			memset(&stru_mqtt, 0, sizeof(mqtt_t));
			stru_mqtt.u8_payload_len = u16_payload_len;
			memcpy(stru_mqtt.au8_payload, au8_at_res_buf, stru_mqtt.u8_payload_len);
			b_write_mqtt_pub_from_server(&stru_mqtt);
			
			UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
		}
		else if (b_compare_at_command("+CMQTTPUB", au8_at_res_buf))
		{
			if (au8_at_res_buf[13] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_PUBLISH_FAIL);				
			}
		}
		else if (b_compare_at_command("+CMQTTCONNLOST", au8_at_res_buf))
		{
			//todo: restart mqtt
			v_gsm_mqtt_network_close(true);
			v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_NETWORK_CLOSE);
		}
		else if (b_compare_at_command("+CMQTTDISC", au8_at_res_buf))
		{
			if (au8_at_res_buf[14] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_DISCONNECT_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_DISCONNECT_FAIL);
			}
		}
		else if (b_compare_at_command("+CFUN", au8_at_res_buf))
		{
			if (au8_at_res_buf[7] == '1')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_CHECK_FUN_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_CHECK_FUN_FAIL);				
			}
		}
		else if (b_compare_at_command("+CFTPSSTART", au8_at_res_buf))
		{
			if (au8_at_res_buf[13] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_START_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_START_FAIL);				
			}
		}
		else if (b_compare_at_command("+CFTPSSTOP", au8_at_res_buf))
		{
			if (au8_at_res_buf[12] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_STOP_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_STOP_FAIL);				
			}
		}
		else if (b_compare_at_command("+CFTPSLOGIN", au8_at_res_buf))
		{
			if (au8_at_res_buf[13] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_OPEN_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_OPEN_FAIL);				
			}
		}
		else if (b_compare_at_command("+CFTPSTYPE", au8_at_res_buf))
		{
			if (au8_at_res_buf[12] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_SET_TYPE_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_SET_TYPE_FAIL);				
			}
		}
		else if (b_compare_at_command("+CFTPSCWD", au8_at_res_buf))
		{
			if (au8_at_res_buf[11] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_SET_CURR_DIR_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_SET_CURR_DIR_FAIL);				
			}
		}
		else if (b_compare_at_command("+CFTPSSIZE", au8_at_res_buf))
		{
			uint32_t u32_file_len = 0;
			u32_file_len = atoi((const char *)au8_at_res_buf + strlen("+CFTPSSIZE:"));			
			if ((u32_file_len > 17) && (u32_file_len != 421))
			{
				v_download_file_len_set(u32_file_len);
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_GET_SIZE_OK);
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_GET_SIZE_FAIL);
			}
		}
		else if (b_compare_at_command("+CFTPSGETFILE: ", au8_at_res_buf))
		{
			if (au8_at_res_buf[15] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_DOWNLOAD_TO_RAM_OK);						
			}
		}
		else if (b_compare_at_command("+CFTRANTX", au8_at_res_buf))
		{
			if (b_compare_at_command("+CFTRANTX: DATA", au8_at_res_buf))
			{
				u16_packet_size = atoi((const char *)au8_at_res_buf + strlen("+CFTPSGET: DATA,"));	
				b_get_file = true;				
				UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
				
				int16_t i16_tmp_cnt = FTP_READ_PACKAGE_TIMEOUT * 100;		
				while(i16_tmp_cnt > 0)
				{
					if(UARTPeekData(au8_at_res_buf, u16_packet_size))
					{ 
						UARTFlushRx();
						i32_at_respone_data_len = 0;	
						i16_tmp_cnt = 0;														
						//UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
						v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_OK);
						v_download_data_append(au8_at_res_buf, u16_packet_size);
						break;
					}
					vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
					i16_tmp_cnt--;
					if (i16_tmp_cnt == 0)
					{							
						UARTFlushRx();
						i32_at_respone_data_len = 0;	
						//UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
						v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_FAIL);		
					}
				}
			}
			else if (au8_at_res_buf[11] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FILE_READ_OK);						
			}
			if (b_get_file)
			{
				b_get_file = false;
			}
		}
		else if (b_compare_at_command("+CFTPSLOGOUT", au8_at_res_buf))
		{
			if (au8_at_res_buf[14] == '0')
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_LOGOUT_OK);						
			}
			else
			{
				v_add_at_resp_status_to_table(AT_RESP_STATUS_FTP_LOGOUT_FAIL);				
			}
		}
		else
		{
#ifdef	_DEBUG_GSM_CMD			
			u32_cnt_at_resp_fail++;
#endif			
			UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
		}
	}
}

/*!
*	@fn b_get_at_resp_status_to_table (at_resp_status_t e_resp_status)
* @brief get AT respone in table
* @param[in] at_resp_status_t e_resp_status
* @param[out] none
* @return \b true if we found e_resp_status in table
*/
bool b_get_at_resp_status_from_table (at_resp_status_t e_resp_status)
{
	if(e_resp_status != AT_RESP_STATUS_NONE)
	{
		uint8_t u8_count = 0;
		for(u8_count = 0; u8_count < MAX_AT_RESP_STATUS_TABLE; u8_count++)
		{
			if( ae_at_resp_status_table[u8_count] == e_resp_status)
			{
				return true;	
			}
		}
	}
	return false;
}

/*!
* @fn b_compare_at_command(const char* c_command, uint8_t *pu8_data)
* @brief Compare a command with recieved data
* @param[in] c_command Comand need to compare
* @param[in] *pu8_data Recieved data
* @return True: command matchs with recieved data
*/
static bool b_compare_at_command(const char* c_command, uint8_t *pu8_data)
{
	bool b_result = false;
	uint16_t u16_cmd_len = strlen(c_command);
	char *c_respond_data;
	c_respond_data = (char *)calloc(u16_cmd_len + 2, sizeof(uint8_t));
	strncpy(c_respond_data, (const char *)pu8_data, u16_cmd_len);
	if(strcasecmp(c_respond_data, c_command) == 0)
	{
		b_result = true;
	}
	free(c_respond_data);
	return b_result;
}


/*!
*	@fn v_add_at_resp_status_to_table (at_resp_status_t e_resp_status)
* @brief Add AT respone status to table
* @param[in] at_resp_status_t e_resp_status
* @param[out] none
* @return \b true if have room available for the new AT respone status
*/
static void v_add_at_resp_status_to_table (at_resp_status_t e_resp_status)
{
	ae_at_resp_status_table[u8_at_resp_status_table_write_index] = e_resp_status;
	u8_at_resp_status_table_write_index = (u8_at_resp_status_table_write_index + 1)
																				% MAX_AT_RESP_STATUS_TABLE;
	if(i32_at_respone_data_len > 0)
	{
		UARTFlushRxAtDeltaIndex(i32_at_respone_data_len);
	}
}

/*!
*	@fn v_delete_all_resp_status_on_table (void)
* @brief Delete all AT_Resp in table.
* @param[in] none
* @param[out] none
* @return none
*/
void v_delete_all_resp_status_on_table (void)
{
	uint8_t u8_count = 0;
	for(u8_count = 0; u8_count < MAX_AT_RESP_STATUS_TABLE; u8_count++)
	{
		ae_at_resp_status_table[u8_count] = AT_RESP_STATUS_NONE;
	}	
}

static void v_gsm_parse_mqtt_mess(uint8_t *pu8_data)
{
	//+QMTRECV: <client_idx>,<msgID>,<topic>,<payload_len>,?payload>?
	memset(&stru_mqtt, 0, sizeof(mqtt_t));
	char* ptr;
	stru_mqtt.u8_client_id = ustrtoul((const char*)(pu8_data + 9),
																											(const char**)&ptr, 10);
	stru_mqtt.u16_msg_id = ustrtoul((const char*)(ptr + 1),
																							(const char**)&ptr, 10);
	ptr += ustrnlcpy(stru_mqtt.au8_topic, ptr + 2, MQTT_TOPIC_LEN_MAX);
	ptr += 3;
	stru_mqtt.u8_payload_len = ustrtoul((const char*)(ptr + 1),
																							(const char**)&ptr, 10);
	if(stru_mqtt.u8_payload_len > 0)
	{
		UARTFlushRxAtDeltaIndex((uint32_t)(ptr - (char*)pu8_data));
		i32_at_respone_data_len = 0;
		//Do a while loop to get all of incoming data len
		uint16_t u16_tmp_cnt = MQTT_PKT_TIMEOUT * 100;
		while (u16_tmp_cnt > 0)
		{
			if(UARTPeekData(pu8_data, stru_mqtt.u8_payload_len + 4))
			{
				i32_at_respone_data_len = 0;
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_RECV_DATA_OK);
				//TODO: process incoming MQTT data
				memcpy(stru_mqtt.au8_payload, pu8_data + 2, stru_mqtt.u8_payload_len);
#ifdef	_DEBUG_GSM_CMD							
				u32_cnt_mqtt_revc_ok++;
#endif							
				b_write_mqtt_pub_from_server(&stru_mqtt);
				break;
			}
			vTaskDelay(INTERVAL_TIME_TO_CHECK_AT_RESP);
			u16_tmp_cnt--;
			if (u16_tmp_cnt == 0)
			{
				UARTFlushRx();
				i32_at_respone_data_len = 0;							
				v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_RECV_DATA_FAIL);		
				break;
			}
		}
	}
	else
	{
			v_add_at_resp_status_to_table(AT_RESP_STATUS_MQTT_RECV_DATA_FAIL);							
	}
}
