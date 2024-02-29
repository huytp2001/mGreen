#include <string.h>
#include "device_setting.h"
#include "csv.h"
#include "Storage.h"
#include "digital_input.h"
#include "sd_card.h"
#include "sd_app.h"
#include "schedule.h"


static struct STRU_CSV_COUNT c;
static STRU_PORT_OUTPUT astr_output_config[MAX_OUTPUT_SETTING];
static STRU_PORT_INPUT astr_input_config[MAX_INPUT_SETTING];
static uint16_t u16_total_row = 0;
static uint8_t u8_csv_field_counter = 0;
static char csv_version[21];
static uint8_t u8_current_cycle = 0;
static uint8_t u8_current_output = 0;
static uint8_t u8_current_input = 0;

static void v_clear_setting(void)
{
	u8_current_output = 0;
	u8_current_input = 0;
	for(uint8_t i = 0; i < MAX_OUTPUT_SETTING; i++)
	{
		astr_output_config[i].u8_port = 0;
		astr_output_config[i].e_output_type = TYPE_O_DEFAULT;
		if(i < MAX_INPUT_SETTING)
		{
			astr_input_config[i].u8_port = 0;
			astr_input_config[i].u8_map_output = 0;
			astr_input_config[i].e_input_type = TYPE_I_DEFAULT;
		}
	}
}

/*!
* public function bodies
*/
/*
function use to read data from csv file
*/
/* Not use in device_setting.h file*/

static bool b_parse_setting_csv_output(uint8_t u8_cylce, uint8_t u8_field, char *s)
{
	switch(u8_field)
	{
		case 1: 				//port number
		{
			astr_output_config[u8_current_output].u8_port = strtoul(s, 0, 10);
		}
		break;
		case 2:					//type
		{
			astr_output_config[u8_current_output].e_output_type = strtoul(s, 0, 10);
			u8_current_output++;
		}
		break;
	}
	return true;
}
static bool b_parse_setting_csv_input(uint8_t u8_cylce, uint8_t u8_field, char *s)
{
	switch(u8_field)
	{
		case 1: 				//port number
		{
			astr_input_config[u8_current_input].u8_port = strtoul(s, 0, 10);
		}
		break;
		case 2:					//type
		{
			astr_input_config[u8_current_input].e_input_type = strtoul(s, 0, 10);
		}
		break;
		case 3:
		{
			astr_input_config[u8_current_input].u8_map_output = strtoul(s, 0, 10);
		}break;
		case 4:
		{
			astr_input_config[u8_current_input].u32_time_effect = strtoul(s, 0, 10);
		}
		break;
		case 5:
		{
			astr_input_config[u8_current_input].d_multi_factor = (double)strtoul(s, 0, 10)/100.0/2;
			u8_current_input++;
		}break;
	}
	return true;
}
static void v_LoadCSV_FieldCallback(void *s, size_t len, void *data)
{
	static uint8_t u8_type_of_data = 0;
	((struct STRU_CSV_COUNT *)data)->u16_fields++;
	if(s != NULL)
	{
		if(u8_csv_field_counter == 0)
		{
			u8_type_of_data = strtoul(s, 0, 10);
		}
		switch(u8_type_of_data)
		{
			case 0:							//csv version
			{
				if(u8_csv_field_counter == 1)
				{
					memcpy((void *)csv_version, (void *)s, strlen(s) + 1);
				}
			}
			break;
			case 1:						//output setting
			{
				b_parse_setting_csv_output(u8_current_cycle, u8_csv_field_counter, s);
			}
			break;
			case 2:							//input setting
			{
				b_parse_setting_csv_input(u8_current_cycle, u8_csv_field_counter, s);
			}
			break;
		}
		u8_csv_field_counter++;
	}
}

static void v_LoadCSV_RowCallback(int c, void * data)
{
	u8_csv_field_counter = 0;
	((struct STRU_CSV_COUNT *)data)->u16_rows++;
}

int i_load_setting_from_sd_card(void)
{
	//Clear old schedule
	v_clear_setting();
	//int result = iHandle_CSVfileCallback(SETTING_CSV_FILE, v_LoadCSV_FieldCallback, v_LoadCSV_RowCallback, &c);
	int8_t i8_result = i8_handle_csv_file_callback(SETTING_CSV_FILE, v_LoadCSV_FieldCallback, v_LoadCSV_RowCallback, &c, false);
	uint8_t u8_counter_pin = 0;
	if(astr_input_config[0].e_input_type == TYPE_I_COUNTER)
		u8_counter_pin |= INPUT_PIN_1;
	if(astr_input_config[1].e_input_type == TYPE_I_COUNTER)
		u8_counter_pin |= INPUT_PIN_2;
	if(astr_input_config[2].e_input_type == TYPE_I_COUNTER)
		u8_counter_pin |= INPUT_PIN_3;
	if(astr_input_config[3].e_input_type == TYPE_I_COUNTER)
		u8_counter_pin |= INPUT_PIN_4;
	// Load new input configuration
	v_digital_input_set_type_counter((DIO_INPUT_PIN)u8_counter_pin);
	return i8_result;
}

bool b_output_setting_get(uint8_t u8_port_number, STRU_PORT_OUTPUT *pstru_port_output)
{
	bool b_result = false;
	if(u8_port_number < MAX_OUTPUT_SETTING)
	{
		*pstru_port_output = astr_output_config[u8_port_number];
		b_result = true;
	}
	return b_result;
}
bool b_input_setting_get(uint8_t u8_port_number, STRU_PORT_INPUT *pstru_port_input)
{
	bool b_result = false;
	if(u8_port_number < MAX_INPUT_SETTING)
	{
		*pstru_port_input = astr_input_config[u8_port_number];
		b_result = true;
	}
	return b_result;
}
