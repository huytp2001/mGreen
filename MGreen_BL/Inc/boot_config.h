#ifndef __BOOT_CONFIG_H__
#define __BOOT_CONFIG_H__

//topic config
#define CLIENT_ID_1 								"mgreen5013/1_1"	//11
#define CLIENT_ID_2 								"mgreen5013/1_2"	//11
#define TOPIC_SUBSCRIBE							"mgreen/control/5013/1"
#define TOPIC_MONITOR								"mgreen/monitor/5013/1"
#define TOPIC_SMS										"mgreen/sms/5013/1"

//hardware config
	//pcb verison 4.x with x is defined below
#define PCB_VERSION										0x02		
	//use RF or not
#define USE_RF
//#define _LORA_E32_
#define _CC1310_

//enable if use main line flow metter
#define USE_MAIN_LINE_FLOW
//define class of flow meter
#define FLOW_METER_CLASS_B
//#define FLOW_METER_CALSS_2

//choose type of fetilizer flowmeter:
//#define USE_NORMAL_FLOWMETER
#define USE_HUABA_CONTROL_FLOWMETER

#ifdef USE_RF
	#define USE_RF_FLAG		1
#else
	#define USE_RF_FLAG		0
#endif 

#ifdef USE_MAIN_LINE_FLOW
	#define USE_MAIN_LINE_FLOW_FLAG		1
#else
	#define USE_MAIN_LINE_FLOW_FLAG		0
#endif

#ifdef FLOW_METER_CLASS_B
	#define MAIN_FLOW_COUNT			4
	#define MAIN_FLOW_FACTOR		0.025
#else
	#define MAIN_FLOW_COUNT			2
	#define MAIN_FLOW_FACTOR		(0.05)
#endif

#ifdef USE_HUABA_CONTROL_FLOWMETER
	#define FERTILIZER_FLOWMETER_TYPE			1
#else
	#define FERTILIZER_FLOWMETER_TYPE			0
#endif

#ifdef _LORA_E32_
	#define _RF_COMPACT_FRAME_		1
	#define RF_UART_BAUDRATE			9600
	#define RF_TIMEOUT						1600000
	#define USE_LORA_FLAG					1
#else
	#define _RF_COMPACT_FRAME_		0
	#define RF_UART_BAUDRATE			115200
	#define RF_TIMEOUT						0
	#define USE_LORA_FLAG					0
#endif
//3V3 Peripheral
#if (PCB_VERSION == 0x00 || PCB_VERSION == 0x01)		//version 1
	#define PER_3V3_SYS			SYSCTL_PERIPH_GPION
	#define PER_3V3_PORT		GPIO_PORTN_BASE
	#define PER_3V3_PIN			GPIO_PIN_0
#else
	#define PER_3V3_SYS			SYSCTL_PERIPH_GPIOQ
	#define PER_3V3_PORT		GPIO_PORTQ_BASE
	#define PER_3V3_PIN			GPIO_PIN_2
#endif
//LED pin
#if (PCB_VERSION == 0x00)
	#define LED_1_SYS				SYSCTL_PERIPH_GPIOK
	#define LED_2_SYS				SYSCTL_PERIPH_GPIOL
	#defien LED_1_PORT			GPIO_PORTK_BASE
	#define LED_2_PORT			GPIO_PORTL_BASE
	#define LED_1_PIN				GPIO_PIN_4
	#define LED_2_PIN				GPIO_PIN_7
#else
	#define LED_1_SYS				SYSCTL_PERIPH_GPIOK
	#define LED_2_SYS				SYSCTL_PERIPH_GPIOK
	#define LED_1_PORT			GPIO_PORTK_BASE
	#define LED_2_PORT			GPIO_PORTK_BASE
	#define LED_1_PIN				GPIO_PIN_4
	#define LED_2_PIN				GPIO_PIN_3
