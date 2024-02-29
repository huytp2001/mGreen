#ifndef __TIMEOUT_H_
#define __TIMEOUT_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define MAX_TIMEOUT_HANDLER			10
typedef enum
{
	TIMEOUT_NULL,
	TIMEOUT_INPUT_1,
	TIMEOUT_INPUT_2,
	TIMEOUT_INPUT_3,
	TIMEOUT_INPUT_4,
	TIMEOUT_MAX
}E_FTG_TIMEOUT_REGISTER;

typedef struct
{
	E_FTG_TIMEOUT_REGISTER e_register;
	uint16_t u16_time_register;
	uint16_t u16_time_count;
}STRU_FTG_TIMEOUT;

extern E_FTG_TIMEOUT_REGISTER e_timeout_input_table[4];

void vmgreen_Timeout_TaskInit(void);
void v_timeout_reg(E_FTG_TIMEOUT_REGISTER e_register_timeout, uint16_t u16_timeout);
void v_timeout_reset(E_FTG_TIMEOUT_REGISTER e_register_timeout);
void v_timeout_unreg(E_FTG_TIMEOUT_REGISTER e_register_timeout);
int i16_get_timeout_status(E_FTG_TIMEOUT_REGISTER e_register_timeout);
#endif	//FTG_TIMEOUT_H_
