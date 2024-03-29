//*****************************************************************************
//
// sd_card.c - Example program for reading files from an SD card.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.0.12573 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "ff.h"
#include "diskio.h"
#include "sd_card.h"
#include "driverlib.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static xSemaphoreHandle x_sd_mutex;

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary data from
// the SD card.  There are two buffers allocated of this size.  The buffer size
// must be large enough to hold the longest expected full path name, including
// the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.  Initially
// it is root ("/").
//
//*****************************************************************************
static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_pcTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
//static char g_pcCmdBuf[CMD_BUF_SIZE];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tSDFResultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// Error reasons returned by ChangeDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES       (sizeof(g_psFResultStrings) /                 \
                                 sizeof(tSDFResultString))

//*****************************************************************************
//
// Storage for the filename listbox widget string table.
//
//*****************************************************************************
#define NUM_LIST_STRINGS 48
const char *g_ppcDirListStrings[NUM_LIST_STRINGS];

//*****************************************************************************
//
// Storage for the names of the files in the current directory.  Filenames
// are stored in format "(D) filename.ext" for directories or "(F) filename.ext"
// for files.
//
//*****************************************************************************
#define MAX_FILENAME_STRING_LEN (4 + 8 + 1 + 3 + 1)
char g_pcFilenames[NUM_LIST_STRINGS][MAX_FILENAME_STRING_LEN];

//*****************************************************************************
//
// Storage for the strings which appear in the status box at the bottom of the
// display.
//
//****************************************************************************
#define NUM_STATUS_STRINGS 6
#define MAX_STATUS_STRING_LEN (36 + 1)
char g_pcStatus[NUM_STATUS_STRINGS][MAX_STATUS_STRING_LEN];

//*****************************************************************************
//
// Storage for the status listbox widget string table.
//
//*****************************************************************************
const char *g_ppcStatusStrings[NUM_STATUS_STRINGS] =
{
    g_pcStatus[0],
    g_pcStatus[1],
    g_pcStatus[2],
    g_pcStatus[3],
    g_pcStatus[4],
    g_pcStatus[5]
};
uint32_t g_ui32StatusStringIndex = 0;

//*****************************************************************************
//
// The system clock frequency in Hz.
//
//*****************************************************************************

//*****************************************************************************
//
// Forward declarations for functions called by the widgets used in the user
// interface.
//
//*****************************************************************************
static FRESULT ChangeToDirectory(char *pcDirectory, uint32_t *pui32Reason);

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a timer tick
// every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
DiskTickHandler(void)
{
	TimerIntClear(TIMER4_BASE, TimerIntStatus(TIMER4_BASE, true));
    //
    // Call the FatFs tick timer.
    //
    disk_timerproc();
}

