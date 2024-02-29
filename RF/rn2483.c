/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
/**
 * @file rf.c
 * @author Linh Nguyen (editor)
 * @date 27 Nov 2020
 * @version: draft 1.0
 * @brief Manage functionalities of RN2483.
 */
/*!
 * Add more include here
 */
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "driverlib.h"
#include "config.h"
#include "rf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/*!
* Variables declare
*/
RN2483_REQUEST e_curr_request = RN2483_NUM_REQUEST;
RN2483_RESP_STATUS e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
static char arr[15];
RN2483_RADIO_STATE e_rn2483_state = RN2483_NUM_STATE;
uint8_t au8_uart_data[RF_BUF_MAX];
uint8_t u8_uart_data_count = 0;
uint8_t au8_rn2483_data[RF_BUF_MAX];
uint8_t u8_rn2483_data_len = 0;
uint8_t u8_rn2483_snr = 128;
/*!
* Function prototype
*/
static void v_rn2483_uart_isr(void);
static void v_rn2483_store_data (uint8_t u8_data_len);
static void v_rn2483_store_snr (uint8_t u8_data_len);
static void v_lora_rn2483_send_data_to_queue(void);
uint8_t ui8_ascii_to_hex(uint8_t, uint8_t);
 
/*!
* Public functions
*/

/*!
* @fn rn2483_init(void)
* @brief Init RF module
* @param[in] None
* @return None
*/
bool rn2483_init(void)
{	
	ROM_UARTIntEnable(RF_UART_BASE, UART_INT_RX|UART_INT_RT);
	UARTIntRegister(RF_UART_BASE, &v_rn2483_uart_isr);
	ROM_IntEnable(RF_UART_INT);
		
	b_rn2483_mac_pause();
	b_rn2483_mac_pause();
#ifdef RF868MHz
	b_rn2483_radio_set_freq(RN2483_RADIO_FREQ_868);
#endif
#ifdef RF433MHz
	b_rn2483_radio_set_freq(RN2483_RADIO_FREQ_433);
#endif
	b_rn2483_radio_set_crc(RN2483_RADIO_CRC_MODE_ON);
	b_rn2483_radio_set_wdt(0);
	b_rn2483_radio_rx(0);
	
	return true;
}

/*!
* @fn v_rn2483_process_data (void)
* @brief Process module state 
* @param[in] None
* @return None
*/
void v_rn2483_process_data (void)
{
	switch(e_rn2483_get_state())
	{
		case RN2483_RADIO_IN_RX:
		{
			if (b_rn2483_check_radio_err() == true)
			{
				b_rn2483_radio_rx(0);
				v_rn2483_set_state(RN2483_RADIO_IN_RX);
			}
		}
		break;			
		case RN2483_RADIO_RX_DONE:
		{
			b_rn2483_radio_get_snr();
		}
		break;
		case RN2483_RADIO_CAL_SNR_DONE:
		{
			b_rn2483_radio_rx(0);
			v_rn2483_set_state(RN2483_RADIO_IN_RX);
		}
		break;
		default: break;
	}	
}

/*!
* @fn *pc_num_to_string(uint32_t num, uint8_t dec, bool pos)
* @brief Convert number to digits, store them in a string
* @param[in]u32_num Number convert
* @param[in] dec Number of decimal digits
* @param[in] pos u32_num sign, If positive, pos = true.  
* @return None
*/
char* pc_num_to_string(uint32_t u32_num, uint8_t dec, bool pos)
{
	char s[10];
	int i = 0, j;
	char c;
	if(u32_num > 9)
	{
		do
		{
			s[i++]= u32_num%10 + '0';
			u32_num = u32_num /10;
			if(dec > 0)
			{
				dec--;
				if(dec == 0)
					s[i++]= '.';
			}
			
		} 
		while((u32_num/10) > 0);

		// Convert to ascii num 
		s[i++]= u32_num + '0';

		if(!pos)
		{
			s[i++]= '-';
		}
		s[i]= '\0';
	}
	else
	{
		s[0] = u32_num + '0';
		s[1] = '\0';
	}
	//reverse
	for(i = 0, j = strlen(s) -1; i<j; i++, j--)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
	for(i = 0; i < 10; i++)
	{
		arr[i] = s[i];
	}
	return arr;
}

