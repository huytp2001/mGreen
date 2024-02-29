#include "mgreen_timeout.h"
//#include "firmware_config.h"
//#include "watchdog_task.h"
#include "wdt.h"

#define FTG_TIMEOUT_TASK_SIZE					(configMINIMAL_STACK_SIZE * 4)
#define FTG_TIMEOUT_TASK_PRIORITY			(tskIDLE_PRIORITY + 2)
#define FTG_TIMEOUT_TASK_DELAY					(portTickType)(1000 / portTICK_RATE_MS) 
static STRU_FTG_TIMEOUT str_timeout[MAX_TIMEOUT_HANDLER];


static StaticTask_t xFTG_Timeout_TaskBuffer;
static StackType_t  xFTG_Timeout_Stack[FTG_TIMEOUT_TASK_SIZE];

E_FTG_TIMEOUT_REGISTER e_timeout_input_table[4] = {TIMEOUT_INPUT_1,
																											TIMEOUT_INPUT_2,
																											TIMEOUT_INPUT_3,
																											TIMEOUT_INPUT_4};

void v_timeout_reg(E_FTG_TIMEOUT_REGISTER e_register_timeout, uint16_t u16_timeout)
{
	for(uint8_t i = 0; i < MAX_TIMEOUT_HANDLER; i++)
	{
		if(str_timeout[i].e_register == TIMEOUT_NULL || str_timeout[i].e_register == e_register_timeout)
		{
			str_timeout[i].e_register = e_register_timeout;
			str_timeout[i].u16_time_register = u16_timeout;
			str_timeout[i].u16_time_count = 0;
			break;
		}
	}
}

void v_timeout_reset(E_FTG_TIMEOUT_REGISTER e_register_timeout)
{
	for(uint8_t i = 0; i < MAX_TIMEOUT_HANDLER; i++)
	{
		if(str_timeout[i].e_register == e_register_timeout)
			str_timeout[i].u16_time_count = 0;
	}
}

void v_timeout_unreg(E_FTG_TIMEOUT_REGISTER e_register_timeout)
{
	for(uint8_t i = 0; i < MAX_TIMEOUT_HANDLER; i++)
	{
		if(str_timeout[i].e_register == e_register_timeout)
		{
			str_timeout[i].e_register = TIMEOUT_NULL;
			str_timeout[i].u16_time_register = 0;
			str_timeout[i].u16_time_count = 0;
		}
	}
}

int i16_get_timeout_status(E_FTG_TIMEOUT_REGISTER e_register_timeout)
{
	for(uint8_t i = 0; i < MAX_TIMEOUT_HANDLER; i++)
	{
		if(str_timeout[i].e_register == e_register_timeout)
		{
			return (str_timeout[i].u16_time_register - str_timeout[i].u16_time_count);
		}
	}
	return -1;
}
static void vFTG_Timout_Task(void *pvParameters)
{
	static uint8_t u8_timeout_task_id = 0;
	while(b_wdt_reg_new_task("fertigation_task", &u8_timeout_task_id) != true){}	
	for(;;)
	{
		//reload wdt counter
		b_wdt_task_reload_counter(u8_timeout_task_id);
		for(uint8_t i = 0; i < MAX_TIMEOUT_HANDLER; i++)
		{
			if(str_timeout[i].e_register != TIMEOUT_NULL)
			{
				if(str_timeout[i].u16_time_count < str_timeout[i].u16_time_register)
				{
					str_timeout[i].u16_time_count++;
				}
			}
		}
		vTaskDelay(FTG_TIMEOUT_TASK_DELAY);
	}
}



void vmgreen_Timeout_TaskInit(void)
{
	xTaskCreateStatic(vFTG_Timout_Task, "FTG_TIMEOUT_TASK",
							FTG_TIMEOUT_TASK_SIZE, NULL, FTG_TIMEOUT_TASK_PRIORITY, xFTG_Timeout_Stack, &xFTG_Timeout_TaskBuffer);
}
