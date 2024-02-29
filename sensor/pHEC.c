/*! @file pHEC.c
* 
* @brief:
*
* @par       
* COPYRIGHT NOTICE: (c)Mimosatek 2019.  
* All rights reserved.
*/
/*!
* @include statements
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "driverlib.h"
#include "config.h"
#include "spi_mgreen.h"
#include "phEC.h"
#include "frame_parser.h"
#include "mqtt_publish.h"
#include "sensor.h"
#include "eeprom_manage.h"
#include "params.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "rtc.h"

#define PARAM_EC_VALID_INDEX			0
#define PARAM_EC_1413_INDEX				1
#define PARAM_EC_3290_INDEX				2
#define PARAM_EC_5000_INDEX				3

#define PARAM_PH_VALID_INDEX			4
#define PARAM_PH_401_INDEX				5
#define PARAM_PH_701_INDEX				6
#define PARAM_PH_1001_INDEX				7

#define RTD_A 3.9083e-3
#define RTD_B -5.775e-7
#define TEMP_DE_OFFSET						4

const unsigned int CALIB_EC_FACTORY_POINT_0 = 0;
const unsigned int CALIB_EC_FACTORY_POINT_1 = 141;
const unsigned int CALIB_EC_FACTORY_POINT_2 = 329;
const unsigned int CALIB_EC_FACTORY_POINT_3 = 500;

static mqtt_data_frame_t stru_ph_ec_data;
/*static variables*/
static uint16_t CALIB_EC_POINT_0 = CALIB_EC_FACTORY_POINT_0;
static uint16_t CALIB_EC_POINT_1 = CALIB_EC_FACTORY_POINT_1;
static uint16_t CALIB_EC_POINT_2 = CALIB_EC_FACTORY_POINT_2;
static uint16_t CALIB_EC_POINT_3 = CALIB_EC_FACTORY_POINT_3;
static uint16_t u16_EC_calib_1413, u16_EC_calib_3290, u16_EC_calib_5000, u16_pH_calib_401, u16_pH_calib_701, u16_pH_calib_1001;
static uint8_t u8_time_read_pH = 0;
static int16_t i16_delta_EC_low, i16_delta_EC_high, i16_delta_pH_401, i16_delta_pH_701, i16_delta_pH_1001;
static double d_A, d_B, d_O, d_ApH1, d_BpH1, d_ApH2, d_BpH2, d_OpH;
static uint32_t u32_pHEC_Request_Time = 0;
static bool b_ReadpHEC = false;
static uint8_t u8_calib_step = 0;
bool b_calib_EC = false, b_calib_pH = false;