/*!
* @fn rn2483_uart_put(char* tx_buf, uint8_t u8_num_to_write)
* @brief Write transmit data buffer to hardware uart of RF module 
* @param[in]tx_buf Buffer 
* @param[in] u8_num_to_write Size of buffer
* @return None
*/
void rn2483_uart_put(char* tx_buf, uint8_t u8_num_to_write)
{
	for (uint8_t i = 0; i < u8_num_to_write; i++)
	{
		UARTCharPut(RF_UART_BASE, *(tx_buf + i));
	}
}

/*!
* @fn rn2483_uart_put_string(char* txBuf)
* @brief Put data string to tx buffer
* @param[in] txBuf Buffer
* @return None
*/
void rn2483_uart_put_string(char* txBuf)
{
	rn2483_uart_put(txBuf,strlen((const char*)txBuf));
}

/*!
* @fn e_rn2483_check_resp (uint8_t* p_data, uint8_t* ui8_data_len)
* @brief Get response from module and classify that response for which request 
* @param[in] p_data Response data
* @param[in] ui8_data_len Response data length
* @return None
*/
void e_rn2483_check_resp (uint8_t* p_data, uint8_t* ui8_data_len)
{
	/*
	* resp<0x0D><0x0A>
	*/
	//TODO: find CR LF
	uint8_t i = 0;
	uint8_t j = 0;
	for(i = 0; i < *ui8_data_len; i++)
	{
		if((p_data[i] == 0x0D) && (p_data[i+1] == 0x0A))	//end with <CR><LF>
		{
			p_data[i] = '\0';
			if(strcmp((const char*)(p_data), (const char*)RN2483_RESP_OK) == 0)
			{		
				switch(e_curr_request)
				{
					case RN2483_RADIO_SET_MODE:
					{
						e_rn2483_resp_status = RN2483_RADIO_SET_MODE_OK;
					}
					break;
					case RN2483_RADIO_SET_FREQ:
					{
						e_rn2483_resp_status = RN2483_RADIO_SET_FREQ_OK;
					}
					break;			
					case RN2483_RADIO_SET_PWR:
					{
						e_rn2483_resp_status = RN2483_RADIO_SET_PWR_OK;
					}
					break;
					case RN2483_RADIO_SET_CRC:
					{
						e_rn2483_resp_status = RN2483_RADIO_SET_CRC_OK;
					}
					break;		
					case RN2483_RADIO_SET_WDT:
					{
						e_rn2483_resp_status = RN2483_RADIO_SET_WDT_OK;
					}
					break;	
					case RN2483_RADIO_START_RX:
					{
						e_rn2483_resp_status = RN2483_RADIO_START_RX_OK;
					}
					break;		
					case RN2483_RADIO_START_TX:
					{
						e_rn2483_resp_status = RN2483_RADIO_START_TX_OK;
					}
					break;					
					default: 
					{
						e_rn2483_resp_status = RN2483_INVALID_PARAM;
					}
					break;
				}				
			}			
			else if(strcmp((const char*)(p_data), (const char*)RN2483_RESP_INVALID_PARAM) == 0)
			{		
				e_rn2483_resp_status = RN2483_INVALID_PARAM;						
			}
			else if(strcmp((const char*)(p_data), (const char*)RN2483_RESP_BUSY) == 0)
			{		
				e_rn2483_resp_status = RN2483_RADIO_BUSY;						
			}
			//rx
			else if((p_data[0] == 'r') && (p_data[1] == 'a') && (p_data[2] == 'd') && (p_data[3] == 'i') &&
							(p_data[4] == 'o') && (p_data[5] == '_') && (p_data[6] == 'r') && (p_data[7] == 'x'))
			{
				//store rf frame
				//radio_rx  data<CR><LF>
				v_rn2483_store_data(i-10);
				v_rn2483_set_state(RN2483_RADIO_RX_DONE);
			}
			//tx ok
			else if(strcmp((const char*)(p_data), (const char*)RN2483_RESP_RADIO_TX_OK) == 0)
			{		
				e_rn2483_resp_status = RN2483_RADIO_TX_OK;						
			}			
			else if(strcmp((const char*)(p_data), (const char*)RN2483_RESP_RADIO_ERR) == 0)
			{		
				e_rn2483_resp_status = RN2483_RADIO_ERR;						
			}			
			else
			{
				switch(e_curr_request)
				{
					case RN2483_MAC_PAUSE:
					{
						e_rn2483_resp_status = RN2483_MAC_PAUSE_OK;
					}
					break;
					case RN2483_RADIO_GET_SNR:
					{
						e_rn2483_resp_status = RN2483_RADIO_GET_SNR_OK;
						//Calculate SNR then append it to lora frame
						//-128<CR><LF>
						v_rn2483_store_snr(i);
						//send data
						v_lora_rn2483_send_data_to_queue();
					}
					break;					
					default: 
					{			
					}
					break;
				}
			}
			//clear buffer
			i +=2;
			memset(p_data, 0, i);		
			*ui8_data_len = *ui8_data_len - i;					
			for(j = 0; j < *ui8_data_len; j++)
			{
				p_data[j] = p_data[i+j];
			}				
		}
	}
}

