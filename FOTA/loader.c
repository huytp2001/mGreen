/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**© Copyright Camlin Technologies Limited, 2012. All rights reserved.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : loader.c
**   Project     : IMT - main CPU - boot-loader.
**   Author      : Nguyen Anh Huy
**   Revision    : 1.0.0.1
**   Date        : 2012/01/30
**   Description : This module contains the function for firmware loading process. 
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   INCLUDE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/* Standard include. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Application include. */
//#include "flash_if.h"
#include "hex_processing.h"
#include "loader.h"
#include "driverlib.h"

#define LOADER_HEX_FILE_NAME_LEN_MAX                       20
#define LOADER_HEX_LINE_LEN_MAX                            64

 /* Buffer to store firmware string line. */
static char stri_hex_line[LOADER_HEX_LINE_LEN_MAX];

/* Buffer to store binary value of string line. */
static uint8_t u8_hex_line_bin[LOADER_HEX_LINE_LEN_MAX];
static char* getstr_line (
	char *in_buf, 	/* Pointer to the input buffer */
	char* buff,	/* Pointer to the string buffer to read */
	int len		/* Size of string buffer (characters) */
);

char* u8_idx_buffer;
/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   PROTOTYPE SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   FUNCTION SECTION
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Function    :
**      int32_t s32_LOADER_Process (void)
**
**   Arguments   :  
**      N/a   
**
**   Return      :
**      0   -   Load firmware successfully.
**     -1   -   load firmware fails.
**
**   Description : Load  firmware hex file to stm32 flash memory. 
**                 
**   Notes       : restrictions, odd modes
**
**   Author      : Nguyen Anh Huy.
**
**   Date        : 2012/01/30
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

int32_t s32_LOADER_Process (uint8_t *line_data, uint8_t line_len)
{
   uint32_t u32_hex_len;
   uint8_t u8_idx;		
   char stri_temp[3]; 
   memset(stri_temp, 0, sizeof(stri_temp));     

	if (line_len > sizeof(stri_hex_line))
		return LOADER_HEX_DATA_ERR;
	memcpy(stri_hex_line, line_data, line_len);
	if (line_len > 2)
		stri_hex_line[line_len - 2] = 0;	//remove CR LF
	else
		return (LOADER_HEX_DATA_ERR);

	/* Clear buffer. */
	memset(u8_hex_line_bin, 0, sizeof(u8_hex_line_bin));

	/* Do not count linefeed (\n) in the hex line. */
	u32_hex_len = strlen(stri_hex_line);

	if(u32_hex_len > LOADER_HEX_LINE_LEN_MAX)
	 return (LOADER_HEX_DATA_ERR);
	
	/* Convert to binary data. */
	u8_hex_line_bin[0] = stri_hex_line[0];
	for (u8_idx = 1; u8_idx <= ((u32_hex_len - 1) / 2); u8_idx++)
	{
		stri_temp[0] = stri_hex_line[u8_idx * 2 - 1];
		stri_temp[1] = stri_hex_line[u8_idx * 2];
		u8_hex_line_bin[u8_idx] = strtoul((const char *)stri_temp, 0, 16);
	}

	/* Process hex record. */
	switch (enum_Hex_Processing(RAM_MEMORY, u8_hex_line_bin, u8_idx))
	{
		case HEX_PROC_BAD:
			return (LOADER_OPEN_FILE_ERR);
		case HEX_PROC_COMPELETED:
			return (LOADER_SUCCESS);
		default:
			return (LOADER_NO_ERR);
	}
	//return (LOADER_NO_ERR);     

}

/* in_buf must contain '\n', this must be check before call this function */
static char* getstr_line (char *in_buf, char* buff,	/* Pointer to the string buffer to read */
										int len		/* Size of string buffer (characters) */
)
{
	int n = 0;
	char c, *p = buff;

	while (n < len - 1) {			/* Read bytes until buffer gets filled */
		c = *in_buf++;
		if (c == '\r') continue;	/* Strip '\r' */		
		*p++ = c;
		n++;
		if (c == '\n') break;		/* Break on EOL */
	}
	*p = 0;
	return n ? buff : 0;			/* When no data read (eof or error), return with error. */
}

/**
  * @brief  Jump to Bootloader
  * @param  None
	* @retval -1: error occured
  */
uint32_t Jump2Bootloader(void)
{
	return 0;
}

/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**   END
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/

