/*
 * MS_camera.c
 *
 * Created: 5/24/2023 10:56:53 AM
 *  Author: Takuya
 */ 

#include "MS_definitions.h"
#include "python480.h"

void imageSensorInit(void){
	// Setup Image Sensor
	// TODO: Work on minimizing power draw
	// Trigger pin gets init'ed as output low and shouldn't need to be adjusted
	gpio_set_pin_level(RESET_CMOS, 0); // Make sure N_RESET of the PYTHON480 is low for a bit before going high. Shouldn't be needed
	delay_ms(100);
	gpio_set_pin_level(RESET_CMOS, 1);
	delay_us(100); // minimum delay is 10us
	chip_id = spi_BB_Read(0x00); // can use this to make sure MCU can talk to Python480
	
	python480Init();
	Enable_Subsample();
	
	#ifndef PRESET_HEADER_ENABLE
	python480SetGain(getPropFromHeader(HEADER_GAIN_POS));
	python480SetFPS(getPropFromHeader(HEADER_FRAME_RATE_POS));
	python480SetFPS(FRAME_RATE);
	#endif
}