/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file sdram_utils.h
 * @author Danh Pham
 * @date 19 Nov 2020
 * @version: 1.0.0
 * @brief This file contain parameters to save and read file from sdram.
 */
 
 #ifndef SDRAM_UTILS_H_
 #define SDRAM_UTILS_H_
 
 #include <stdint.h>
 #include "sdram.h"



/*!
*	Extern functions
*/
extern void v_sdram_save_schedule_csv(uint8_t *pu8_data, uint32_t u32_len);
extern void v_sdram_save_threshold_csv(uint8_t *pu8_data, uint32_t u32_len);
extern int8_t i8_sdram_parse_schedule_csv(void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set);
extern int8_t i8_sdram_parse_threshold_csv(void (*cb1)(void *, size_t, void *),
														void (*cb2)(int c, void *), void *c, uint32_t u32_crc_set);

#endif /* SDRAM_UTILS_H_ */

