/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file file.h
 * @author Danh Pham
 * @date 17 Nov 2020
 * @version: 1.0.0
 * @brief This file contains parameters for manage CAN bus.
 */
 
 #ifndef CAN_TASK_H_
 #define CAN_TASK_H_
 
 #include "can_app.h"
 #include "can_parser.h"
/*!
*	Extern functions
*/
extern void v_can_task_init(void);
extern void v_can_in_send_queue(STRU_CANAPP_DATA_T *pstru_in_sm_data);
extern void v_can_out_send_queue(STRU_CANAPP_DATA_T *pstru_out_data);
#endif /* CAN_TASK_H_ */
