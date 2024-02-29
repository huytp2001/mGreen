#ifndef SDRAM_H_
#define SDRAM_H_

#include <stdint.h>
#include <stdbool.h>

//*****************************************************************************
//
// The Mapping address space for the EPI SDRAM.
//
//*****************************************************************************
#define SDRAM_MAPPING_ADDRESS 							0x60000000

#define SDRAM_FOTA_DATA_MAPPING_ADDRESS 		0x60000000

extern void v_sdram_init(uint32_t ui32SysClock);

#endif
