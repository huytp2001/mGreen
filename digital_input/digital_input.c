/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file pressure_switch.c
 * @brief functions process pressure switch 
 */ 
 /*!
 * Add more include here
 */
 #include <stdio.h>
 #include <stdbool.h>

 #include "config.h"
 #include "digital_input.h"
 #include "HAL_BSP.h"
 #include "device_setting.h"
 #include "config.h"
 #include "setting.h"
 #include "lora.h"
 
 #include "FreeRTOS.h"
 #include "task.h"
 #include "queue.h"

#define DI_TASK_STACK_SIZE												(configMINIMAL_STACK_SIZE * 1)
#define DI_TASK_PRIORITY													(tskIDLE_PRIORITY + 2)
#define DI_TASK_DELAY															(portTickType)(100 / portTICK_RATE_MS) //100ms

static StaticTask_t xInDITaskBuffer;
static StackType_t  xInDI_Stack[DI_TASK_STACK_SIZE];
static uint8_t input_state = 0;
static uint32_t au32_di_value[4] = {0, 0, 0, 0};
static DIO_INPUT_PIN e_dio_pin = INPUT_PIN_NOT_USE;
static bool b_input3_pre_state = false;
/*!
* private function bodies
*/

static void v_di_init(void)
{
	//SW enable
	SW1_CLK_ENABLE();
	SW2_CLK_ENABLE();
	SW3_CLK_ENABLE();
	SW4_CLK_ENABLE();
	SW5_CLK_ENABLE();
	
	GPIOPinTypeGPIOInput(SW1_GPIO_PORT, SW1_PIN);
	GPIOPinTypeGPIOInput(SW2_GPIO_PORT, SW2_PIN);
	GPIOPinTypeGPIOInput(SW3_GPIO_PORT, SW3_PIN);
	GPIOPinTypeGPIOInput(SW4_GPIO_PORT, SW4_PIN);
	GPIOPinTypeGPIOInput(SW5_GPIO_PORT, SW5_PIN);
	//Input enable
	INPUT_1_CLK_ENABLE();
	INPUT_2_CLK_ENABLE();
	INPUT_3_CLK_ENABLE();
	INPUT_4_CLK_ENABLE();
	
	
	//Unlock PD7
	HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0x80;
	
	GPIOPinTypeGPIOInput(INPUT_1_GPIO_PORT, INPUT_1_PIN);
	GPIOPinTypeGPIOInput(INPUT_2_GPIO_PORT, INPUT_2_PIN);

	GPIOPinTypeGPIOInput(INPUT_3_GPIO_PORT, INPUT_3_PIN);
	GPIOPinTypeGPIOInput(INPUT_4_GPIO_PORT, INPUT_4_PIN);
	if (SW4_READ())
	{
		v_digital_input_set_type_counter((DIO_INPUT_PIN)(INPUT_PIN_1|INPUT_PIN_2));
		#if INPUT_3_CNT_MANUAL
		v_digital_input_set_type_counter(INPUT_PIN_1|INPUT_PIN_2|INPUT_PIN_3);
		#endif
	}
}


static void v_digital_input_task (void *pvParameters)
{
	static bool b_read = false;
	static uint32_t u32_cnt[5] = {0};
	b_input3_pre_state = INPUT_3_READ();
	for(;;)
	{	
		uint8_t u8_temp_state = 0;
		if((e_dio_pin & INPUT_PIN_1) == 0)
		{
			if(INPUT_1_READ())
			{
				u8_temp_state |= 1;			
			}
		}
		if((e_dio_pin & INPUT_PIN_2) == 0)
		{
			if(INPUT_2_READ())
			{
				u8_temp_state |= (1 << 1);
			}
		}
		if((e_dio_pin & INPUT_PIN_3) == 0)
		{
			if(INPUT_3_READ())
			{
				u8_temp_state |= (1 << 2);
			}			
		}
		#if INPUT_3_CNT_MANUAL
		else
		{
			//event both edge
			if (b_input3_pre_state != INPUT_3_READ())
			{
				au32_di_value[2]++;
				b_input3_pre_state = INPUT_3_READ();
			}
		}
		#endif	
		if((e_dio_pin & INPUT_PIN_4) == 0)
		{
			if(INPUT_4_READ())
			{
				u8_temp_state |= (1 << 3);
			}
		}
		//v_di_get_value(au32_di_value);
		if(input_state != u8_temp_state)
		{
			input_state = u8_temp_state;
			/* TODO: toggle led to performce the input change, publish state of input port */
			//v_led_err_toggle();
			//vMQTTPubData(0, DATA_ID_INPUT_PORT, input_state);
		}
		/*todo: test read counter*/
		if (b_read)
		{
			memset(u32_cnt, 0, sizeof(u32_cnt));
			v_di_get_value(u32_cnt);
			b_read = false;
		}
		
		vTaskDelay(DI_TASK_DELAY);
	}
}

