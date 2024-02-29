#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "hw_flash.h"
#include "hw_gpio.h"
#include "hw_memmap.h"
#include "hw_nvic.h"
#include "hw_sysctl.h"
#include "hw_types.h"
#include "gpio.h"
#include "pin_map.h"
#include "rom.h"
#include "rom_map.h"
#include "sysctl.h"
#include "udma.h"
#include "usblib.h"
#include "usbmsc.h"
#include "host/usbhost.h"
#include "host/usbhmsc.h"
#include "simple_fs.h"
#include "diskio.h"
#include "params.h"
#include "sdram.h"
#include "crc.h"
#include "boot_config.h"
#include "ff.h"
#include "sd_card.h"

typedef enum
{
	NEW_FIRMWARE_IS_CHECKING		= 0x00,
	NEW_FIRMWARE_IS_FALSE 			= 0x01,
	NEW_FIRMWARE_IS_TRUE 			= 0x02
} FOTA_FIRMWARE_STATUS;

/* Definition for 3V3 PERIPHERAL ENABLE Pin */
#if (PCB_VERSION < 0x02)
#define X		1
#define PER_3V3_GPIO_CLK_ENABLE()							ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION)
#define PER_3V3_GPIO_PORT											GPIO_PORTN_BASE
#define PER_3V3_PIN														GPIO_PIN_0
#else
#define X		2
#define PER_3V3_GPIO_CLK_ENABLE()							ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ)
#define PER_3V3_GPIO_PORT											GPIO_PORTQ_BASE
#define PER_3V3_PIN														GPIO_PIN_2
#endif
#define PER_3V3_SET()													ROM_GPIOPinWrite(PER_3V3_GPIO_PORT, PER_3V3_PIN, PER_3V3_PIN)
#define PER_3V3_RESET()												ROM_GPIOPinWrite(PER_3V3_GPIO_PORT, PER_3V3_PIN, 0x00)

/* Definition for 3V3 OUT ENABLE Pin */
#define OUT_3V3_GPIO_CLK_ENABLE()							ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK)
#define OUT_3V3_GPIO_PORT											GPIO_PORTK_BASE
#define OUT_3V3_PIN														GPIO_PIN_4
#define OUT_3V3_SET()													ROM_GPIOPinWrite(OUT_3V3_GPIO_PORT, OUT_3V3_PIN, OUT_3V3_PIN)
#define OUT_3V3_RESET()												ROM_GPIOPinWrite(OUT_3V3_GPIO_PORT, OUT_3V3_PIN, 0x00)

/* Definition for config Input Pin */
#define INPUT_GPIO_PER												SYSCTL_PERIPH_GPIOP
#define INPUT_GPIO_PORT												GPIO_PORTP_BASE
#define BUTTON_PIN														GPIO_PIN_5
#define SWITCH_1_PIN													GPIO_PIN_3	
#define SWITCH_2_PIN													GPIO_PIN_4

#define LED_DB_1_PER													SYSCTL_PERIPH_GPION
#define LED_DB_1_PORT													GPIO_PORTN_BASE
#define LED_DB_1_PIN													GPIO_PIN_0
#define LED_DB_1_ON()													ROM_GPIOPinWrite(LED_DB_1_PORT, LED_DB_1_PIN, LED_DB_1_PIN)
#define LED_DB_1_OFF()												ROM_GPIOPinWrite(LED_DB_1_PORT, LED_DB_1_PIN, 0x00)

#define LED_DB_2_PER													SYSCTL_PERIPH_GPION
#define LED_DB_2_PORT													GPIO_PORTN_BASE
#define LED_DB_2_PIN													GPIO_PIN_1
#define LED_DB_2_ON()													ROM_GPIOPinWrite(LED_DB_2_PORT, LED_DB_2_PIN, LED_DB_2_PIN)
#define LED_DB_2_OFF()												ROM_GPIOPinWrite(LED_DB_2_PORT, LED_DB_2_PIN, 0x00)

