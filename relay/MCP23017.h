#ifndef __MCP23017_H__
#define __MCP23017_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

//IOCON.BANK = 0
#define IODIRA									0x00 //RST: 1111 1111 | 1: input| 0: Output
#define IPOLA										0x02
#define GPINTENA								0x04
#define GPPUA										0x0C
#define GPIOA										0x12 //RST: 0000 0000
#define OLATA										0x14 //RST: 0000 0000 

#define IODIRB									0x01
#define IPOLB										0x03
#define GPINTENB								0x05
#define GPPUB										0x0D
#define GPIOB										0x13
#define OLATB										0x15

#define IOCON										0x0A//
//   |  7 |   6  |   5 |  4   |  3 | 2 |  1   |0|
//   |BANK|MIRROR|SEQOP|DISSLW|HAEN|ODR|INTPOL|x|
//RST|  0 |   0  |   0 |  0   |  0 | 0 |  0   |0|
// BANK  : 0 --> the A/B register are paired| 1--> the A/B register are separated.
// SEQOP : 0 --> Sequential enabled. |        1--> Sequential disabled.

#define SLAVE_ADDR_MASK					0x20
#define IOCON_REG								0x20

#define RELAY_I2C1_ADDR					0x01
#define RELAY_I2C2_ADDR					0x02
#define RELAY_I2C3_ADDR					0x03
#define RELAY_I2C4_ADDR					0x04

#define MCP23017_MAX_DATA				16

void v_MCP23017_hardware_Init(void);
void v_MCP23017_software_Init(void);
void v_MCP23017_write_reg(uint8_t u8_slave_addr, uint8_t reg_addr, uint8_t u8_value);
uint8_t u8_MCP23017_read_reg(uint8_t u8_slave_addr, uint8_t reg_addr);
bool b_MCP23017_latch_output(uint8_t u8_slave_addr, uint8_t u8_GPIO_A, uint8_t u8_GPIO_B,
														uint8_t* pu8_GPIO_A, uint8_t* pu8_GPIO_B);
#endif /*__MCP23017_H__*/