/*!
* public function bodies
*/

void v_digital_input_task_init (void)
{				
	v_di_init();
	
	xTaskCreateStatic(v_digital_input_task,"DIGITAL INUPUT TASK", DI_TASK_STACK_SIZE,
							NULL, DI_TASK_PRIORITY, xInDI_Stack, &xInDITaskBuffer);
}

void v_digital_input_set_type_counter(DIO_INPUT_PIN e_pin)
{
	e_dio_pin = e_pin;
	switch(e_pin & (INPUT_PIN_1 | INPUT_PIN_2))
	{
		case INPUT_PIN_1:
		{
			GPIOPinTypeTimer(INPUT_1_GPIO_PORT, INPUT_1_PIN);
			GPIOPinConfigure(GPIO_PD4_T3CCP0);
			
			INPUT_12_COUNTER_CLK_EN();
			TimerClockSourceSet(INPUT_12_TIMER_PORT, TIMER_CLOCK_SYSTEM);
			TimerDisable(INPUT_12_TIMER_PORT, TIMER_A);
			TimerConfigure(INPUT_12_TIMER_PORT, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT);
			TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_A, 10000);
			TimerControlEvent(INPUT_12_TIMER_PORT, TIMER_A, TIMER_EVENT_POS_EDGE);
			TimerEnable(INPUT_12_TIMER_PORT, TIMER_A);
		}break;
		case INPUT_PIN_2:
		{
			GPIOPinTypeGPIOInput(INPUT_2_GPIO_PORT, INPUT_2_PIN);
			GPIOPinTypeTimer(INPUT_2_GPIO_PORT, INPUT_2_PIN);
			GPIOPinConfigure(GPIO_PD5_T3CCP1);
			
			INPUT_12_COUNTER_CLK_EN();
			TimerClockSourceSet(INPUT_12_TIMER_PORT, TIMER_CLOCK_SYSTEM);
			TimerDisable(INPUT_12_TIMER_PORT, TIMER_B);
			TimerConfigure(INPUT_12_TIMER_PORT, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_CAP_COUNT);
			TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_B, 10000);
			TimerControlEvent(INPUT_12_TIMER_PORT, TIMER_B, TIMER_EVENT_POS_EDGE);
			TimerEnable(INPUT_12_TIMER_PORT, TIMER_B);
		}break;
		case INPUT_PIN_1 | INPUT_PIN_2:
		{
			GPIOPinTypeTimer(INPUT_1_GPIO_PORT, INPUT_1_PIN);
			GPIOPinTypeTimer(INPUT_2_GPIO_PORT, INPUT_2_PIN);
			GPIOPinConfigure(GPIO_PD4_T3CCP0);
			GPIOPinConfigure(GPIO_PD5_T3CCP1);
			
			INPUT_12_COUNTER_CLK_EN();
			TimerClockSourceSet(INPUT_12_TIMER_PORT, TIMER_CLOCK_SYSTEM);
			TimerDisable(INPUT_12_TIMER_PORT, TIMER_BOTH);
			TimerConfigure(INPUT_12_TIMER_PORT, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT | TIMER_CFG_B_CAP_COUNT);
			TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_BOTH, 10000);
			TimerControlEvent(INPUT_12_TIMER_PORT, TIMER_BOTH, TIMER_EVENT_POS_EDGE);
			TimerEnable(INPUT_12_TIMER_PORT, TIMER_BOTH);
		}break;
	}
	switch(e_pin & (INPUT_PIN_3 | INPUT_PIN_4))
	{
		case INPUT_PIN_3:
		{
			#if INPUT_3_CNT_MANUAL
			
			#else
			GPIOPinTypeTimer(INPUT_3_GPIO_PORT, INPUT_3_PIN);
			GPIOPinConfigure(GPIO_PD6_T4CCP0);
			
			INPUT_34_COUNTER_CLK_EN();
			TimerClockSourceSet(INPUT_34_TIMER_PORT, TIMER_CLOCK_SYSTEM);
			TimerDisable(INPUT_34_TIMER_PORT, TIMER_A);
			TimerConfigure(INPUT_34_TIMER_PORT, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT);
			TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_A, 10000);
			TimerControlEvent(INPUT_34_TIMER_PORT, TIMER_A, TIMER_EVENT_POS_EDGE);
			TimerEnable(INPUT_34_TIMER_PORT, TIMER_A);
			#endif
		}break;
		case INPUT_PIN_4:
		{
			GPIOPinTypeGPIOInput(INPUT_4_GPIO_PORT, INPUT_4_PIN);
			GPIOPinTypeTimer(INPUT_4_GPIO_PORT, INPUT_4_PIN);
			GPIOPinConfigure(GPIO_PD7_T4CCP1);

			INPUT_34_COUNTER_CLK_EN();
			TimerDisable(INPUT_34_TIMER_PORT, TIMER_B);
			TimerClockSourceSet(INPUT_34_TIMER_PORT, TIMER_CLOCK_SYSTEM);
			TimerConfigure(INPUT_34_TIMER_PORT, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_CAP_COUNT);
			TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_B, 10000);
			TimerControlEvent(INPUT_34_TIMER_PORT, TIMER_B, TIMER_EVENT_POS_EDGE);
			TimerEnable(INPUT_34_TIMER_PORT, TIMER_B);
		}break;
		case INPUT_PIN_3 | INPUT_PIN_4:
		{
			//GPIOPinTypeGPIOInput(INPUT_3_GPIO_PORT, INPUT_4_PIN);
			//GPIOPinTypeGPIOInput(INPUT_4_GPIO_PORT, INPUT_4_PIN);
			GPIOPinTypeTimer(INPUT_3_GPIO_PORT, INPUT_3_PIN);
			GPIOPinTypeTimer(INPUT_4_GPIO_PORT, INPUT_4_PIN);
			GPIOPinConfigure(GPIO_PD6_T4CCP0);
			GPIOPinConfigure(GPIO_PD7_T4CCP1);

			
			INPUT_34_COUNTER_CLK_EN();
			TimerDisable(INPUT_34_TIMER_PORT, TIMER_BOTH);
			TimerClockSourceSet(INPUT_34_TIMER_PORT, TIMER_CLOCK_SYSTEM);
			TimerConfigure(INPUT_34_TIMER_PORT, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_COUNT | TIMER_CFG_B_CAP_COUNT);
			TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_BOTH, 10000);
			TimerControlEvent(INPUT_34_TIMER_PORT, TIMER_BOTH, TIMER_EVENT_POS_EDGE);
			TimerEnable(INPUT_34_TIMER_PORT, TIMER_BOTH);
		}break;
	}
}
/*!
 * @fn uint32_t u32_get_di1_status(void)
 * @brief 
 * @param[in] None.
 * @return DI mode: return 1 if active else 0
* 				 Counter mode: return value of counter
 */
