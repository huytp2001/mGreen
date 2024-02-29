/*! @file pHEC.h
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
* All rights reserved.
*/
#ifndef _PHEC_H
#define _PHEC_H
#ifdef __cplusplus
extern “C” {
#endif	
#include "driverlib.h"
#include "config.h"	
#include "protocol_mgreen.h"
#include "spi_mgreen.h"
/*!
* @data types, constants and macro defintions
*/
#define SPI_EC				0
#define SPI_PH				1

//#define TIMEOUT_PHEC	2


#define TIME_DELAY_READ_PH			(3.5 * 1000/50)	//3s
#define TIME_DELAY_READ_EC			(4 * 1000/50)  //4s

#define MID_PH				7
#define LOW_PH 				4.01
#define HIGH_PH				10.01
#define MID_RAW				2701
#define LOW_RAW				2461
#define HIGH_RAW			2925

#define FB_FIT_1			0.000000000274797 * 0.001
#define FB_FIT_2			0.000056969310306 * 0.001
#define FB_FIT_3			(-0.161203403404237 * 0.001)

#define A_PARA				(-0.136690647)
#define B_PARA				(0.191366906)

#define nH						0.001672

#define NUM_READ_PH		10
#define CALIB_EC_HIGH 500
#define CALIB_EC_LOW	141
#define CALIB_PH_401	401
#define CALIB_PH_701 	701
#define CALIB_PH_1001	1001

#define EC_VARIANT_0_1        0
#define EC_VARIANT_0_2        20
#define EC_VARIANT_1_1        70
#define EC_VARIANT_1_2        60
#define EC_VARIANT_2_1        150
#define EC_VARIANT_2_2        150
#define EC_VARIANT_3_1        200
#define EC_VARIANT_3_2        150

#define EC_POINT_0                 0   //uS/cm
#define EC_POINT_1                 141.3
#define EC_POINT_2                 329.0
#define EC_POINT_3                 500.0

typedef enum
{
	CALIB_POINT_EC1 = 0,
	CALIB_POINT_EC2,
	CALIB_POINT_EC3,
	CALIB_POINT_PH1,
	CALIB_POINT_PH2,
	CALIB_POINT_PH3,
}e_calib_point;

typedef enum
{
	PHEC_STATE_IDLE = 0,
	PHEC_STATE_START,
	PHEC_STATE_WAIT,
	PHEC_STATE_READ,
	PHEC_STATE_SEND,
}E_READ_PHEC_STATE;
typedef enum
{
	ECPH_CALIB_IDLE = 0,
	EC_CALIB_START = 1,
	EC_CALIB_WAIT = 2,
	EC_CALIB_READ = 3,
	EC_CALIB_CAL_1413 = 4,
	EC_CALIB_CAL_3000 = 5,
	EC_CALIB_CAL_5000 = 6,
	EC_CALIB_FINISH = 7,
	PH_CALIB_START = 8,
	PH_CALIB_WAIT = 9,
	PH_CALIB_READ = 10,
	PH_CALIB_CAL_401 = 11,
	PH_CALIB_CAL_701 = 12,
	PH_CALIB_CAL_1001 = 13,
	PH_CALIB_FINISH = 14,
	
}E_CALIB_EC_STATE;
void pHECInit(void);
void pHStart(void);
void pHStop(void);
bool pHRead(uint8_t* pHdata);
void ECStart(void);
void ECStop(void);
bool ECRead(uint8_t* ECdata);
uint16_t convertpHdata(uint16_t u16_pH);
uint16_t convertECdata(uint16_t u16_EC, float f_temperature);
float f_convert_pt100_data(float RTDnominal, float refResistor, float Rt);
uint16_t u16_add_calib_EC(uint16_t u16_EC);
uint16_t u16_add_calib_pH(uint16_t u16_pH);
bool b_set_calib_point(e_calib_point e_point, uint16_t u16_value);
void v_save_EC_calib_point(void);
void v_save_pH_calib_point(void);
//larange function
double lf_lagrange_poly3(double lf_x0, double lf_x1, double lf_x2, double lf_x3,
                            double lf_y0, double lf_y1, double lf_y2, double lf_y3,
                            double lf_x);
double lf_linear_poly3(double lf_x0, double lf_x1, double lf_x2, double lf_x3,
                            double lf_y0, double lf_y1, double lf_y2, double lf_y3,
                            double lf_x);
bool b_pH_EC_process(uint32_t *u32_pHEC_value);
void v_enable_pHEC(bool b_enable, uint32_t u32_time_take_sample);
bool b_phEC_calib_process(uint8_t *u8_point);
void v_calib_pHEC(uint8_t u8_point);
#endif  /*__PHEC_H_*/
