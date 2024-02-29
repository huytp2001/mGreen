#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/i2c.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib.h"
#include "I2Clib.h"

void v_sensor_i2c_init(void)
{
	// Enable the peripherals
	SENSOR_I2Cx_CLK_ENABLE();
	SENSOR_I2Cx_SDA_GPIO_CLK_ENABLE();
	SENSOR_I2Cx_SCL_GPIO_CLK_ENABLE();
	// Initialize the I2C master - Transfer data at 100 Kbps
	I2CMasterInitExpClk(SENSOR_I2Cx, SYSTEM_CLOCK_GET(), false);
	// Configure the appropriate pins to be I2C instead of GPIO.
	GPIOPinTypeI2C(SENSOR_I2Cx_SDA_GPIO_PORT, SENSOR_I2Cx_SDA_PIN);
	GPIOPinTypeI2C(SENSOR_I2Cx_SCL_GPIO_PORT, SENSOR_I2Cx_SCL_PIN);
	GPIOPadConfigSet(SENSOR_I2Cx_SDA_GPIO_PORT, SENSOR_I2Cx_SDA_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);//STD
	GPIOPadConfigSet(SENSOR_I2Cx_SCL_GPIO_PORT, SENSOR_I2Cx_SCL_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_OD);
	I2CMasterEnable(SENSOR_I2Cx);
}

void v_sensor_i2c_write(unsigned char SlaveAddress, unsigned char *ucData, unsigned int uiCount, unsigned char ucStart_add )
{
	// Set the slave address and setup for a transmit operation.
	I2CMasterSlaveAddrSet(SENSOR_I2Cx, SlaveAddress, false);
	// Place the address to be written in the data register.
	I2CMasterDataPut(SENSOR_I2Cx, ucStart_add);
	if (uiCount == 0)
		// Initiate send of character from Master to Slave
		I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_SINGLE_SEND);
	else
	{
		// Start the burst cycle, writing the address as the first byte.
		I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_BURST_SEND_START);
		// Write the next byte to the data register.
		while (I2CMasterBusy(SENSOR_I2Cx))
		{
		}
		I2CMasterDataPut(SENSOR_I2Cx, *ucData++);
		uiCount--;
		for( ; uiCount > 0; uiCount--)        //Loop to send data if not the last byte
		{
			// Continue the burst write.
			I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_BURST_SEND_CONT);
			// Write the next byte to the data register.
			while (I2CMasterBusy(SENSOR_I2Cx));
			I2CMasterDataPut(SENSOR_I2Cx, *ucData++);
		}
		// Finish the burst write.
		I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_BURST_SEND_FINISH);
	}
	while (I2CMasterBusy(SENSOR_I2Cx))
	{
	}
	//while (!(I2CMasterErr(SENSOR_I2Cx) == I2C_MASTER_ERR_NONE));

}

void v_sensor_i2c_read(unsigned char SlaveAddress, unsigned char *ucRec_Data, unsigned int uiCount, unsigned char ucStart_add)
{
 	// Set the slave address and setup for a transmit operation.
	I2CMasterSlaveAddrSet(SENSOR_I2Cx, SlaveAddress, false);
	// Place the address to be written in the data register.
	I2CMasterDataPut(SENSOR_I2Cx, ucStart_add);
	// Start the burst cycle, writing the address as the first byte.
	I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_SINGLE_SEND);

	while (I2CMasterBusy(SENSOR_I2Cx))
	{
	}
	//while (!(I2CMasterErr(SENSOR_I2Cx) == I2C_MASTER_ERR_NONE));

 	I2CMasterSlaveAddrSet(SENSOR_I2Cx, SlaveAddress, true);
 	if (uiCount == 1)
 	{
 		I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_SINGLE_RECEIVE);
 		while (I2CMasterBusy(SENSOR_I2Cx))
		{
		}
 		*ucRec_Data  = I2CMasterDataGet(SENSOR_I2Cx);
 	}

 	else
 	{
 		I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_BURST_RECEIVE_START);
 		while (I2CMasterBusy(SENSOR_I2Cx));
 		uiCount--;
 		*ucRec_Data++  = I2CMasterDataGet(SENSOR_I2Cx);
 		for ( ; uiCount > 1; uiCount--)
 		{
 			I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
 			while (I2CMasterBusy(SENSOR_I2Cx))
			{
			}
 			*ucRec_Data++  = I2CMasterDataGet(SENSOR_I2Cx);
 		}
 		I2CMasterControl(SENSOR_I2Cx, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
 		while (I2CMasterBusy(SENSOR_I2Cx))
		{
		}
		*ucRec_Data  = I2CMasterDataGet(SENSOR_I2Cx);
 	}

 	while(I2CMasterIntStatus(SENSOR_I2Cx, false) == 0)
 	{
 	}
 	while (I2CMasterBusy(SENSOR_I2Cx))
	{
	}
}
