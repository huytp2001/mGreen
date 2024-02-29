#include "i2c_lcd.h"

#include "FreeRTOS.h"
#include "task.h"
#include "driverlib.h"

#define I2C_LCD_SET_CURSOR_DELAY      (10)
#define I2C_LCD_CLEAR_DISP_DELAY      (10)

#define  __SYS_DELAY_MS(ms_cnt)               ROM_SysCtlDelay((SYSTEM_CLOCK_GET()/3000) * ms_cnt);// Unit: ms

#define __I2C_LCD_POWERUP_DELAY()     do { vTaskDelay(1000); } while(0);
#define __LCD_DELAY_MS(n)             __SYS_DELAY_MS(n); 
#define __I2C_LCD_CHECK_BUSY(timeout)        __i2c_lcd_check_busy(timeout)
#define __I2C_LCD_CHECK_BUS_BUSY(timeout)        do {while(I2CMasterBusBusy(I2C_MGREEN_BASE) == true) {  };} while(0); 

static void __i2c_bus_init(void);
static void __i2c_lcd_init(void);
static void __i2c_send_data(uint8_t value);
//static void __i2c_send_byte(I2C_LCD_COMMAND_REG,uint8_t cmd);
static void __i2c_set_rgb(uint8_t r, uint8_t g, uint8_t b);
static void __i2c_set_reg(uint8_t addr, uint8_t value);
static void __i2c_send_byte(uint8_t reg_addr,  uint8_t pu8_data);
void __i2c_send_bytes(uint8_t reg_addr,  
                            uint8_t *pu8_data, uint8_t u8_length);

static uint8_t _displaycontrol = 0;
static  uint8_t _displayfunction = 0;
static uint8_t _displaymode = 0;

#define I2C_LCD_COMMAND_REG    (0x80)
#define I2C_LCD_DATA_REG 	     (0x40)

// The display is cleared by the Clear.  A pair of custom characters that are
// repeated across the display clearing the displayed message and leaving behind
// blank characters.
#define  i2c_lcd_clear() do {  __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_CLEARDISPLAY);  __LCD_DELAY_MS(I2C_LCD_CLEAR_DISP_DELAY); } while(0);

static uint8_t const row_offsets[2] = { 0x80, 0xc0 };

#define i2c_lcd_set_cursor(col, row){ __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_SETDDRAMADDR | (col + row_offsets[row]) ); __LCD_DELAY_MS(I2C_LCD_SET_CURSOR_DELAY);}while(0);

static void __i2c_lcd_check_busy(uint8_t u8_timeout_ms)
{
	static uint8_t count = 0;
	while(I2CMasterBusy(I2C_MGREEN_BASE) == true) 
	{  
		count++;
		if(count < u8_timeout_ms)
		{
			__LCD_DELAY_MS(1);
		}
		else
		{
			break;
		}
	}
}
// Write character arry data to the first line of the display.
void i2c_lcd_write(const uint8_t* buf, uint8_t length )
{
   uint8_t y= 0;
	 
   for(y=0; y< length ;y++) 
	 {
			//__i2c_send_data(buf[y]);
		 __i2c_send_byte(I2C_LCD_DATA_REG, buf[y]);
		 __LCD_DELAY_MS(1);
	 } 
}



// Low level routine that clocks in the serial __i2c_send_cmd/data to the diaplay
// Data is clocked into display on the rising edge of the Clock
// 10 bits of data must be clocked into the display.

void i2c_lcd_init(void)
{ 
  __i2c_bus_init();
  __i2c_lcd_init();
}


void i2c_lcd_gohome(void)
{
    __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_RETURNHOME);        // set cursor position to zero
    //__LCD_DELAY_MS(200);        
}

void i2c_lcd_set_display(uint8_t mode)
{  
  _displaycontrol |= (mode)?LCD_DISPLAYON:LCD_DISPLAYOFF;
    __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_DISPLAYCONTROL | _displaycontrol);
	__LCD_DELAY_MS(200);    
}

