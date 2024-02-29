/*! @file spi_mgreen.c
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
#include "driverlib.h"
#include "spi_mgreen.h"

void v_spi_mgreen_init(void)
{
	SysCtlPeripheralEnable(SPI_PH_CLK_PER);
	while(!(SysCtlPeripheralReady(SPI_PH_CLK_PER)));	
	GPIOPinTypeGPIOOutput(SPI_PH_CLK_PORT,SPI_PH_CLK_PIN);

	SysCtlPeripheralEnable(SPI_PH_MOSI_PER);
	while(!(SysCtlPeripheralReady(SPI_PH_MOSI_PER)));		
	GPIOPinTypeGPIOOutput(SPI_PH_MOSI_PORT,SPI_PH_MOSI_PIN);

	SysCtlPeripheralEnable(SPI_PH_MISO_PER);
	while(!(SysCtlPeripheralReady(SPI_PH_MISO_PER)));		
	GPIOPinTypeGPIOInput(SPI_PH_MISO_PORT,SPI_PH_MISO_PIN);
	
	SysCtlPeripheralEnable(SPI_PH_CS_PER);
	while(!(SysCtlPeripheralReady(SPI_PH_CS_PER)));	
	GPIOPinTypeGPIOOutput(SPI_PH_CS_PORT,SPI_PH_CS_PIN);
	//
	SysCtlPeripheralEnable(SPI_EC_CLK_PER);
	while(!(SysCtlPeripheralReady(SPI_EC_CLK_PER)));	
	GPIOPinTypeGPIOOutput(SPI_EC_CLK_PORT,SPI_EC_CLK_PIN);

	SysCtlPeripheralEnable(SPI_EC_MOSI_PER);
	while(!(SysCtlPeripheralReady(SPI_EC_MOSI_PER)));		
	GPIOPinTypeGPIOOutput(SPI_EC_MOSI_PORT,SPI_EC_MOSI_PIN);

	SysCtlPeripheralEnable(SPI_EC_MISO_PER);
	while(!(SysCtlPeripheralReady(SPI_EC_MISO_PER)));		
	GPIOPinTypeGPIOInput(SPI_EC_MISO_PORT,SPI_EC_MISO_PIN);
	
	SysCtlPeripheralEnable(SPI_EC_CS_PER);
	while(!(SysCtlPeripheralReady(SPI_EC_CS_PER)));	
	GPIOPinTypeGPIOOutput(SPI_EC_CS_PORT,SPI_EC_CS_PIN);

	PH_CS_ON();
	EC_CS_ON();
	PH_CLK_ON();		
	EC_CLK_ON();
}

uint8_t u8_spi_mgreen_tx_rx_byte(uint8_t u8_data, bool b_is_ph)
{
	uint8_t u8_tmp = 0;
	if(b_is_ph)
	{
		PH_CLK_ON();
		for(uint8_t i=0; i<8; i++)
		{
			if(u8_data & 0x80)
			{
				PH_MOSI_ON();
			}
			else	
			{
				PH_MOSI_OFF();
			}
			
			u8_data <<= 1;
			delayCLK();
			PH_CLK_OFF();
			delayCLK();
			u8_tmp <<= 1;

			if(PH_MISO_READ())
			{
				u8_tmp++;
			}
			PH_CLK_ON();
			delayCLK();
		}
	}
	else
	{
		EC_CLK_ON();
		for(uint8_t i=0; i<8; i++)
		{
			if(u8_data & 0x80)
			{
				EC_MOSI_ON();
			}
			else	
			{
				EC_MOSI_OFF();
			}
			
			u8_data <<= 1;
			delayCLK();
			EC_CLK_OFF();
			delayCLK();
			u8_tmp <<= 1;

			if(EC_MISO_READ())
			{
				u8_tmp++;
			}
			EC_CLK_ON();
			delayCLK();
		}		
	}
	return u8_tmp;	
}
