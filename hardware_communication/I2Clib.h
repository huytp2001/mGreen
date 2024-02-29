/*
 * I2Clib.h
 *
 *  Created on: Jul 29, 2013
 *      Author: Admin
 */

#ifndef I2CLIB_H_
#define I2CLIB_H_

#define SENSOR_I2Cx                          		I2C2_BASE
#define SENSOR_I2Cx_CLK_ENABLE()              	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2)
#define SENSOR_I2Cx_SDA_GPIO_CLK_ENABLE()      	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG)
#define SENSOR_I2Cx_SCL_GPIO_CLK_ENABLE()      	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG)

/* Definition for USARTx Pins */
#define SENSOR_I2Cx_SDA_PIN                    	GPIO_PIN_3
#define SENSOR_I2Cx_SDA_GPIO_PORT              	GPIO_PORTG_BASE
#define SENSOR_I2Cx_SDA_AF                     	GPIO_PG3_I2C2SDA
#define SENSOR_I2Cx_SCL_PIN                    	GPIO_PIN_2
#define SENSOR_I2Cx_SCL_GPIO_PORT              	GPIO_PORTG_BASE 
#define SENSOR_I2Cx_SCL_AF                     	GPIO_PG2_I2C2SCL


//#define SlaveAddress 0x68

void v_sensor_i2c_init(void);
void v_sensor_i2c_write(unsigned char SlaveAddress, unsigned char *ucData, unsigned int uiCount, unsigned char ucStart_add );
void v_sensor_i2c_read(unsigned char SlaveAddress, unsigned char *ucRec_Data, unsigned int uiCount, unsigned char ucStart_add);

#endif /* I2CLIB_H_ */