#define EXTERNAL_CLOCK
//#define CRYSTAL_CLOCK													SYSCTL_XTAL_25MHZ	
#define CRYSTAL_CLOCK												SYSCTL_XTAL_16MHZ
#define SYSTEM_CLOCK_GET(x)    ((uint32_t)120000000)

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>USB Memory Stick Updater (usb_stick_update)</h1>
//!
//! This example application behaves the same way as a boot loader.  It resides
//! at the beginning of flash, and will read a binary file from a USB memory
//! stick and program it into another location in flash.  Once the user
//! application has been programmed into flash, this program will always start
//! the user application until requested to load a new application.
//!
//! When this application starts, if there is a user application already in
//! flash (at \b APP_START_ADDRESS), then it will just run the user application.
//! It will attempt to load a new application from a USB memory stick under
//! the following conditions:
//!
//! - no user application is present at \b APP_START_ADDRESS
//! - the user application has requested an update by transferring control
//! to the updater
//! - the user holds down the eval board push button when the board is reset
//!
//! When this application is attempting to perform an update, it will wait
//! forever for a USB memory stick to be plugged in.  Once a USB memory stick
//! is found, it will search the root directory for a specific file name, which
//! is \e FIRMWARE.BIN by default.  This file must be a binary image of the
//! program you want to load (the .bin file), linked to run from the correct
//! address, at \b APP_START_ADDRESS.
//!
//! The USB memory stick must be formatted as a FAT16 or FAT32 file system
//! (the normal case), and the binary file must be located in the root
//! directory.  Other files can exist on the memory stick but they will be
//! ignored.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines the number of times to call to check if the attached device is
// ready.
//
//*****************************************************************************
#define USBMSC_DRIVE_RETRY      4

//*****************************************************************************
//
// The name of the binary firmware file on the USB stick.  This is the user
// application that will be searched for and loaded into flash if it is found.
// Note that the name of the file must be 11 characters total, 8 for the base
// name and 3 for the extension.  If the actual file name has fewer characters
// then it must be padded with spaces.  This macro should not contain the dot
// "." for the extension.
//
// Examples: firmware.bin --> "FIRMWAREBIN"
//           myfile.bn    --> "MYFILE  BN "
//
//*****************************************************************************
#define UPDATE_FILENAME     "FERTI.BIN"
#define ORIGINAL_FILENAME   "ROM.BIN"

//*****************************************************************************
//
// The size of the flash for this microcontroller.
//
//*****************************************************************************
#define FLASH_SIZE (1 * 1024 * 1024)

//*****************************************************************************
//
// The starting address for the application that will be loaded into flash
// memory from the USB stick.  This address must be high enough to be above
// the USB stick updater, and must be on a 1K boundary.
// Note that the application that will be loaded must also be linked to run
// from this address.
//
//*****************************************************************************
#define APP_START_ADDRESS 0x10000

//*****************************************************************************
//
// The prototype for the function that is used to call the user application.
//
//*****************************************************************************
void CallApplication(uint_fast32_t ui32StartAddr);

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 6 channels are needed.
//
//*****************************************************************************
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the MSC class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHostMSCClassDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
#define NUM_CLASS_DRIVERS       (sizeof(g_ppHostClassDrivers) /               \
                                 sizeof(g_ppHostClassDrivers[0]))

//*****************************************************************************
//
// Hold the current state for the application.
//
//*****************************************************************************
volatile enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Mass storage device is being enumerated.
    //
    STATE_DEVICE_ENUM,
}
g_eState;

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
tUSBHMSCInstance *g_psMSCInstance = 0;

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pHCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// A buffer for holding sectors read from the storage device
//
//*****************************************************************************
static uint8_t g_ui8SectorBuf[512];

//*****************************************************************************
//
// Global to hold the clock rate. Set once read many.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

volatile uint_fast32_t ui32FlashEnd;
volatile uint_fast32_t ui32FileSize;
volatile uint_fast32_t ui32DataSize;
volatile uint_fast32_t ui32Remaining;
volatile uint_fast32_t ui32ProgAddr;
volatile uint_fast32_t ui32SavedRegs[2];
volatile uint_fast32_t ui32Idx;
volatile uint_fast32_t ui32DriveTimeout;
volatile bool b_reset = false;

/* Firmware Binary data */
static uint8_t au8_fw_bin[FLASH_SIZE-APP_START_ADDRESS] __attribute__((at(SDRAM_FOTA_DATA_MAPPING_ADDRESS)));

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

#define LED_DB_CLK_ENABLE()										ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK); ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK)
#define LED_DB1_PORT													GPIO_PORTK_BASE
#define LED_DB1_PIN														GPIO_PIN_4
#define LED_DB2_PORT													GPIO_PORTK_BASE
#define LED_DB2_PIN														GPIO_PIN_3

#define LED_DB1_ON()													ROM_GPIOPinWrite(LED_DB1_PORT, LED_DB1_PIN, LED_DB1_PIN)
#define LED_DB1_OFF()													ROM_GPIOPinWrite(LED_DB1_PORT, LED_DB1_PIN, 0x00)
#define LED_DB2_ON()													ROM_GPIOPinWrite(LED_DB2_PORT, LED_DB2_PIN, LED_DB2_PIN)
#define LED_DB2_OFF()													ROM_GPIOPinWrite(LED_DB2_PORT, LED_DB2_PIN, 0x00)