#endif
//output
#if (PCB_VERSION == 0x00)
	#define RL1_SYS					SYSCTL_PERIPH_GPIOE
	#define RL1_PORT				GPIO_PORTE_BASE
	#define RL1_PIN         GPIO_PIN_0

	#define RL2_SYS					SYSCTL_PERIPH_GPIOK
	#define RL2_PORT				GPIO_PORTK_BASE
	#define RL2_PIN         GPIO_PIN_2

	#define RL3_SYS					SYSCTL_PERIPH_GPION
	#define RL3_PORT				GPIO_PORTN_BASE
	#define RL3_PIN         GPIO_PIN_2

	#define RL4_SYS					SYSCTL_PERIPH_GPIOL
	#define RL4_PORT				GPIO_PORTL_BASE
	#define RL4_PIN         GPIO_PIN_5

	#define RL5_SYS					SYSCTL_PERIPH_GPIOQ
	#define RL5_PORT				GPIO_PORTQ_BASE
	#define RL5_PIN         GPIO_PIN_2

	#define RL6_SYS					SYSCTL_PERIPH_GPIOK
	#define RL6_PORT				GPIO_PORTK_BASE
	#define RL6_PIN         GPIO_PIN_7
#else	/*Greater than V4.0*/
	#define RL1_SYS					SYSCTL_PERIPH_GPIOE
	#define RL1_PORT				GPIO_PORTE_BASE
	#define RL1_PIN         GPIO_PIN_0

	#define RL2_SYS					SYSCTL_PERIPH_GPIOK
	#define RL2_PORT				GPIO_PORTK_BASE
	#define RL2_PIN         GPIO_PIN_2

	#define RL3_SYS					SYSCTL_PERIPH_GPION
	#define RL3_PORT				GPIO_PORTN_BASE
	#define RL3_PIN         GPIO_PIN_2

	#define RL4_SYS					SYSCTL_PERIPH_GPIOE
	#define RL4_PORT				GPIO_PORTE_BASE
	#define RL4_PIN         GPIO_PIN_1

	#define RL5_SYS					SYSCTL_PERIPH_GPIOQ
	#define RL5_PORT				GPIO_PORTQ_BASE
	#define RL5_PIN         GPIO_PIN_2

	#define RL6_SYS					SYSCTL_PERIPH_GPIOK
	#define RL6_PORT				GPIO_PORTK_BASE
	#define RL6_PIN         GPIO_PIN_7
#endif	/*Output pin*/

//input
#if (PCB_VERSION == 0x00)
	#define INPUT1_SYS			SYSCTL_PERIPH_GPIOE
	#define INPUT1_PORT			GPIO_PORTE_BASE
	#define INPUT1_PIN			GPIO_PIN_1

	#define INPUT2_SYS			SYSCTL_PERIPH_GPIOK
	#define INPUT2_PORT			GPIO_PORTK_BASE
	#define INPUT2_PIN			GPIO_PIN_3

	#define INPUT3_SYS			SYSCTL_PERIPH_GPIOQ
	#define INPUT3_PORT			GPIO_PORTQ_BASE
	#define INPUT3_PIN			GPIO_PIN_3

	#define INPUT4_SYS			SYSCTL_PERIPH_GPIOK
	#define INPUT4_PORT			GPIO_PORTK_BASE
	#define INPUT4_PIN			GPIO_PIN_6

	#define INPUT5_SYS			SYSCTL_PERIPH_GPION
	#define INPUT5_PORT			GPIO_PORTN_BASE
	#define INPUT5_PIN			GPIO_PIN_1

	#define INPUT6_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT6_PORT			GPIO_PORTL_BASE
	#define INPUT6_PIN			GPIO_PIN_6
#elif (PCB_VERSION == 0x01)/*version 4.1*/
	#define INPUT1_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT1_PORT			GPIO_PORTL_BASE
	#define INPUT1_PIN			GPIO_PIN_4

	#define INPUT2_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT2_PORT			GPIO_PORTL_BASE
	#define INPUT2_PIN			GPIO_PIN_5

	#define INPUT3_SYS			SYSCTL_PERIPH_GPIOA
	#define INPUT3_PORT			GPIO_PORTA_BASE
	#define INPUT3_PIN			GPIO_PIN_4

	#define INPUT4_SYS			SYSCTL_PERIPH_GPIOA
	#define INPUT4_PORT			GPIO_PORTA_BASE
	#define INPUT4_PIN			GPIO_PIN_5

	#define INPUT5_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT5_PORT			GPIO_PORTL_BASE
	#define INPUT5_PIN			GPIO_PIN_6

	#define INPUT6_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT6_PORT			GPIO_PORTL_BASE
	#define INPUT6_PIN			GPIO_PIN_7
