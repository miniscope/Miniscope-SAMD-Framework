/**
@file
@brief Functions for bitbang I2C interface
@author Daniel, Takuya
*/

#include "MS_config.h"
#include "MS_definitions.h"

#ifdef EWL_ENABLE
#include "i2c_bb.h"

//I2C_BB_SDA and I2C_BB_SCL pins are defined in atmel_start_pins.h
//I2C_BB_SDA GPIO(GPIO_PORTB, 0)
//I2C_BB_SCL GPIO(GPIO_PORTB, 2)  Reminder for Federico

void SDA_low()
{
	gpio_set_pin_direction(I2C_BB_SDA, GPIO_DIRECTION_OUT);	
}
void SDA_high()
{
	gpio_set_pin_direction(I2C_BB_SDA, GPIO_DIRECTION_IN);
}
uint8_t SDA_read()
{
	return gpio_get_pin_level(I2C_BB_SDA);
}
void SCL_low()
{
	gpio_set_pin_direction(I2C_BB_SCL, GPIO_DIRECTION_OUT);
}
void SCL_high()
{
	gpio_set_pin_direction(I2C_BB_SCL, GPIO_DIRECTION_IN);
}

void I2C_BB_sleep(void)
{
	delay_us(SLEEP_LENGTH);
}


void I2C_BB_end()
{
	SDA_low();
	I2C_BB_sleep();
	SCL_high();
	I2C_BB_sleep();
	SDA_high();
	I2C_BB_sleep();
}

bool I2C_BB_begin(uint8_t addr, uint8_t read)
{
	SDA_low();
	I2C_BB_sleep();
	SCL_low();
	addr=addr<<1;		//7 bit address shifted one bit to make room for the RW bit
	if (read)
		addr++;		
		
	return I2C_BB_byte_out(addr);	//should receive a bool?
}

bool I2C_BB_byte_out(uint8_t byte)
{
	uint8_t i, ack;

	// send byte	
	for (i = 0; i < 8; i++) {
		if (byte & 0x80)
			SDA_high();
		else
			SDA_low();
		SCL_high();
		I2C_BB_sleep();
		SCL_low();
		byte <<= 1;
		I2C_BB_sleep();
	}
	// read ack bit
	SDA_high();
	SCL_high();
	I2C_BB_sleep();
	ack = SDA_read();
	SCL_low();
	I2C_BB_sleep();
	SDA_low();
	return (ack == 0) ? 1:0;   //
}

void I2C_BB_init(void)
{
	// Set I2C lines initially as input to let pull-up resistors hold the lines high
	
	// SDA
	gpio_set_pin_level(I2C_BB_SDA, false);
	gpio_set_pin_direction(I2C_BB_SDA, GPIO_DIRECTION_IN);
	gpio_set_pin_function(I2C_BB_SDA, GPIO_PIN_FUNCTION_OFF);
	
	// SCL
	gpio_set_pin_level(I2C_BB_SCL, false);
	gpio_set_pin_direction(I2C_BB_SCL, GPIO_DIRECTION_IN);
	gpio_set_pin_function(I2C_BB_SCL, GPIO_PIN_FUNCTION_OFF);
}



uint8_t I2C_BB_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t length)
{
	// The HV892 is write only. 
	return 0;
}

uint8_t I2C_BB_write(uint8_t addr, uint8_t value)
{
	uint8_t rc = 0;

	rc = I2C_BB_begin(addr, 0);    //read ack
	if (rc == 1) { // ack was received
		I2C_BB_byte_out(value);
	}

	I2C_BB_end();
	return 0;
}

#endif //EWL_ENABLE

// this is where I am including the drivers for the tunable lens on the MS-US
#ifdef NANEYE_ENABLE
//This portion of the I2C drivers is written for the Polight Tunable Lens  
// Initialization on SERCOM0, PB13-15
static uint8_t I2C_0_read_str[1] ; //
static uint8_t I2C_0_write_str[1] = {0b00000111}; // HS 0x7

static uint8_t I2C_0_write_seq[5] = {0x0,0b00000001,0x2,0b10000000, 0b11001000}; // Set 10V HS 0x0 bit written to 0x0 to turn on
// static uint8_t I2C_0_write_seq[5] = {0x0,0b00000001,0x2,0b10000000, 0b11111111}; // Set 13.84V HS 0x0 bit written to 0x0 to turn on
// static uint8_t I2C_0_write_seq[5] = {0x0,0b00000001,0x2,0b10001010, 0b11111111}; // Set 42.55V HS 0x0 bit written to 0x0 to turn on


static uint8_t I2C_0_write_active_address[1] = {0x0}; //HS 0x0 bit written to 0x0 to turn on
static uint8_t I2C_0_write_active[1] = {0b00000001}; // HS 0x0 bit written to 0x0 to turn on

void I2C_0_tx_complete_HS(struct i2c_m_async_desc *const i2c)
{
}

void T_LENS_I2C(void)
{
	struct io_descriptor *I2C_0_io;

	i2c_m_async_get_io_descriptor(&I2C_0, &I2C_0_io);
	i2c_m_async_enable(&I2C_0);
	i2c_m_async_register_callback(&I2C_0, I2C_M_ASYNC_TX_COMPLETE, (FUNC_PTR)I2C_0_tx_complete_HS);
	i2c_m_async_set_slaveaddr(&I2C_0, 0xC, I2C_M_SEVEN);
	

	io_write(I2C_0_io, I2C_0_write_seq, 5); // write sequence to i2c address (fake way of getting sub address)


}

#endif