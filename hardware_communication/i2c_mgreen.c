/*! @file i2c_mgreen.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "driverlib.h"
#include "i2c_mgreen.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*!
* static data declaration
*/
static xSemaphoreHandle x_i2c_mgreen_mutex;
static StaticSemaphore_t xMutex_i2c_Buffer;
/*!
* private function prototype
*/
static int i16_i2c_take_semaphore(int block_time);
static int i16_i2c_give_semaphore(void);
static void v_relay_onboard_init (void);
/*!
* public function bodies
*/

void v_i2c_hardware_init (void)
{   
	//Init I2C reset pin
	SysCtlPeripheralEnable(I2C_MGREEN_RST_PER);
	while(!(SysCtlPeripheralReady(I2C_MGREEN_RST_PER)));	
	GPIOPinTypeGPIOOutput(I2C_MGREEN_RST_PORT, I2C_MGREEN_RST_PIN);
	GPIOPinWrite(I2C_MGREEN_RST_PORT, I2C_MGREEN_RST_PIN, I2C_MGREEN_RST_PIN);
	
	//Init I2C
	SysCtlPeripheralEnable(I2C_MGREEN_PER); 		// Enable I2C1 peripheral
	while(!(SysCtlPeripheralReady(I2C_MGREEN_PER)));
	SysCtlDelay(2); 					// Insert a few cycles after enabling the peripheral to allow the clock
														// to be fully activated

	SysCtlPeripheralEnable(I2C_MGREEN_SCL_SDA_PER); 		// Enable GPIOA peripheral
	while(!(SysCtlPeripheralReady(I2C_MGREEN_SCL_SDA_PER)));
	SysCtlDelay(2); 					// Insert a few cycles after enabling the peripheral to allow the clock
														// to be fully activated
	GPIOPinConfigure(I2C_MGREEN_SCL_AF);
	GPIOPinConfigure(I2C_MGREEN_SDA_AF);

	GPIOPinTypeI2CSCL(I2C_MGREEN_SCL_SDA_PORT, I2C_MGREEN_SCL_PIN); // Use pin with I2C SCL peripheral
	GPIOPinTypeI2C(I2C_MGREEN_SCL_SDA_PORT, I2C_MGREEN_SDA_PIN); 		// Use pin with I2C peripheral


	I2CMasterInitExpClk(I2C_MGREEN_BASE, SYSTEM_CLOCK_GET(), false); // Enable and set frequency to 400 kHz if 
																													// 3rd parameter  is true(100KHz if false)

	SysCtlDelay(2); 					// Insert a few cycles after enabling the I2C to allow the clock
														// to be fully activated	
	//Reset pin
	GPIOPinWrite(I2C_MGREEN_RST_PORT, I2C_MGREEN_RST_PIN, 0);
	SysCtlDelay(1200); 
	GPIOPinWrite(I2C_MGREEN_RST_PORT, I2C_MGREEN_RST_PIN, I2C_MGREEN_RST_PIN);
	
}
void v_i2c_software_init(void)
{
	/* Create i2c mutex. */
	x_i2c_mgreen_mutex = xSemaphoreCreateMutexStatic(&xMutex_i2c_Buffer);	
}

void v_i2c_hardware_reset(void)
{
	while(i16_i2c_take_semaphore(I2C_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(1);
	}	
	GPIOPinWrite(I2C_MGREEN_RST_PORT, I2C_MGREEN_RST_PIN, 0);
	vTaskDelay(500);
	GPIOPinWrite(I2C_MGREEN_RST_PORT, I2C_MGREEN_RST_PIN, I2C_MGREEN_RST_PIN);
	i16_i2c_give_semaphore();
}

bool b_i2c_mgreen_busy(uint32_t u32_timeout)
{
	uint32_t u32_i = 0;	
	for(u32_i = 0; u32_i < u32_timeout; u32_i++)
	{
		vTaskDelay(1);
		if(I2CMasterBusy(I2C_MGREEN_BASE) == false)
		{
			return true;
		}
	}
	return false;
}

/*************************************************************/
/*
 * Bref: write some bytes from master to slaver
 * Input parameter:
 * 		1. uint8_t addr : Slaver address
 * 		2. uint8_t regAddr: Slaver register address to write data
 * 		3. uint8_t *data: pointer to data will be write
 * 		4. uint8_t lenght: the number of byte to be write
 * Output parameter: None
 * Return: None
 * Notes:
 */
void v_i2c_mgreen_write_data(uint8_t u8_addr, uint8_t u8_reg_addr, 
														uint8_t *pu8_data, uint8_t u8_length)
{
	while(i16_i2c_take_semaphore(I2C_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(1);
	}	
	I2CMasterSlaveAddrSet(I2C_MGREEN_BASE, u8_addr, false); // Set to write mode

	I2CMasterDataPut(I2C_MGREEN_BASE, u8_reg_addr); // Place address into data register
	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_SEND_START); // Send start condition
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	uint8_t i = 0;
	for (i = 0; i < u8_length - 1; i++)
	{
			I2CMasterDataPut(I2C_MGREEN_BASE, pu8_data[i]); // Place data into data register
			I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_SEND_CONT); // Send continues condition
			b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	}

	I2CMasterDataPut(I2C_MGREEN_BASE, pu8_data[u8_length - 1]); // Place data into data register
	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); // Send finish condition
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	i16_i2c_give_semaphore();
}

