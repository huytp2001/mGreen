/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file realy_control.c
 * @author Danh Pham
 * @date 18 Nov 2020
 * @version: 1.0.0
 * @brief This file contain functions to control relay.
 */ 
 /*!
 * Add more include here
 */
 #include <stdint.h>
 #include <stdbool.h>
 #include "relay_control.h"
 #include "FreeRTOS.h"
 #include "task.h"
 
 #include "MCP23017.h"
 #include "i2c_mgreen.h"
 #include "wdt.h"
 #include "mqtt_publish.h"

/*!
* @def RELAY_TASK_SIZE
* Memory size of relay control task (minsize * 4)
*/
#define RELAY_TASK_SIZE			(configMINIMAL_STACK_SIZE * 1)

/*!
*	@def RELAY_TASK_PRIORITY
* Priority of relay control task (2)
*/
#define RELAY_TASK_PRIORITY	(tskIDLE_PRIORITY + 1)

/*!
*	@def RELAY_TASK_DELAY
* Relay control task delay (1000ms)
*/
#define RELAY_TASK_DELAY			(portTickType)(1000 / portTICK_RATE_MS)
#define RELAY_CHECK_STATUS_TIME   20	//increase from 10 to 20 	

/*!
*	Variables declare
*/
static StaticTask_t xRelay_TaskBuffer;
static StackType_t  xRelay_Stack[RELAY_TASK_SIZE*2];

static uint64_t u64_curr_relay_state = 0x00;

/*!
*	Private functions prototype
*/
static void v_relay_task(void *pvParameters);
static bool b_get_relay_onboard_status (uint8_t* pu8_status);
//static void v_relay_onboard_control(relay_onboard_name_t e_relay_onboard_name,
//																		relay_onboard_status_t e_relay_onboard_status);
static void v_relay_onboard_turn_off_all (void);
static void v_relay_onboard_turn_on_all (void);
/*!
*	Public functions
*/
void v_relay_onboard_init (void)
{
	SysCtlPeripheralEnable(RELAY_ONBOARD_1_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_1_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_1_PORT, RELAY_ONBOARD_1_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_2_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_2_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_2_PORT, RELAY_ONBOARD_2_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_3_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_3_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_3_PORT, RELAY_ONBOARD_3_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_4_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_4_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_4_PORT, RELAY_ONBOARD_4_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_5_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_5_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_5_PORT, RELAY_ONBOARD_5_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_6_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_6_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_6_PORT, RELAY_ONBOARD_6_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_7_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_7_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_7_PORT, RELAY_ONBOARD_7_PIN);
	
	SysCtlPeripheralEnable(RELAY_ONBOARD_8_PER);
	while(!(SysCtlPeripheralReady(RELAY_ONBOARD_8_PER)));	
	GPIOPinTypeGPIOOutput(RELAY_ONBOARD_8_PORT, RELAY_ONBOARD_8_PIN);	
}