//*****************************************************************************
//
// This function implements the "ls" command.  It opens the current directory
// and enumerates through the contents, and prints a line for each item it
// finds.  It shows details such as file attributes, time and date, and the
// file size, along with the name.  It shows a summary of file sizes at the end
// along with free space.
//
//*****************************************************************************
int
Cmd_ls(int argc, char *argv[])
{
    uint32_t ui32TotalSize, ui32ItemCount, ui32FileCount, ui32DirCount;
    FRESULT iFResult;
    FATFS *psFatFs;
#if _USE_LFN
    char *pcFileName;
    char pucLfn[_MAX_LFN + 1];
    g_sFileInfo.lfname = pucLfn;
    g_sFileInfo.lfsize = sizeof(pucLfn);
#endif


    //
    // Open the current directory for access.
    //
    iFResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(iFResult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        return(iFResult);
    }

    ui32TotalSize = 0;
    ui32FileCount = 0;
    ui32DirCount = 0;
    ui32ItemCount = 0;

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        iFResult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(iFResult != FR_OK)
        {
            return((int)iFResult);
        }

        //
        // If the file name is blank, then this is the end of the listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

#if _USE_LFN
        pcFileName = ((*g_sFileInfo.lfname)?g_sFileInfo.lfname:g_sFileInfo.fname);
#else
//        pcFileName = g_sFileInfo.fname;
#endif

        //
        // Add the information as a line in the listbox widget.
        //
        if(ui32ItemCount < NUM_LIST_STRINGS)
        {
        }

        //
        // If the attribute is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ui32DirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ui32FileCount++;
            ui32TotalSize += g_sFileInfo.fsize;
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ui32ItemCount++;

    }   // endfor
    //
    // Get the free space.
    //
    iFResult = f_getfree("/", (DWORD *)&ui32TotalSize, &psFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifes the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory to
// make sure it exists.  If the new path is opened successfully, then the
// current working directory (cwd) is changed to the new path.
//
// In cases of error, the pui32Reason parameter will be written with one of
// the following values:
//
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pcDirectory, uint32_t *pui32Reason)
{
    uint32_t ui32Idx;
    FRESULT iFResult;

    //
    // Copy the current working path into a temporary buffer so it can be
    // manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If the first character is /, then this is a fully specified path, and it
    // should just be used as-is.
    //
    if(pcDirectory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pcDirectory) + 1 > sizeof(g_pcCwdBuf))
        {
            *pui32Reason = NAME_TOO_LONG_ERROR;
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too long, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_pcTmpBuf, pcDirectory, sizeof(g_pcTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pcDirectory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        ui32Idx = strlen(g_pcTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/) is
        // found, or until we bump up to the start of the path.
        //
        while((g_pcTmpBuf[ui32Idx] != '/') && (ui32Idx > 1))
        {
            //
            // Back up one character.
            //
            ui32Idx--;
        }

        //
        // Now we are either at the lowest level separator in the current path,
        // or at the beginning of the string (root).  So set the new end of
        // string here, effectively removing that last part of the path.
        //
        g_pcTmpBuf[ui32Idx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current directory,
    // and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is added on to
        // the current path, there is room in the buffer for the full new path.
        // It needs to include a new separator, and a trailing null character.
        //
        if(strlen(g_pcTmpBuf) + strlen(pcDirectory) + 1 + 1 > sizeof(g_pcCwdBuf))
        {
            *pui32Reason = NAME_TOO_LONG_ERROR;
            return(FR_INVALID_OBJECT);
        }

        //
        // The new path is okay, so add the separator and then append the new
        // directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_pcTmpBuf, "/"))
            {
                strcat(g_pcTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_pcTmpBuf, pcDirectory);
        }
    }

    //
    // At this point, a candidate new directory path is in chTmpBuf.  Try to
    // open it to make sure it is valid.
    //
    iFResult = f_opendir(&g_sDirObject, g_pcTmpBuf);

    //
    // If it can't be opened, then it is a bad path.  Inform the user and
    // return.
    //
    if(iFResult != FR_OK)
    {
        *pui32Reason = OPENDIR_ERROR;
        return(iFResult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD and update
    // the screen.
    //
    else
    {
        strncpy(g_pcCwdBuf, g_pcTmpBuf, sizeof(g_pcCwdBuf));
    }

    //
    // Return success.
    //
    return(FR_OK);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifes the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
//*****************************************************************************
int
Cmd_cd(int argc, char *argv[])
{
    uint32_t ui32Reason;
    FRESULT iFResult;

    //
    // Try to change to the directory provided on the command line.
    //
    iFResult = ChangeToDirectory(argv[1], &ui32Reason);

    //
    // If an error was reported, try to offer some helpful information.
    //
    if(iFResult != FR_OK)
    {
        switch(ui32Reason)
        {
            case OPENDIR_ERROR:
                break;
            case NAME_TOO_LONG_ERROR:
                break;
            default:
                break;
        }
    }
    else
    {
        //
        // Enable the "Up" button if we are no longer in the root directory.
        //
        if((g_pcCwdBuf[0] == '/') && (g_pcCwdBuf[1] == '\0'))
        {
        }
        else
        {
        }
    }

    //
    // Return the appropriate error code.
    //
    return(iFResult);
}

//*****************************************************************************
//
// This function implements the "pwd" command.  It simply prints the current
// working directory.
//
//*****************************************************************************
int
Cmd_pwd(int argc, char *argv[])
{
    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of a file
// and prints it to the console.  This should only be used on text files.  If
// it is used on a binary file, then a bunch of garbage is likely to printed on
// the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT iFResult;
    uint32_t ui32BytesRead;

    //
    // First, check to make sure that the current path (CWD), plus the file
    // name, plus a separator and trailing null, will all fit in the temporary
    // buffer that will be used to hold the file name.  The file name must be
    // fully specified, with path, to FatFs.
    //
    if(strlen(g_pcCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcTmpBuf))
    {
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_pcCwdBuf))
    {
        strcat(g_pcTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_pcTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return an error.
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it, until
    // the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit in the
        // temporary buffer, including a space for the trailing null.
        //
        iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1,
                          (UINT *)&ui32BytesRead);

        //
        // If there was an error reading, then print a newline and return the
        // error to the user.
        //
        if(iFResult != FR_OK)
        {
            return((int)iFResult);
        }

        //
        // Null terminate the last block that was read to make it a null
        // terminated string that can be used with printf.
        //
        g_pcTmpBuf[ui32BytesRead] = 0;

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(ui32BytesRead == sizeof(g_pcTmpBuf) - 1);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
//
//*****************************************************************************
StaticSemaphore_t xMutex_sd_Buffer;
int i16_sd_init(void)
{
	FRESULT iFResult;
	uint32_t return_msg;

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);
	ROM_TimerConfigure(TIMER4_BASE, TIMER_CFG_PERIODIC);
	ROM_TimerLoadSet(TIMER4_BASE, TIMER_A, (SYSTEM_CLOCK_GET() / 1000) * 10);
	TimerIntRegister(TIMER4_BASE, TIMER_A, &DiskTickHandler);
	ROM_IntEnable(INT_TIMER4A);
	ROM_TimerIntEnable(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerIntClear(TIMER4_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerEnable(TIMER4_BASE, TIMER_A);

	   
	/* Create sd card mutex. */
	x_sd_mutex = xSemaphoreCreateMutexStatic(&xMutex_sd_Buffer);
	
	while (i16_sd_take_semaphore(10))
	{
		vTaskDelay(1);
	}
	
	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}
	//
	// Mount the file system, using logical disk 0.
	//
	iFResult = f_mount(0, &g_sFatFs);
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();	
		i16_sd_give_semaphore();
		return(iFResult);
	}
	
	iFResult = ChangeToDirectory("/", &return_msg);

	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();	
		i16_sd_give_semaphore();
		return iFResult;
	}
	
	iFResult = f_opendir(&g_sDirObject, g_pcCwdBuf);
	
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();	
		i16_sd_give_semaphore();
		return iFResult;
	}
	
//	Cmd_ls(0, 0);

	i16_ff_give_semaphore();	
	
	i16_sd_give_semaphore();
	return 0;
}


int i16_sd_open_file(const char *txtname, bool RnW)
{
	FRESULT iFResult;
	static char g_pcTmpBuf[PATH_BUF_SIZE];

	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	memset(g_pcTmpBuf, 0, sizeof(g_pcTmpBuf));

	//
	// Now finally, append the file name to result in a fully specified file.
	//
	strcat(g_pcTmpBuf, txtname);

	if (RnW)
	{
		//
		// Open the file for reading.
		//
		iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);
	}
	else
	{
		iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_CREATE_NEW);
		if (iFResult == FR_EXIST)
		{
			iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_WRITE);
		}
		else
		{
			iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_WRITE);
		}
	}
	//
	// If there was some problem opening the file, then return an error.
	//
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return((int)iFResult);
	}

	i16_ff_give_semaphore();
	return (0);
}

