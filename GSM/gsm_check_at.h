 /****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file gsm_check_at.h
 * @author Danh Pham
 * @date 15 Dec 2020
 * @version: 1.0.0
 * @brief 
 */
 
#ifndef GSM_CHECK_AT_H_
#define GSM_CHECK_AT_H_

#include "gsm_cmd.h"

void v_gsm_check_at_respone (void);
bool b_get_at_resp_status_from_table (at_resp_status_t e_resp_status);
void v_delete_all_resp_status_on_table (void);
#endif /* GSM_CHECK_AT_H_ */