/*!
*	@fn v_relay_task_init(void)
* @brief Init relay control task
* @param None
* @return None
*/
void v_relay_task_init(void)
{
	/* Hardware init */
	v_relay_hardware_init();
	/* Software init */
	v_relay_software_init();
	/* Task init */
	xTaskCreateStatic( v_relay_task,                       /* The function that implements the task. */
						"relay task", 	                  /* The text name assigned to the task - for debug only as it is not used by the kernel. */
						RELAY_TASK_SIZE,           /* The size of the stack to allocate to the task. */
						NULL,                               /* The parameter passed to the task */
						RELAY_TASK_PRIORITY,                   /* The priority assigned to the task. */
						xRelay_Stack ,
			&xRelay_TaskBuffer);
}
/*!
*	@fn v_relay_hardware_init(void)
* @brief Init hardware for relay control
* @param[in] none
* @return None
*/
void v_relay_hardware_init(void)
{
	v_relay_onboard_init();
	v_MCP23017_hardware_Init();
}
/*!
*	@fn v_relay_software_init (void)
* @brief Init software for relay control
* @param[in] none
* @return None
*/
void v_relay_software_init (void)
{
	v_MCP23017_software_Init();
}
/*!
*	@fn b_relay_control ()
* @brief Init software for relay control
* @param[in] u64_new_relay_state
* @param[inout] pu64_act_relay_state
* @return true if successfully
*/
bool b_relay_control (uint64_t u64_new_relay_state, uint64_t* pu64_act_relay_state)
{
	uint64_t u64_relay_tmp = 0;
	uint8_t u8_tmp;
	uint8_t u8_req_GPIO_A = 0;
	uint8_t u8_req_GPIO_B = 0;
	
	uint8_t u8_ret_GPIO_A = 0;
	uint8_t u8_ret_GPIO_B = 0;	
	if(u64_new_relay_state != u64_curr_relay_state)
	{
		//v_i2c_hardware_reset();
		for(uint8_t u8_i = 0; u8_i < 8; u8_i++)
		{
			if((((u64_new_relay_state ^ u64_curr_relay_state) >> u8_i) & 0x01) == 0x01)
			{
				v_relay_onboard_control((relay_onboard_name_t)(u8_i+1),
											(relay_onboard_status_t)((u64_new_relay_state >> u8_i) & 0x01));
			}				
		}
		if(b_get_relay_onboard_status(&u8_tmp))
		{
			u64_relay_tmp = u8_tmp;
		}
		
		if(((u64_new_relay_state ^ u64_curr_relay_state) & 0x00FFFF00) != 0)
		{			
			u8_req_GPIO_A = (uint8_t)((u64_new_relay_state & 0x00FFFF00)>>8);
			u8_req_GPIO_B = (uint8_t)((u64_new_relay_state & 0x00FFFF00)>>16);
			if(b_MCP23017_latch_output(RELAY_I2C1_ADDR, u8_req_GPIO_A, u8_req_GPIO_B, &u8_ret_GPIO_A, &u8_ret_GPIO_B))
			{
				u64_relay_tmp |= ((uint64_t)u8_ret_GPIO_A)<<8;
				u64_relay_tmp |= ((uint64_t)u8_ret_GPIO_B)<<16;
			}
		}
		else
		{
			u64_relay_tmp |= (u64_new_relay_state & 0x00000F00);
			u64_relay_tmp |= (u64_new_relay_state & 0x0000F000);		
		}
		if(((u64_new_relay_state ^ u64_curr_relay_state) & 0x00FFFF000000) != 0)
		{
			u8_req_GPIO_A = (uint8_t)((u64_new_relay_state & 0x00FFFF000000)>>24);
			u8_req_GPIO_B = (uint8_t)((u64_new_relay_state & 0x00FFFF000000)>>32);
			if(b_MCP23017_latch_output(RELAY_I2C1_ADDR+1, u8_req_GPIO_A, u8_req_GPIO_B, &u8_ret_GPIO_A, &u8_ret_GPIO_B))
			{
				u64_relay_tmp |= ((uint64_t)u8_ret_GPIO_A)<<24;
				u64_relay_tmp |= ((uint64_t)u8_ret_GPIO_B)<<32;
			}	
		}
		else
		{
			u64_relay_tmp |= (u64_new_relay_state & 0x000F000000);
			u64_relay_tmp |= (u64_new_relay_state & 0x00F0000000);				
		}		
	}
	else
	{
		u64_relay_tmp = u64_curr_relay_state;
	}
	u64_curr_relay_state = u64_relay_tmp;
	*pu64_act_relay_state = u64_curr_relay_state;
	if(u64_curr_relay_state == u64_new_relay_state)
	{
		return true;
	}
	return false;
}

bool b_relay_turn_off_all (void)
{
	uint64_t u64_tmp = 0;
	u64_curr_relay_state = 0xFFFFFFFFFFFFFFFF;
	if(b_relay_control(0, &u64_tmp) == true)
	{
		
	}
	else
	{
		v_i2c_hardware_reset();
		u64_curr_relay_state = 0;
		return false;
	}
	return true;
}

bool b_get_ex_relay_status(uint8_t u8_slave_addr, uint64_t *u64_relay_status)
{
	uint8_t au8_read_data[4];
	memset((void *)au8_read_data, 0, 4);
	
	v_i2c_mgreen_read_data(SLAVE_ADDR_MASK | u8_slave_addr,
													GPIOA, au8_read_data, 4);
	if((au8_read_data[0] | au8_read_data[1] << 8) == (au8_read_data[2] | au8_read_data[3] << 8))
	{
		*u64_relay_status = (au8_read_data[0] | au8_read_data[1] << 8);
		*u64_relay_status = *u64_relay_status << (16 * (u8_slave_addr - RELAY_I2C1_ADDR));
		return true;
	}
	return false;
}