void v_LED_Init(void)
{
	LED_DB_CLK_ENABLE();
	GPIOPinTypeGPIOOutput(LED_DB1_PORT, LED_DB1_PIN);
	GPIOPinTypeGPIOOutput(LED_DB2_PORT, LED_DB2_PIN);
}
//*****************************************************************************
//
//Init watchdog timer
//
//*****************************************************************************
void v_IWDG_Init (void)
{
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
	ROM_WatchdogReloadSet(WATCHDOG0_BASE, (uint32_t)SYSTEM_CLOCK_GET() * 60);
	ROM_WatchdogResetEnable(WATCHDOG0_BASE);
	ROM_WatchdogEnable(WATCHDOG0_BASE);
	//ROM_IntEnable(INT_WATCHDOG);
	ROM_IntMasterEnable();
} 
void v_IWD_DeInit(void)
{
	ROM_WatchdogResetDisable(WATCHDOG0_BASE);
	
}
//*****************************************************************************
//
//Reload watchdog timer
//
//*****************************************************************************
void v_IWDG_Reload(void)
{
	ROM_WatchdogReloadSet(WATCHDOG0_BASE, (uint32_t)SYSTEM_CLOCK_GET() * 60);
}
//*****************************************************************************
//
// Read a sector from the USB mass storage device.
//
// \param ui32Sector is the sector to read from the connected USB mass storage
// device (memory stick)
// \param pui8Buf is a pointer to the buffer where the sector data should be
// stored
//
// This is the application-specific implementation of a function to read
// sectors from a storage device, in this case a USB mass storage device.
// This function is called from the \e simple_fs.c file when it needs to read
// data from the storage device.
//
// \return Non-zero if data was read from the device, 0 if no data was read.
//
//*****************************************************************************
uint32_t
SimpleFsReadMediaSector(uint_fast32_t ui32Sector, uint8_t *pui8Buf)
{
    //
    // Return the requested sector from the connected USB mass storage
    // device.
    //
//    return(USBHMSCBlockRead(g_psMSCInstance, ui32Sector, pui8Buf, 1));
	return disk_read(0, pui8Buf, ui32Sector, 1);
}

//*****************************************************************************
//
// This is the callback from the MSC driver.
//
// \param ui32Instance is the driver instance which is needed when communicating
// with the driver.
// \param ui32Event is one of the events defined by the driver.
// \param pvData is a pointer to data passed into the initial call to register
// the callback.
//
// This function handles callback events from the MSC driver.  The only events
// currently handled are the \b MSC_EVENT_OPEN and \b MSC_EVENT_CLOSE.  This
// allows the main routine to know when an MSC device has been detected and
// enumerated and when an MSC device has been removed from the system.
//
// \return Returns \e true on success or \e false on failure.
//
//*****************************************************************************
static void
MSCCallback(tUSBHMSCInstance *psMSCInstance, uint32_t ui32Event, void *pvData)
{
    //
    // Determine the event.
    //
    switch(ui32Event)
    {
        //
        // Called when the device driver has successfully enumerated an MSC
        // device.
        //
        case MSC_EVENT_OPEN:
        {
            //
            // Proceed to the enumeration state.
            //
            g_eState = STATE_DEVICE_ENUM;
            break;
        }

        //
        // Called when the device driver has been unloaded due to error or
        // the device is no longer present.
        //
        case MSC_EVENT_CLOSE:
        {
            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_eState = STATE_NO_DEVICE;
            break;
        }

        default:
        {
            break;
        }
    }
}

