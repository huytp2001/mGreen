/*******************************************************************************
* Contains proprietary and confidential information of .
* May not be used or disclosed to any other party except in accordance
* with a license from .
*
* Copyright (c) 2016 . All Rights Reserved.
*******************************************************************************/

#ifndef __DEBUGLOG_H__
#define __DEBUGLOG_H__

#include <stdint.h>

/* Define logger level
 * System messages are always logged
 * 0:   No logger
 * 1:   Log error messages
 * 2:   Log error & warning messages
 * 3:   Log all messages
 */
#define DEBUG_LEVEL         3

//#define DEBUG_LOG_FILE      "Log.bin"
#define DEBUG_LOG_FILE      "Log.txt"

#define DEBUG_LOG_QUEUE_WAIT              10//1000
/* Maximum items in received logger queue. */
#define DEBUG_LOG_QUEUE_LEN               (10)
/* Size in bytes of each logger queue item. */
#define DEBUG_LOG_MSG_SIZE                (sizeof(STRU_LOGGER_MSG))

typedef enum
{
    LOG_MSG_SYSTEM          = 0,
    LOG_MSG_TYPE_ERROR      = 1,
    LOG_MSG_TYPE_WARNING    = 2,
    LOG_MSG_TYPE_INFO       = 3
} ENM_LOG_MSG_TYPE;

typedef struct
{
    uint8_t u8_msg_len;
    uint8_t au8_rsrv[15];
    uint8_t au8_log_msg[128-15-1];
} STRU_LOGGER_MSG;

#define IS_LOG_MSG_TYPE(x)  ((x == LOG_MSG_SYSTEM)          \
                            || (x == LOG_MSG_TYPE_ERROR)    \
                            || (x == LOG_MSG_TYPE_WARNING)  \
                            || (x == LOG_MSG_TYPE_INFO))

extern void v_Init_Debug_Log(void);
extern bool b_Debug_Log_Event(ENM_LOG_MSG_TYPE enm_msg_type, uint8_t *pu8_msg);

#endif // __DEBUGLOG_H__
