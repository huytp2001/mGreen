/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file error_manage.h
 * @author Danh Pham
 * @date 16 Nov 2020
 * @version: 1.0.0
 * @brief This file contains struct and define for error manage.
 */
 
 #ifndef ERROR_MANAGE_H_
 #define ERROR_MANAGE_H_
 
 #include <stdint.h>
 #include <stdbool.h>

/*!
* @def ERROR_MAX_NAME
* Maximum lenght of error handler name
*/
#define ERROR_MAX_NAME			20
/*!
* @def PRESSURE_ERROR_REPEAT_TIME	
*/
#define ERROR_REPEAT_TIMEOUT		300
/*! @struct STRU_ERROR
* Contains parameters of an error handle.
*/
typedef struct
{
	uint8_t u8_error_id;	/**<Id which is auto generated when register of error handler */
	uint32_t u32_error_timeout; /**<Maximum time counter before call callback function */
	uint32_t u32_error_counter; /**< Current counter */
	char c_error_name[ERROR_MAX_NAME];	/**< Name of error handler, name must be unique */
	void (*pv_error_func)(void);	/**<Callback function */
}STRU_ERROR;

/*!
*	Extern functions
*/
extern bool b_error_handler_reg(char* pc_error_name , uint32_t u32_timeout,  void (*pc_callback_func)(void), 
													uint8_t *pu8_id);
extern bool b_error_unreg(uint8_t u8_error_id);
extern bool b_error_reset_counter(uint8_t u8_error_id);;
extern bool b_error_change_timeout(uint8_t u8_error_id, uint32_t u32_timeout);
extern void v_error_handler_task_init(void);
#endif /* ERROR_MANAGE_H_ */