/*!
* @fn b_rn2483_mac_pause(void)
* @brief Pause MAC
* @param[in] None
* @return true If no error occurs
*/
bool b_rn2483_mac_pause(void)
{
	//TODO: Set e_curr_request to RN2483_MAC_PAUSE
	e_curr_request = RN2483_MAC_PAUSE;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;

	rn2483_uart_put_string("mac pause");	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_MAC_PAUSE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_MAC_PAUSE_OK)
		{
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}		
		vTaskDelay(10);
	}
	return false;
}	

/*!
* @fn b_rn2483_radio_set_mode(RN2483_RADIO_MODE e_mode)
* @brief Set RF mode LORA or FSK 
* @param[in] e_mode Enum values declared in RN2483_RADIO_MODE
* @return true If no error occurs
*/	
bool b_rn2483_radio_set_mode(RN2483_RADIO_MODE e_mode)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_SET_MODE;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	rn2483_uart_put_string("radio set mod ");	
	
	if (e_mode == RN2483_RADIO_LORA_MODE)
	{
		rn2483_uart_put_string("lora");	
	}
	else if (e_mode == RN2483_RADIO_FSK_MODE)
	{		
		rn2483_uart_put_string("fsk");	
	}	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_SET_MODE_OK)
		{
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}
		vTaskDelay(10);
	}
	return false;
}

/*!
* @fn b_rn2483_radio_set_freq(RN2483_RADIO_FREQ e_freq)
* @brief Set radio frequency 
* @param[in] e_freq Enum RN2483_RADIO_FREQ value: 433MHz or 868MHz 
* @return true If no error occurs
*/
bool b_rn2483_radio_set_freq(RN2483_RADIO_FREQ e_freq)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_SET_FREQ;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	rn2483_uart_put_string("radio set freq ");	
	
	if (e_freq == RN2483_RADIO_FREQ_433)
	{	
		rn2483_uart_put_string("434000000");			
	}
	else if (e_freq == RN2483_RADIO_FREQ_868)
	{		
		rn2483_uart_put_string("868939000");			
	}	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_SET_FREQ_OK)
		{
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}
		vTaskDelay(10);
	}
	return false;
}