/**************************** LOW LEVEL FUNCTION ***************************/
static void __i2c_bus_init(void)
{
//  portENTER_CRITICAL();
  //Init I2C
	SysCtlPeripheralDisable(I2C_MGREEN_PER); 
  SysCtlPeripheralEnable(I2C_MGREEN_PER);     // Enable I2C1 peripheral
	SysCtlPeripheralReset(I2C_MGREEN_PER);
  while(!(SysCtlPeripheralReady(I2C_MGREEN_PER)));
  SysCtlDelay(2);           // Insert a few cycles after enabling the peripheral to allow the clock
                            // to be fully activated
	
 
	SysCtlPeripheralReset(I2C_MGREEN_PER);
  SysCtlPeripheralEnable(I2C_MGREEN_SCL_SDA_PER);     // Enable GPIOA peripheral
  while(!(SysCtlPeripheralReady(I2C_MGREEN_SCL_SDA_PER)));
  SysCtlDelay(2);           // Insert a few cycles after enabling the peripheral to allow the clock
                            // to be fully activated
  GPIOPinConfigure(I2C_MGREEN_SCL_AF);
  GPIOPinConfigure(I2C_MGREEN_SDA_AF);

  GPIOPinTypeI2CSCL(I2C_MGREEN_SCL_SDA_PORT, I2C_MGREEN_SCL_PIN); // Use pin with I2C SCL peripheral
  GPIOPinTypeI2C(I2C_MGREEN_SCL_SDA_PORT, I2C_MGREEN_SDA_PIN);    // Use pin with I2C peripheral

  I2CMasterInitExpClk(I2C_MGREEN_BASE, SYSTEM_CLOCK_GET(), true); // Enable and set frequency to 400 kHz if 
                                                          // 3rd parameter  is true(100KHz if false)
  SysCtlDelay(2);           // Insert a few cycles after enabling the I2C to allow the clock
                            // to be fully activated   													
// portEXIT_CRITICAL();
}
static void __i2c_lcd_init(void)
{
    _displayfunction |= LCD_2LINE;
    _displayfunction |= LCD_5x10DOTS;

     // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending __i2c_send_cmds. Arduino can turn on way befer 4.5V so we'll wait 50
     __LCD_DELAY_MS(1);

     // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set __i2c_send_cmd sequence
    __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_FUNCTIONSET | _displayfunction);
    __LCD_DELAY_MS(5);  // wait more than 4.1ms

    // second try
    __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_FUNCTIONSET | _displayfunction);
    __LCD_DELAY_MS(1);

    // third go
    __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_FUNCTIONSET | _displayfunction);


    // finally, set # lines, font size, etc.
    __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_FUNCTIONSET | _displayfunction);
		

    // turn the display on with no cursor or blinking default
    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
		 __i2c_send_byte(I2C_LCD_COMMAND_REG,LCD_DISPLAYCONTROL | _displaycontrol);
		 


// Initialize to default text direction (for romance languages)
    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
		 __i2c_send_byte(I2C_LCD_COMMAND_REG, LCD_ENTRYMODESET | _displaymode);
		 
    // clear it off
    i2c_lcd_clear(); //TODO:		 
}



/**************************** MCU FUNCTION ***************************/
static void __i2c_send_byte(uint8_t reg_addr,  uint8_t pu8_data)
{
	
	portENTER_CRITICAL();
  I2CMasterSlaveAddrSet(I2C_MGREEN_BASE, LCD_ADDRESS, false); // Set to write mode
	__I2C_LCD_CHECK_BUSY(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
   I2CMasterDataPut(I2C_MGREEN_BASE, reg_addr); // Place address into data register
	 I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_SEND_START ); // Send start condition
	//__I2C_LCD_CHECK_BUSY(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done
	ROM_SysCtlDelay(25*(SYSTEM_CLOCK_GET()/1000000));
	I2CMasterDataPut(I2C_MGREEN_BASE,pu8_data);
  I2CMasterControl(I2C_MGREEN_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH); // Send finish condition
  __I2C_LCD_CHECK_BUSY(I2C_DEFAULT_TIMEOUT); // Wait until transfer is done

	portEXIT_CRITICAL();
}

/* redefine clear macro as function */
void v_i2c_lcd_clear(void)
{
	i2c_lcd_clear();
}
/* redefine set cursor macro as function */
void v_i2c_lcd_set_cursor(uint8_t col, uint8_t row)
{
	i2c_lcd_set_cursor(col, row);
}
//}
	#define LCD_TEST_MAX_LEN  16
const char* disp_text = "1234567890123456";
const char* disp_text2 = "abcdefghijklmnop";

/**************************** UNIT TEST FUNCTION ***************************/
void i2c_lcd_test(void)
{
	i2c_lcd_clear();
	__LCD_DELAY_MS(200);
	{
		i2c_lcd_set_cursor(0, 0);
		i2c_lcd_write((const uint8_t*)disp_text, LCD_TEST_MAX_LEN );
		__LCD_DELAY_MS(200);
		
		i2c_lcd_set_cursor(0, 1);
		i2c_lcd_write((const uint8_t*)disp_text2, LCD_TEST_MAX_LEN );
		__LCD_DELAY_MS(200);
	}
}


