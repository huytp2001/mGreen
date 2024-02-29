#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "DebugLog.h"
#include "Storage.h"
#include "sd_card.h"
#include "rtc.h"

#define  DEBUG_LOG_THREAD_STACK_SIZE    (configMINIMAL_STACK_SIZE * 8) /*TODO: optimize this size*/
/* Provate Functions */
static void v_Debug_Log_Task (void *pvParameters);

/* Private Variables */
static xQueueHandle x_logger_msg_queue;
static STRU_LOGGER_MSG stru_logger_message;
static T_DATETIME t_current_time;
static uint32_t u32_current_time;

static char ac_write_msg[150];

StaticQueue_t xLoggerQueueBuffer;
uint8_t ucLoggerQueueStorage[DEBUG_LOG_QUEUE_LEN*DEBUG_LOG_MSG_SIZE];

static StaticTask_t xDebugLogTaskBuffer;
static StackType_t  xDebugLogStack[DEBUG_LOG_THREAD_STACK_SIZE];

/**
  * @brief  Initialize the debug logger.
  * @param  None
  * @retval None
  */
void v_Init_Debug_Log(void)
{
    memset(ac_write_msg, 0, sizeof(ac_write_msg));
    
    x_logger_msg_queue = xQueueCreateStatic(DEBUG_LOG_QUEUE_LEN, DEBUG_LOG_MSG_SIZE, &ucLoggerQueueStorage[0], &xLoggerQueueBuffer);

    /* Create logger task. */
    #if 0
    xTaskCreateStatic(v_Debug_Log_Task,
               "DEBUGLOG",
               (DEBUG_LOG_THREAD_STACK_SIZE),
               NULL,
               (tskIDLE_PRIORITY),
               xDebugLogStack,
               &xDebugLogTaskBuffer);
    #endif
}

/**
  * @brief  Debug logger task.
  * @param  None
  * @retval None
  */
static void v_Debug_Log_Task (void *pvParameters)
{
    static STRU_LOGGER_MSG stru_rcv_message;

    for (;;)
    {
        /* No need to check for task running
         * This is just a debug task
         */

        if (!b_Storage_Is_Ready())
        {
            vTaskDelay(DEBUG_LOG_QUEUE_WAIT);
            continue;
        }
        
         if (xQueueReceive (x_logger_msg_queue, &stru_rcv_message, DEBUG_LOG_QUEUE_WAIT))
         {
             /* Write message to SD Card */
             while (i16_sd_take_semaphore(10))
             {
                 vTaskDelay(2);
             }

             //Open file for writing
             if (i16_sd_open_file(DEBUG_LOG_FILE, 0) != 0)
             {
                 i16_sd_give_semaphore();
                 continue;
             }

             //write data to log file
             i16_sd_write_bytes((char *)stru_rcv_message.au8_log_msg,
                                stru_logger_message.u8_msg_len);

             //Get as much data as possible
             while (xQueueReceive(x_logger_msg_queue, &stru_rcv_message, DEBUG_LOG_QUEUE_WAIT) == pdTRUE)
             {
                 //write data to log file
                 i16_sd_write_bytes((char *)stru_rcv_message.au8_log_msg,
                                    stru_logger_message.u8_msg_len);
             }

             //Close file to save data
             i16_sd_close_file();

             //Release semaphore for another tasks
             i16_sd_give_semaphore();
         }
    }
}

/**
  * @brief  Log events.
  * @param  enm_msg_type: Type of message
  *         pu8_msg     : pointer to message
  * @retval true        : Log done
  *         false       : Error occurs
  */
bool b_Debug_Log_Event(ENM_LOG_MSG_TYPE enm_msg_type, uint8_t *pu8_msg)
{
    volatile uint32_t u32_strlen;
    configASSERT (IS_LOG_MSG_TYPE(enm_msg_type));

    if ((int)enm_msg_type <= (int)DEBUG_LEVEL)
    {
        t_rtc_read_time(&t_current_time);
        
        memset(&stru_logger_message, 0, sizeof(stru_logger_message));

        u32_current_time = DateTimetoLong(&t_current_time);
        LongToTimeStampString(u32_current_time, (char *)stru_logger_message.au8_log_msg);
        
        u32_strlen = strlen((char *)stru_logger_message.au8_log_msg);
        stru_logger_message.au8_log_msg[u32_strlen++] = ':';
        stru_logger_message.au8_log_msg[u32_strlen++] = ' ';
        /* Write message to log file */
        if ((strlen((char *)pu8_msg) + u32_strlen)
            < sizeof(stru_logger_message.au8_log_msg))
        {
            stru_logger_message.u8_msg_len = strlen((char *)pu8_msg) + u32_strlen;
            
            memcpy((char *)&stru_logger_message.au8_log_msg[u32_strlen], 
                    pu8_msg, stru_logger_message.u8_msg_len);

            while (1)
            {
                if (xQueueSend (x_logger_msg_queue, &stru_logger_message, DEBUG_LOG_QUEUE_WAIT))
                {
                    break;
                }
                /*
                 * If SD card is not ready, ignore messages
                 */
                else if (!b_Storage_Is_Ready())
                {
                    break;
                }
                else
                {
                    vTaskDelay(2);
                }
            }
        }
    }
    return true;
}
