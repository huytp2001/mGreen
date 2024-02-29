/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file relay_control.h
 * @author Danh Pham
 * @date 18 Nov 2020
 * @version: 1.0.0
 * @brief This file contains parameters for relay control.
 */
 
 #ifndef RELAY_CONTROL_H_
 #define RELAY_CONTROL_H_
 
#include "driverlib.h"
#include "config.h"	

#define MAX_NUM_RELAY_BOARD			2

typedef enum
{
	NONE,
	RELAY_ONBOARD_1,
	RELAY_ONBOARD_2,
	RELAY_ONBOARD_3,
	RELAY_ONBOARD_4,
	RELAY_ONBOARD_5,
	RELAY_ONBOARD_6,
	RELAY_ONBOARD_7,
	RELAY_ONBOARD_8
}relay_onboard_name_t;

typedef enum
{
	OFF_RELAY_ONBOARD,
	ON_RELAY_ONBOARD
}relay_onboard_status_t;

typedef struct
{
	uint16_t relay_onboard_status;
	uint16_t relay_1_status;
	uint16_t relay_2_status;
	uint16_t relay_3_status;
	uint16_t relay_4_status;
}relay_status_t;

/*!
*	Extern functions
*/
void v_relay_task_init(void);
void v_relay_hardware_init(void);
void v_relay_software_init (void);
bool b_relay_turn_on(uint8_t u8_Relay);
bool b_relay_turn_off(uint8_t u8_Relay);
bool b_get_ex_relay_status(uint8_t u8_slave_addr, uint64_t *u64_relay_status);
bool b_relay_turn_off_all (void);
bool b_relay_control (uint64_t u64_new_relay_state, uint64_t* pu64_act_relay_state);
void v_relay_onboard_init (void);
void v_manual_port_control(uint8_t u8_port, bool b_turn_on);
void v_relay_onboard_control(relay_onboard_name_t e_relay_onboard_name,
																		relay_onboard_status_t e_relay_onboard_status);
#endif /* RELAY_CONTROL_H_ */

