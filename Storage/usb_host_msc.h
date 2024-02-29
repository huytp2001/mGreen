#ifndef USB_HOST_MSC_H
#define USB_HOST_MSC_H
//*****************************************************************************
//
// usb_host_msc.c - Example program for reading files from a USB flash drive.
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
#include "driverlib/udma.h"
#include "usblib/usblib.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "ff.h"
#include "diskio.h"
#include "usb_host_msc.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

void USBTimerTickHandler(void);
void USBHCDEvents(void *pvData);
uint32_t GetTickms(void);
int i16_host_msc_init(void);
bool b_host_msc_process(void);

#endif