/*!
* @fn b_rn2483_radio_set_pwr(int8_t i8_pwr)
* @brief Setting power of RF. The output power from -3 to 15 (max 14dbm)
* @param[in] i8_pwr Power value
* @return true If no error occurs 
*/
bool b_rn2483_radio_set_pwr(int8_t i8_pwr)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_SET_MODE;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	
	rn2483_uart_put_string("radio set pwr ");			
	
	//validation input
	if (i8_pwr < (-3))
	{
		i8_pwr = -3;
	}
	else if (i8_pwr > 14)
	{
		i8_pwr = 14;
	}
	if(i8_pwr < 0)
	{
		i8_pwr = - i8_pwr;
		rn2483_uart_put_string(pc_num_to_string((uint32_t)i8_pwr,0,0));	
	}
	else
	{
		rn2483_uart_put_string(pc_num_to_string((uint32_t)i8_pwr,0,1));	
	}
	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_SET_PWR_OK)
		{
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}
		vTaskDelay(10);
	}
	return false;
}


/*!
* @fn b_rn2483_radio_set_crc(RN2483_RADIO_CRC_MODE e_crc_mode)
* @brief Enable and disable CRC mode 
* @param[in] e_crc_mode CRC mode ON or OFF
* @return true If session is done without error
*/
bool b_rn2483_radio_set_crc(RN2483_RADIO_CRC_MODE e_crc_mode)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_SET_CRC;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	
	rn2483_uart_put_string("radio set crc ");
	
	if (e_crc_mode == RN2483_RADIO_CRC_MODE_ON)
	{	
		rn2483_uart_put_string("on");		
	}
	else if (e_crc_mode == RN2483_RADIO_CRC_MODE_OFF)
	{
		rn2483_uart_put_string("off");				
	}	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_SET_CRC_OK)
		{
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}
		vTaskDelay(10);
	}
	return false;
}


/*!
* @fn b_rn2483_radio_set_wdt(uint32_t  u32_wdt_timeoout)
* @brief Set watchdog timer for radio
* @param[in] u32_wdt_timeoout Timeout value for wdt
* @return true If set WDT OK
*/
bool b_rn2483_radio_set_wdt(uint32_t  u32_wdt_timeoout)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_SET_WDT;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	
	rn2483_uart_put_string("radio set wdt ");
	rn2483_uart_put_string(pc_num_to_string(u32_wdt_timeoout, 0 , 1));
	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_SET_WDT_OK)
		{
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}	
		vTaskDelay(10);
	}
	return false;
}


/*!
* @fn b_rn2483_radio_get_snr(void)
* @brief Get SNR of radio 
* @param[in] None
* @return true If calculating SNR is done 
*/
bool b_rn2483_radio_get_snr(void)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_GET_SNR;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	
	rn2483_uart_put_string("radio get snr");	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_GET_SNR_OK)
		{
			v_rn2483_set_state(RN2483_RADIO_CAL_SNR_DONE);
			return true;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}	
		vTaskDelay(10);
	}
	return false;
}


/*!
* @fn b_rn2483_radio_rx(uint16_t  u16_rx_window_size)
* @brief 
* @param[in] u16_rx_window_size Receive buffer size
* @return	true 	If receive without error response
* 			false 	If INVALICD_PARAM or RADIO_BUSY
*/
bool b_rn2483_radio_rx(uint16_t  u16_rx_window_size)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_START_RX;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	
	rn2483_uart_put_string("radio rx ");
	rn2483_uart_put_string(pc_num_to_string(u16_rx_window_size, 0 , 1));
	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_START_RX_OK)
		{
			break;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}
		else if(e_rn2483_resp_status == RN2483_RADIO_BUSY)
		{
			return false;
		}		
		vTaskDelay(10);
	}
	return true;
}