#else	/*greater than 4.1*/
	#define INPUT1_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT1_PORT			GPIO_PORTL_BASE
	#define INPUT1_PIN			GPIO_PIN_4

	#define INPUT2_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT2_PORT			GPIO_PORTL_BASE
	#define INPUT2_PIN			GPIO_PIN_5

	#define INPUT3_SYS			SYSCTL_PERIPH_GPIOA
	#define INPUT3_PORT			GPIO_PORTA_BASE
	#define INPUT3_PIN			GPIO_PIN_4

	#define INPUT4_SYS			SYSCTL_PERIPH_GPIOA
	#define INPUT4_PORT			GPIO_PORTA_BASE
	#define INPUT4_PIN			GPIO_PIN_5

	#define INPUT5_SYS			SYSCTL_PERIPH_GPIOB
	#define INPUT5_PORT			GPIO_PORTB_BASE
	#define INPUT5_PIN			GPIO_PIN_2

	#define INPUT6_SYS			SYSCTL_PERIPH_GPIOL
	#define INPUT6_PORT			GPIO_PORTL_BASE
	#define INPUT6_PIN			GPIO_PIN_7
#endif	/*input*/
//soft SPI pin
#if (PCB_VERSION == 0x00)
	#define SPI1_CS_SYS			SYSCTL_PERIPH_GPIOL
	#define SPI1_CS_PORT		GPIO_PORTL_BASE
	#define SPI1_CS_PIN			GPIO_PIN_4
#else		/*PCB version greater than 4.0*/
	#define SPI1_CS_SYS			SYSCTL_PERIPH_GPION
	#define SPI1_CS_PORT		GPIO_PORTN_BASE
	#define SPI1_CS_PIN			GPIO_PIN_1
#endif	/*soft SPI pin*/

#define SPI1_MOSI_SYS		SYSCTL_PERIPH_GPIOM
#define SPI1_MOSI_PORT	GPIO_PORTM_BASE
#define SPI1_MOSI_PIN		GPIO_PIN_4

#define SPI1_MISO_SYS		SYSCTL_PERIPH_GPIOM
#define SPI1_MISO_PORT  GPIO_PORTM_BASE
#define SPI1_MISO_PIN	  GPIO_PIN_5

#define SPI1_CLK_SYS		SYSCTL_PERIPH_GPIOM
#define SPI1_CLK_PORT   GPIO_PORTM_BASE
#define SPI1_CLK_PIN    GPIO_PIN_6

#define SPI2_CS_SYS			SYSCTL_PERIPH_GPIOM
#define SPI2_CS_PORT		GPIO_PORTM_BASE
#define SPI2_CS_PIN			GPIO_PIN_7

#define SPI2_MOSI_SYS		SYSCTL_PERIPH_GPION
#define SPI2_MOSI_PORT	GPIO_PORTN_BASE
#define SPI2_MOSI_PIN		GPIO_PIN_3

#define SPI2_MISO_SYS		SYSCTL_PERIPH_GPIOQ
#define SPI2_MISO_PORT  GPIO_PORTQ_BASE
#define SPI2_MISO_PIN	  GPIO_PIN_4

#define SPI2_CLK_SYS		SYSCTL_PERIPH_GPIOP
#define SPI2_CLK_PORT   GPIO_PORTP_BASE
#define SPI2_CLK_PIN    GPIO_PIN_5

