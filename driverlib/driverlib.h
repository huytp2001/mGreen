/*
 * -------------------------------------------
 *    TM4C129X DriverLib - v01_04_00_18 
 * -------------------------------------------
 *
 * --COPYRIGHT--,BSD,BSD
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef __DRIVERLIB__H_
#define __DRIVERLIB__H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "adc.h"
#include "aes.h"
#include "can.h"
#include "comp.h"
#include "cpu.h"
#include "crc.h"
#include "debug.h"
#include "des.h"
#include "eeprom.h"
#include "emac.h"
#include "epi.h"
#include "flash.h"
#include "fpu.h"
#include "gpio.h"
//#include "hibernate.h"
#include "i2c.h"
#include "interrupt.h"
//#include "lcd.h"
#include "mpu.h"
#include "onewire.h"
#include "pin_map.h"
#include "pwm.h"
#include "qei.h"
#include "rom.h"
#include "rom_map.h"
#include "rtos_bindings.h"
#include "shamd5.h"
#include "ssi.h"
#include "sw_crc.h"
#include "sysctl.h"
#include "sysexc.h"
#include "systick.h"
#include "timer.h"
#include "uart.h"
#include "udma.h"
#include "usb.h"
#include "watchdog.h"

#include "asmdefs.h"
#include "hw_adc.h"
#include "hw_aes.h"
#include "hw_can.h"
#include "hw_ccm.h"
#include "hw_comp.h"
#include "hw_des.h"
#include "hw_eeprom.h"
#include "hw_emac.h"
#include "hw_epi.h"
#include "hw_fan.h"
#include "hw_flash.h"
#include "hw_gpio.h"
#include "hibernate.h"
#include "hw_i2c.h"
#include "hw_ints.h"
#include "hw_lcd.h"
#include "hw_memmap.h"
#include "hw_nvic.h"
#include "hw_pwm.h"
#include "hw_qei.h"
#include "hw_shamd5.h"
#include "hw_ssi.h"
#include "hw_sysctl.h"
#include "hw_sysexc.h"
#include "hw_timer.h"
#include "hw_types.h"
#include "hw_uart.h"
#include "hw_udma.h"
#include "hw_usb.h"
#include "hw_watchdog.h"

#define SYSTEM_CLOCK_GET(x)    ((uint32_t)120000000)
#ifndef SYSTEM_CLOCK_GET
#error "Please define system clock, the SysCtlClockGet() may be wrong on some cases"
#endif

#endif