//*****************************************************************************
//
// Read the application image from the file system and program it into flash.
//
// This function will attempt to open and read the firmware image file from
// the mass storage device.  If the file is found it will be programmed into
// flash.  The name of the file to be read is configured by the macro
// \b UPDATE_FILENAME.  It will be programmed into flash starting at the
// address specified by APP_START_ADDRESS.
//
// \return Zero if successful or non-zero if the file cannot be read or
// programmed.
//
//*****************************************************************************
uint32_t
ReadAppAndProgram(void)
{
//    uint_fast32_t ui32FlashEnd;
//    uint_fast32_t ui32FileSize;
//    uint_fast32_t ui32DataSize;
//    uint_fast32_t ui32Remaining;
//    uint_fast32_t ui32ProgAddr;
//    uint_fast32_t ui32SavedRegs[2];
//    //volatile uint_fast32_t ui32Idx;
//    uint_fast32_t ui32DriveTimeout;
		volatile uint32_t u32_file_addr = 0;

    //
    // Attempt to open the firmware file, retrieving the file/image size.
    // A file size of error means the file was not there, or there was an
    // error.
    //
		ui32FileSize = 0;
    if (i16_sd_open_file(b_reset ? ORIGINAL_FILENAME : UPDATE_FILENAME, true, &ui32FileSize))
    {
        return(1);
    }
    
    if(ui32FileSize == 0)
    {
        return(1);
    }

    //
    // Get the size of flash.  This is the ending address of the flash.
    // If reserved space is configured, then the ending address is reduced
    // by the amount of the reserved block.
    //
    ui32FlashEnd = FLASH_SIZE;
#ifdef FLASH_RSVD_SPACE
    ui32FlashEnd -= FLASH_RSVD_SPACE;
#endif

    //
    // If flash code protection is not used, then change the ending address
    // to be the ending of the application image.  This will be the highest
    // flash page that will be erased and so only the necessary pages will
    // be erased.  If flash code protection is enabled, then all of the
    // application area pages will be erased.
    //
#ifndef FLASH_CODE_PROTECTION
    ui32FlashEnd = ui32FileSize + APP_START_ADDRESS;
#endif

    //
    // Check to make sure the file size is not too large to fit in the flash.
    // If it is too large, then return an error.
    //
    if((ui32FileSize + APP_START_ADDRESS) > ui32FlashEnd)
    {
        return(1);
    }

    //
    // Enter a loop to erase all the requested flash pages beginning at the
    // application start address (above the USB stick updater).
    //
    for(ui32Idx = APP_START_ADDRESS; ui32Idx < ui32FlashEnd; ui32Idx += 1024)
    {
        ROM_FlashErase(ui32Idx);
    }

    //
    // Enter a loop to read sectors from the application image file and
    // program into flash.  Start at the user app start address (above the USB
    // stick updater).
    //
    ui32ProgAddr = APP_START_ADDRESS;
    ui32Remaining = ui32FileSize;
		u32_file_addr = 0;
    while(ui32Remaining)
    {
			if (i16_sd_read_bytes(u32_file_addr, (uint8_t *)g_ui8SectorBuf, 512) == 0)
			{
				u32_file_addr += 512;
				
				//
				// Compute how much data was read from this sector and adjust the
				// remaining amount.
				//
				ui32DataSize = ui32Remaining >= 512 ? 512 : ui32Remaining;
				ui32Remaining -= ui32DataSize;

				//
				// Special handling for the first block of the application.
				// This block contains as the first two location the application's
				// initial stack pointer and instruction pointer.  The USB updater
				// relied on the values in these locations to determine if a valid
				// application is present.  When there is a valid application the
				// updater runs the user application.  Otherwise the updater attempts
				// to load a new application.
				// In order to prevent a partially programmed imaged (due to some
				// error occurring while programming), the first two locations are
				// not programmed until all of the rest of the image has been
				// successfully loaded into the flash.  This way if there is some error,
				// the updater will detect that a user application is not present and
				// will not attempt to run it.
				//
				// For the first block, do not program the first two word locations
				// (8 bytes).  These two words will be programmed later, after
				// everything else.
				//
				if(ui32ProgAddr == APP_START_ADDRESS)
				{
						uint32_t *pui32Temp;

						pui32Temp = (uint32_t *)g_ui8SectorBuf;
						ui32SavedRegs[0] = pui32Temp[0];
						ui32SavedRegs[1] = pui32Temp[1];

						//
						// Call the function to program a block of flash.  Skip the first
						// two words (8 bytes) since these contain the initial SP and PC.
						//
						ROM_FlashProgram((uint32_t *)&g_ui8SectorBuf[8],
														 ui32ProgAddr + 8,
														 ((ui32DataSize - 8) + 3) & ~3);
				}

				//
				// All other blocks except the first block
				//
				else
				{
						//
						// Call the function to program a block of flash.  The length of the
						// block passed to the flash function must be divisible by 4.
						//
						ROM_FlashProgram((uint32_t *)g_ui8SectorBuf, ui32ProgAddr,
														 (ui32DataSize + 3) & ~3);
				}

				//
				// If there is more image to program, then update the programming
				// address.  Progress will continue to the next iteration of
				// the while loop.
				//
				if(ui32Remaining)
				{
						ui32ProgAddr += ui32DataSize;
				}

				//
				// Otherwise we are done programming so perform final steps.
				// Program the first two words of the image that were saved earlier,
				// and return a success indication.
				//
				else
				{
						ROM_FlashProgram((uint32_t *)ui32SavedRegs, APP_START_ADDRESS,
															8);

						return(0);
				}
			}
			else
			{
			}
    }

    //
    // If we make it here, that means that an attempt to read a sector of
    // data from the device was not successful.  That means that the complete
    // user app has not been programmed into flash, so just return an error
    // indication.
    //
    return(1);
}

