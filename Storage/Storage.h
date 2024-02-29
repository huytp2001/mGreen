#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#define STORAGE_FIMWARE_FILE							"FERTI.BIN"

extern void vStorageHALInit(void);
extern int32_t i32_Storage_Write_Firmware_File(char *p_data, uint32_t u32_data_len);
extern bool b_Storage_Is_Ready(void);
extern void v_firmware_file_errase(void);

#endif
