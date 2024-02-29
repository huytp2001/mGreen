/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file schedule_task.h
 * @author Danh Pham
 * @date 13 Nov 2020
 * @version: 1.0.0
 * @brief This file contains defines and struct used for schedule task.
 * 
 */
 
 #ifndef _SCHEDULE_TASK_H_
 #define _SCHEDULE_TASK_H_
 
 #include <stdint.h>

/*!
* @enum E_SCHEDULE_STAGE
* @brief Define stage of read schedule routine.
*/
typedef enum
{
	SCHE_STAGE_LOAD = 0,				/**<Default stage of process after controller reset or
																after new schedule is updated */
	SCHE_STAGE_SEEK_EVENT = 1,	/**<Stage which the program find the suitable irrigation event */
	SCHE_STAGE_CHECK_TIME = 2,	/**<Stage which the program wait for start point of irrigation event */
	SCHE_STAGE_TIME_OVER = 3,		/**<All irrigation events in schedule have already read */
}E_SCHEDULE_STAGE;

/*!
*	Extern functions
*/
extern void v_schedule_task_init(void);
extern T_DATETIME t_schedule_remain_time_get(void);
extern void v_new_schedule_available_set(void);
extern void v_schedule_back(void);
extern E_SCHEDULE_STAGE e_schedule_sate_get(void);
#endif /* _SCHEDULE_TASK_H_ */

