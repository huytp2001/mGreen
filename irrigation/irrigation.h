/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file IRRIGATION.h
 * @author Danh Pham
 * @date 12 Nov 2020
 * @version: 2.0.0
 * @brief This file contains structs and function support a IRRIGATION event.
 * 
 */
 

 #ifndef IRRIGATION_H
 #define IRRIGATION_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include "setting.h"
 /*!
 * @def AIR_SUCTION_TIME
 * The aeration time before the irrigation start point in seconds
 */
 #define AIR_SUCTION_TIME	90	
 /*!
 * @def AIR_PUMP_PORT
 * The aeration pump relay
 */
 #define AERATION_PUMP_PORT	4
 /*!
 * @def MACHINE_STATE_STOPED
 */
 #define MACHINE_STATE_STOPED					0x00
 /*!
 * @def MACIHNE_STATE_RUNNING
 */
 #define MACHINE_STATE_RUNNING					0x01
 /*!
 * @def MACHINE_STATE_PREPARE
 */
 #define MACHINE_STATE_PREPARE					0x02
/*!
* @def TIME_REPEAT_PHEC
* How many time to read pH EC before make a adjustment
*/
#define TIME_REPEAT_PHEC						4 // minimum is 3

#define USE_EC_SENSOR								(stru_neces_params_get().b_use_ec_sensor)

#define TIME_START_REPEAT_ERROR 6
#define TIME_STOP_REPEAT_ERROR  18

#define FERTILIZER_FLOWMETER_TYPE			(stru_neces_params_get().e_ferti_flow_type)
#define NORMAL_FLOWMETER_TYPE					(stru_neces_params_get().e_normal_flow_type)
#define FLOW_RATE											(stru_neces_params_get().u8_flow_rate)/10 //Lit/m
#define TANK_1_DIVISION								(stru_neces_params_get().au8_tank_division[0])
#define TANK_2_DIVISION								(stru_neces_params_get().au8_tank_division[1])
#define USE_MAIN_FLOW									(stru_neces_params_get().b_is_use_main_flow)
#define MAIN_FLOWMETER_ERROR_TIME 		(stru_neces_params_get().u8_main_flow_time)
#define PRESSURE_ERROR_TIME						(stru_neces_params_get().u8_wait_pressure_enough_time)

#define TIME_UPDATE_FLOW_HUBA					10 //s
#define HUBA_UPDATE_VOLUME_PULSES			100
#define Q0				(-0.3)
#define Kf				(0.0398)

#define MAX_RETRY_CHECK					40 //20*500ms = 10s
#define UNDER_FLOW_THRESHOLD		1.4 //40%

/*!
* @enum E_DOSING_TYPE
* Type of fertilizer mixing program
*/
typedef enum
{
	TYPE_DOSING_L = 0,		/**<Mix fertilizer with  predefine volume */
	TYPE_DOSING_L_L = 1,	/**<Mix fertilizer with rate identify
													how many L of fertilizer per 1000L of water */
	TYPE_DOSING_MAX,			/**<Identify maximum value in enum */
}E_DOSING_TYPE;

/*! 
* @enum E_DURATION_UNIT
* Unit of  irrigation duration
*/
typedef enum
{
	UNIT_TIME = 0,			/**<Time duration (how long) */
	UNIT_M3,						/**<Volume duration (how many m3) */
	UNIT_MAX,						/**<Identify maximum value in enum */
}E_DURATION_UNIT;

typedef enum
{
	VALVE_STATE_NONE = 0,
	VALVE_STATE_ON,
	VALVE_STATE_OFF,
}E_FER_VALVE_STATE;

/*! 
* @struct STRU_VALVE
* Defines the parametes of irrigation valve
*/
typedef struct
{
	uint8_t u8_valve_id;	/**<id of valve */		
	uint16_t u16_flow;		/**<flow rate of valve (m3/h) */
	uint8_t u8_port;			/**<relay port (from 1)*/
}STRU_VALVE;
/*! 
* @struct STRU_IRRIGATION_PROGRAM
* Defines the parameters of the IRRIGATION program
*/
typedef struct
{
	uint8_t u8_location_id; /**<id of location to be fertigated */
	char location_name[21]; /**<name of location to be fertigated*/
	E_DOSING_TYPE e_dosing_type; /**<type of mixing program */
	E_DURATION_UNIT e_duration_unit; /**<unit of IRRIGATION program */
	uint8_t u8_no_valves;		/**<number of used valve */
	bool b_phec_adjust;			/**<adjust pH EC or not */
	uint16_t u16_ph;				/**<pH set point (= real pH value * 100) */
	uint16_t u16_ec;				/**<pH set point (= real EC value * 100) */
	uint32_t u32_total_duration; /**<total duration of IRRIGATION program,
																	in second if unit is time or m3 * 100 if unit is m3 */
	uint32_t u32_before_duration; /**<watering duration before fertilizing, 
																	in second if unit is time or m3 * 100 if unit is m3 */
	uint32_t u32_after_duration;	/**<watering duration after fertilizing,
																	in second if unit is time or m3 * 100 if unit is m3 */
	double d_default_flowrate;	/**<default flowrate of area, calculated based on number of drips */
	uint32_t au32_rate[5];	/**<array contains set rates for each channel (= real value * 100) */
	STRU_VALVE astru_valve[10];	/**<array contains struct of used valve */
}STRU_IRRIGATION_PROGRAM;

