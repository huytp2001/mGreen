#include "FreeRTOS.h"
#include "task.h"

#include "soft_SPI.h"

//global functions
void vSoftSPI_Init(void)
{
	//spi1 init
	SPI1_CLK_ENABLE();
	ROM_GPIOPinTypeGPIOOutput(SPI1_CS_PORT, SPI1_CS_PIN);
	ROM_GPIOPinTypeGPIOOutput(SPI1_MOSI_PORT, SPI1_MOSI_PIN);
	ROM_GPIOPinTypeGPIOOutput(SPI1_CLK_PORT, SPI1_CLK_PIN);
	ROM_GPIOPinTypeGPIOInput(SPI1_MISO_PORT, SPI1_MISO_PIN);
	//spi2 init
	SPI2_CLK_ENABLE();
	ROM_GPIOPinTypeGPIOOutput(SPI2_CS_PORT, SPI2_CS_PIN);
	ROM_GPIOPinTypeGPIOOutput(SPI2_MOSI_PORT, SPI2_MOSI_PIN);
	ROM_GPIOPinTypeGPIOOutput(SPI2_CLK_PORT, SPI2_CLK_PIN);
	ROM_GPIOPinTypeGPIOInput(SPI2_MISO_PORT, SPI2_MISO_PIN);
	//spi3 init
//	SPI3_CLK_ENABLE();
//	ROM_GPIOPinTypeGPIOOutput(SPI3_CS_PORT, SPI3_CS_PIN);
//	ROM_GPIOPinTypeGPIOOutput(SPI3_MOSI_PORT, SPI3_MOSI_PIN);
//	ROM_GPIOPinTypeGPIOOutput(SPI3_CLK_PORT, SPI3_CLK_PIN);
//	ROM_GPIOPinTypeGPIOOutput(SPI3_MISO_PORT, SPI3_MISO_PIN);
}
/*TNPHU: add enter/exit critical when starting Soft SPI */
uint8_t u8SpiTxRxByte(uint8_t data, uint8_t spi_module)
{
	uint8_t i,temp;
	temp=0;
	taskENTER_CRITICAL();
	switch(spi_module)
	{
		case SOFT_SPI_1:
		{
			CLK_1_ON();
			for(i=0;i<8;i++)
			{
				if(data&0x80)
				{
					MOSI_1_ON();
				}
				else	MOSI_1_OFF();
				data<<=1;
				delayCLK();
				CLK_1_OFF();
				delayCLK();
				temp<<=1;
				//a=MISORead;
				if(MISO_1_READ())
					temp++;
				CLK_1_ON();
				delayCLK();
			}
		}
		break;
		case SOFT_SPI_2:
		{
			CLK_2_ON();
			for(i=0;i<8;i++)
			{
				if(data&0x80)
				{
					MOSI_2_ON();
				}
				else	MOSI_2_OFF();
				data<<=1;
				delayCLK();
				CLK_2_OFF();
				delayCLK();
				temp<<=1;
				//a=MISORead;
				if(MISO_2_READ())
					temp++;
				CLK_2_ON();
				delayCLK();
			}
		}
		break;
	}
	taskEXIT_CRITICAL();
	return temp;
}
