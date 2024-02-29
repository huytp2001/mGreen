/*! @file spi_mgreen.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/

#ifndef _SPI_MGREEN_H
#define _SPI_MGREEN_H
#ifdef __cplusplus
extern “C” {
#endif

/*!
* @data types, constants and macro defintions
*/
#include "driverlib.h"
#include "config.h"

#ifdef HARDWARE_V5
#define SPI_PH_CLK_PER											SYSCTL_PERIPH_GPIOK
#define SPI_PH_CLK_PORT											GPIO_PORTK_BASE
#define SPI_PH_CLK_PIN											GPIO_PIN_3

#define SPI_PH_MOSI_PER											SYSCTL_PERIPH_GPIOK
#define SPI_PH_MOSI_PORT										GPIO_PORTK_BASE
#define SPI_PH_MOSI_PIN											GPIO_PIN_1

#define SPI_PH_MISO_PER											SYSCTL_PERIPH_GPIOK
#define SPI_PH_MISO_PORT										GPIO_PORTK_BASE
#define SPI_PH_MISO_PIN											GPIO_PIN_0

#define SPI_PH_CS_PER												SYSCTL_PERIPH_GPIOC
#define SPI_PH_CS_PORT											GPIO_PORTC_BASE
#define SPI_PH_CS_PIN												GPIO_PIN_4

#define SPI_EC_CLK_PER											SYSCTL_PERIPH_GPIOC
#define SPI_EC_CLK_PORT											GPIO_PORTC_BASE
#define SPI_EC_CLK_PIN											GPIO_PIN_7

#define SPI_EC_MOSI_PER											SYSCTL_PERIPH_GPIOC
#define SPI_EC_MOSI_PORT										GPIO_PORTC_BASE
#define SPI_EC_MOSI_PIN											GPIO_PIN_6

#define SPI_EC_MISO_PER											SYSCTL_PERIPH_GPIOC
#define SPI_EC_MISO_PORT										GPIO_PORTC_BASE
#define SPI_EC_MISO_PIN											GPIO_PIN_5

#define SPI_EC_CS_PER												SYSCTL_PERIPH_GPIOQ
#define SPI_EC_CS_PORT											GPIO_PORTQ_BASE
#define SPI_EC_CS_PIN												GPIO_PIN_3
#endif

#ifdef HARDWARE_V5_1
#define SPI_PH_CLK_PER											SYSCTL_PERIPH_GPIOH
#define SPI_PH_CLK_PORT											GPIO_PORTH_BASE
#define SPI_PH_CLK_PIN											GPIO_PIN_3

#define SPI_PH_MOSI_PER											SYSCTL_PERIPH_GPIOH
#define SPI_PH_MOSI_PORT										GPIO_PORTH_BASE
#define SPI_PH_MOSI_PIN											GPIO_PIN_1

#define SPI_PH_MISO_PER											SYSCTL_PERIPH_GPIOK
#define SPI_PH_MISO_PORT										GPIO_PORTK_BASE
#define SPI_PH_MISO_PIN											GPIO_PIN_3

#define SPI_PH_CS_PER												SYSCTL_PERIPH_GPIOK
#define SPI_PH_CS_PORT											GPIO_PORTK_BASE
#define SPI_PH_CS_PIN												GPIO_PIN_1

#define SPI_EC_CLK_PER											SYSCTL_PERIPH_GPIOH
#define SPI_EC_CLK_PORT											GPIO_PORTH_BASE
#define SPI_EC_CLK_PIN											GPIO_PIN_2

#define SPI_EC_MOSI_PER											SYSCTL_PERIPH_GPIOH
#define SPI_EC_MOSI_PORT										GPIO_PORTH_BASE
#define SPI_EC_MOSI_PIN											GPIO_PIN_0

#define SPI_EC_MISO_PER											SYSCTL_PERIPH_GPIOK
#define SPI_EC_MISO_PORT										GPIO_PORTK_BASE
#define SPI_EC_MISO_PIN											GPIO_PIN_2

#define SPI_EC_CS_PER												SYSCTL_PERIPH_GPIOK
#define SPI_EC_CS_PORT											GPIO_PORTK_BASE
#define SPI_EC_CS_PIN												GPIO_PIN_0
#endif

//MOSI
#define	PH_MOSI_ON()												GPIOPinWrite(SPI_PH_MOSI_PORT,SPI_PH_MOSI_PIN,SPI_PH_MOSI_PIN)
#define PH_MOSI_OFF()													GPIOPinWrite(SPI_PH_MOSI_PORT,SPI_PH_MOSI_PIN,0)

#define	EC_MOSI_ON()													GPIOPinWrite(SPI_EC_MOSI_PORT,SPI_EC_MOSI_PIN,SPI_EC_MOSI_PIN)
#define EC_MOSI_OFF()													GPIOPinWrite(SPI_EC_MOSI_PORT,SPI_EC_MOSI_PIN,0)

//MISO
#define	PH_MISO_ON()													GPIOPinWrite(SPI_PH_MISO_PORT,SPI_PH_MISO_PIN,SPI_PH_MISO_PIN)
#define PH_MISO_OFF()													GPIOPinWrite(SPI_PH_MISO_PORT,SPI_PH_MISO_PIN,0)
#define PH_MISO_READ()												GPIOPinRead(SPI_PH_MISO_PORT,SPI_PH_MISO_PIN)

#define	EC_MISO_ON()													GPIOPinWrite(SPI_EC_MISO_PORT,SPI_EC_MISO_PIN,SPI_EC_MISO_PIN)
#define EC_MISO_OFF()													GPIOPinWrite(SPI_EC_MISO_PORT,SPI_EC_MISO_PIN,0)
#define EC_MISO_READ()												GPIOPinRead(SPI_EC_MISO_PORT,SPI_EC_MISO_PIN)

//CLK
#define	PH_CLK_ON()														GPIOPinWrite(SPI_PH_CLK_PORT,SPI_PH_CLK_PIN,SPI_PH_CLK_PIN)
#define PH_CLK_OFF()													GPIOPinWrite(SPI_PH_CLK_PORT,SPI_PH_CLK_PIN,0)

#define	EC_CLK_ON()														GPIOPinWrite(SPI_EC_CLK_PORT,SPI_EC_CLK_PIN,SPI_EC_CLK_PIN)
#define EC_CLK_OFF()													GPIOPinWrite(SPI_EC_CLK_PORT,SPI_EC_CLK_PIN,0)

//CS
#define	PH_CS_ON()														GPIOPinWrite(SPI_PH_CS_PORT,SPI_PH_CS_PIN,SPI_PH_CS_PIN)
#define PH_CS_OFF()														GPIOPinWrite(SPI_PH_CS_PORT,SPI_PH_CS_PIN,0)

#define	EC_CS_ON()														GPIOPinWrite(SPI_EC_CS_PORT,SPI_EC_CS_PIN,SPI_EC_CS_PIN)
#define EC_CS_OFF()														GPIOPinWrite(SPI_EC_CS_PORT,SPI_EC_CS_PIN,0)

//delay
#define delay(x)														ROM_SysCtlDelay(x)
#define delayCLK()													delay(5000)
#define delayTrobe()												delay(5000000)
#define	delayRST()													ROM_SysCtlDelay(ROM_SysCtlClockGet()/3*40)

void v_spi_mgreen_init(void);
uint8_t u8_spi_mgreen_tx_rx_byte(uint8_t u8_data, bool b_is_ph);


#ifdef __cplusplus
}
#endif

#endif /* SPI_MGREEN_H_ */