/*!
*	@fn b_relay_turn_on(uint8_t u8_Relay)
* @brief Turn on relay
* @param[in] u8_Relay Id of relay, from 1
* @return None
*/
bool b_relay_turn_on(uint8_t u8_Relay)
{
	uint64_t u64_tmp = 0;
	uint64_t u64_new_curr_relay_state = 0;
	u64_new_curr_relay_state = u64_curr_relay_state | ((uint64_t)0x01 << (u8_Relay - 1));
	if(b_relay_control(u64_new_curr_relay_state, &u64_tmp))
	{
		
	}
	else
	{
		v_i2c_hardware_reset();
		u64_curr_relay_state = 0;
		b_relay_control(u64_new_curr_relay_state, &u64_tmp);
		return false;
	}
	return true;
}

/*!
*	@fn b_relay_turn_off(uint8_t u8_Relay)
* @brief Turn off relay
* @param[in] u8_Relay Id of relay, from 1
* @return None
*/
bool b_relay_turn_off(uint8_t u8_Relay)
{
	uint64_t u64_tmp = 0;
	static uint64_t u64_new_curr_relay_state = 0;
	if (u8_Relay == 0)
	{
		b_relay_turn_off_all();
	}
	else
	{
		u64_new_curr_relay_state = u64_curr_relay_state & (~((uint64_t)0x01 << (u8_Relay - 1)));
	}
	if(b_relay_control(u64_new_curr_relay_state, &u64_tmp))
	{
		
	}
	else
	{
		v_i2c_hardware_reset();
		u64_curr_relay_state = 0;
		b_relay_control(u64_new_curr_relay_state, &u64_tmp);
		return false;
	}
	return true;
}


void v_manual_port_control(uint8_t u8_port, bool b_turn_on)
{
	if (0 == u8_port)
	{
		if (b_turn_on)
		{
			v_relay_onboard_turn_on_all();
		}
		else
		{
			v_relay_onboard_turn_off_all();
			b_relay_turn_off_all();
		}
	}
	else if (u8_port <= 8)
	{
		if (b_turn_on)
		{
			v_relay_onboard_control((relay_onboard_name_t)u8_port, ON_RELAY_ONBOARD);
		}
		else
		{
			v_relay_onboard_control((relay_onboard_name_t)u8_port, OFF_RELAY_ONBOARD);
		}
	}
	else if(u8_port <= (8+16))
	{
		if (b_turn_on)
		{
			b_relay_turn_on(u8_port);
		}
		else
		{
			b_relay_turn_off(u8_port);
		}
	}
	else
	{
		//do nothing
	}
}
static bool b_get_relay_onboard_status (uint8_t* pu8_status)
{
	uint8_t u8_tmp = 0;
	if(GPIOPinRead(RELAY_ONBOARD_1_PORT, RELAY_ONBOARD_1_PIN) == RELAY_ONBOARD_1_PIN)
	{
		u8_tmp |= 0x01;
	}
	if(GPIOPinRead(RELAY_ONBOARD_2_PORT, RELAY_ONBOARD_2_PIN) == RELAY_ONBOARD_2_PIN)
	{
		u8_tmp |= 0x02;
	}
	if(GPIOPinRead(RELAY_ONBOARD_3_PORT, RELAY_ONBOARD_3_PIN) == RELAY_ONBOARD_3_PIN)
	{
		u8_tmp |= 0x04;
	}
	if(GPIOPinRead(RELAY_ONBOARD_4_PORT, RELAY_ONBOARD_4_PIN) == RELAY_ONBOARD_4_PIN)
	{
		u8_tmp |= 0x08;
	}
	if(GPIOPinRead(RELAY_ONBOARD_5_PORT, RELAY_ONBOARD_5_PIN) == RELAY_ONBOARD_5_PIN)
	{
		u8_tmp |= 0x10;
	}
	if(GPIOPinRead(RELAY_ONBOARD_6_PORT, RELAY_ONBOARD_6_PIN) == RELAY_ONBOARD_6_PIN)
	{
		u8_tmp |= 0x20;
	}
	if(GPIOPinRead(RELAY_ONBOARD_7_PORT, RELAY_ONBOARD_7_PIN) == RELAY_ONBOARD_7_PIN)
	{
		u8_tmp |= 0x40;
	}
	if(GPIOPinRead(RELAY_ONBOARD_8_PORT, RELAY_ONBOARD_8_PIN) == RELAY_ONBOARD_8_PIN)
	{
		u8_tmp |= 0x80;
	}
	*pu8_status = u8_tmp;
	return true;
}


