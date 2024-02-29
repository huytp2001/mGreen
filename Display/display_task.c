/*! @file display_task.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2020.  
* All rights reserved.
*/
/*!
* @include statements
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "driverlib.h"
#include "display_task.h"
#include "i2c_lcd.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/*!
* static data declaration
*/

static StaticTask_t xDisplayTaskBuffer;
static StackType_t  xDisplayStack[DISPLAY_TASK_STACK_SIZE];

static DISPLAY_INFO_STRUCT display_info;
static uint8_t u8_display_state = 0;
static char c_lcd_data[I2C_LCD_MAX_LEN];

/*!
* private function prototype
*/
static void v_display_task (void *pvParameters);

/*!
* public function bodies
*/
/*!
 * @fn void v_display_task_init (void)
 * @brief 
 * @param[in] none
 * @return none
 */
 void v_display_task_init (void)
{		
	xTaskCreateStatic(v_display_task,"display task", DISPLAY_TASK_STACK_SIZE,
							NULL, DISPLAY_TASK_PRIORITY, xDisplayStack, &xDisplayTaskBuffer);
}

void v_display_pH_2_LCD(uint16_t u16_pH_set, uint16_t u16_pH_current)
{
	display_info.u16_pH_current = u16_pH_current;
	display_info.u16_pH_set = u16_pH_set;
}
void v_display_waiting_LCD(void)
{
	display_info.u8_state = 1;
}
void v_display_no_schedule(void)
{
	display_info.u8_state = 0;
	u8_display_state = 0;
}
void v_display_active(void)
{
	display_info.u8_state = 2;
}
void v_display_active_zone(char *cname)
{
	memset((void *)display_info.cname, 0, I2C_LCD_MAX_LEN);
	memcpy((void *)display_info.cname, (void *)cname, I2C_LCD_MAX_LEN);
}
/*!
* private function bodies
*/
static void v_display_task (void *pvParameters)
{
	static uint8_t u8_ports_control_state_tmp = 0;
	//vTaskSetApplicationTaskTag( NULL, (TaskHookFunction_t)( void * ) TASK_TRACE_PORT_CONTROL_ID );
	#ifdef NEUTRALIZER_MODE
	i2c_lcd_init();
	#endif
	for(;;)
	{
		switch(u8_display_state)
		{
			case 0x00: 	//machine state
				switch(display_info.u8_state)
				{
					case 0x00: 	//stop						
						v_i2c_lcd_clear();
						memset((void *)c_lcd_data, 0, I2C_LCD_MAX_LEN);
						v_i2c_lcd_set_cursor(0, 0);
						snprintf(c_lcd_data, 20, "STOPPING...");
						i2c_lcd_write((const uint8_t*)c_lcd_data, strlen(c_lcd_data));
						break;
					case 0x01:	//waiting
						v_i2c_lcd_clear();
						memset((void *)c_lcd_data, 0, I2C_LCD_MAX_LEN);
						v_i2c_lcd_set_cursor(0, 0);
						snprintf(c_lcd_data, 20, "WAITING...");
						i2c_lcd_write((const uint8_t*)c_lcd_data, strlen(c_lcd_data));
						break;
					case 0x02:	//running
						v_i2c_lcd_clear();
						memset((void *)c_lcd_data, 0, I2C_LCD_MAX_LEN);
						v_i2c_lcd_set_cursor(0, 0);
						snprintf(c_lcd_data, 20, "RUNNING...");
						i2c_lcd_write((const uint8_t*)c_lcd_data, strlen(c_lcd_data));
						v_i2c_lcd_set_cursor(1, 1);
						i2c_lcd_write((const uint8_t*)display_info.cname, strlen(display_info.cname));
						//switch to next info
						u8_display_state++;
						break;
				}
				break;
			case 0xFF:	//no display
				v_i2c_lcd_clear();
				i2c_lcd_write((const uint8_t*)display_info.cname, strlen(display_info.cname));
				//switch to next info
				u8_display_state++;
				break;
			case 0x01:	//pH infomation
				v_i2c_lcd_clear();
				//pH set
				memset((void *)c_lcd_data, 0, I2C_LCD_MAX_LEN);
				snprintf(c_lcd_data, 20, "pH SET: %.2f", (float)display_info.u16_pH_set/100);
				v_i2c_lcd_set_cursor(0, 0);
				i2c_lcd_write((const uint8_t*)c_lcd_data, strlen(c_lcd_data));
				//pH current
				v_i2c_lcd_set_cursor(0, 1);
				memset((void *)c_lcd_data, 0, I2C_LCD_MAX_LEN);
				snprintf(c_lcd_data, 20, "pH : %.2f", (float)display_info.u16_pH_current/100);
				i2c_lcd_write((const uint8_t*)c_lcd_data, strlen(c_lcd_data));
				//switch to next info
				u8_display_state = 0;
				break;
		}
		vTaskDelay(DISPLAY_TASK_DELAY);
	}
}
