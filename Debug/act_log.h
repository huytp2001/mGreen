#ifndef __ATC_LOG_H_
#define __ATC_LOG_H_

#include "Storage.h"
#include "sd_card.h"
#include "rtc.h"
#include "sd_manage_lib.h"


extern bool b_write_action_log(char * c_log_message, ...);
bool b_delete_action_file(T_DATETIME t_date);
#endif
