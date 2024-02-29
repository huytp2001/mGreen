/*! @file i2c_mgreen.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2018.  
* All rights reserved.
*/
#ifndef _I2C_MGREEN_H
#define _I2C_MGREEN_H
#ifdef __cplusplus
extern “C” {
#endif
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
/*!
* @data types, constants and macro defintions
*/
#ifdef HARDWARE_V5
#define I2C_MGREEN_PER						SYSCTL_PERIPH_I2C0
#define I2C_MGREEN_BASE						I2C0_BASE
#define I2C_MGREEN_SCL_SDA_PER		SYSCTL_PERIPH_GPIOB
#define I2C_MGREEN_SCL_SDA_PORT		GPIO_PORTB_BASE
#define I2C_MGREEN_SCL_PIN				GPIO_PIN_2
#define I2C_MGREEN_SDA_PIN				GPIO_PIN_3
#define I2C_MGREEN_SCL_AF					GPIO_PB2_I2C0SCL
#define I2C_MGREEN_SDA_AF					GPIO_PB3_I2C0SDA
#endif

#ifdef HARDWARE_V5_1
#define I2C_MGREEN_PER						SYSCTL_PERIPH_I2C4
#define I2C_MGREEN_BASE						I2C4_BASE
#define I2C_MGREEN_SCL_SDA_PER		SYSCTL_PERIPH_GPIOK
#define I2C_MGREEN_SCL_SDA_PORT		GPIO_PORTK_BASE
#define I2C_MGREEN_SCL_PIN				GPIO_PIN_6
#define I2C_MGREEN_SDA_PIN				GPIO_PIN_7
#define I2C_MGREEN_SCL_AF					GPIO_PK6_I2C4SCL
#define I2C_MGREEN_SDA_AF					GPIO_PK7_I2C4SDA

#define I2C_MGREEN_RST_PER				SYSCTL_PERIPH_GPIOK
#define I2C_MGREEN_RST_PORT				GPIO_PORTK_BASE
#define I2C_MGREEN_RST_PIN				GPIO_PIN_5

#define RELAY_ONBOARD_1_PER												SYSCTL_PERIPH_GPIOE
#define RELAY_ONBOARD_1_PORT											GPIO_PORTE_BASE
#define RELAY_ONBOARD_1_PIN												GPIO_PIN_4

#define RELAY_ONBOARD_2_PER												SYSCTL_PERIPH_GPIOE
#define RELAY_ONBOARD_2_PORT											GPIO_PORTE_BASE
#define RELAY_ONBOARD_2_PIN												GPIO_PIN_5

#define RELAY_ONBOARD_3_PER												SYSCTL_PERIPH_GPIOD
#define RELAY_ONBOARD_3_PORT											GPIO_PORTD_BASE
#define RELAY_ONBOARD_3_PIN												GPIO_PIN_0

#define RELAY_ONBOARD_4_PER												SYSCTL_PERIPH_GPIOD
#define RELAY_ONBOARD_4_PORT											GPIO_PORTD_BASE
#define RELAY_ONBOARD_4_PIN												GPIO_PIN_1

#define RELAY_ONBOARD_5_PER												SYSCTL_PERIPH_GPIOD
#define RELAY_ONBOARD_5_PORT											GPIO_PORTD_BASE
#define RELAY_ONBOARD_5_PIN												GPIO_PIN_2

#define RELAY_ONBOARD_6_PER												SYSCTL_PERIPH_GPIOD
#define RELAY_ONBOARD_6_PORT											GPIO_PORTD_BASE
#define RELAY_ONBOARD_6_PIN												GPIO_PIN_3

#define RELAY_ONBOARD_7_PER												SYSCTL_PERIPH_GPIOQ
#define RELAY_ONBOARD_7_PORT											GPIO_PORTQ_BASE
#define RELAY_ONBOARD_7_PIN												GPIO_PIN_0

#define RELAY_ONBOARD_8_PER												SYSCTL_PERIPH_GPIOQ
#define RELAY_ONBOARD_8_PORT											GPIO_PORTQ_BASE
#define RELAY_ONBOARD_8_PIN												GPIO_PIN_1

#endif

#define I2C_TAKE_SEMAPHORE_TIMEOUT					5
#define I2C_DEFAULT_TIMEOUT									1000 //ms
/*!
* @public functions prototype
*/
void v_i2c_hardware_init (void);
void v_i2c_software_init(void);
void v_i2c_hardware_reset(void);
bool b_i2c_mgreen_busy(uint32_t u32_timeout);
void v_i2c_mgreen_write(uint8_t u8_addr, uint8_t u8_reg_addr, uint8_t u8_data);
void v_i2c_mgreen_write_data(uint8_t u8_addr, uint8_t u8_reg_addr, 
														uint8_t *pu8_data, uint8_t u8_length);
uint8_t u8_i2c_mgreen_read(uint8_t u8_addr, uint8_t u8_reg_addr);
void v_i2c_mgreen_read_data(uint8_t u8_addr, uint8_t u8_reg_addr, 
														uint8_t *pu8_data, uint8_t u8_length);
	
#ifdef __cplusplus
}
#endif

#endif /* _I2C_MGREEN_H */	