/*!
* @fn b_rn2483_radio_tx(char* u8_tx_buf)
* @brief Write to transmit buffer and wait for module response 
* @param[in] u8_tx_buf
* @return 	true 	If no error occurs after transmit
*			false	If INVALID_PARAM or RADIO_BUSY or RADIO_ERR 
*/
bool b_rn2483_radio_tx(char* u8_tx_buf)
{
	//TODO: Set e_curr_request to RN2483_RADIO_SET_MODE
	e_curr_request = RN2483_RADIO_START_TX;
	e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
	
	rn2483_uart_put_string("radio tx ");
	rn2483_uart_put_string(u8_tx_buf);
	
	UARTCharPut(RF_UART_BASE, 0x0D);
	UARTCharPut(RF_UART_BASE, 0x0A);

	//TODO: waiting for RN2483_RADIO_SET_MODE_OK
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_START_TX_OK)
		{
			break;
		}
		else if(e_rn2483_resp_status == RN2483_INVALID_PARAM)
		{
			return false;
		}
		else if(e_rn2483_resp_status == RN2483_RADIO_BUSY)
		{
			return false;
		}		
		vTaskDelay(10);
	}
	
	for(uint8_t i = 0; i < RN2483_REQUEST_MAC_PAUSE_TIMEOUT; i++)
	{
		if(e_rn2483_resp_status == RN2483_RADIO_TX_OK)
		{
			break;
		}
		else if(e_rn2483_resp_status == RN2483_RADIO_ERR)
		{
			return false;
		}
		vTaskDelay(10);
	}	
	
	/*return false;*/
	return true;
}


/*!
* @fn e_rn2483_get_state(void)
* @brief Get module's state
* @param[in] None
* @return e_rn2483_state State in enum E_RN2483_RADIO_STATE
*/
RN2483_RADIO_STATE e_rn2483_get_state(void)
{
	return e_rn2483_state;
}

/*!
* @fn v_rn2483_set_state(RN2483_RADIO_STATE e_state)
* @brief Set module state
* @param[in] e_state State declared in enum E_RN2483_RADIO_STATE
* @return None
*/
void v_rn2483_set_state(RN2483_RADIO_STATE e_state)
{
	e_rn2483_state = e_state;
}

/*!
* @fn e_rn2483_get_resp_status(void)
* @brief Get module's status
* @param[in] 
* @return None
*/
RN2483_RESP_STATUS e_rn2483_get_resp_status(void)
{
	return e_rn2483_resp_status;
}

/*!
* @fn b_rn2483_check_radio_err (void)
* @brief Check radio error
* @param[in] None
* @return 	true 	Radio error
* 		  	false 	No error
*/
bool b_rn2483_check_radio_err (void)
{
	if (e_rn2483_get_resp_status() == RN2483_RADIO_ERR)
	{
		e_rn2483_resp_status = RN2483_NUM_RESP_STATUS;
		return true;
	}
	return false;
}

/*!
*	Private functions
*/

/*!
* @fn v_rn2483_uart_isr(void)
* @brief UART Interrupt Service Routine of RN2483 for buffering incoming UART data
* @param[in] None
* @return None
*/
static void v_rn2483_uart_isr(void)
{
	uint32_t u32_status;
	u32_status = UARTIntStatus(RF_UART_BASE, true); //get interrupt status
	UARTIntClear(RF_UART_BASE, u32_status);
	
	v_rf_clear_higher_prio_task();
	
	while (ROM_UARTCharsAvail(RF_UART_BASE))
	{
		if(u8_uart_data_count >= RF_BUF_MAX)
		{
			u8_uart_data_count = 0;
		}
		au8_uart_data[u8_uart_data_count] = UARTCharGet(RF_UART_BASE);
		u8_uart_data_count++;	
	}	
	e_rn2483_check_resp(au8_uart_data, &u8_uart_data_count);
	/* We can switch context if necessary. */
	v_rf_end_switching_context();
}

/*!
* @fn v_rn2483_store_data (uint8_t u8_data_len)
* @brief Store UART data to buffer (au8_rn2483_data[])
* @param[in] u8_data_len Length of data array, for checking overload buffer
* @return None
*/
static void v_rn2483_store_data (uint8_t u8_data_len)
{
	if (u8_data_len > RF_BUF_MAX)
	{
		u8_data_len = RF_BUF_MAX;
	}
	for(uint8_t i = 0; i < (u8_data_len - 2); i++)
	{
		au8_rn2483_data[i] = au8_uart_data[10 + i];
		u8_rn2483_data_len = i + 1;
	}
}

