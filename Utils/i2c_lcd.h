#ifndef  _I2C_LCD_H_
#define  _I2C_LCD_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "driverlib.h"
#include "i2c_mgreen.h"
#define I2C_LCD_MAX_LEN						20

#define I2C_TAKE_SEMAPHORE_TIMEOUT					5
#define I2C_DEFAULT_TIMEOUT									1000 //ms
/*
Pinout wiring
PB0: RS 
PB1: E
DATA: PD3  --  D7
      PD2  -- D6
      PD1  -- D5
      PD0  -- D4

*/


// color define
#define WHITE           0
#define RED             1
#define GREEN           2
#define BLUE            3

#define REG_RED         0x04        // pwm2
#define REG_GREEN       0x03        // pwm1
#define REG_BLUE        0x02        // pwm0

#define REG_MODE1       0x00
#define REG_MODE2       0x01
#define REG_OUTPUT      0x08


// Device I2C Arress
#define LCD_ADDRESS     (0x3E)

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00


void i2c_lcd_test(void);
void i2c_lcd_init(void);
void v_i2c_lcd_clear(void);
void v_i2c_lcd_set_cursor(uint8_t col, uint8_t row);
void i2c_lcd_write(const uint8_t* buf, uint8_t length );

#endif /* _I2C_LCD_H_ */