static uint8_t u8SpiTxRxByte(uint8_t u8_data, bool spi_channel)
{
	return u8_spi_mgreen_tx_rx_byte(u8_data, spi_channel);
}
/*Global functions*/
void pHECInit(void)
{
//	EC_CS_ON();
//	PH_CS_ON();
//	EC_CLK_ON();
//	PH_CLK_ON();
	//init eeprom
//	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
//	ROM_EEPROMInit();
	//read calib value
	uint16_t au16_calib_data[8];
	
	while (s32_params_get (PARAMS_ID_EC_CALIB_VALUES, (uint8_t *)au16_calib_data) != 0)
	{
		vTaskDelay (2);
	}
	if(au16_calib_data[PARAM_EC_VALID_INDEX] == 1)
	{
		CALIB_EC_POINT_1 = au16_calib_data[PARAM_EC_1413_INDEX];
		if(CALIB_EC_POINT_1 == 0)
			CALIB_EC_POINT_1 = CALIB_EC_FACTORY_POINT_1;
		CALIB_EC_POINT_2 = au16_calib_data[PARAM_EC_3290_INDEX];
		if(CALIB_EC_POINT_2 == 0)
			CALIB_EC_POINT_2 = CALIB_EC_FACTORY_POINT_2;
		CALIB_EC_POINT_3= au16_calib_data[PARAM_EC_5000_INDEX];
		if(CALIB_EC_POINT_3 == 0)
			CALIB_EC_POINT_3 = CALIB_EC_FACTORY_POINT_3;
	}
	while (s32_params_get (PARAMS_ID_PH_CALIB_VALUES, (uint8_t *)(au16_calib_data + 4)) != 0)
	{
		vTaskDelay (2);
	}
	if(au16_calib_data[PARAM_PH_VALID_INDEX] == 1)
	{
		u16_pH_calib_1001 = au16_calib_data[PARAM_PH_1001_INDEX];
		u16_pH_calib_701 = au16_calib_data[PARAM_PH_701_INDEX];
		u16_pH_calib_401 = au16_calib_data[PARAM_PH_401_INDEX];
		i16_delta_pH_401 = u16_pH_calib_401 - CALIB_PH_401;
		i16_delta_pH_701 = u16_pH_calib_701 - CALIB_PH_701;
		i16_delta_pH_1001 = u16_pH_calib_1001 - CALIB_PH_1001;
		//calculate parameters for calib lines
		d_ApH1 = (double)(i16_delta_pH_701 - i16_delta_pH_401)/(double)(u16_pH_calib_701 - u16_pH_calib_401);
		d_BpH1 = ((double)i16_delta_pH_401 - d_ApH1 * (double)u16_pH_calib_401)/100;
		d_OpH = (double)i16_delta_pH_401/(double)u16_pH_calib_401;
		
		d_ApH2 = (double)(i16_delta_pH_1001 - i16_delta_pH_701)/(double)(u16_pH_calib_1001 - u16_pH_calib_701);
		d_BpH2 = ((double)i16_delta_pH_701 - d_ApH2 * (double)u16_pH_calib_701)/100;
	}
	else
	{
		d_ApH1 = 0;
		d_BpH1 = 0;
		d_ApH2 = 0;
		d_BpH2 = 0;
		d_OpH = 0;
	}
}
void pHStart(void)
{
	PH_CS_OFF();
	delayTrobe();

	u8SpiTxRxByte('R', SPI_PH);
	u8SpiTxRxByte(0x0D, SPI_PH);
	
	PH_CS_ON();
}
void pHStop(void)
{
	PH_CS_OFF();
	delayTrobe();

	u8SpiTxRxByte('S', SPI_PH);
	u8SpiTxRxByte(0x0D, SPI_PH);
	
	PH_CS_ON();
}
void ECStart(void)
{
	EC_CS_OFF();
	delayTrobe();
	u8SpiTxRxByte('R', SPI_EC);
	u8SpiTxRxByte(0x0D, SPI_EC);
	
	EC_CS_ON();
}
void ECStop(void)
{
	EC_CS_OFF();
	delayTrobe();
	u8SpiTxRxByte('S', SPI_EC);
	u8SpiTxRxByte(0x0D, SPI_EC);
	
	EC_CS_ON();
}
//pH read
bool pHRead(uint8_t* pHECdata)
{
	uint32_t data_cnt=0;
	uint8_t data_byte;
	uint8_t done=0;
	uint32_t timeout=0;
	PH_CS_OFF();
	delay(2);
	
	while(1)
	{
		data_byte=u8SpiTxRxByte(0x00, SPI_PH);
		if (((data_byte >= '0') && (data_byte <= '9')) || (data_byte == ',') || (data_byte == '.') 
			|| (data_byte == '\r') || (data_byte == '\n'))
		{
			
			pHECdata[data_cnt++] = data_byte;
			if (data_cnt == 20)
				data_cnt = 0;
			if (data_byte == '\n')
			{			
				pHECdata[data_cnt++] = 0;
				if (data_cnt < 6)//0,0\r\n0
				{
					done = 0;
					break;
				}
				else
				{
					if ((pHECdata[0]>'9') || (pHECdata[0]<'0'))
					{						
						done = 0;
						break;
					}
					else//success
					{		
						done = 1;
						break;
					}
				}
			}		
		}
		timeout++;
		if (timeout > 100)
		{
			done = 0;
			break;
		}
	}
	PH_CS_ON(); 
	delayCLK();
	return done;
}
//EC read
bool ECRead(uint8_t* pHECdata)
{
	uint32_t data_cnt=0;
	uint8_t data_byte;
	uint8_t done=0;
	uint32_t timeout=0;
	EC_CS_OFF();
	delay(2);
	
	while(1)
	{
		data_byte=u8SpiTxRxByte(0x00, SPI_EC);
		if (((data_byte >= '0') && (data_byte <= '9')) || (data_byte == ',') || (data_byte == '.') 
			|| (data_byte == '\r') || (data_byte == '\n'))
		{
			
			pHECdata[data_cnt++] = data_byte;
			if (data_cnt == 20)
				data_cnt = 0;
			if (data_byte == '\n')
			{			
				pHECdata[data_cnt++] = 0;
				if (data_cnt < 6)//0,0\r\n0
				{
					done = 0;
					break;
				}
				else
				{
					if ((pHECdata[0]>'9') || (pHECdata[0]<'0'))
					{						
						done = 0;
						break;
					}
					else//success
					{		
						done = 1;
						break;
					}
				}
			}		
		}
		timeout++;
		if (timeout > 100)
		{
			done = 0;
			break;
		}
	}
	EC_CS_ON(); 
	delayCLK();
	return done;
}
//convert pH from raw to real value
uint16_t convertpHdata(uint16_t u16_pH)
{
	uint16_t data;
	if(u16_pH < MID_RAW)
	{
		data = 100*(((float)MID_PH - (float)LOW_PH)/((float)MID_RAW-(float)LOW_RAW) * ((float)u16_pH - (float)LOW_RAW) + (float)LOW_PH);
	}
	else
	{
		data = (100*((float)(HIGH_PH - MID_PH)/(float)(HIGH_RAW-MID_RAW) * (float)(u16_pH - MID_RAW) + (float)MID_PH));
	}
	return data;
}
//convert EC from raw to real value
uint16_t convertECdata(uint16_t u16_EC, float f_temperature)
{
	float y1_internal, f;
	double data;
	y1_internal = (FB_FIT_1*u16_EC*u16_EC) + (FB_FIT_2*u16_EC) + FB_FIT_3;
	f = 100*((1000 * y1_internal)/(1-(100*y1_internal)));
	if(f < 0)
	{
		data = 0;
	}
	else
	{
		data =  (f * 1.307);
	}
	if(data < 140 && data > 0)
	{
		data += (A_PARA * ((double)data/100) + B_PARA)*100;
	}
	//temperature compesate
	if((f_temperature > TEMP_DE_OFFSET) && (f_temperature < 80))
	{
		f_temperature = f_temperature - TEMP_DE_OFFSET;
		f_temperature = (f_temperature - 25)/10;
		f_temperature = 1 - 0.20346 * f_temperature + 0.03822 * f_temperature * f_temperature
										- 0.00555 * pow(f_temperature, 3);
		data = f_temperature * data;
	}
	return ((uint16_t)(data));	
}
float f_convert_pt100_data(float RTDnominal, float refResistor, float Rt)
{
	double Z1, Z2, Z3, Z4, temp;
	double lf_Rt = 0.0;
	lf_Rt = (double)Rt;
	
  lf_Rt /= 32768;
  lf_Rt *= refResistor;
  
  // Serial.print("\nResistance: "); Serial.println(lf_Rt, 8);

  Z1 = -RTD_A;
  Z2 = RTD_A * RTD_A - (4 * RTD_B);
  Z3 = (4 * RTD_B) / RTDnominal;
  Z4 = 2 * RTD_B;

  temp = Z2 + (Z3 * lf_Rt);
  temp = (sqrt(temp) + Z1) / Z4;
  
  if (temp >= 0) return temp;

  // ugh.
  lf_Rt /= RTDnominal;
  lf_Rt *= 100;      // normalize to 100 ohm

  float rpoly = lf_Rt;

  temp = -242.02;
  temp += 2.2228 * rpoly;
  rpoly *= lf_Rt;  // square
  temp += 2.5859e-3 * rpoly;
  rpoly *= lf_Rt;  // ^3
  temp -= 4.8260e-6 * rpoly;
  rpoly *= lf_Rt;  // ^4
  temp -= 2.8183e-8 * rpoly;
  rpoly *= lf_Rt;  // ^5
  temp += 1.5243e-10 * rpoly;

  return temp;
}
bool b_set_calib_point(e_calib_point e_point, uint16_t u16_value)
{
	bool ret = false;
	switch(e_point)
	{
		case CALIB_POINT_EC1:
		{
			if(u16_value > (CALIB_EC_FACTORY_POINT_1 - EC_VARIANT_1_1) && u16_value < (CALIB_EC_FACTORY_POINT_1 + EC_VARIANT_1_2))
			{
				CALIB_EC_POINT_1 = u16_value;
				ret = true;
			}
		}
		break;
		case CALIB_POINT_EC2:
		{
			if(u16_value > (CALIB_EC_FACTORY_POINT_2 - EC_VARIANT_2_1) && u16_value < (CALIB_EC_FACTORY_POINT_2 + EC_VARIANT_2_2))
			{
				CALIB_EC_POINT_2 = u16_value;
				ret = true;
			}
		}
		break;
		case CALIB_POINT_EC3:
		{
			if(u16_value > (CALIB_EC_FACTORY_POINT_3 - EC_VARIANT_3_1) && u16_value < (CALIB_EC_FACTORY_POINT_3 + EC_VARIANT_3_2))
			{
				CALIB_EC_POINT_3 = u16_value;
				ret = true;
			}
		}
		break;
		case CALIB_POINT_PH1:
		{
			if(u16_value > 250 && u16_value < 550)
			{
				u16_pH_calib_401 = u16_value;
				ret = true;
			}
		}
		break;
		case CALIB_POINT_PH2:
		{
			if(u16_value > 550 && u16_value < 850)
			{
				u16_pH_calib_701 = u16_value;
				ret = true;
			}
		}
		break;
		case CALIB_POINT_PH3:
		{
			if(u16_value > 850 && u16_value < 1150)
			{
				u16_pH_calib_1001 = u16_value;
				ret = true;
			}
		}
		break;
		default:
			break;
	}
	return ret;
}

