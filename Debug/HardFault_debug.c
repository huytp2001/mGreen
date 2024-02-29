#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "HardFault_debug.h"
#include "rom.h"

#pragma NOINIT(u32_HardFault_Data)
__attribute__((zero_init)) uint32_t u32_HardFault_Data[7] __attribute__((at(0x2003FFD4)));
/*
*Data in u32_HardFault_Data:
*[0] HardFault occurred
*[1] r1
*[2] pc1
*[3] pc2
*[4] lr		//Link register
*[5] pc   //Program counter
*[6] psr	//Program status register
*/
void v_hardfault_params_get(uint32_t *u32_hardfault_params)
{
	memcpy(u32_hardfault_params, u32_HardFault_Data,
																7 * sizeof(uint32_t));
	memset(u32_HardFault_Data, 0, sizeof(uint32_t));
}
__asm void asm_func()
{
	TST lr, #4
	ITE eq
	MRSEQ r0, msp
	MRSNE r0, psp
	//LDR r1, [r0, #24]
	MOV r1, r0
	
}
static void prvGetRegistersFromStack(uint32_t _r0, uint32_t _r1)
{
	static uint32_t *u32_address;
	u32_address = (uint32_t *)_r1;
	u32_HardFault_Data[0] = 1;
	u32_HardFault_Data[1] = _r1;
	u32_HardFault_Data[2] = u32_address[5];
	u32_HardFault_Data[3] = u32_address[6];
	__asm
	{
		MOV u32_HardFault_Data[4], __return_address()
		MOV u32_HardFault_Data[5], __current_pc()
		MOV u32_HardFault_Data[6], __current_sp()
	}
	//ROM_SysCtlReset();
}
__attribute__( (naked) ) void HardFault_Handler(void)
{
	
	//asm_func();
	__asm
	{
		BL prvGetRegistersFromStack
	}
	ROM_SysCtlReset();
}