typedef struct
{
	uint8_t u8_id;
	bool b_valve_use[4];
	E_FER_VALVE_STATE e_valve_state[4];
	uint32_t u32_volume_set[4];
	double d_volume[4];	
	double d_main_volume;
	uint16_t u16_main_time;
	double d_running_duration;	
}STRU_IRRIGATION_EVENT;

/*! 
* @enum E_IRRIGATION_EVENT_STATE
* state of IRRIGATION event
*/
typedef enum
{
	IRRIGATION_EVENT_BEGIN, 		/**<state begin: get value from IRRIGATION event*/
	IRRIGATION_EVENT_RUN, 	 		/**<state run: implement IRRIGATION event*/
	IRRIGATION_EVENT_PAUSE,		/**<state pause: pause IRRIGATION event*/
	IRRIGATION_EVENT_WAIT_CONTINUE, /**<state wait continue*/
	IRRIGATION_EVENT_CONTINUE, /**<state continue: continue IRRIGATION event paused*/
	IRRIGATION_EVENT_STOP,			/**<state stop: event comlete*/
	IRRIGATION_EVENT_WAIT			/**<state wait: do nothing, wait next IRRIGATION event*/
}E_IRRIGATION_EVENT_STATE;

/*! 
* @enum E_IRRIGATION_PROCESS_STATE
* state of IRRIGATION process
*/
typedef enum
{
	IRRIGATION_PROCESS_BEGIN,			/**<calculate, setup parameters*/
	IRRIGATION_PROCESS_BEFORE_RUN, /**<water before fertilizing*/
	IRRIGATION_PROCESS_RUN,				/**<fertilizing*/
	IRRIGATION_PROCESS_AFTER_RUN, 	/**<water after fertilizing*/
	IRRIGATION_PROCESS_BEFORE_STOP,/**<turn off device*/
	IRRIGATION_PROCESS_STOP				/**<IRRIGATION process done*/
}E_IRRIGATION_PROCESS_STATE;

/*! @enum E_PUMP_STAGE
* Define stages of pump.
*/
typedef enum
{
	PUMP_OFF = 0,  /**<Pump is off */
	PUMP_REQ_OFF = 1, /**<Pump is requesting to stop */
	PUMP_WAIT_VALVE_CLOSE = 2,
	PUMP_REQ_RUN_BY_TIME = 3, /**<Pump is requesting to run by time */
	PUMP_REQ_RUN_BY_VOLUME = 4, /**<Pump is requesting to run by volume */
	PUMP_RUNNING_BY_TIME = 5, /**<Pump is running and monitoring by time  */
	PUMP_RUNNING_BY_VOLUME = 6, /**<Pump is running and monitoring by volume */
	PUMP_STAGE_MAX,	/**<Maximum stage of pump */
}E_PUMP_STATUS;

typedef struct
{
	uint32_t u32_Sumvalue;
	uint16_t u16_CountUpdate;
	uint16_t u16_MinValue;
	uint16_t u16_MaxValue;
	uint16_t u16_AvgValue;
}STRU_STATISTICAL_MODULE;

/*!
*	Extern functions
*/
extern void v_irrigation_manager_taskinit(void);
extern bool b_irrigation_event_add(STRU_IRRIGATION_PROGRAM IRRIGATION_event);
extern E_IRRIGATION_EVENT_STATE e_get_irrigation_state(void);
extern E_IRRIGATION_PROCESS_STATE e_get_irrigation_process_state(void);
extern void v_irrigation_event_pause(void);
extern void v_irrigation_event_continue(void);
extern void v_irrigation_event_run(void);
extern E_DURATION_UNIT e_duration_unit_get(void);
extern void v_aeration_control(bool b_start);
extern uint8_t u8_irrigation_prog_state_get(void);
extern uint32_t u32_irrigation_time_begin_get(void);
extern bool b_has_error_get(void);
extern void v_irrigation_queue_reset(void);
extern void v_irrigation_error(void);
extern bool b_irrigation_phec_get(void);
extern void v_clear_all_error(void);
extern void v_fault_code_reset(bool b_reset);
extern uint8_t u8_location_running_get(void);
#endif /* IRRIGATION_H */