int i16_sd_close_file(void)
{
	FRESULT iFResult;

	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	iFResult = f_close(&g_sFileObject);

	//
	// If there was some problem opening the file, then return an error.
	//
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return((int)iFResult);
	}

	i16_ff_give_semaphore();
	return (0);
}


int i16_sd_read_bytes(uint32_t u32_offset, uint8_t *readData, uint32_t NumofBytes)
{
	FRESULT iFResult;
	uint16_t ui16BytesRead;

	if (u32_offset >= g_sFileObject.fsize)
		return FR_INVALID_PARAMETER;
	
	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	iFResult = f_lseek(&g_sFileObject, u32_offset);
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return(iFResult);
	}

	iFResult = f_read(&g_sFileObject, readData, NumofBytes, (UINT *)&ui16BytesRead);
	//
	// If there was an error reading, then print a newline and return the
	// error to the user.
	//
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return((int)iFResult);
	}

	i16_ff_give_semaphore();
	//
	// Return success.
	//
	return(FR_OK);
}

int i16_sd_write_bytes(char *data, uint32_t NumofBytes)
{
	volatile FRESULT iFResult;
	volatile uint16_t ui16BytesWrite = 0;

	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	iFResult = f_lseek(&g_sFileObject, g_sFileObject.fsize);
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return(iFResult);
	}
	//
	// Enter a loop to repeatedly read data from the file and display it, until
	// the end of the file is reached.
	//
	iFResult = f_write(&g_sFileObject, (void *)data, NumofBytes,
			(UINT *)&ui16BytesWrite);


	i16_ff_give_semaphore();
	//
	// Return success.
	//
	return(iFResult);
}