void v_save_EC_calib_point(void)
{
	uint16_t a[4] = {1, CALIB_EC_POINT_1, CALIB_EC_POINT_2, CALIB_EC_POINT_3};
	while (s32_params_set (PARAMS_ID_EC_CALIB_VALUES, 8, (uint8_t *)a) != 0)
	{
		vTaskDelay (2);
	}
}

uint8_t u8_temp;
void v_save_pH_calib_point(void)
{
	uint16_t a[4] = {1, u16_pH_calib_401, u16_pH_calib_701, u16_pH_calib_1001};
	while (s32_params_set (PARAMS_ID_PH_CALIB_VALUES, 8, (uint8_t *)a) != 0)
	{
		vTaskDelay (2);
	}
	i16_delta_pH_401 = u16_pH_calib_401 - CALIB_PH_401;
	i16_delta_pH_701 = u16_pH_calib_701 - CALIB_PH_701;
	i16_delta_pH_1001 = u16_pH_calib_1001 - CALIB_PH_1001;
	//calculate parameters for calib lines
	d_ApH1 = (double)(i16_delta_pH_701 - i16_delta_pH_401)/(double)(u16_pH_calib_701 - u16_pH_calib_401);
	d_BpH1 = ((double)i16_delta_pH_401 - d_ApH1 * (double)u16_pH_calib_401)/100;
	d_OpH = (double)i16_delta_pH_401/(double)u16_pH_calib_401;
	
	d_ApH2 = (double)(i16_delta_pH_1001 - i16_delta_pH_701)/(double)(u16_pH_calib_1001 - u16_pH_calib_701);
	d_BpH2 = ((double)i16_delta_pH_701 - d_ApH2 * (double)u16_pH_calib_701)/100;
}
//add calib to real EC value
uint16_t u16_add_calib_EC(uint16_t u16_EC)
{
	u16_EC = lf_linear_poly3((double)CALIB_EC_POINT_0, (double)CALIB_EC_POINT_1, (double)CALIB_EC_POINT_2,(double)CALIB_EC_POINT_3,
														(double)EC_POINT_0, (double)EC_POINT_1, (double)EC_POINT_2, (double)EC_POINT_3,
														(double)(u16_EC));		
	return u16_EC;
}
//add calib to real pH value
uint16_t u16_add_calib_pH(uint16_t u16_pH)
{
	//TODO: add ofset
	if(u16_pH < CALIB_PH_401)
	{
		u16_pH -= d_OpH*(double)u16_pH;
	}
	else if(u16_pH >= CALIB_PH_401 && u16_pH < CALIB_PH_701)
	{
		u16_pH -= (d_ApH1*(double)u16_pH/100 + d_BpH1)*100;
	}
	else
	{
		u16_pH -= (d_ApH2*(double)u16_pH/100 + d_BpH2)*100;
	}
	return u16_pH;
}
//larange function
double lf_lagrange_poly3(double lf_x0, double lf_x1, double lf_x2, double lf_x3,
                            double lf_y0, double lf_y1, double lf_y2, double lf_y3,
                            double lf_x)
{
    //L0 = ((x - x1) * (x - x2) * (x - x3)) / ((x0 - x1) * (x0 - x2) * (x0 - x3))
    //L1 = ((x - x0) * (x - x2) * (x - x3)) / ((x1 - x0) * (x1 - x2) * (x1 - x3))
    //L2 = ((x - x0) * (x - x1) * (x - x3)) / ((x2 - x0) * (x2 - x1) * (x2 - x3))
    //L3 = ((x - x0) * (x - x1) * (x - x2)) / ((x3 - x0) * (x3 - x1) * (x3 - x2))
    // f(x) = y0 * L0 + y1 * L1 + y2 * L2 + y3 * L3

    // check input
    if((lf_x0 == lf_x1) || (lf_x0 == lf_x2) || (lf_x0 == lf_x3) || (lf_x1 == lf_x2) || (lf_x1 == lf_x3) || (lf_x2 == lf_x3))
    {
        return 0;
    }
    double lf_L0, lf_L1, lf_L2, lf_L3;
    double lf_res = 0;

    lf_L0 = (double)((lf_x - lf_x1) * (lf_x - lf_x2) * (lf_x - lf_x3)) / (double)((lf_x0 - lf_x1) * (lf_x0 - lf_x2) * (lf_x0 - lf_x3));
    lf_L1 = (double)((lf_x - lf_x0) * (lf_x - lf_x2) * (lf_x - lf_x3)) / (double)((lf_x1 - lf_x0) * (lf_x1 - lf_x2) * (lf_x1 - lf_x3));
    lf_L2 = (double)((lf_x - lf_x0) * (lf_x - lf_x1) * (lf_x - lf_x3)) / (double)((lf_x2 - lf_x0) * (lf_x2 - lf_x1) * (lf_x2 - lf_x3));
    lf_L3 = (double)((lf_x - lf_x0) * (lf_x - lf_x1) * (lf_x - lf_x2)) / (double)((lf_x3 - lf_x0) * (lf_x3 - lf_x1) * (lf_x3 - lf_x2));

    lf_res = (double)lf_y0 * lf_L0 + (double)lf_y1 * lf_L1 + (double)lf_y2 * lf_L2 + (double)lf_y3 * lf_L3;
    if(lf_res < 0)
    {
        lf_res = 0;
    }
    return ((double)lf_res);
}
double lf_linear_poly3(double lf_x0, double lf_x1, double lf_x2, double lf_x3,
                            double lf_y0, double lf_y1, double lf_y2, double lf_y3,
                            double lf_x)
{
    // check input
    if((lf_x0 == lf_x1) || (lf_x0 == lf_x2) || (lf_x0 == lf_x3) || (lf_x1 == lf_x2) || (lf_x1 == lf_x3) || (lf_x2 == lf_x3))
    {
        return 0;
    }
		//y= ax +b;
    double lf_res = 0;
		double tmp_x1, tmp_x2, tmp_y1, tmp_y2;
		if(lf_x <= lf_x1)
		{
			tmp_x1 = lf_x0;
			tmp_x2 = lf_x1;
			tmp_y1 = lf_y0;
			tmp_y2 = lf_y1;
		}
		else if(lf_x <= lf_x2)
		{
			tmp_x1 = lf_x1;
			tmp_x2 = lf_x2;
			tmp_y1 = lf_y1;
			tmp_y2 = lf_y2;			
		}
		else
		{
			tmp_x1 = lf_x2;
			tmp_x2 = lf_x3;
			tmp_y1 = lf_y2;
			tmp_y2 = lf_y3;			
		}
		double lf_a, lf_b;
		
    lf_a = (tmp_y2 - tmp_y1)/(tmp_x2 - tmp_x1);
		lf_b = tmp_y2 - lf_a*tmp_x2;
		
		lf_res = lf_a * lf_x + lf_b;
		
    if(lf_res < 0)
    {
        lf_res = 0;
    }
    return ((double)lf_res);
}
/*End*/

