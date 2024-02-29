#ifndef __SOFT_SPI_H_
#define __SOFT_SPI_H_

#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"
#include "HAL_BSP.h"

#define SOFT_SPI_1				0
#define SOFT_SPI_2				1
#define SOFT_SPI_3				2

//SPI1
	#define SPI1_CLK_ENABLE() 	ROM_SysCtlPeripheralEnable(SPI1_CS_SYS_ADDR); ROM_SysCtlPeripheralEnable(SPI1_MOSI_SYS_ADDR); ROM_SysCtlPeripheralEnable(SPI1_MISO_SYS_ADDR); ROM_SysCtlPeripheralEnable(SPI1_CLK_SYS_ADDR)
	#define SPI1_CS_PORT				SPI1_CS_PORT_ADDR
	#define SPI1_CS_PIN					SPI1_CS_PIN_ADDR

	#define SPI1_MOSI_PORT			SPI1_MOSI_PORT_ADDR
	#define SPI1_MOSI_PIN				SPI1_MOSI_PIN_ADDR

	#define SPI1_MISO_PORT			SPI1_MISO_PORT_ADDR
	#define SPI1_MISO_PIN				SPI1_MISO_PIN_ADDR

	#define SPI1_CLK_PORT				SPI1_CLK_PORT_ADDR
	#define SPI1_CLK_PIN				SPI1_CLK_PIN_ADDR
//SPI2
	#define SPI2_CLK_ENABLE() 	ROM_SysCtlPeripheralEnable(SPI2_CS_SYS_ADDR); ROM_SysCtlPeripheralEnable(SPI2_MOSI_SYS_ADDR); ROM_SysCtlPeripheralEnable(SPI2_MISO_SYS_ADDR); ROM_SysCtlPeripheralEnable(SPI2_CLK_SYS_ADDR)
	#define SPI2_CS_PORT				SPI2_CS_PORT_ADDR
	#define SPI2_CS_PIN					SPI2_CS_PIN_ADDR

	#define SPI2_MOSI_PORT			SPI2_MOSI_PORT_ADDR
	#define SPI2_MOSI_PIN				SPI2_MOSI_PIN_ADDR

	#define SPI2_MISO_PORT			SPI2_MISO_PORT_ADDR
	#define SPI2_MISO_PIN				SPI2_MISO_PIN_ADDR

	#define SPI2_CLK_PORT				SPI2_CLK_PORT_ADDR
	#define SPI2_CLK_PIN				SPI2_CLK_PIN_ADDR

//SPI 3 PORT-PIN
//#define SPI3_CLK_ENABLE()			ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE)
//#define SPI3_CS_PORT					GPIO_PORTB_BASE
//#define SPI3_CS_PIN						GPIO_PIN_4

//#define SPI3_CLK_PORT					GPIO_PORTB_BASE
//#define SPI3_CLK_PIN					GPIO_PIN_5

//#define SPI3_MOSI_PORT				GPIO_PORTE_BASE
//#define SPI3_MOSI_PIN					GPIO_PIN_4

//#define SPI3_MISO_PORT				GPIO_PORTE_BASE
//#define SPI3_MISO_PIN					GPIO_PIN_5

//SPI 1 defination
#define MOSI_1_ON()								ROM_GPIOPinWrite(SPI1_MOSI_PORT, SPI1_MOSI_PIN, SPI1_MOSI_PIN)
#define MOSI_1_OFF()							ROM_GPIOPinWrite(SPI1_MOSI_PORT, SPI1_MOSI_PIN, 0)
#define MOSI_1_READ()							(ROM_GPIOPinRead(SPI1_MOSI_PORT, SPI1_MOSI_PIN))

#define MISO_1_ON()								ROM_GPIOPinWrite(SPI1_MISO_PORT, SPI1_MISO_PIN, SPI1_MISO_PIN)
#define MISO_1_OFF()							ROM_GPIOPinWrite(SPI1_MISO_PORT, SPI1_MISO_PIN, 0)
#define MISO_1_READ()							(ROM_GPIOPinRead(SPI1_MISO_PORT, SPI1_MISO_PIN))		

