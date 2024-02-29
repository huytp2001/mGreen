/*
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   Copyright (C) 2013 Inteliss Ltd.
**   All Rights Reserved.
**
**   The information contained herein is copyrighted by and
**   is the sole property of Inteliss Ltd.
**   Any unauthorized use, copying, transmission, distribution
**   or disclosure of such information is strictly prohibited.
**
**   This Copyright notice shall not be removed or modified
**   without prior written consent of Inteliss Ltd.
**
**   Inteliss Ltd. reserves the right to modify this software
**   without notice.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
**
**   File Name   : crc.h
**   Project     : 
**   Description : Header file for crc.c to calculate crc 32 bits.
**
** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*/


#ifndef __CRC_H__
#define __CRC_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Prototypes for the functions.
//
//*****************************************************************************

extern uint32_t u32_CRC32 (uint32_t u32_crc, const uint8_t *pu8_data,
                           uint32_t u32_count);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __CRC_H__
