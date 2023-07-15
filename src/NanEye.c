/*
 * NanEye.c
 *
 * Created: 7/7/2023 3:17:10 PM
 *  Author: HSemwal
 */ 
#include "MS_definitions.h"

#include "GS_definitions.h"
#include <utils.h>
#include <atmel_start.h>

#define INTERFACE_MODE_SIZE		972 // (648 * 12 / 8) in bytes

volatile uint8_t naneye_new_reg_val_received = 0;
volatile uint16_t naneyec_reg_val[2];
volatile uint8_t spi_interface_mode_tx_buffer[INTERFACE_MODE_SIZE]; // Used to tx during interface mode

void interface_buffer_init(void) {
	// Fill interface tx buffer with all 1's
	for (uint32_t i = 0; i < INTERFACE_MODE_SIZE; i++)
	spi_interface_mode_tx_buffer[i] = 0xFF;
}

void interface_buffer_reg_set(uint32_t reg0, uint32_t reg1) {
	// Cannot update regs in the first SPI clock of interface mode so we will start in the second byte location
	
	// For reg0
	spi_interface_mode_tx_buffer[1] = 0b10010000 | ((reg0 >> 15) & 0x01);
	spi_interface_mode_tx_buffer[2] = ((reg0 >> 7) & 0xFF);
	spi_interface_mode_tx_buffer[3] = ((reg0 << 1) & 0xFF);
	
	// For reg1
	// Lets give a 3 byte gap between writing the 2 registers
	spi_interface_mode_tx_buffer[7] = 0b10010010 | ((reg1 >> 15) & 0x01);
	spi_interface_mode_tx_buffer[8] = ((reg1 >> 7) & 0xFF);
	spi_interface_mode_tx_buffer[9] = ((reg1 << 1) & 0xFF);
}


// send data 
void NanEyeInit(void)
{
	// Let's start naneye communication. To do this we will finish setting up the interface mode stuff and then turn on the DMAs
	naneye_new_reg_val_received = 1;
		
	naneyec_reg_val[0] = NANEYE_REG0_DEFAULT_VALUE | 0b1100; // Sets offset ramp to recommended 2.2V value
	naneyec_reg_val[0] = naneyec_reg_val[0] | 0b11; // Sets output current to max (might only effect LVDS mode)
	// sets naneye register
	// |= sets particular value to 1
	// &= with a ~ means and not, sets value to 0
	naneyec_reg_val[1] = NANEYE_REG1_DEFAULT_VALUE;
	naneyec_reg_val[1] |= (1<<10); // Increase 2x bias current, reduces settling time for high speed apps (not sure what this does)
	naneyec_reg_val[1] &= ~(1<<9); // Sets CDS gain to recommended value of 1.3 (turns a 1--> 0, which sets CDS gain to 1.3)
	naneyec_reg_val[1] &= ~(1<<8); // Sets mode to SEIM
	naneyec_reg_val[1] = (naneyec_reg_val[1] & (0b1111111111001111)) | (0b10 << 4); // Sets vref to recommended value of 2.1V
	naneyec_reg_val[1] = (naneyec_reg_val[1] & (0b1111111111110011)) | (0b01 << 2); // Sets CVC current to recommended value
	naneyec_reg_val[1] &= ~(1<<1); // Turns off idle mode
	
	interface_buffer_reg_set(naneyec_reg_val[0],naneyec_reg_val[1]);
	
	for (int i = 0; i<INTERFACE_MODE_SIZE; i++)
	{
		SERCOM4->SPI.DATA.reg = spi_interface_mode_tx_buffer[i];
	}
//	SERCOM4->SPI.DATA.reg = naneyec_reg_val[1];
}
void startRecordingNE()
{
	writeFrameNum=0;
	writeBufferCount=0;
	droppedBufferCount= 0;
	droppedFrameCount = 0;
	framesToDrop = 0;
	
	deviceState &= ~(DEVICE_STATE_IDLE);
	deviceState &= ~(DEVICE_STATE_START_RECORDING);
	deviceState |= DEVICE_STATE_START_RECORDING_WAITING;
	
		
}

void stopRecordingNE()
{
	deviceState &= ~(DEVICE_STATE_STOP_RECORDING);
	deviceState &= ~(DEVICE_STATE_RECORDING);
	deviceState |= DEVICE_STATE_IDLE;
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_RECORDED_POS, writeBufferCount);
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_DROPPED_POS, droppedBufferCount);
}