int i16_sd_write_data(char *data, uint32_t NumofBytes,DWORD SD_Goto)
{
	volatile FRESULT iFResult;
	volatile uint16_t ui16BytesWrite = 0;

	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	iFResult = f_lseek(&g_sFileObject, SD_Goto);
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return(iFResult);
	}
	//
	// Enter a loop to repeatedly read data from the file and display it, until
	// the end of the file is reached.
	//
	iFResult = f_write(&g_sFileObject, (void *)data, NumofBytes,
			(UINT *)&ui16BytesWrite);


	i16_ff_give_semaphore();
	//
	// Return success.
	//
	return(iFResult);
}

int sd_remove_storage_file(char *file)
{
	volatile FRESULT iFResult;
	volatile uint16_t ui16BytesWrite = 0;

	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	//
	// Enter a loop to repeatedly read data from the file and display it, until
	// the end of the file is reached.
	//
	iFResult = f_unlink(file);

	i16_ff_give_semaphore();
	//
	// Return success.
	//
	return(iFResult);
	
}

int i16_sd_take_semaphore(int block_time)
{
	if (xSemaphoreTake(x_sd_mutex, block_time) == pdTRUE)
	{
		return 0;
	}
	return -1;
}

int i16_sd_give_semaphore(void)
{
	if (xSemaphoreGive(x_sd_mutex) == pdTRUE)
	{
		return 0;
	}
	return -1;
}

TCHAR* sd_read_one_line(char *buffer, int len)
{
	return f_gets(buffer, len, &g_sFileObject);
}

int i16_sd_read_one_line(uint32_t u32_offset, char *buffer, int len)
{
	FRESULT iFResult;
	uint16_t ui16BytesRead;

	if (u32_offset >= g_sFileObject.fsize)
		return FR_INVALID_PARAMETER;
	
	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}

	iFResult = f_lseek(&g_sFileObject, u32_offset);
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return(iFResult);
	}
	TCHAR* rc;
	rc = f_gets(buffer, len, &g_sFileObject);
	//
	// If there was an error reading, then print a newline and return the
	// error to the user.
	//
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();
		return((int)iFResult);
	}

	i16_ff_give_semaphore();
	//
	// Return success.
	//
	return(FR_OK);
}

int i16_sd_format(void)
{
	FRESULT iFResult;
	uint32_t return_msg;

	while (i16_ff_take_semaphore(10, true))
	{
		vTaskDelay(2);
	}	
	//
	// Format sd card
	//
	iFResult = f_mkfs (0,0,0);
	vTaskDelay(30000);	
	if(iFResult != FR_OK)
	{
		i16_ff_give_semaphore();	
		return iFResult;
	}	
	i16_ff_give_semaphore();	
	return 0;	
}