//RAM address from 61E00000
#define CLIENT_ID_ADDR									((volatile unsigned int *)0x61E00000u)
#define TOPIC_SUBSCRIBE_ADDR						((volatile unsigned int *)0x61E00030u)
#define TOPIC_MONITOR_ADDR							((volatile unsigned int *)0x61E00060u)
#define TOPIC_SMS_ADDR									((volatile unsigned int *)0x61E00090u)
#define PCB_VERSION_ADDR								(*(volatile unsigned int *)0x61E00090u)
#define PER_3V3_SYS_ADDR								(*(volatile unsigned int *)0x61E00094u)	
#define PER_3V3_PORT_ADDR								(*(volatile unsigned int *)0x61E00098u)
#define PER_3V3_PIN_ADDR								(*(volatile unsigned int *)0x61E0009Cu)
#define LED_1_SYS_ADDR									(*(volatile unsigned int *)0x61E000A0u)
#define LED_2_SYS_ADDR									(*(volatile unsigned int *)0x61E000A4u)
#define LED_1_PORT_ADDR									(*(volatile unsigned int *)0x61E000A8u)
#define LED_2_PORT_ADDR									(*(volatile unsigned int *)0x61E000ACu)
#define LED_1_PIN_ADDR									(*(volatile unsigned int *)0x61E000B0u)
#define LED_2_PIN_ADDR									(*(volatile unsigned int *)0x61E000B4u)
	
#define RL1_SYS_ADDR										(*(volatile unsigned int *)0x61E000B8u)			
#define RL1_PORT_ADDR										(*(volatile unsigned int *)0x61E000BCu)
#define RL1_PIN_ADDR										(*(volatile unsigned int *)0x61E000C0u)

#define RL2_SYS_ADDR										(*(volatile unsigned int *)0x61E000C4u)			
#define RL2_PORT_ADDR										(*(volatile unsigned int *)0x61E000C8u)
#define RL2_PIN_ADDR										(*(volatile unsigned int *)0x61E000CCu)

#define RL3_SYS_ADDR										(*(volatile unsigned int *)0x61E000D0u)			
#define RL3_PORT_ADDR										(*(volatile unsigned int *)0x61E000D4u)
#define RL3_PIN_ADDR										(*(volatile unsigned int *)0x61E000D8u)

#define RL4_SYS_ADDR										(*(volatile unsigned int *)0x61E000DCu)			
#define RL4_PORT_ADDR										(*(volatile unsigned int *)0x61E000E0u)
#define RL4_PIN_ADDR										(*(volatile unsigned int *)0x61E000E4u)
	
#define RL5_SYS_ADDR										(*(volatile unsigned int *)0x61E000E8u)			
#define RL5_PORT_ADDR										(*(volatile unsigned int *)0x61E000ECu)
#define RL5_PIN_ADDR										(*(volatile unsigned int *)0x61E000F0u)

#define RL6_SYS_ADDR										(*(volatile unsigned int *)0x61E000F4u)			
#define RL6_PORT_ADDR										(*(volatile unsigned int *)0x61E000F8u)
#define RL6_PIN_ADDR										(*(volatile unsigned int *)0x61E000FCu)

#define IN1_SYS_ADDR										(*(volatile unsigned int *)0x61E00100u)
#define IN1_PORT_ADDR										(*(volatile unsigned int *)0x61E00104u)
#define IN1_PIN_ADDR										(*(volatile unsigned int *)0x61E00108u)	

#define IN2_SYS_ADDR										(*(volatile unsigned int *)0x61E0010Cu)
#define IN2_PORT_ADDR										(*(volatile unsigned int *)0x61E00110u)
#define IN2_PIN_ADDR										(*(volatile unsigned int *)0x61E00114u)

#define IN3_SYS_ADDR										(*(volatile unsigned int *)0x61E00118u)
#define IN3_PORT_ADDR										(*(volatile unsigned int *)0x61E0011Cu)
#define IN3_PIN_ADDR										(*(volatile unsigned int *)0x61E00120u)

#define IN4_SYS_ADDR										(*(volatile unsigned int *)0x61E00124u)
#define IN4_PORT_ADDR										(*(volatile unsigned int *)0x61E00128u)
#define IN4_PIN_ADDR										(*(volatile unsigned int *)0x61E0012Cu)

#define IN5_SYS_ADDR										(*(volatile unsigned int *)0x61E00130u)
#define IN5_PORT_ADDR										(*(volatile unsigned int *)0x61E00134u)
#define IN5_PIN_ADDR										(*(volatile unsigned int *)0x61E00138u)