#define CLK_1_ON()								ROM_GPIOPinWrite(SPI1_CLK_PORT, SPI1_CLK_PIN, SPI1_CLK_PIN)		
#define CLK_1_OFF()								ROM_GPIOPinWrite(SPI1_CLK_PORT, SPI1_CLK_PIN, 0)

#define CS_1_ON()									ROM_GPIOPinWrite(SPI1_CS_PORT, SPI1_CS_PIN, SPI1_CS_PIN)	
#define CS_1_OFF()								ROM_GPIOPinWrite(SPI1_CS_PORT, SPI1_CS_PIN, 0)
//SPI2 defination
#define MOSI_2_ON()								ROM_GPIOPinWrite(SPI2_MOSI_PORT, SPI2_MOSI_PIN, SPI2_MOSI_PIN)
#define MOSI_2_OFF()							ROM_GPIOPinWrite(SPI2_MOSI_PORT, SPI2_MOSI_PIN, 0)
#define MOSI_2_READ()							(ROM_GPIOPinRead(SPI2_MOSI_PORT, SPI2_MOSI_PIN))

#define MISO_2_ON()								ROM_GPIOPinWrite(SPI2_MISO_PORT, SPI2_MISO_PIN, SPI2_MISO_PIN)
#define MISO_2_OFF()							ROM_GPIOPinWrite(SPI2_MISO_PORT, SPI2_MISO_PIN, 0)
#define MISO_2_READ()							(ROM_GPIOPinRead(SPI2_MISO_PORT, SPI2_MISO_PIN))			

#define CLK_2_ON()								ROM_GPIOPinWrite(SPI2_CLK_PORT, SPI2_CLK_PIN, SPI2_CLK_PIN)		
#define CLK_2_OFF()								ROM_GPIOPinWrite(SPI2_CLK_PORT, SPI2_CLK_PIN, 0)	

#define CS_2_ON()									ROM_GPIOPinWrite(SPI2_CS_PORT, SPI2_CS_PIN, SPI2_CS_PIN)	
#define CS_2_OFF()								ROM_GPIOPinWrite(SPI2_CS_PORT, SPI2_CS_PIN, 0)
//SPI3 defination
#define MOSI_3_ON()								ROM_GPIOPinWrite(SPI3_MOSI_PORT, SPI3_MOSI_PIN, SPI3_MOSI_PIN)
#define MOSI_3_OFF()							ROM_GPIOPinWrite(SPI3_MOSI_PORT, SPI3_MOSI_PIN, 0)
#define MOSI_3_READ()							ROM_GPIOPinRead(SPI3_MOSI_PORT, SPI3_MOSI_PIN)

#define MISO_3_ON()								ROM_GPIOPinWrite(SPI3_MISO_PORT, SPI3_MISO_PIN, SPI3_MISO_PIN)
#define MISO_3_OFF()							ROM_GPIOPinWrite(SPI3_MISO_PORT, SPI3_MISO_PIN, 0)
#define MISO_3_READ()							ROM_GPIOPinRead(SPI3_MISO_PORT, SPI3_MISO_PIN)			

#define CLK_3_ON()								ROM_GPIOPinWrite(SPI3_CLK_PORT, SPI3_CLK_PIN, SPI3_CLK_PIN)		
#define CLK_3_OFF()								ROM_GPIOPinWrite(SPI3_CLK_PORT, SPI3_CLK_PIN, 0)	

#define CS_3_ON()									ROM_GPIOPinWrite(SPI3_CS_PORT, SPI3_CS_PIN, SPI3_CS_PIN)	
#define CS_3_OFF()								ROM_GPIOPinWrite(SPI3_CS_PORT, SPI3_CS_PIN, 0)

#define delay(x)								ROM_SysCtlDelay((SYSTEM_CLOCK_GET() / 3000000) * x) //1us
#define delayCLK()							delay(100)	//100us
#define delayTrobe()						delay(10000)//10ms
#define delayRST()							delay(4000000)//4s

void vSoftSPI_Init(void);
uint8_t u8SpiTxRxByte(uint8_t data, uint8_t spi_module);
#endif /*SOFT_SPI_H_*/