static E_READ_PHEC_STATE e_phec_state = PHEC_STATE_IDLE;
static uint8_t u8_pHECdata[20];
static uint8_t u8_retry = 0;
static uint16_t u16_pH, u16_EC, u16_temperature;
static uint32_t u32_timeout_pHEC, u8_timeout_calib_pHEC;
static uint32_t u32_ec_max = 0;
bool b_pH_EC_process(uint32_t *u32_pHEC_value)
{
	*u32_pHEC_value = 0xFFFFFFFF;
	switch(e_phec_state)
	{
		case PHEC_STATE_IDLE:
		{
			u32_timeout_pHEC++;
		 if(u32_timeout_pHEC >= u32_pHEC_Request_Time)
		 {
			 u32_timeout_pHEC = 0;
			 if(b_ReadpHEC)
				 e_phec_state = PHEC_STATE_START;
		 }
		}break;
		case PHEC_STATE_START:
		{
			ECStart();
			pHStart();
			e_phec_state = PHEC_STATE_WAIT;
		}break;
		case PHEC_STATE_WAIT:
		{
			u32_timeout_pHEC++;
			if(u32_timeout_pHEC >= TIME_DELAY_READ_EC)
			{
				u32_timeout_pHEC = 0;
			 e_phec_state = PHEC_STATE_READ;
			}
		}break;
		case PHEC_STATE_READ:
		{
			bool b_done = pHRead(u8_pHECdata);
			u8_retry++;
			u16_pH = atoi((char *)u8_pHECdata);
			for(int i = 0; i < 20; i++)
			{
			 u8_pHECdata[i] = 0;
			}
			b_done &= ECRead(u8_pHECdata);
			char *index;
			if(b_done)
			{
				index = strstr((const char *)u8_pHECdata, ",");
				u32_ec_max = atoi((char *) u8_pHECdata);
				if (u32_ec_max > 0xFFFF)
				{
					u32_ec_max = 0xFFFF;
				}
				u16_EC = (uint16_t)u32_ec_max;
				u16_temperature = atoi(index + 1);
			}
			if(u16_pH < 2000 || u16_pH > 3400)
			{
			 b_done = false;
			}
			if(u16_EC < 100)
			{
			 b_done = false;
			}
			if(!b_done && u8_retry < 5)		//retry
			{
			 vTaskDelay(2000);
			 e_phec_state = PHEC_STATE_START;
			}
			else
			{
			 if(!b_done)		//over retry
			 {
				 *u32_pHEC_value = 0;
			 }
			 else
			 {
				 u16_EC = convertECdata(u16_EC, f_convert_pt100_data(100, 430, u16_temperature));
				 u16_EC = u16_add_calib_EC(u16_EC);
				 u16_pH = convertpHdata(u16_pH);
				 u16_pH = u16_add_calib_pH(u16_pH);
			 }
			 //TODO: clear data buffer
			 for(int i = 0; i < 20; i++)
			 {
				 u8_pHECdata[i] = 0;
			 }
			 b_done = false;
			 u8_retry = 0;
			 e_phec_state = PHEC_STATE_SEND;
			}
		}break;
		case PHEC_STATE_SEND:
		{
			if(b_ReadpHEC)
			{
			 *u32_pHEC_value = u16_EC << 16 | u16_pH;
			}
			e_phec_state = PHEC_STATE_IDLE;
			return true;
		}break;
	}
	return false;
}