/*!
* @fn v_rn2483_store_snr (uint8_t u8_data_len)
* @brief Convert SNR from uart_data array (1 digit/element) to uint8_t SNR value
* @param[in] u8_data_len Length of data array, for checking overload buffer
* @return None
*/
static void v_rn2483_store_snr (uint8_t u8_data_len)
{
	if (u8_data_len > RF_BUF_MAX)
	{
		u8_data_len = RF_BUF_MAX;
	}
	u8_rn2483_snr = 128;
	if (au8_uart_data[0] == '-')
	{	
		if (u8_data_len == 2)
		{
			u8_rn2483_snr -= (au8_uart_data[1] - '0');
		}
		else if (u8_data_len == 3)
		{
			u8_rn2483_snr -= ((au8_uart_data[1] - '0') * 10 + (au8_uart_data[2] - '0'));
		}		
		else if (u8_data_len == 4)
		{
			u8_rn2483_snr -= ((au8_uart_data[1] - '0') * 100 + (au8_uart_data[2] - '0') * 10 
												+ (au8_uart_data[3] - '0'));
		}		
	}
	else
	{
		if (u8_data_len == 1)
		{
			u8_rn2483_snr += (au8_uart_data[0] - '0');
		}
		else if (u8_data_len == 2)
		{
			u8_rn2483_snr += ((au8_uart_data[0] - '0') * 10 + (au8_uart_data[1] - '0'));
		}		
		else if (u8_data_len == 3)
		{
			u8_rn2483_snr += ((au8_uart_data[0] - '0') * 100 + (au8_uart_data[1] - '0') * 10 
												+ (au8_uart_data[2] - '0'));
		}			
	}
}

/*!
* @fn v_lora_rn2483_send_data_to_queue(void)
* @brief COvert data to HEX. Put data to q_rf_out queue
* @param[in] 
* @return None
*/
static void v_lora_rn2483_send_data_to_queue(void)
{
	uint8_t i = 0;
	for(i = 0; i < (u8_rn2483_data_len/2); i++)
	{
		au8_uart_data[i] = ui8_ascii_to_hex(au8_rn2483_data[2*i], au8_rn2483_data[2*i + 1]);
	}	
	au8_uart_data[au8_uart_data[0] + 1] = u8_rn2483_snr;
	b_write_frame_to_rf_from_isr(au8_uart_data);
}

/*!
* @fn ui8_ascii_to_hex(uint8_t ui8_AS1,uint8_t ui8_AS2)
* @brief Covert 2 byte ASCII character in 8-bit HEX format to 8-bit HEX number
* @param[in] ui8_AS1 Higher byte
* @param[in] ui8_AS2 Lower byte
* @return None
*/
static uint8_t ui8_ascii_to_hex(uint8_t ui8_AS1,uint8_t ui8_AS2)
{
	uint8_t ui8_result = 0;
	switch(ui8_AS1)
	{
		case 'A':
		case 'a':
			ui8_result = 0xA<<4;	break;
		case 'B':
		case 'b':
			ui8_result = 0xB<<4;	break;
		case 'C':
		case 'c':
			ui8_result = 0xC<<4;	break;
		case 'D':
		case 'd':
			ui8_result = 0xD<<4;	break;
		case 'E':
		case 'e':
			ui8_result = 0xE<<4;	break;
		case 'F':
		case 'f':
			ui8_result = 0xF<<4;	break;
		default: ui8_result = (ui8_AS1-'0')<<4; break;
	}
	switch(ui8_AS2)
	{
		case 'A':
		case 'a':
			ui8_result |= 0xA;	break;
		case 'B':
		case 'b':
			ui8_result |= 0xB;	break;
		case 'C':
		case 'c':
			ui8_result |= 0xC;	break;
		case 'D':
		case 'd':
			ui8_result |= 0xD;	break;
		case 'E':
		case 'e':
			ui8_result |= 0xE;	break;
		case 'F':
		case 'f':
			ui8_result |= 0xF;	break;
		default: ui8_result |= ui8_AS2-'0'; break;
	}
	return ui8_result;
}