uint32_t u32_get_di1_status(void)
{
	if(e_dio_pin & INPUT_PIN_1)
		return au32_di_value[0];
	else
		return INPUT_1_READ();
}

/*!
 * @fn uint32_t u32_get_di2_status(void)
 * @brief 
 * @param[in] None.
 * @return DI mode: return 1 if active else 0
* 				 Counter mode: return value of counter
 */
uint32_t u32_get_di2_status(void)
{
	if(e_dio_pin & INPUT_PIN_2)
		return au32_di_value[1];
	else
		return INPUT_2_READ();
}

/*!
 * @fn uint32_t u32_get_di3_status(void)
 * @brief 
 * @param[in] None.
 * @return DI mode: return 1 if active else 0
* 				 Counter mode: return value of counter
 */
uint32_t u32_get_di3_status(void)
{
	if(e_dio_pin & INPUT_PIN_3)
		return au32_di_value[2];
	else
		return INPUT_3_READ();
}

/*!
 * @fn uint32_t u32_get_di4_status(void)
 * @brief 
 * @param[in] None.
 * @return DI mode: return 1 if active else 0
* 				 Counter mode: return value of counter
 */
uint32_t u32_get_di4_status(void)
{
	if(e_dio_pin & INPUT_PIN_4)
		return au32_di_value[3];
	else
		return INPUT_4_READ();
}

