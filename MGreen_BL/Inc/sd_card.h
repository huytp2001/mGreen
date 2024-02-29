#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stdint.h>
#include "ff.h"
#include "diskio.h"

void DiskTickHandler(void);
int Cmd_ls(int argc, char *argv[]);
static int PopulateFileListBox(bool bRepaint);
static FRESULT ChangeToDirectory(char *pcDirectory, uint32_t *pui32Reason);
int Cmd_cd(int argc, char *argv[]);
int Cmd_pwd(int argc, char *argv[]);
int Cmd_cat(int argc, char *argv[]);
int i16_sd_init(void);
int i16_sd_open_file(char *txtname, bool RnW, uint32_t *file_size);
int i16_sd_close_file(void);
int i16_sd_read_bytes(uint32_t u32_offset, uint8_t *readData, uint32_t NumofBytes);
int i16_sd_write_bytes(char *data, uint32_t NumofBytes);
int sd_remove_storage_file(char *file);
int i16_sd_take_semaphore(int block_time);
int i16_sd_give_semaphore(void);

#endif
