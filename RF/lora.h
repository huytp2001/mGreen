/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    	*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file lora.h
 * @author Linh Nguyen (editor)
 * @date 27 Nov 2020
 * @version: draft 1.0
 * @brief Manage functionality of LORA Module.
 */
#ifndef LORA_H_
#define LORA_H_
#ifdef __cplusplus
extern �C� {
#endif

/*!
*	Extern functions
*/
bool v_lora_apc_init (void);
bool v_lora_e32_init (void);
void v_lora_send_package (uint8_t* pu8_package, uint8_t u8_size);	
void v_lora_process_mess(void);
#ifdef __cplusplus
}
#endif

#endif /* _LORA_H */