void v_get_all_di_status(uint32_t *status)
{
	status[0] = u32_get_di1_status();
	status[1] = u32_get_di2_status();
	status[2] = u32_get_di3_status();
	status[3] = u32_get_di4_status();													
}
/*!
 * @fn void v_di_get_value(uint32_t *u32_return_value)
 * @brief 
 * @param[in] pointer to return value
 * @return None.
 */

void v_di_get_value(uint32_t *u32_return_value)
{
	uint32_t u32_counter_value = 0;
	//memset(au32_di_value, 0, sizeof(au32_di_value));
	//input 1
	if(e_dio_pin & INPUT_PIN_1)
	{
		u32_counter_value = TimerValueGet(INPUT_12_TIMER_PORT, TIMER_A) & 0xFFFF;
		if(u32_counter_value < 10000)
		{
			au32_di_value[0] = (10000 - u32_counter_value);
			TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_A, 10000);	//reset counter
		}
	}
	//input 2
	if(e_dio_pin & INPUT_PIN_2)
	{
		
		u32_counter_value = TimerValueGet(INPUT_12_TIMER_PORT, TIMER_B);
		if(u32_counter_value < 10000)
		{
			au32_di_value[1] = (10000 - u32_counter_value);
			TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_B, 10000);	//reset counter
		}
	}
	//input 3
	if(e_dio_pin & INPUT_PIN_3)
	{
		#if INPUT_3_CNT_MANUAL
		#else
		u32_counter_value = TimerValueGet(INPUT_34_TIMER_PORT, TIMER_A);
		if(u32_counter_value < 10000)
		{
			au32_di_value[2] = (10000 - u32_counter_value);
			TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_A, 10000);	//reset counter
		}
		#endif
	}
	//input 4
	if(e_dio_pin & INPUT_PIN_4)
	{
		u32_counter_value = TimerValueGet(INPUT_34_TIMER_PORT, TIMER_B);
		if(u32_counter_value < 10000)
		{
			au32_di_value[3] = (10000 - u32_counter_value);
			TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_B, 10000);	//reset counter
		}
	}
	memcpy((void *)u32_return_value , (void *)au32_di_value, 4 * sizeof(uint32_t)); 
}

/*!
 * @fn double d_get_flow_meter (void)
 * @brief 
 * @param[in] none
 * @return total volume was counted by flow metter.
 */
double d_get_flow_meter(uint8_t u8_output_port)
{
	uint32_t u32_counter_value[4] = {0, 0, 0, 0};
	uint32_t u32_result = 0;
	v_di_get_value(u32_counter_value);
	for(uint8_t i = 0; i < MAX_INPUT_SETTING; i++)
	{
		STRU_PORT_INPUT stru_input_setting;
		if(b_input_setting_get(i, &stru_input_setting))
		{
			if((stru_input_setting.u8_map_output == u8_output_port) 
					&& (TYPE_I_COUNTER == stru_input_setting.e_input_type))
			{
				u32_result = (u32_counter_value[i] * stru_input_setting.d_multi_factor);
			}
		}
	}
	return u32_result;
}


/*!
 * @fn void v_clear_flow_meter (void)
 * @brief 
 * @param[in] none
 * @return none.
 */
void v_clear_flow_meter(uint8_t u8_output_port)
{
	#ifdef FS_MODE
		memset(au32_di_value, 0, sizeof(au32_di_value));
		TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_A, 10000);	//reset counter
		TimerLoadSet(INPUT_12_TIMER_PORT, TIMER_B, 10000);	//reset counter
		TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_A, 10000);	//reset counter
		TimerLoadSet(INPUT_34_TIMER_PORT, TIMER_B, 10000);	//reset counter
	#else
		for(uint8_t i = 0; i < MAX_INPUT_SETTING; i++)
		{	
			STRU_PORT_INPUT stru_input_setting;
			if(b_input_setting_get(i, &stru_input_setting))
			{
				if((stru_input_setting.u8_map_output == u8_output_port)
						&& (stru_input_setting.e_input_type == TYPE_I_COUNTER))
				{
					au32_di_value[i] = 0;
				}
			}	
			
		}
	#endif
	
}
