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
 * @brief Manage functionalities of LORA Module.
 */
/*!
 * Add more include here
 */
#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "rf.h"
#include "lora.h"
#include "cc1310.h"
#include "wdt.h"
#include "HAL_BSP.h"
#include "rtc.h"
#include "setting.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "frame_parser.h"

#include "timers.h"

#define DEBUG

/*!
* @def USE_RF_FLAG
* Use RF or not
*/
#define USE_RF_FLAG								USE_RF //(stru_neces_params_get().b_is_use_RF)
/*!
* @def USE_LORA_FLAG
* Use LORA (1) or CC1310 (0)
*/
#define USE_LORA_FLAG							RF_TYPE //(stru_neces_params_get().e_rf_type)
/*!
*	Variables declare
*/
xQueueHandle q_rf_in;
xQueueHandle q_rf_out;
portBASE_TYPE x_higher_prio_task = pdFALSE;
/*!
*	Private variables
*/
static mqtt_data_frame_t stru_rf_data;
static uint8_t au8_rf_data[RF_BUF_MAX];
static uint8_t u8_rf_rssi = 0;
static uint8_t ui8_rf_task_wdt_id = 0;
static bool b_process_data = false;
static uint32_t RF_UART_BAUDRATE	= 0;
static bool _RF_COMPACT_FRAME_ = false;
uint8_t package[16] = {0x01, 0x01, 0x06, 0x0D, 0x43, 0x01, 0x01, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7D};
/*!
*	Functions prototype
*/
static void v_rf_uart_init (void);
static void v_rf_gpio_init (void);
static void v_rf_task (void *pvParameters);
static void v_rf_frame_process (void);
static void v_parse_value_to_array(uint8_t* , uint32_t);

StaticQueue_t x_qrf_in_queue_buffer;
uint8_t uc_qrf_in_storage[RF_QUEUE_LEN * RF_BUF_MAX];

StaticQueue_t uc_qrf_out_queue_buffer;
uint8_t uc_qrf_out_storage[RF_QUEUE_LEN * RF_BUF_MAX];

static StaticTask_t xRF_TaskBuffer;
static StackType_t  xRF_Stack[RF_TASK_STACK_SIZE];

/*!
*	Public functions 
*/

/*!
* @fn v_rf_task_init(void)
* @brief Setup and init 1 task and 2 queues. 
* @param[in] None
* @return None
*/
void v_rf_task_init (void)
{
	q_rf_in = xQueueCreateStatic(RF_QUEUE_LEN, sizeof(au8_rf_data), &( uc_qrf_in_storage[ 0 ]), &x_qrf_in_queue_buffer);			
	q_rf_out = xQueueCreateStatic(RF_QUEUE_LEN, sizeof(au8_rf_data), &( uc_qrf_out_storage[ 0 ]), &uc_qrf_out_queue_buffer);

	//create RF task
	xTaskCreateStatic(v_rf_task, "RF_TASK", RF_TASK_STACK_SIZE,
								NULL, RF_TASK_PRIORITY, xRF_Stack, &xRF_TaskBuffer);
	
}

/*!
* @fn v_rf_clear_higher_prio_task (void)
* @brief Clear higher prio task by entering ISR
* @param[in] None
* @return None
*/
void v_rf_clear_higher_prio_task (void)
{
	x_higher_prio_task = pdFALSE;
}

/*!
* @fn v_rf_task_init(void)
* @brief Switch to higher prio task from an ISR
* @param[in] None
* @return None
*/
void v_rf_end_switching_context (void)
{
	portEND_SWITCHING_ISR(x_higher_prio_task);	
}

/*!
* @fn b_write_frame_to_rf_from_isr(uint8_t* pu8_data_frame)
* @brief Write frame to RF from ISR through q_rf_out queue
* @param[in] pu8_data_frame Pointer to data frame
* @return None
*/
bool b_write_frame_to_rf_from_isr(uint8_t* pu8_data_frame)
{
	if (xQueueSendFromISR(q_rf_out, pu8_data_frame, NULL) == pdTRUE)
	{
		return true;
	}
	return false;
}

/*!
* @fn b_write_frame_to_rf (uint8_t* pu8_data_frame)
* @brief Write frame to RF outside ISR through q_rf_out queue
* @param[in] pu8_data_frame Pointer to data frame
* @return None
*/
bool b_write_frame_to_rf (uint8_t* pu8_data_frame)
{	
	if (xQueueSend(q_rf_out, pu8_data_frame, RF_TASK_DELAY) == pdTRUE)	
	{
		return true;
	}
	return false;
}

/*!
* @fn b_rf_has_frame (void)
* @brief Check if there is any data frame in q_rf_out queue
* @param[in] None
* @return None
*/
bool b_rf_has_frame (void)
{
	if(uxQueueMessagesWaiting(q_rf_out))
		return true;
	return false;
}

