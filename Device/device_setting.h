#ifndef __SETTING_H__
#define __SETTING_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>


#define SETTING_CSV_FILE					"SETTING.CSV"
#define MAX_CSV_SETING_ROW				50
#define MAX_NAME_LEN							20

#define MAX_OUTPUT_SETTING			64


typedef enum
{
	TYPE_O_DEFAULT = 0,		//valve or orther type
	TYPE_O_PUMP = 1,
	TYPE_O_ENGINE = 2,
	TYPE_O_FERTILIZER_VALVE = 3,
}E_OUTPUT_TYPE;

typedef enum
{
	TYPE_I_DEFAULT = 0,		//port is not in use
	TYPE_I_ON_INDICATOR = 1,
	TYPE_I_OFF_INDICATOR = 2,
	TYPE_I_SWITCH_NO = 3,
	TYPE_I_SWITCH_NC = 4,
	TYPE_I_COUNTER = 5,
}E_INPUT_TYPE;

typedef struct
{
	uint8_t u8_port;
	E_OUTPUT_TYPE e_output_type;
}STRU_PORT_OUTPUT;

typedef struct
{
	uint8_t u8_port;
	E_INPUT_TYPE e_input_type;
	uint8_t u8_map_output;
	uint32_t u32_time_effect;
	double d_multi_factor;
}STRU_PORT_INPUT;

int i_load_setting_from_sd_card(void);
bool b_output_setting_get(uint8_t u8_port_number, STRU_PORT_OUTPUT *pstru_port_output);
bool b_input_setting_get(uint8_t u8_port_number, STRU_PORT_INPUT *pstru_port_input);

#endif	/*__SETTING_H__*/
