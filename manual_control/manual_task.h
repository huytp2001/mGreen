/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file manual_task.h
 * @author Danh Pham
 * @date 01 Dec 2020
 * @version 1.0.0
 * @brief .
 */
 
 #ifndef MANUAL_TASK_H_
 #define MANUAL_TASK_H_
 
 #include <stdint.h>
 #include <stdbool.h>

/*!
* @def MAX_MANUAL_AREA
* Maximum number can handle in manual task 
*/
#define MAX_MANUAL_AREA					64

typedef enum
{
	MANUAL_REQ_STOP = 0,
	MANUAL_STOPPING = 1,
	MANUAL_REQ_RUN = 2,
	MANUAL_RUNNING = 3,
}manual_control_state_t;
/*!
* @struct manual_control_t
* Params of manual irrigated location 
*/
typedef struct
{
	uint8_t u8_location_id;			/**<ID of irrigated location */
	manual_control_state_t e_manual_state;   /**<State of locaiton */
	uint32_t u32_time_start;		/**<Start time */
	uint32_t u32_time_stop;			/**<Stop time */
	uint64_t u64_bitwise_port;				/**<Port of location */
}manual_control_t;

extern void v_manual_task_init(void);
extern void v_manual_port_control(uint8_t u8_port, bool b_turn_on);
extern void v_manual_port_timer_control(uint8_t u8_location_id, uint32_t u32_time_on,
								uint64_t u64_valves);
 #endif /* MANUAL_TASK_H_ */