void v_relay_onboard_control(relay_onboard_name_t e_relay_onboard_name,
																		relay_onboard_status_t e_relay_onboard_status)
{
	if(e_relay_onboard_status == ON_RELAY_ONBOARD)
	{
		switch(e_relay_onboard_name)
		{
		case RELAY_ONBOARD_1:
			GPIOPinWrite(RELAY_ONBOARD_1_PORT, RELAY_ONBOARD_1_PIN, RELAY_ONBOARD_1_PIN);
			break;
		case RELAY_ONBOARD_2:
			GPIOPinWrite(RELAY_ONBOARD_2_PORT, RELAY_ONBOARD_2_PIN, RELAY_ONBOARD_2_PIN);
			break;
		case RELAY_ONBOARD_3:
			GPIOPinWrite(RELAY_ONBOARD_3_PORT, RELAY_ONBOARD_3_PIN, RELAY_ONBOARD_3_PIN);
			break;
		case RELAY_ONBOARD_4:
			GPIOPinWrite(RELAY_ONBOARD_4_PORT, RELAY_ONBOARD_4_PIN, RELAY_ONBOARD_4_PIN);
			break;
		case RELAY_ONBOARD_5:
			GPIOPinWrite(RELAY_ONBOARD_5_PORT, RELAY_ONBOARD_5_PIN, RELAY_ONBOARD_5_PIN);
			break;
		case RELAY_ONBOARD_6:
			GPIOPinWrite(RELAY_ONBOARD_6_PORT, RELAY_ONBOARD_6_PIN, RELAY_ONBOARD_6_PIN);
			break;
		case RELAY_ONBOARD_7:
			GPIOPinWrite(RELAY_ONBOARD_7_PORT, RELAY_ONBOARD_7_PIN, RELAY_ONBOARD_7_PIN);
			break;
		case RELAY_ONBOARD_8:
			GPIOPinWrite(RELAY_ONBOARD_8_PORT, RELAY_ONBOARD_8_PIN, RELAY_ONBOARD_8_PIN);
			break;		
		default: break;
		}
	}
	else
	{
		switch(e_relay_onboard_name)
		{
		case RELAY_ONBOARD_1:
			GPIOPinWrite(RELAY_ONBOARD_1_PORT, RELAY_ONBOARD_1_PIN, 0);
			break;
		case RELAY_ONBOARD_2:
			GPIOPinWrite(RELAY_ONBOARD_2_PORT, RELAY_ONBOARD_2_PIN, 0);
			break;
		case RELAY_ONBOARD_3:
			GPIOPinWrite(RELAY_ONBOARD_3_PORT, RELAY_ONBOARD_3_PIN, 0);
			break;
		case RELAY_ONBOARD_4:
			GPIOPinWrite(RELAY_ONBOARD_4_PORT, RELAY_ONBOARD_4_PIN, 0);
			break;
		case RELAY_ONBOARD_5:
			GPIOPinWrite(RELAY_ONBOARD_5_PORT, RELAY_ONBOARD_5_PIN, 0);
			break;
		case RELAY_ONBOARD_6:
			GPIOPinWrite(RELAY_ONBOARD_6_PORT, RELAY_ONBOARD_6_PIN, 0);
			break;
		case RELAY_ONBOARD_7:
			GPIOPinWrite(RELAY_ONBOARD_7_PORT, RELAY_ONBOARD_7_PIN, 0);
			break;
		case RELAY_ONBOARD_8:
			GPIOPinWrite(RELAY_ONBOARD_8_PORT, RELAY_ONBOARD_8_PIN, 0);
			break;		
		default: break;
		}
	}
}

static void v_relay_onboard_turn_off_all (void)
{
	GPIOPinWrite(RELAY_ONBOARD_1_PORT, RELAY_ONBOARD_1_PIN, 0);
	GPIOPinWrite(RELAY_ONBOARD_2_PORT, RELAY_ONBOARD_2_PIN, 0);
	GPIOPinWrite(RELAY_ONBOARD_3_PORT, RELAY_ONBOARD_3_PIN, 0);
	GPIOPinWrite(RELAY_ONBOARD_4_PORT, RELAY_ONBOARD_4_PIN, 0);
	GPIOPinWrite(RELAY_ONBOARD_5_PORT, RELAY_ONBOARD_5_PIN, 0);	
	GPIOPinWrite(RELAY_ONBOARD_6_PORT, RELAY_ONBOARD_6_PIN, 0);
	GPIOPinWrite(RELAY_ONBOARD_7_PORT, RELAY_ONBOARD_7_PIN, 0);	
	GPIOPinWrite(RELAY_ONBOARD_8_PORT, RELAY_ONBOARD_8_PIN, 0);	
}