//*****************************************************************************
//
// This is the main routine for performing an update from a mass storage
// device.
//
// This function forms the main loop of the USB stick updater.  It polls for
// a USB mass storage device to be connected,  Once a device is connected
// it will attempt to read a firmware image from the device and load it into
// flash.
//
// \return None.
//
//*****************************************************************************
void
UpdaterUSB(void)
{
    //
    // Loop forever, running the USB host driver.
    //
    while(1)
    {
        USBHCDMain();

        //
        // Check for a state change from the USB driver.
        //
        switch(g_eState)
        {
            //
            // This state means that a mass storage device has been
            // plugged in and enumerated.
            //
            case STATE_DEVICE_ENUM:
            {
                //
                // Attempt to read the application image from the storage
                // device and load it into flash memory.
                //
                if(ReadAppAndProgram())
                {
                    //
                    // There was some error reading or programming the app,
                    // so reset the state to no device which will cause a
                    // wait for a new device to be plugged in.
                    //
                    g_eState = STATE_NO_DEVICE;
                }
                else
                {
                    //
                    // User app load and programming was successful, so reboot
                    // the micro.  Perform a software reset request.  This
                    // will cause the microcontroller to reset; no further
                    // code will be executed.
                    //
                    HWREG(NVIC_APINT) = NVIC_APINT_VECTKEY |
                                        NVIC_APINT_SYSRESETREQ;

                    //
                    // The microcontroller should have reset, so this should
                    // never be reached.  Just in case, loop forever.
                    //
                    while(1)
                    {
                    }
                }
                break;
            }

            //
            // This state means that there is no device present, so just
            // do nothing until something is plugged in.
            //
            case STATE_NO_DEVICE:
            {
                break;
            }
        }
    }
}

//*****************************************************************************
//
// Configure the USB controller and power the bus.
//
// This function configures the USB controller for host operation.
// It is assumed that the main system clock has been configured at this point.
//
// \return None.
//
//*****************************************************************************
void
ConfigureUSBInterface(void)
{
    //
    // Enable the uDMA controller and set up the control table base.
    // This is required by usblib.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(80);
    uDMAEnable();
    uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Enable the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Set the USB pins to be controlled by the USB controller.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    ROM_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    //
    // Register the host class driver
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, NUM_CLASS_DRIVERS);

    //
    // Open an instance of the mass storage class driver.
    //
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Force the USB mode to host with no callback on mode changes since
    // there should not be any.
    //
    USBStackModeSet(0, eUSBModeForceHost, 0);

    //
    // Wait 10ms for the pin to go low.
    //
    SysCtlDelay(g_ui32SysClock/100);

    //
    // Initialize the host controller.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);
}

//*****************************************************************************
//
// Generic configuration is handled in this function.
//
// This function is called by the start up code to perform any configuration
// necessary before calling the update routine.  It is responsible for setting
// the system clock to the expected rate and setting flash programming
// parameters prior to calling ConfigureUSBInterface() to set up the USB
// hardware.
//
// \return None.
//
//*****************************************************************************
void
UpdaterMain(void)
{
    //
    // Call the updater function.  This will attempt to load a new image
    // into flash from a USB memory stick.
    //
    UpdaterUSB();
}

uint32_t
CheckUpdateFile(void)
{
    //
    // Initialize the drive timeout.
    //
    ui32DriveTimeout = USBMSC_DRIVE_RETRY;

    //
    // Check to see if the mass storage device is ready.  Some large drives
    // take a while to be ready, so retry until they are ready.
    //
    while(USBHMSCDriveReady(g_psMSCInstance))
    {
        //
        // Wait about 500ms before attempting to check if the
        // device is ready again.
        //
        SysCtlDelay(g_ui32SysClock/(3*2));

        //
        // Decrement the retry count.
        //
        ui32DriveTimeout--;

        //
        // If the timeout is hit then return with a failure.
        //
        if(ui32DriveTimeout == 0)
        {
            return(1);
        }
    }

    //
    // Initialize the file system and return if error.
    //
    if(SimpleFsInit(g_ui8SectorBuf))
    {
        return(1);
    }

    //
    // Attempt to open the firmware file, retrieving the file/image size.
    // A file size of error means the file was not there, or there was an
    // error.
    //
    ui32FileSize = SimpleFsOpen(b_reset ? ORIGINAL_FILENAME : UPDATE_FILENAME);
    if(ui32FileSize == 0)
    {
        return(1);
    }

    //
    // If we make it here, that means that an attempt to read a sector of
    // data from the device was successful.
    //
    return(0);
}

