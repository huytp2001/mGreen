#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "driverlib.h"
#include "boot_config.h"
#include "sysctl.h"
#include "rom.h"
#include "eeprom.h"

void v_save_boot_config(void)
{
	strcpy((void *)CLIENT_ID_ADDR, CLIENT_ID_1);
	strcpy((void *)TOPIC_SUBSCRIBE_ADDR, TOPIC_SUBSCRIBE);
	strcpy((void *)TOPIC_MONITOR_ADDR, TOPIC_MONITOR);
	strcpy((void *)TOPIC_SMS_ADDR, TOPIC_SMS);
	strcpy((void *)CLIENT_ID_2_ADDR, CLIENT_ID_2);
	
	PCB_VERSION_ADDR = PCB_VERSION;
	//PER 3V3
	PER_3V3_SYS_ADDR = PER_3V3_SYS;
	PER_3V3_PORT_ADDR = PER_3V3_PORT;
	PER_3V3_PIN_ADDR = PER_3V3_PIN;
	//LED debug
	LED_1_SYS_ADDR = LED_1_SYS;
	LED_2_SYS_ADDR = LED_2_SYS;
	LED_1_PORT_ADDR = LED_1_PORT;
	LED_2_PORT_ADDR = LED_2_PORT;
	LED_1_PIN_ADDR = LED_1_PIN;
	LED_2_PIN_ADDR = LED_2_PIN;
	//Relay output
	RL1_SYS_ADDR 	= RL1_SYS;
	RL1_PORT_ADDR = RL1_PORT;
	RL1_PIN_ADDR 	=	RL1_PIN;
	
	RL2_SYS_ADDR 	=	RL2_SYS;
	RL2_PORT_ADDR = RL2_PORT;
	RL2_PIN_ADDR 	=	RL2_PIN;
	
	RL3_SYS_ADDR 	= RL3_SYS;
	RL3_PORT_ADDR = RL3_PORT;
	RL3_PIN_ADDR 	=	RL3_PIN;
	
	RL4_SYS_ADDR 	= RL4_SYS;
	RL4_PORT_ADDR = RL4_PORT;
	RL4_PIN_ADDR 	=	RL4_PIN;
	
	RL5_SYS_ADDR 	= RL5_SYS;
	RL5_PORT_ADDR = RL5_PORT;
	RL5_PIN_ADDR 	=	RL5_PIN;
	
	RL6_SYS_ADDR 	= RL6_SYS;
	RL6_PORT_ADDR = RL6_PORT;
	RL6_PIN_ADDR 	=	RL6_PIN;
	//Digital input
	IN1_SYS_ADDR  = INPUT1_SYS;
	IN1_PORT_ADDR = INPUT1_PORT;
	IN1_PIN_ADDR 	= INPUT1_PIN;
	
	IN2_SYS_ADDR  = INPUT2_SYS;
	IN2_PORT_ADDR = INPUT2_PORT;
	IN2_PIN_ADDR 	= INPUT2_PIN;
	
	IN3_SYS_ADDR  = INPUT3_SYS;
	IN3_PORT_ADDR = INPUT3_PORT;
	IN3_PIN_ADDR 	= INPUT3_PIN;
	
	IN4_SYS_ADDR  = INPUT4_SYS;
	IN4_PORT_ADDR = INPUT4_PORT;
	IN4_PIN_ADDR 	= INPUT4_PIN;
	
	IN5_SYS_ADDR  = INPUT5_SYS;
	IN5_PORT_ADDR = INPUT5_PORT;
	IN5_PIN_ADDR 	= INPUT5_PIN;
	
	IN6_SYS_ADDR  = INPUT6_SYS;
	IN6_PORT_ADDR = INPUT6_PORT;
	IN6_PIN_ADDR 	= INPUT6_PIN;
	
	SPI1_CS_SYS_ADDR 	= SPI1_CS_SYS;
	SPI1_CS_PORT_ADDR = SPI1_CS_PORT;
	SPI1_CS_PIN_ADDR 	= SPI1_CS_PIN;
	
	SPI1_MOSI_SYS_ADDR 	= SPI1_MOSI_SYS;
	SPI1_MOSI_PORT_ADDR = SPI1_MOSI_PORT;
	SPI1_MOSI_PIN_ADDR 	= SPI1_MOSI_PIN;
	
	SPI1_MISO_SYS_ADDR 	= SPI1_MISO_SYS;
	SPI1_MISO_PORT_ADDR = SPI1_MISO_PORT;
	SPI1_MISO_PIN_ADDR 	= SPI1_MISO_PIN;
	
	SPI1_CLK_SYS_ADDR 	= SPI1_CLK_SYS;
	SPI1_CLK_PORT_ADDR 	= SPI1_CLK_PORT;
	SPI1_CLK_PIN_ADDR 	= SPI1_CLK_PIN;
	
	SPI2_CS_SYS_ADDR 	= SPI2_CS_SYS;
	SPI2_CS_PORT_ADDR = SPI2_CS_PORT;
	SPI2_CS_PIN_ADDR 	= SPI2_CS_PIN;
	
	SPI2_MOSI_SYS_ADDR 	= SPI2_MOSI_SYS;
	SPI2_MOSI_PORT_ADDR = SPI2_MOSI_PORT;
	SPI2_MOSI_PIN_ADDR 	= SPI2_MOSI_PIN;
	
	SPI2_MISO_SYS_ADDR 	= SPI2_MISO_SYS;
	SPI2_MISO_PORT_ADDR = SPI2_MISO_PORT;
	SPI2_MISO_PIN_ADDR 	= SPI2_MISO_PIN;
	
	SPI2_CLK_SYS_ADDR 	= SPI2_CLK_SYS;
	SPI2_CLK_PORT_ADDR 	= SPI2_CLK_PORT;
	SPI2_CLK_PIN_ADDR 	= SPI2_CLK_PIN;
	
	USE_RF_ADDR 					= USE_RF_FLAG;
	USE_LORA_FLAG_ADDR		= USE_LORA_FLAG;
	RF_COMPACT_FRAME_ADDR	= _RF_COMPACT_FRAME_;
	RF_UART_BAUDRATE_ADDR	= RF_UART_BAUDRATE;
	RF_TIMEOUT_ADDR				= RF_TIMEOUT;
	
	USE_MAIN_LINE_FLOW_ADDR = USE_MAIN_LINE_FLOW_FLAG;
	MAIN_FLOW_COUNT_ADDR = MAIN_FLOW_COUNT;
	MAIN_FLOW_FACTOR_ADDR	= (MAIN_FLOW_FACTOR * 100);
	
	FERTILIZER_FLOWMETER_TYPE_ADDR = FERTILIZER_FLOWMETER_TYPE;
}
void v_read_boot_config(void)
{
	char temp[30];
	uint32_t len = 0;
	memcpy(temp, (void *)CLIENT_ID_ADDR, strlen((const char *)CLIENT_ID_ADDR) + 1);
	len = PCB_VERSION_ADDR;
}