#define IN6_SYS_ADDR										(*(volatile unsigned int *)0x61E0013Cu)
#define IN6_PORT_ADDR										(*(volatile unsigned int *)0x61E00140u)
#define IN6_PIN_ADDR										(*(volatile unsigned int *)0x61E00148u)

#define SPI1_CS_SYS_ADDR								(*(volatile unsigned int *)0x61E0014Cu)
#define SPI1_CS_PORT_ADDR								(*(volatile unsigned int *)0x61E00150u)
#define SPI1_CS_PIN_ADDR                (*(volatile unsigned int *)0x61E00154u)
                                        
#define SPI1_MOSI_SYS_ADDR              (*(volatile unsigned int *)0x61E00158u)
#define SPI1_MOSI_PORT_ADDR             (*(volatile unsigned int *)0x61E0015Cu)
#define SPI1_MOSI_PIN_ADDR              (*(volatile unsigned int *)0x61E00160u)
                                        
#define SPI1_MISO_SYS_ADDR              (*(volatile unsigned int *)0x61E00164u)
#define SPI1_MISO_PORT_ADDR             (*(volatile unsigned int *)0x61E00168u)
#define SPI1_MISO_PIN_ADDR              (*(volatile unsigned int *)0x61E0016Cu)
                                        
#define SPI1_CLK_SYS_ADDR               (*(volatile unsigned int *)0x61E00170u)
#define SPI1_CLK_PORT_ADDR              (*(volatile unsigned int *)0x61E00174u)
#define SPI1_CLK_PIN_ADDR               (*(volatile unsigned int *)0x61E00178u)
                                        
#define SPI2_CS_SYS_ADDR								(*(volatile unsigned int *)0x61E0017Cu)
#define SPI2_CS_PORT_ADDR								(*(volatile unsigned int *)0x61E00180u)
#define SPI2_CS_PIN_ADDR                (*(volatile unsigned int *)0x61E00184u)
                                        
#define SPI2_MOSI_SYS_ADDR              (*(volatile unsigned int *)0x61E00188u)
#define SPI2_MOSI_PORT_ADDR             (*(volatile unsigned int *)0x61E0018Cu)
#define SPI2_MOSI_PIN_ADDR              (*(volatile unsigned int *)0x61E00190u)
                                        
#define SPI2_MISO_SYS_ADDR              (*(volatile unsigned int *)0x61E00194u)
#define SPI2_MISO_PORT_ADDR             (*(volatile unsigned int *)0x61E00198u)
#define SPI2_MISO_PIN_ADDR              (*(volatile unsigned int *)0x61E0019Cu)
                                        
#define SPI2_CLK_SYS_ADDR               (*(volatile unsigned int *)0x61E001A0u)
#define SPI2_CLK_PORT_ADDR              (*(volatile unsigned int *)0x61E001A4u)
#define SPI2_CLK_PIN_ADDR               (*(volatile unsigned int *)0x61E001A8u)  

#define USE_RF_ADDR											(*(volatile unsigned int *)0x61E001ACu)  
#define RF_COMPACT_FRAME_ADDR						(*(volatile unsigned int *)0x61E001B0u)
#define RF_UART_BAUDRATE_ADDR						(*(volatile unsigned int *)0x61E001B4u)
#define RF_TIMEOUT_ADDR									(*(volatile unsigned int *)0x61E001B8u)
#define USE_LORA_FLAG_ADDR							(*(volatile unsigned int *)0x61E001BCu) 
	
#define USE_MAIN_LINE_FLOW_ADDR					(*(volatile unsigned int *)0x61E001C0u) 
#define MAIN_FLOW_COUNT_ADDR						(*(volatile unsigned int *)0x61E001C4u)
#define MAIN_FLOW_FACTOR_ADDR						(*(volatile unsigned int *)0x61E001C8u)

#define FERTILIZER_FLOWMETER_TYPE_ADDR	(*(volatile unsigned int *)0x61E001CCu)

#define CLIENT_ID_2_ADDR								((volatile unsigned int *)0x61E001D0u)
	//warning: next address is 0x61E001F0u
                                        
void v_save_boot_config(void);          
void v_read_boot_config(void);          
                                        
#endif	/*BOOT_CONFIG_H*/               
                                        