uint32_t program_fw(void)
{
	 volatile uint32_t u32_file_addr = 0;
    uint32_t *pui32Temp;
	
		ui32FileSize = 0;
    
    if (i16_sd_open_file(b_reset ? ORIGINAL_FILENAME : UPDATE_FILENAME, true, &ui32FileSize))
    {
        return(1);
    }
    
    if(ui32FileSize == 0)
    {
        return(1);
    }
		
    //
    // Get the size of flash.  This is the ending address of the flash.
    // If reserved space is configured, then the ending address is reduced
    // by the amount of the reserved block.
    //
    ui32FlashEnd = FLASH_SIZE;
#ifdef FLASH_RSVD_SPACE
    ui32FlashEnd -= FLASH_RSVD_SPACE;
#endif

    //
    // If flash code protection is not used, then change the ending address
    // to be the ending of the application image.  This will be the highest
    // flash page that will be erased and so only the necessary pages will
    // be erased.  If flash code protection is enabled, then all of the
    // application area pages will be erased.
    //
#ifndef FLASH_CODE_PROTECTION
    ui32FlashEnd = ui32FileSize + APP_START_ADDRESS;
#endif

    //
    // Enter a loop to erase all the requested flash pages beginning at the
    // application start address.
    //
    for(ui32Idx = APP_START_ADDRESS; ui32Idx < ui32FlashEnd; ui32Idx += 1024)
    {
        ROM_FlashErase(ui32Idx);
    }

    //
    // Program firmware on SDRam into flash.  Start at the user app start address.
    //
    ui32ProgAddr = APP_START_ADDRESS;
    pui32Temp = (uint32_t *)au8_fw_bin;
    ui32SavedRegs[0] = pui32Temp[0];
    ui32SavedRegs[1] = pui32Temp[1];
		ui32Remaining = ui32FileSize;
		u32_file_addr = 0;

    //
    // Call the function to program a block of flash.  Skip the first
    // two words (8 bytes) since these contain the initial SP and PC.
    //
    ROM_FlashProgram((uint32_t *)&au8_fw_bin[8],
                     ui32ProgAddr + 8,
                     ((sizeof(au8_fw_bin) - 8) + 3) & ~3);

    ROM_FlashProgram((uint32_t *)ui32SavedRegs, APP_START_ADDRESS,
                      8);
		i16_sd_read_bytes(u32_file_addr, (uint8_t *)g_ui8SectorBuf, 512);
    return(0);
}

uint32_t usb_read_fw(void)
{
    uint8_t *pui8_ram_buff = au8_fw_bin;
    //
    // Check to make sure the file size is not too large to fit in the flash.
    // If it is too large, then return an error.
    //
    if((ui32FileSize + APP_START_ADDRESS) > FLASH_SIZE)
    {
        return(1);
    }

    //
    // Enter a loop to read sectors from the application image file
    //
    ui32Remaining = ui32FileSize;
    while(SimpleFsReadFileSector())
    {
        //
        // Compute how much data was read from this sector and adjust the
        // remaining amount.
        //
        ui32DataSize = ui32Remaining >= 512 ? 512 : ui32Remaining;
        ui32Remaining -= ui32DataSize;

        memcpy(pui8_ram_buff, g_ui8SectorBuf, ui32DataSize);
        pui8_ram_buff += ui32DataSize;
    }

	//
	// Success
	//
	return(0);
}

int i16_check_update_file(void)
{
	int i;
	i = 1;
	
	while (i--)
	{
		USBHCDMain();

		//
		// Check for a state change from the USB driver.
		//
		switch(g_eState)
		{
			//
			// This state means that a mass storage device has been
			// plugged in and enumerated.
			//
			case STATE_DEVICE_ENUM:
			{
				//
				// Attempt to read the application image from the storage
				// device and load it into flash memory.
				//
				if(CheckUpdateFile())
				{
					//
					// There was some error reading or programming the app,
					// so reset the state to no device which will cause a
					// wait for a new device to be plugged in.
					//
					g_eState = STATE_NO_DEVICE;
					return 0;
				}
				else
				{
					return 1;
				}
				break;
			}

			//
			// This state means that there is no device present, so just
			// do nothing until something is plugged in.
			//
			case STATE_NO_DEVICE:
			{
				break;
			}
		}
	}

	return 0;
}


uint32_t
sd_CheckUpdateFile(void)
{
	//
	// Wait about 500ms before attempting to check if the
	// device is ready again.
	//
	SysCtlDelay(g_ui32SysClock/(3*2));

	//
	// Initialize the file system and return if error.
	//
	if(SimpleFsInit(g_ui8SectorBuf))
	{
			return(1);
	}

	//
	// Attempt to open the firmware file, retrieving the file/image size.
	// A file size of error means the file was not there, or there was an
	// error.
	//
	ui32FileSize = SimpleFsOpen(b_reset ? ORIGINAL_FILENAME : UPDATE_FILENAME);
	if(ui32FileSize == 0)
	{
		return(1);
	}

	//
	// If we make it here, that means that an attempt to read a sector of
	// data from the device was successful.
	//
	return(0);
}