void v_enable_pHEC(bool b_enable, uint32_t u32_time_take_sample)
{
	b_ReadpHEC = b_enable;
	u32_pHEC_Request_Time = u32_time_take_sample;
	if(b_enable == false)
	{
		ECStop();
		pHStop();
	}
}

void v_calib_pHEC(uint8_t u8_point)
{
	u8_calib_step = u8_point;
	b_ReadpHEC = false;
	if(u8_point > 0 && u8_point <= 4)
	{
		b_calib_EC = true;
	}
	else
	{
		b_calib_pH = true;
	}
}

static E_CALIB_EC_STATE e_EC_calib_state = ECPH_CALIB_IDLE;
static uint32_t u32_timeout_calib_pHEC = 0;
bool b_phEC_calib_process(uint8_t *u8_point)
{
	bool ret = false;
	switch(e_EC_calib_state)
	{
	 case ECPH_CALIB_IDLE:
	 {
		 if(e_phec_state == PHEC_STATE_IDLE && b_calib_EC && !b_ReadpHEC)
		 {
			 u8_retry = 0;
			 if(u8_calib_step < 4)
			 {
				 e_EC_calib_state = EC_CALIB_START;
			 }
			 else
				 e_EC_calib_state = EC_CALIB_FINISH;
		 }
		 else if((e_phec_state == PHEC_STATE_IDLE) && (b_calib_pH) && (!b_ReadpHEC))
		 {
				u8_retry = 0;
				if(u8_calib_step < 8 && u8_calib_step > 4)
					e_EC_calib_state = PH_CALIB_START;
				else
					e_EC_calib_state = PH_CALIB_FINISH;
		 }
	 }
	 break;
	 case EC_CALIB_START:
	 {
		 ECStart();
		 e_EC_calib_state = EC_CALIB_WAIT;
	 }
	 break;
	 case EC_CALIB_WAIT:
	 {
		 u32_timeout_calib_pHEC++;
		 if(u32_timeout_calib_pHEC >= TIME_DELAY_READ_EC)
		 {
			 u32_timeout_calib_pHEC = 0;
			 e_EC_calib_state = EC_CALIB_READ;
		 }
	 }
	 break;
	 case EC_CALIB_READ:
	 {
		 bool b_done = ECRead(u8_pHECdata);
		 u8_retry++;
		 char *index;
		 if(b_done)
		 {
				index = strstr((const char *)u8_pHECdata, ",");
				u16_EC = atoi((char *) u8_pHECdata);
				u16_temperature = atoi(index + 1);
		 }
		 if(u16_EC < 100)
		 {
			 b_done = false;
		 }
		 if(!b_done && u8_retry < 3)
		 {
			 e_EC_calib_state = EC_CALIB_START;
		 }
		 else
		 {
			 if(!b_done)
			 {
				 e_EC_calib_state = ECPH_CALIB_IDLE;
				 u8_calib_step = 0;
				 *u8_point = u8_calib_step;
				 v_mqtt_data_pub(0xfb, u8_calib_step);
				 break;
			 }
			 else
			 {
				 u16_EC = convertECdata(u16_EC, f_convert_pt100_data(100, 430, u16_temperature));
			 }
			 if(u8_calib_step == 1)
			 {
				 e_EC_calib_state = EC_CALIB_CAL_1413;
			 }
			 else if(u8_calib_step == 2)
			 {
				 e_EC_calib_state = EC_CALIB_CAL_3000;
			 }
			 else if(u8_calib_step == 3)
			 {
				e_EC_calib_state = EC_CALIB_CAL_5000;
			 }
			 v_mqtt_data_pub(0xfb, u8_calib_step);
		 }
	 }
	 break;
	 case EC_CALIB_CAL_1413:
	 {
		 if(b_set_calib_point(CALIB_POINT_EC1, u16_EC))
		 {
			 e_EC_calib_state = ECPH_CALIB_IDLE;
			 *u8_point = u8_calib_step;
			 u8_calib_step = 0;
			 b_calib_EC = false;
			 v_mqtt_data_pub(0xec, u16_EC);
		 }
		 else
		 {
				e_EC_calib_state = ECPH_CALIB_IDLE;
				u8_calib_step = 0;
			 *u8_point = u8_calib_step;
				b_calib_EC = false;
			 v_mqtt_data_pub(0xec, u16_EC);
		 }
		 ret = true;
	 }
	 break;
	 case EC_CALIB_CAL_3000:
	 {
		 if(b_set_calib_point(CALIB_POINT_EC2, u16_EC))
		 {
			 e_EC_calib_state = ECPH_CALIB_IDLE;
			 *u8_point = u8_calib_step;
			 u8_calib_step = 0;
			 b_calib_EC = false;
		 }
		 else
		 {
				e_EC_calib_state = ECPH_CALIB_IDLE;
				u8_calib_step = 0;
				*u8_point = u8_calib_step;
				b_calib_EC = false;
		 }
		 ret = true;
	 }
	 break;
	 case EC_CALIB_CAL_5000:
	 {
		 if(b_set_calib_point(CALIB_POINT_EC3, u16_EC))
		 {
			 e_EC_calib_state = ECPH_CALIB_IDLE;
			 *u8_point = u8_calib_step;
			 u8_calib_step = 0;
			 b_calib_EC = false;
		 }
		 else
		 {
				e_EC_calib_state = ECPH_CALIB_IDLE;
				u8_calib_step = 0;
				*u8_point = u8_calib_step;
				b_calib_EC = false;
		 }
		 ret = true;
	 }
	 break;
	 case EC_CALIB_FINISH:
	 {
		 v_save_EC_calib_point();
		 *u8_point = u8_calib_step;
		 e_EC_calib_state = ECPH_CALIB_IDLE;
		 u8_calib_step = 0;
		 b_calib_EC = false;
		 ret = true;
	 }
	 break;
	 case PH_CALIB_START:
	 {
		 pHStart();
		 e_EC_calib_state = PH_CALIB_WAIT;
	 }
	 break;
	 case PH_CALIB_WAIT:
	 {
		 u32_timeout_calib_pHEC++;
		 if(u32_timeout_calib_pHEC >= TIME_DELAY_READ_PH)
		 {
			 u32_timeout_calib_pHEC = 0;
			 e_EC_calib_state = PH_CALIB_READ;
		 }
	 }
	 break;
	 case PH_CALIB_READ:
	 {
		 bool b_done = pHRead(u8_pHECdata);
		 u8_retry++;
		 u16_pH = atoi((char *)u8_pHECdata);
		 if(u16_pH < 2000 || u16_pH > 3400)
		 {
			 b_done = false;
		 }
		 if(!b_done && u8_retry < 3)
		 {
			 e_EC_calib_state = PH_CALIB_START;
		 }
		 else
		 {
			 if(!b_done)
			 {
				 e_EC_calib_state = ECPH_CALIB_IDLE;
				 u8_calib_step = 0;
				 *u8_point = u8_calib_step;
				 break;
			 }
			 else
			 {
				 if(u8_time_read_pH < NUM_READ_PH)
				 {
					 u8_time_read_pH++;
					 e_EC_calib_state = PH_CALIB_START;
				 }
				 else
				 {
					 u8_time_read_pH = 0;
					 b_calib_pH = false;
					 u16_pH = convertpHdata(u16_pH);
					 if(u8_calib_step == 5)
					 {
						 e_EC_calib_state = PH_CALIB_CAL_401;
					 }
					 else if(u8_calib_step == 6)
					 {
						 e_EC_calib_state = PH_CALIB_CAL_701;
					 }
					 else if(u8_calib_step == 7)
					 {
						e_EC_calib_state = PH_CALIB_CAL_1001;
					 }
				 }
			 }
			 for(int i = 0; i < 20; i++)
			 {
				 u8_pHECdata[i] = 0;
			 }
			 b_done = false;
			 u8_retry = 0;
		 }
	 }
	 break;
	 case PH_CALIB_CAL_401:
	 {
		 if(b_set_calib_point(CALIB_POINT_PH1, u16_pH))
		 {
			 e_EC_calib_state = ECPH_CALIB_IDLE;
			 *u8_point = u8_calib_step;
			 u8_calib_step = 0;
			 b_calib_pH = false;
		 }
		 else
		 {
				e_EC_calib_state = ECPH_CALIB_IDLE;
				u8_calib_step = 0;
			 *u8_point = u8_calib_step;
				b_calib_pH = false;
		 }
		 ret = true;
	 }
	 break;
	 case PH_CALIB_CAL_701:
	 {
		 if(b_set_calib_point(CALIB_POINT_PH2, u16_pH))
		 {
			 e_EC_calib_state = ECPH_CALIB_IDLE;
			 *u8_point = u8_calib_step;
			 u8_calib_step = 0;
			 b_calib_pH = false;
		 }
		 else
		 {
				e_EC_calib_state = ECPH_CALIB_IDLE;
				u8_calib_step = 0;
			 *u8_point = u8_calib_step;
				b_calib_pH = false;
		 }
		 ret = true;
	 }
	 break;
	 case PH_CALIB_CAL_1001:
	 {
		 if(b_set_calib_point(CALIB_POINT_PH3, u16_pH))
		 {
			 e_EC_calib_state = ECPH_CALIB_IDLE;
			 *u8_point = u8_calib_step;
			 u8_calib_step = 0;
			 b_calib_pH = false;
		 }
		 else
		 {
				e_EC_calib_state = ECPH_CALIB_IDLE;
				u8_calib_step = 0;
			 *u8_point = u8_calib_step;
				b_calib_pH = false;
		 }
		 ret = true;
	 }
	 break;
	 case PH_CALIB_FINISH:
	 {
		 v_save_pH_calib_point();
		 *u8_point = u8_calib_step;
		 e_EC_calib_state = ECPH_CALIB_IDLE;
		 u8_calib_step = 0;
		 b_calib_pH = false;
		 ret = true;
	 }
	 break;
	}
	return ret;
}
