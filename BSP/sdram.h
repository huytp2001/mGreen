/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file sdram.h
 * @author Danh Pham
 * @date 12 Nov 2020
 * @version: 2.0.0
 * @brief This file contains address of used SDRAM.
 */
#ifndef SDRAM_H_
#define SDRAM_H_

#include <stdint.h>
#include <stdbool.h>

/*!
* @def SDRAM_MAPPING_ADDRESS	0x60000000
* Start address of SDRAM
*/
#define SDRAM_MAPPING_ADDRESS 					0x60000000

/*!
* @def SDRAM_UI_USED					(1024*1024)
* Size of RAM used for UI (1MB)
*/
#define SDRAM_UI_USED										(1024*1024)	

/*!
* @def SDRAM_LOG_FRAME_USED								200
* Size of RAM used for saving frame 		(200B)
*/
#define SDRAM_LOG_FRAME_USED						200				

/*!
* @def SDRAM_FOTA_FIRMWARE_USED					(1024*1024)
* Size of RAM used for saving firmware when OTA (1MB)
*/
#define SDRAM_FOTA_FIRMWARE_USED				(1024*1024)		

/*!
* @def SDRAM_UART_BUFFER_USED						2000
* Size of RAM used for buffering UART data (2000B)
*/
#define SDRAM_UART_BUFFER_USED					2000

/*!
* @def SDRAM_SCHEDULE_USED							(200*1024)
* Size of RAM used for saving schedule (200kB)
*/
#define SDRAM_SCHEDULE_USED							(200*1024)

/*!
* @def SDRAM_DOWNLOAD_BUFFER_USED					(1024 * 1024)
* Size of RAM used of saving download data (1MB) 
*/
#define SDRAM_DOWNLOAD_BUFFER_USED					(1024 * 1024)

/*!
* @def SDRAM_CSV_SCHEDULE_USED					(1024 * 1024)
* Size of RAM used of saving csv file of schedule, 
* used in case sd card has error (1MB) 
*/
#define SDRAM_CSV_SCHEDULE_USED							(1024 * 1024)

/*!
* @def SDRAM_CSV_THRESHOLD_UESD					(1024 * 1024)
* Size of RAM used of saving csv file of threshold, 
* used in case sd card has error (1MB) 
*/
#define SDRAM_CSV_THRESHOLD_UESD						(1024 * 1024)

/*!
* @def #define SDRAM_SNAPSHOT_USED				(1024*1024)
* Size of RAM used for backuping data when hardfault reset
* It's located on last 1MB of SDRAM (from 0x61F00000 to 0x61FFFFFF)
*/
#define SDRAM_SNAPSHOT_USED									(1024*1024)			//1 last MB of SDRAM, from 0x61F00000 to 0x61FFFFFF


/*!
* @def SDRAM_UI_DATA_MAPPING_ADDRSS				(SDRAM_MAPPING_ADDRESS)
* Start address of UI data
*/
#define SDRAM_UI_DATA_MAPPING_ADDRSS				(SDRAM_MAPPING_ADDRESS)

/*!
* @def SDRAM_LOG_DATA_MAPPING_ADDRESS 			(SDRAM_UI_DATA_MAPPING_ADDRSS + SDRAM_UI_USED)
* Start address of logging data
*/
#define SDRAM_LOG_DATA_MAPPING_ADDRESS 			(SDRAM_UI_DATA_MAPPING_ADDRSS + SDRAM_UI_USED)

/*!
* @def SDRAM_FOTA_DATA_MAPPING_ADDRESS			(SDRAM_LOG_DATA_MAPPING_ADDRESS + SDRAM_LOG_FRAME_USED)
* Start address of OTA data
*/
#define SDRAM_FOTA_DATA_MAPPING_ADDRESS 		(SDRAM_LOG_DATA_MAPPING_ADDRESS + SDRAM_LOG_FRAME_USED)

/*!
* @def SDRAM_UART_BUFFER_ADDRESS						(SDRAM_FOTA_DATA_MAPPING_ADDRESS + SDRAM_FOTA_FIRMWARE_USED)
* Start address of UART's buffer 
*/
#define SDRAM_UART_BUFFER_ADDRESS						(SDRAM_FOTA_DATA_MAPPING_ADDRESS + SDRAM_FOTA_FIRMWARE_USED)

/*!
* @def SDRAM_SCHEDULE_DATA_MAPPING_ADDRESS	(SDRAM_UART_BUFFER_ADDRESS + SDRAM_UART_BUFFER_USED)
* Start address of schedule data
*/
#define SDRAM_SCHEDULE_DATA_MAPPING_ADDRESS	(SDRAM_UART_BUFFER_ADDRESS + SDRAM_UART_BUFFER_USED)

/*!
* @def SDRAM_DOWNLOAD_BUFFER_MAPPING_ADDRESS (SDRAM_SCHEDULE_DATA_MAPPING_ADDRESS + SDRAM_SCHEDULE_USED)
* Start address of downloading data
*/
#define SDRAM_DOWNLOAD_BUFFER_MAPPING_ADDRESS (SDRAM_SCHEDULE_DATA_MAPPING_ADDRESS + SDRAM_SCHEDULE_USED)

/*!
* @def SDRAM_CSV_SCHEDULE_ADDRESS (SDRAM_SCHEDULE_DATA_MAPPING_ADDRESS + SDRAM_SCHEDULE_USED)
* Start address of schedule csv file data
*/
#define SDRAM_CSV_SCHEDULE_ADDRESS 						(SDRAM_DOWNLOAD_BUFFER_MAPPING_ADDRESS + SDRAM_DOWNLOAD_BUFFER_USED)

/*!
* @def SDRAM_CSV_SCHEDULE_ADDRESS (SDRAM_SCHEDULE_DATA_MAPPING_ADDRESS + SDRAM_SCHEDULE_USED)
* Start address of schedule csv file data
*/
#define SDRAM_CSV_THRESHOLD_ADDRESS 					(SDRAM_CSV_SCHEDULE_ADDRESS + SDRAM_CSV_SCHEDULE_USED)

/*!
* @note RAM address from 0x61E00000 use save config parameter from bootloader
*/
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

/* TNPHU: location to save last reset cause */
#define WATCHDOG_RESET_FLAG_ADDR				(*(volatile unsigned int *)0x61E003C4u)

extern void v_sdram_init(uint32_t ui32SysClock);

#endif