uint32_t sd_read_fw(void)
{
    uint8_t *pui8_ram_buff = au8_fw_bin;

	//
	// Check to make sure the file size is not too large to fit in the flash.
	// If it is too large, then return an error.
	//
	if((ui32FileSize + APP_START_ADDRESS) > FLASH_SIZE)
	{
			return(1);
	}

	//
	// Enter a loop to read sectors from the application image file and
	// program into flash.  Start at the user app start address (above the USB
	// stick updater).
	//
	ui32Remaining = ui32FileSize;
	while(SimpleFsReadFileSector())
	{
		//
		// Compute how much data was read from this sector and adjust the
		// remaining amount.
		//
		ui32DataSize = ui32Remaining >= 512 ? 512 : ui32Remaining;
		ui32Remaining -= ui32DataSize;


        memcpy(pui8_ram_buff, g_ui8SectorBuf, ui32DataSize);
        pui8_ram_buff += ui32DataSize;
	}

	//
	// Success
	//
	return(0);
}

int i16_sd_check_update_file(void)
{
	//
	// Attempt to read the application image from the storage
	// device and load it into flash memory.
	//
	if(sd_CheckUpdateFile())
	{
		return 0;
	}
	else
	{
		return 1;
	}

	return 0;
}

//*****************************************************************************
//
// Main entry point for the USB stick update example.
//
// This function will check to see if a flash update should be performed from
// the USB memory stick, or if the user application should just be run without
// any update.
//
// The following checks are made, any of which mean that an update should be
// performed:
// - the PC and SP for the user app do not appear to be valid
// - a memory location contains a certain value, meaning the user app wants
//   to force an update
// - the user button on the eval board is being pressed, meaning the user wants
//   to force an update even if there is a valid user app in memory
//
// If any of the above checks are true, then that means that an update should
// be attempted.  The USB stick updater will then wait for a USB stick to be
// plugged in, and once it is look for a firmware update file.
//
// If none of the above checks are true, then the user application that is
// already in flash is run and no update is performed.
//
// \return None.
//
//*****************************************************************************
int
main(void)
{
    uint32_t *pui32App;
    bool ret = false;
    bool b_force_update = false;
    FOTA_FIRMWARE_STATUS t_fota_status = NEW_FIRMWARE_IS_FALSE;
    uint32_t u32_store_crc = 0, u32_calc_crc = 0;
    uint32_t u32_BL_timeout;

    //
    // Make sure NVIC points at the correct vector table.
    //
    HWREG(NVIC_VTABLE) = 0;

    //
    // Run from the PLL at 120 MHz.
    //
#ifdef EXTERNAL_CLOCK
    g_ui32SysClock = MAP_SysCtlClockFreqSet((CRYSTAL_CLOCK|
                                             SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);
#else
	    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ|
                                             SYSCTL_OSC_INT | SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);
#endif

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    ROM_FPULazyStackingEnable();
		//Enable watchdog timer
		v_IWDG_Init();
		
		PER_3V3_GPIO_CLK_ENABLE();
		GPIOPinTypeGPIOOutput(PER_3V3_GPIO_PORT, PER_3V3_PIN);
		PER_3V3_SET();
		
    ROM_SysCtlPeripheralEnable(INPUT_GPIO_PER);
    ROM_GPIOPinTypeGPIOInput(INPUT_GPIO_PORT, BUTTON_PIN);
		
    SysCtlPeripheralEnable(LED_DB_1_PER);
		while(!(SysCtlPeripheralReady(LED_DB_1_PER)));	
		GPIOPinTypeGPIOOutput(LED_DB_1_PORT, LED_DB_1_PIN);
		
		SysCtlPeripheralEnable(LED_DB_2_PER);
		while(!(SysCtlPeripheralReady(LED_DB_2_PER)));	
		GPIOPinTypeGPIOOutput(LED_DB_1_PORT, LED_DB_2_PIN);
	
    u32_BL_timeout = 500;
    while (!ROM_GPIOPinRead(INPUT_GPIO_PORT, BUTTON_PIN))
    {
        ROM_SysCtlDelay(g_ui32SysClock / 300);   //10ms
        u32_BL_timeout--;
        if (u32_BL_timeout == 0)
        {
            b_reset = true;
            ret = true;
            b_force_update = true;
						LED_DB_2_ON()	;
            break;
        }
    }
		v_IWDG_Reload();
    //
    // See if the first location is 0xfffffffff or something that does not
    // look like a stack pointer, or if the second location is 0xffffffff or
    // something that does not look like a reset vector.
    //
    pui32App = (uint32_t *)APP_START_ADDRESS;
    if((pui32App[0] == 0xffffffff) ||
        ((pui32App[0] & 0xfff00000) != 0x20000000) ||
        (pui32App[1] == 0xffffffff) ||
        ((pui32App[1] & 0xfff00001) != 0x00000001))
    {
        //
        // App starting stack pointer or PC is not valid, so force an update.
        //
        ret = true;
        b_force_update = true;
    }

    if (!ret)
    {
        // Init parameter module to get firmware status
        s32_PARAMS_Init();
        
        // Check if there is a new update
        // Get the CRC to check if there is no error while store firmware
        s32_PARAMS_Get(PARAMS_ID_FOTA_STATUS, (uint8_t *)&t_fota_status);
        s32_PARAMS_Get(PARAMS_ID_FOTA_FW_CRC, (uint8_t *)&u32_store_crc);
        
        // If there is a new firmware
        if (t_fota_status == NEW_FIRMWARE_IS_TRUE)
        {
            ret = true;
        }
			
    }
		
    v_IWDG_Reload();
    if (ret)
    {
        v_sdram_init(g_ui32SysClock);
        //
        // Configure the USB interface and power the bus.
        //
        ConfigureUSBInterface();

			if (i16_check_update_file())
			{
				usb_read_fw();
			}
			else
			{
					PER_3V3_GPIO_CLK_ENABLE();
					GPIOPinTypeGPIOOutput(PER_3V3_GPIO_PORT, PER_3V3_PIN);
					OUT_3V3_GPIO_CLK_ENABLE();
					GPIOPinTypeGPIOOutput(OUT_3V3_GPIO_PORT, OUT_3V3_PIN);
					//Turn off a sec to completely shut down SD card
					PER_3V3_RESET();
					OUT_3V3_RESET();
					
					ROM_SysCtlDelay((g_ui32SysClock / 3) * 2);   //2000ms
					PER_3V3_SET();
					OUT_3V3_SET();
					ROM_SysCtlDelay(SYSTEM_CLOCK_GET() * 2);
					ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
					//
					// PD0-3 are used for SSI0 to the second booster pack.
					//
					ROM_GPIOPinConfigure(GPIO_PF1_SSI3XDAT0);
					ROM_GPIOPinConfigure(GPIO_PF0_SSI3XDAT1);
					ROM_GPIOPinConfigure(GPIO_PF3_SSI3CLK);
					ROM_GPIOPinTypeSSI(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);
					ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
					ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
					ROM_SysCtlDelay(g_ui32SysClock); 
					disk_initialize(0);
			}
        
			if (ret)
			{
				v_IWDG_Reload();
				if (!i16_sd_init())
				{
					ReadAppAndProgram();
				}
				//Turn off a sec to completely shut down SD card
				PER_3V3_RESET();
				OUT_3V3_RESET();
				
				ROM_SysCtlDelay((g_ui32SysClock / 3) * 2);   //2000ms
				PER_3V3_SET();
				OUT_3V3_SET();
				
				LED_DB_2_OFF();
			}
    }
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOG);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOH);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOJ);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOK);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOL);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOM);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPION);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOP);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOQ);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOR);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOS);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_GPIOT);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_EEPROM0);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_EPI0);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_UDMA);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_USB0);
    ROM_SysCtlPeripheralDisable(SYSCTL_PERIPH_SSI2);
		v_sdram_init(g_ui32SysClock);
    v_save_boot_config();
		v_read_boot_config();
		v_IWD_DeInit();
    //
    // If we get to here that means that none of the conditions that should
    // cause an update are true.  Therefore, call the application.
    //
		
    CallApplication(APP_START_ADDRESS);
}

//*****************************************************************************
//
// This function is used to call the user application.  It will set the NVIC
// to point at the user app's vector table, load up the user app's stack
// pointer, and then jump to the application.
//
// This function must be programmed in assembly since it needs to directly
// manipulate the value in the stack pointer, and because it needs to perform
// a direct branch to the user app and not a function call (bl).
//
//*****************************************************************************
__asm void
CallApplication(uint_fast32_t ui32StartAddr)
{
    //
    // Set the vector table address to the beginning of the application.
    //
    ldr     r1, =0xe000ed08
    str     r0, [r1]

    //
    // Load the stack pointer from the application's vector table.
    //
    ldr     r1, [r0]
    mov     sp, r1

    //
    // Load the initial PC from the application's vector table and branch to
    // the application's entry point.
    //
    ldr     r0, [r0, #4]
    bx      r0
}