/*!
* @fn b_get_frame_from_rf (uint8_t* pu8_data_frame)
* @brief If there is any frame in queue then get it.
* @param[in] None
* @return None
*/
bool b_get_frame_from_rf (uint8_t* pu8_data_frame)
{
	if(b_rf_has_frame() == false)
		return false;
	if(xQueueReceive(q_rf_out, pu8_data_frame, 0) == pdPASS)
		return true;
	return false;
}

/*!
* @fn v_rf_set_rssi (uint8_t u8_rssi)
* @brief Get RSSI value
* @param[in] u8_rssi RSSI value
* @return None
*/
void v_rf_set_rssi (uint8_t u8_rssi)
{
	u8_rf_rssi = u8_rssi;
}

/*!
* @fn u8_rf_get_rssi (void)
* @brief Return stored RSSI value.
* @param[in] None
* @return None
*/
uint8_t u8_rf_get_rssi (void)
{
	return u8_rf_rssi;
}

/*!
* @fn v_process_rf_isr( void *pvParam1, uint32_t ulParam2 )
* @brief For CC1310. CC1310 have interrupt pin, trigger this funcion.
* @param[in] None
* @param[in] u32_param2 Last interrupt status
* @return None
*/
static void v_process_rf_isr( void *pvParam1, uint32_t ulParam2 )
{
	uint32_t u32_last_int_status = ulParam2;
	uint32_t a = GPIOPinRead(RF_AUX_PORT, RF_AUX_PIN);
	if(a == u32_last_int_status)
	{
		if(USE_LORA_FLAG == 0)
		{
			b_process_data = true;
		}			
	}
}

/*!
* @fn v_rf_task_init(void)
* @brief Auxillary ISR, call RF frame proccessing function 
* @param[in] None
* @return None
*/
void v_rf_aux_isr (void)
{
	v_rf_clear_higher_prio_task();	
	//Get the current interrupt status
	uint32_t u32_interrupt_status = GPIOIntStatus(RF_AUX_PORT, true);
	uint32_t i32_interrupt_value = GPIOPinRead(RF_AUX_PORT, RF_AUX_PIN);
	
	// INT_PIN_3 (RF_AUX_INTERRUPT) is interrupted then 
	if((u32_interrupt_status & RF_AUX_INTERRUPT) == RF_AUX_INTERRUPT)
	{
		GPIOIntClear(RF_AUX_PORT, RF_AUX_INTERRUPT);
		// Defer a long proccessing function to RTOS Deamon. 
		v_process_rf_isr( NULL, i32_interrupt_value);
	}
	/* We can switch context if necessary. */
	v_rf_end_switching_context();		
}

/*!
*	Private functions
*/