static void v_relay_onboard_turn_on_all (void)
{
	GPIOPinWrite(RELAY_ONBOARD_1_PORT, RELAY_ONBOARD_1_PIN, RELAY_ONBOARD_1_PIN);
	GPIOPinWrite(RELAY_ONBOARD_2_PORT, RELAY_ONBOARD_2_PIN, RELAY_ONBOARD_2_PIN);
	GPIOPinWrite(RELAY_ONBOARD_3_PORT, RELAY_ONBOARD_3_PIN, RELAY_ONBOARD_3_PIN);
	GPIOPinWrite(RELAY_ONBOARD_4_PORT, RELAY_ONBOARD_4_PIN, RELAY_ONBOARD_4_PIN);
	GPIOPinWrite(RELAY_ONBOARD_5_PORT, RELAY_ONBOARD_5_PIN, RELAY_ONBOARD_5_PIN);	
	GPIOPinWrite(RELAY_ONBOARD_6_PORT, RELAY_ONBOARD_6_PIN, RELAY_ONBOARD_6_PIN);
	GPIOPinWrite(RELAY_ONBOARD_7_PORT, RELAY_ONBOARD_7_PIN, RELAY_ONBOARD_7_PIN);	
	GPIOPinWrite(RELAY_ONBOARD_8_PORT, RELAY_ONBOARD_8_PIN, RELAY_ONBOARD_8_PIN);	
}

/*!
*	Private functions
*/

/*!
*	@fn static void v_relay_task(void *pvParameters)
* @brief Relay control task
* @param None
* @return None
*/
static void v_relay_task(void *pvParameters)
{
	static uint8_t u8_relay_task_id = 0;
	static uint8_t u8_check_relay_status_counter = 0;
	//static uint8_t u8_check_relay_status_counter = 0;
	while(b_wdt_reg_new_task("relay_task", &u8_relay_task_id) != true) {}
	
	for( ; ; )
	{
		/* Reload wdt */
		b_wdt_task_reload_counter(u8_relay_task_id);
		
		// Check status of relay 
		u8_check_relay_status_counter++;
		if(u8_check_relay_status_counter > RELAY_CHECK_STATUS_TIME)
		{
			u8_check_relay_status_counter = 0;
			uint64_t u64_tmp_relay_state = 0;
			uint64_t u64_tmp_relay_state_2 = 0;
			for(uint8_t i = 1; i < MAX_NUM_RELAY_BOARD; i++)
			{
				u64_tmp_relay_state = 0;
				if(b_get_ex_relay_status(RELAY_I2C1_ADDR, &u64_tmp_relay_state))
				{
					if(u64_tmp_relay_state != ((u64_curr_relay_state >> 8) & 0xFFFF << ((i-RELAY_I2C1_ADDR)* 16)))
					{
						v_mqtt_data_pub(0x0e, 0000);
						u64_tmp_relay_state_2 = u64_tmp_relay_state;
						u64_tmp_relay_state = u64_curr_relay_state;
						u64_curr_relay_state = u64_tmp_relay_state_2;
						if(b_relay_control(u64_tmp_relay_state, &u64_tmp_relay_state))
						{
							
						}
						else
						{
							if(b_relay_control(u64_tmp_relay_state, &u64_tmp_relay_state))
							{
							}
							else
							{
								u64_curr_relay_state = u64_tmp_relay_state;
							}
						}
					}
					else
					{
					}
				}
				else
				{
					v_i2c_hardware_reset();
					u64_tmp_relay_state = u64_curr_relay_state;
					u64_curr_relay_state = 0x00;
					if(b_relay_control(u64_tmp_relay_state, &u64_tmp_relay_state))
					{
						
					}
					else
					{
						if(b_relay_control(u64_tmp_relay_state, &u64_tmp_relay_state))
						{
						}
						else
						{
							u64_curr_relay_state = u64_tmp_relay_state;
						}
					}
				}
			}
		}
		
		vTaskDelay(RELAY_TASK_DELAY);
	}
}
