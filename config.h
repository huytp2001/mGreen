/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.h
 * @author Danh Pham
 * @date 10 Nov 2020
 * @version: draft 2.0.0
 * @brief Template for doxygen automation document generate.
 * Add more detail desciption here
 */
 
 #ifndef CONFIG_H_
 #define CONFIG_H_		

/*!
*	@def DEVICE_ID
*	@def DEVICE_SUB_ID
*	@def BROKER_APP
*	@def BROKER_DEMO
*/

#define DEVICE_ID											3121
#define DEVICE_SUB_ID									1
#define BROKER_APP
//#define BROKER_DEMO

#define FW_VERSION										0x520
#define HW_VERSION										0x51
//#define HARDWARE_V5
#define HARDWARE_V5_1
// #define PROTOCOL_VER_1
#define PROTOCOL_VER_2
/*!
*	@def CONFIG_SCHEDULE_FILENAME	
* Name of schedule csv file
*/
#define CONFIG_SCHEDULE_FILENAME			"MAINFTG.CSV"
#define CONFIG_THRESHOLD_FILENAME			"THRES.CSV"
#define CONFIG_FIRMWARE_FILENAME			"FERTI.HEX"

//#define AQUA_MODE_ENABLE
//#define PH_EC_SENSOR_ENABLE
//#define NEUTRALIZER_MODE
//#define SCHEDULE_MODE
//#define THRESHOLD_MODE
//#define VPD_CONTROL_MODE
#define MANUAL_MODE
#define FS_MODE //Fertilizing simple mode
#define MAX_CHANNEL	 2
#define VALVE_1_PORT RELAY_ONBOARD_2
#define VALVE_2_PORT RELAY_ONBOARD_3
#define MAIN_FLOW_INPUT_PORT 3

#define FLOW_FACTOR_OF05ZAT						(6.8) //mL/pulse
#define FLOW_FACTOR_FS400							(4.8)		//4.8*60 pulses per liter
#define FS400_DIFF										(1.24)
#define FLOW_FACTOR_FS300							(5.5)		//5.5*60 pulses per liter

#define FLOW_RATE_DEFAULT     (5) //  L/m
#define FLOW_RATE_MAX			    (10) // L/m 
#define TANK_DIVISION_DEFAULT (100)
#define TIME_WARNING_MAIN_FLOWMETER		120 //s
#define TIME_WAIT_PRESSURE						60  //s

#define OPERATING_MODE	(stru_neces_params_get().e_mode)
#define USE_RF	(stru_neces_params_get().b_is_use_RF)
#define RF_TYPE (stru_neces_params_get().e_rf_type)
#define USE_MAIN_LINE_FLOW 					(stru_neces_params_get().b_is_use_main_flow)
#define MAIN_FLOW_COUNT								(stru_neces_params_get().u8_main_flow_pulses)//MAIN_FLOW_COUNT_ADDR
#define MAIN_FLOW_FACTOR							(stru_neces_params_get().d_main_flowmeter_factor)//((double)(MAIN_FLOW_FACTOR_ADDR) / 100)
/*define tasks*/
#define WDT_ENABLE										(1)
#define PARAMS_ENABLE									(1)
#define ERROR_HANDLER_ENABLE					(1)
#define RELAY_ENABLE									(1)
#define WRITE_ACTION_LOG							(1)
#define MQTT_ENABLE										(1)
#define LED_BLINKY_ENABLE							(1)
#define SENSOR_ENABLE									(1)
#define DIGITAL_INPUT_EN							(1)
#define RF_ENABLE											(1)
#define PORT_CONTROL_ENABLE						(1)
#define SCHEDULE_TASK_ENALBE					(1)
#define IRRIGATION_TASK_ENABLE				(1)
#define MANUAL_TASK_ENABLE						(1)
#define CAN_ENABLE										(0)

/*Test mode*/
#define RTC_AUTO_SET									(1)
#define INPUT_3_CNT_MANUAL						(0)
#define CHECK_PRESSURE_ENABLE					(0)
/*!
*	Extern functions
*/

#endif /* CONFIG_H_ */