/*********************************************************/
/*
 * Bref: write one byte from master to slaver
 * Input parameter:
 * 		1. uint8_t addr : Slaver address
 * 		2. uint8_t regAddr: Slaver register address to write data
 * 		3. uint8_t data: Data will be write
 * Output parameter: None
 * Return: None
 * Notes:
 */
void v_i2c_mgreen_write(uint8_t u8_addr, uint8_t u8_reg_addr, uint8_t u8_data)
{
    v_i2c_mgreen_write_data(u8_addr, u8_reg_addr, &u8_data, 1);
}
/*****************************************************************************/
uint8_t u8_i2c_mgreen_read(uint8_t u8_addr, uint8_t u8_reg_addr)
{
	while(i16_i2c_take_semaphore(I2C_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(1);
	}	
	I2CMasterSlaveAddrSet(I2C_MGREEN_BASE, u8_addr, false); // Set to write mode

	I2CMasterDataPut(I2C_MGREEN_BASE, u8_reg_addr); // Place address into data register
	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_SINGLE_SEND); // Send data
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done

	I2CMasterSlaveAddrSet(I2C_MGREEN_BASE, u8_addr, true); // Set to read mode

	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE); // Tell master to read data
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	i16_i2c_give_semaphore();
	return I2CMasterDataGet(I2C_MGREEN_BASE); // Read data
}

/**************************************************************************/
void v_i2c_mgreen_read_data(uint8_t u8_addr, uint8_t u8_reg_addr, 
														uint8_t *pu8_data, uint8_t u8_length)
{
	while(i16_i2c_take_semaphore(I2C_TAKE_SEMAPHORE_TIMEOUT))
	{
		vTaskDelay(1);
	}	
	I2CMasterSlaveAddrSet(I2C_MGREEN_BASE, u8_addr, false); // Set to write mode

	I2CMasterDataPut(I2C_MGREEN_BASE, u8_reg_addr); // Place address into data register
	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_SINGLE_SEND); // Send data
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done

	I2CMasterSlaveAddrSet(I2C_MGREEN_BASE, u8_addr, true); // Set to read mode

	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START); // Send start condition
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	pu8_data[0] = I2CMasterDataGet(I2C_MGREEN_BASE); // Place data into data register
	uint8_t i = 1;
	for (i = 1; i < u8_length - 1; i++) 
	{
		I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT); // Send continues condition
		b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
		pu8_data[i] = I2CMasterDataGet(I2C_MGREEN_BASE); // Place data into data register
	}

	I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH); // Send finish condition
	b_i2c_mgreen_busy(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	pu8_data[u8_length - 1] = I2CMasterDataGet(I2C_MGREEN_BASE); // Place data into data register
	i16_i2c_give_semaphore();
}

static int i16_i2c_take_semaphore(int block_time)
{
	if(x_i2c_mgreen_mutex != NULL)
	{
		if (xSemaphoreTake(x_i2c_mgreen_mutex, block_time) == pdTRUE)
		{
			return 0;
		}
		return -1;
	}
	else
		return 0;
}

static int i16_i2c_give_semaphore(void)
{
	if (xSemaphoreGive(x_i2c_mgreen_mutex) == pdTRUE)
	{
		return 0;
	}
	return -1;
}

static void v_relay_onboard_init (void)
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
