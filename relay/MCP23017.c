/*! @file MCP23017.c
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
#include "driverlib.h"
#include "config.h"
#include "MCP23017.h"
#include "i2c_mgreen.h"

/*!
* static data declaration
*/
static uint8_t au8_mcp23017_write_data[MCP23017_MAX_DATA];
static uint8_t au8_mcp23017_read_data[MCP23017_MAX_DATA];

void v_MCP23017_hardware_Init(void)
{
	v_i2c_hardware_init();
}
void v_MCP23017_software_Init(void)
{
	v_i2c_software_init();
	memset(au8_mcp23017_write_data, 0, MCP23017_MAX_DATA);
	memset(au8_mcp23017_read_data, 0, MCP23017_MAX_DATA);
}
void v_MCP23017_write_reg(uint8_t u8_slave_addr, uint8_t reg_addr, uint8_t u8_value)
{
	v_i2c_mgreen_write(SLAVE_ADDR_MASK | u8_slave_addr, reg_addr, u8_value);
}

uint8_t u8_MCP23017_read_reg(uint8_t u8_slave_addr, uint8_t reg_addr)
{
	return (u8_i2c_mgreen_read(SLAVE_ADDR_MASK|u8_slave_addr, reg_addr));
}

bool b_MCP23017_latch_output(uint8_t u8_slave_addr, uint8_t u8_GPIO_A, uint8_t u8_GPIO_B,
																										uint8_t* pu8_GPIO_A, uint8_t* pu8_GPIO_B)
{
	//TODO: IOCONA: 0000 0000
	//TODO: IOCONB: 0000 0000
	au8_mcp23017_write_data[0] = 0x00;
	au8_mcp23017_write_data[1] = 0x00;
	v_i2c_mgreen_write_data(SLAVE_ADDR_MASK | u8_slave_addr,
													IOCON, au8_mcp23017_write_data, 2);
	//TODO: read back
	memset(au8_mcp23017_read_data, 0, 2);
	v_i2c_mgreen_read_data(SLAVE_ADDR_MASK | u8_slave_addr,
													IOCON, au8_mcp23017_read_data, 2);
	if((au8_mcp23017_read_data[0] != 0) || (au8_mcp23017_read_data[1] != 0))
		return false;
	//TODO: IODIRA: 0x00 ; IODIRB: 0x00
	au8_mcp23017_write_data[0] = 0x00;				//ADDR: 0x00
	au8_mcp23017_write_data[1] = 0x00;				//ADDR: 0x01
	v_i2c_mgreen_write_data(SLAVE_ADDR_MASK | u8_slave_addr,
													IODIRA, au8_mcp23017_write_data, 2);	
	//TODO: read back
	memset(au8_mcp23017_read_data, 0, 2);
	v_i2c_mgreen_read_data(SLAVE_ADDR_MASK | u8_slave_addr,
													IODIRA, au8_mcp23017_read_data, 2);
	if((au8_mcp23017_read_data[0] != 0) || (au8_mcp23017_read_data[1] != 0))
		return false;
	
	//TODO: GPIOA : u8_GPIO_A; GPIOB: u8_GPIO_B
	//TODO: OLATA : 0xFF; OLATB: 0xFF	
	memset(au8_mcp23017_read_data, 0, 4);
	au8_mcp23017_write_data[0] = u8_GPIO_A;		//ADDR: 0x12
	au8_mcp23017_write_data[1] = u8_GPIO_B;		//ADDR: 0x13
	au8_mcp23017_write_data[2] = u8_GPIO_A;		//ADDR: 0x14
	au8_mcp23017_write_data[3] = u8_GPIO_B;		//ADDR: 0x15
	v_i2c_mgreen_write_data(SLAVE_ADDR_MASK | u8_slave_addr,
													GPIOA, au8_mcp23017_write_data, 4);	
	//TODO: read back
	memset(au8_mcp23017_read_data, 0, 4);
	v_i2c_mgreen_read_data(SLAVE_ADDR_MASK | u8_slave_addr,
													GPIOA, au8_mcp23017_read_data, 4);
	*pu8_GPIO_A = au8_mcp23017_read_data[0];
	*pu8_GPIO_B = au8_mcp23017_read_data[1];
	if((au8_mcp23017_read_data[0] != u8_GPIO_A) ||
		 (au8_mcp23017_read_data[1] != u8_GPIO_B) ||
		 (au8_mcp23017_read_data[2] != u8_GPIO_A) ||
		 (au8_mcp23017_read_data[3] != u8_GPIO_B))
		return false;
	return true;
}