/*!
* @fn v_rf_uart_init (void)
* @brief Setup UART for RF
* @param[in] None
* @return None
*/
static void v_rf_uart_init (void)
{
	SysCtlPeripheralEnable(RF_UART_PER);
	while(!(SysCtlPeripheralReady(RF_UART_PER)));	
	
	SysCtlPeripheralEnable(RF_UART_RXTX_PER);
	while(!(SysCtlPeripheralReady(RF_UART_RXTX_PER)));
	
	GPIOPinConfigure(RF_UART_RX);
	GPIOPinConfigure(RF_UART_TX);
	GPIOPinTypeUART(RF_UART_PORT, RF_UART_RX_PIN | RF_UART_TX_PIN);
	UARTConfigSetExpClk(RF_UART_BASE, SYSTEM_CLOCK_GET(), RF_UART_BAUDRATE, 
	UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
	SysCtlDelay(2); 					// Insert a few cycles after enabling the UART to allow the clock
										// to be fully activated		
	UARTEnable(RF_UART_BASE);
}

/*!
* @fn v_rf_gpio_init (void)
* @brief Setup GPIOs
* @param[in] None
* @return None
*/
static void v_rf_gpio_init (void)
{
	SysCtlPeripheralEnable(RF_SETA_PER);
	while(!(SysCtlPeripheralReady(RF_SETA_PER)));				
	GPIOPinTypeGPIOOutput(RF_SETA_PORT, RF_SETA_PIN);
	
	SysCtlPeripheralEnable(RF_SETB_PER);
	while(!(SysCtlPeripheralReady(RF_SETB_PER)));			
	GPIOPinTypeGPIOOutput(RF_SETB_PORT, RF_SETB_PIN);

	SysCtlPeripheralEnable(RF_AUX_PER);
	while(!(SysCtlPeripheralReady(RF_AUX_PER)));			
	GPIOPinTypeGPIOInput(RF_AUX_PORT, RF_AUX_PIN);
	GPIOPadConfigSet(RF_AUX_PORT, RF_AUX_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);	
	GPIOIntTypeSet(RF_AUX_PORT, RF_AUX_PIN, GPIO_BOTH_EDGES);
	
	GPIOIntRegister(RF_AUX_PORT, &v_rf_aux_isr);	
	//ROM_IntPrioritySet(RF_AUX_INT_PARAMS, (5<<5));
	ROM_IntPrioritySet(RF_AUX_INTERRUPT, (5<<5));
	GPIOIntEnable(RF_AUX_PORT, RF_AUX_PIN);
			
}

/*!
* @fn v_rf_task(void)
* @brief Main RF task.
* @param[in] None
* @return None
*/
static void v_rf_task (void *pvParameters)
{
	while (b_wdt_reg_new_task("v_rf_task", &ui8_rf_task_wdt_id) != true)
	{
	}
	if(1 == USE_RF_FLAG)
	{
		if(RF_TYPE_LORA == USE_LORA_FLAG)
		{
			_RF_COMPACT_FRAME_ = 1;
			RF_UART_BAUDRATE = 9600;
		}
		else
		{
			_RF_COMPACT_FRAME_ = 0;
			RF_UART_BAUDRATE = 115200;
		}
		v_rf_uart_init();
		v_rf_gpio_init();

	}
	if (1 == USE_RF_FLAG)
	{
		if(USE_LORA_FLAG == 1)
		{
			v_lora_e32_init();	
		}
		else if(USE_LORA_FLAG == 0)
		{	
			v_cc1310_init();
		}
	}
	for (;;) // loop
	{
		// Register WDT for this task
		b_wdt_task_reload_counter(ui8_rf_task_wdt_id);
		/*Parse data from nodes */
		if(1 == USE_RF_FLAG)
		{
			if(1 == USE_LORA_FLAG)
			{
				v_lora_process_mess();
			}
			else
			{
				if(b_process_data)
				{
					b_process_data = false;
					v_cc1310_process_data();
				}
			}
			/*Send data to server */
			if (b_get_frame_from_rf(au8_rf_data) == true)
			{
				v_rf_frame_process();
			}
		}
		
		if (GPIOPinRead(LED_DB_1_PORT, LED_DB_1_PIN) == 0) {
			GPIOPinWrite(LED_DB_1_PORT, LED_DB_1_PIN, 1);
		} else {
			GPIOPinWrite(LED_DB_1_PORT, LED_DB_1_PIN, 0);
		}
		
		v_lora_send_package(package, 16);
		vTaskDelay(5000);
		
		vTaskDelay(RF_TASK_DELAY);		
	}
}

/*!
* @fn v_parse_value_to_array
* @brief Parse 32 bit value to 8-bit/element array. 
* @param[in] None
* @return None
*/
static void v_parse_value_to_array(uint8_t* u8_out_array, uint32_t u32_in_value)
{
	u8_out_array[0] = u32_in_value>>24 & 0xFF;
	u8_out_array[1] = u32_in_value>>16 & 0xFF;
	u8_out_array[2] = u32_in_value>>8 & 0xFF;
	u8_out_array[3] = u32_in_value & 0xFF;
}

/*!
* @fn v_rf_frame_process (void)
* @brief Processing frames after get from queue
* @param[in] None
* @return None
*/
static void v_rf_frame_process (void)
{
	/*!
	* RF frame struct: MSB
	* |Len  |FrameType|Node |NumData|DataField|...|SumData|
	* |1byte|1byte    |5byte|1byte  |6byte    |...|2byte  |
	*
	* |Node             |
	* |NodeTypeID|NodeID|
	* |1byte     |4byte |
	*
	* |DataField              |
	* |DataTypeID|DataID|Value|
	* |1byte     |1byte |4byte|
	*/
	if(_RF_COMPACT_FRAME_ == 1)
	{
		uint32_t u32_tmp = 0;
		
		stru_rf_data.u8_frame_type = FRAME_TYPE_DATA_NODE;
		stru_rf_data.u8_node_type_id = au8_rf_data[1];
		
		u32_tmp = ((au8_rf_data[2] << 8) | (au8_rf_data[3]));
		stru_rf_data.u32_node_id = u32_tmp;
		
		stru_rf_data.u8_num_data_field = au8_rf_data[4];

		if (stru_rf_data.u8_num_data_field > 0)
		{
			for (uint8_t u8_i = 0; u8_i < stru_rf_data.u8_num_data_field; u8_i++)
			{
				stru_rf_data.stru_data_field[u8_i].u8_type_id = DATATYPEID_SENSOR;
				stru_rf_data.stru_data_field[u8_i].u16_id = (au8_rf_data[5 + 4 * u8_i] << 8) | (au8_rf_data[6 + 4 * u8_i]);
				
				stru_rf_data.stru_data_field[u8_i].u8_value_len = 4;
				
				u32_tmp = (au8_rf_data[7 + 4 * u8_i] << 8) | (au8_rf_data[8 + 4 * u8_i]);	
				v_parse_value_to_array( stru_rf_data.stru_data_field[u8_i].au8_payload, u32_tmp);				
			}
		}

		if(USE_LORA_FLAG == 1)
		{
			stru_rf_data.u8_rssi = 0;
		}
		else if(USE_LORA_FLAG == 0)
		{
			stru_rf_data.u8_rssi = au8_rf_data[5 + stru_rf_data.u8_num_data_field * 4 + 2 + 1];	
		}	
	}
	else if(_RF_COMPACT_FRAME_ == 0)		//NO_RF_COMPACT_FRAME
	{
		uint32_t u32_tmp = 0;
		if(USE_LORA_FLAG ==0)
		{
			stru_rf_data.u8_frame_type = au8_rf_data[1];
			stru_rf_data.u8_node_type_id = au8_rf_data[2];
			u32_tmp = (au8_rf_data[3] << 24) | (au8_rf_data[4] << 16) |
								(au8_rf_data[5] << 8) | (au8_rf_data[6]);
			stru_rf_data.u32_node_id= u32_tmp;
			
			stru_rf_data.u8_num_data_field = au8_rf_data[7];
			// Get RSSI
			// 2-byte CheckSum
			// 1-byte status
			stru_rf_data.u8_rssi = au8_rf_data[8 + stru_rf_data.u8_num_data_field * MQTT_DATA_FIELD_LEN_IN_BYTES + 2 + 1];
			
			if (stru_rf_data.u8_num_data_field > 0)
			{
				for (uint8_t u8_i = 0; u8_i < stru_rf_data.u8_num_data_field; u8_i++)
				{
					stru_rf_data.stru_data_field[u8_i].u8_type_id = au8_rf_data[8 + 8 * u8_i];
					stru_rf_data.stru_data_field[u8_i].u16_id = (au8_rf_data[9 + 8 * u8_i] << 8) | (au8_rf_data[10 + 8 * u8_i]);
					
					stru_rf_data.stru_data_field[u8_i].u8_value_len = au8_rf_data[11 + 8 * u8_i];
					
					u32_tmp = (au8_rf_data[12 + 8 * u8_i] << 24) | (au8_rf_data[13 + 8 * u8_i] << 16) |
										(au8_rf_data[14 + 8 * u8_i] << 8) | (au8_rf_data[15 + 8 * u8_i]);
					v_parse_value_to_array(stru_rf_data.stru_data_field[u8_i].au8_payload, u32_tmp);
				}
			}
		}
		else if(USE_LORA_FLAG == 1)
		{
			stru_rf_data.u8_frame_type = au8_rf_data[3];
			stru_rf_data.u8_node_type_id = NODETYPEID_SOIMOITURE;
			
			u32_tmp = (au8_rf_data[1] << 8) | (au8_rf_data[2]);
			stru_rf_data.u32_node_id = u32_tmp;
			
			stru_rf_data.u8_num_data_field = (au8_rf_data[0] - 6)/3;
			
			// Get RSSI
			// 2-byte checkSum
			// 1-byte status
			stru_rf_data.u8_rssi = au8_rf_data[au8_rf_data[0] + 1];
			
			if (stru_rf_data.u8_num_data_field > 0)
			{
				for (uint8_t u8_i = 0; u8_i < stru_rf_data.u8_num_data_field; u8_i++)
				{
					stru_rf_data.stru_data_field[u8_i].u8_type_id = DATATYPEID_SENSOR;
					stru_rf_data.stru_data_field[u8_i].u16_id = (au8_rf_data[6 + 3 * u8_i]);
					
					stru_rf_data.stru_data_field[u8_i].u8_value_len = 4;
					
					u32_tmp = (au8_rf_data[7 + 3 * u8_i] << 8) | (au8_rf_data[8 + 3 * u8_i]);
					v_parse_value_to_array(stru_rf_data.stru_data_field[u8_i].au8_payload, u32_tmp);
				}
			}
		}	
	}
	
	// Send this frame to mqtt
	stru_rf_data.u16_seq_number++;
	stru_rf_data.u32_unix_time = u32_rtc_unix_time_get();
	/* Send frame */
	mqtt_t stru_mqtt;
	v_build_mqtt_struct(TOPIC_MONITOR, 0, 0, &stru_rf_data, &stru_mqtt);
	b_write_mqtt_pub_to_server(&stru_mqtt);
	//toggle led
	if(stru_rf_data.u16_seq_number % 2)
	{
		//LED_DB1_ON();
	}
	else
	{
		//LED_DB1_OFF();
	}
}
