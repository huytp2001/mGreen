#ifndef RN2483_H_
#define RN2483_H_
#include "driverlib.h"
#include "stdint.h"
#include "stdbool.h"

#define RN2483_REQUEST_MAC_PAUSE_TIMEOUT			200		//100*10 = 1s


#define	RN2483_RESP_OK												"ok"
#define	RN2483_RESP_INVALID_PARAM							"invalid_param"
#define	RN2483_RESP_BUSY											"busy"
#define	RN2483_RESP_RADIO_ERR									"radio_err"
#define	RN2483_RESP_RADIO_TX_OK								"radio_tx_ok"

typedef enum
{
	RN2483_MAC_PAUSE = 0,
	RN2483_RADIO_SET_MODE,
	RN2483_RADIO_SET_FREQ,
	RN2483_RADIO_SET_PWR,
	RN2483_RADIO_SET_CRC,
	RN2483_RADIO_SET_WDT,
	RN2483_RADIO_GET_SNR,
	RN2483_RADIO_START_RX,
	RN2483_RADIO_START_TX,
	RN2483_RADIO_TX,
	RN2483_NUM_REQUEST
}RN2483_REQUEST;

typedef enum
{
	RN2483_MAC_PAUSE_OK = 0,
	RN2483_RADIO_SET_MODE_OK,
	RN2483_RADIO_SET_FREQ_OK,
	RN2483_RADIO_SET_PWR_OK,
	RN2483_RADIO_SET_CRC_OK,
	RN2483_RADIO_SET_WDT_OK,
	RN2483_RADIO_GET_SNR_OK,
	RN2483_RADIO_START_RX_OK,
	RN2483_RADIO_BUSY,
	RN2483_RADIO_START_TX_OK,
	RN2483_RADIO_TX_OK,
	RN2483_INVALID_PARAM,
	RN2483_RADIO_ERR,
	RN2483_NUM_RESP_STATUS
}RN2483_RESP_STATUS;

typedef enum
{
	RN2483_RADIO_LORA_MODE = 0,
	RN2483_RADIO_FSK_MODE,
}RN2483_RADIO_MODE;

typedef enum
{
	RN2483_RADIO_FREQ_433 = 0,
	RN2483_RADIO_FREQ_868,
}RN2483_RADIO_FREQ;

typedef enum
{
	RN2483_RADIO_CRC_MODE_ON = 0,
	RN2483_RADIO_CRC_MODE_OFF,
}RN2483_RADIO_CRC_MODE;

typedef enum
{
	RN2483_RADIO_IN_RX = 0,
	RN2483_RADIO_RX_DONE,
	RN2483_RADIO_CAL_SNR,
	RN2483_RADIO_CAL_SNR_DONE,
	RN2483_NUM_STATE
}RN2483_RADIO_STATE;


/*!
* Public function
*/
bool rn2483_init(void);
void v_rn2483_process_data (void);
void e_rn2483_check_resp (uint8_t* p_data, uint8_t* ui8_data_len);
char* pc_num_to_string(uint32_t num, uint8_t dec, bool pos);
void rn2483_uart_put(char* tx_buf, uint8_t u8_num_to_write);
void rn2483_uart_put_string(char* txBuf);
void e_rn2483_check_resp (uint8_t* p_data, uint8_t* ui8_data_len);
bool b_rn2483_mac_pause(void);
bool b_rn2483_radio_set_mode(RN2483_RADIO_MODE e_mode);
bool b_rn2483_radio_set_freq(RN2483_RADIO_FREQ e_freq);
bool b_rn2483_radio_set_pwr(int8_t i8_pwr);
bool b_rn2483_radio_set_crc(RN2483_RADIO_CRC_MODE e_crc_mode);
bool b_rn2483_radio_set_wdt(uint32_t  u32_wdt_timeoout);
bool b_rn2483_radio_get_snr(void);
bool b_rn2483_radio_rx(uint16_t  u16_rx_window_size);
bool b_rn2483_radio_tx(char* u8_tx_buf);
RN2483_RADIO_STATE e_rn2483_get_state(void);
void v_rn2483_set_state(RN2483_RADIO_STATE e_state);
RN2483_RESP_STATUS e_rn2483_get_resp_status(void);
bool b_rn2483_check_radio_err (void);
#endif
