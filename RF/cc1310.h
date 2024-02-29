/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    	*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file cc1310.h
 * @author Linh Nguyen (editor)
 * @date 27 Nov 2020
 * @version: draft 1.0
 * @brief Manage functionality of CC1310 Module.
 */

#ifndef CC1310_H_
#define CC1310_H_
#ifdef __cplusplus
extern �C� {
#endif

/*!
* Public functions
*/
bool v_cc1310_init (void);
void v_cc1310_process_data (void);
void v_cc1310_send_package (uint8_t* pu8_package, uint8_t u8_size);	
#ifdef __cplusplus
}
#endif

#endif /* CC1310_H_ */

