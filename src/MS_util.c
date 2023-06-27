/*
* MS_util.c
*
* Created: 5/20/2023 7:16:23 PM
*  Author: Takuya
*/

#include <atmel_start.h>
#include "MS_definitions.h"

#include "python480.h"
#include "i2c_bb.h"


#ifdef DMA_TO_SD_ENABLE
#include "sd_mmc.h"
#endif

#ifdef DMA_TO_SD_ENABLE
uint32_t lastTime = 0;
bool lastMonitor0 = 0;
bool thisMonitor0 = 0;
#endif

#ifdef DMA_TO_SD_ENABLE
uint8_t loadSDCardHeader(void){
	sd_mmc_init_read_blocks(0,HEADER_BLOCK,1);
	sd_mmc_start_read_blocks(headerBlock,1);
	if (sd_mmc_wait_end_of_read_blocks(false) == SD_MMC_OK)
	return MS_SUCCESS;
	else
	return MS_ERROR;
}
#endif // DMA_TO_SD_ENABLE

uint32_t getPropFromHeader(uint8_t headerPos) {
	uint32_t *header32bit = (uint32_t *)headerBlock;
	
	return header32bit[headerPos];
}

void debugHeaderProp(void){
	ewlvalue = getPropFromHeader(HEADER_EWL_POS);
	batteryvalue = getPropFromHeader(HEADER_BATT_CUTOFF_POS);
	ledvalue = getPropFromHeader(HEADER_LED_POS);
	frameratevalue = getPropFromHeader(HEADER_FRAME_RATE_POS);
	delayvalue = getPropFromHeader(HEADER_DELAY_START_POS);
	reclengthvalue = getPropFromHeader(HEADER_RECORD_LENGTH_POS);
}

void getBuffersPerFrame(void)
{
	#ifdef PYTHON480_ENABLE
	numBuffersPerFrame = (WIDTH * HEIGHT) / (BUFFER_BLOCK_LENGTH * SD_BLOCK_SIZE - (BUFFER_HEADER_LENGTH * 4));
	if((WIDTH * HEIGHT) % (BUFFER_BLOCK_LENGTH * SD_BLOCK_SIZE - (BUFFER_HEADER_LENGTH * 4)) != 0)
	numBuffersPerFrame += 1;
	// Need to add 1 to account for partially filled buffer
	#endif
}

void peripheralInit(void)
{
	
	#ifdef EXLED_PWM_ENABLE
	// We need to change the PWM mode from MPWM to NPWM because we are using WO[0] as waveform output
	hri_tc_write_WAVE_reg(TC0, TC_WAVE_WAVEGEN_NPWM_Val);
	#endif
	
	#ifdef PYTHON480_ENABLE
	// Enable the 3.3V regulator
	gpio_set_pin_level(EN_3V3, true);
	#endif

	#ifdef BATTERY_ENABLE
	// Enable ADC for checking battery voltage
	adc_sync_enable_channel(&ADC_0, 0);
	#endif
	
	#ifdef WPT_ADC_ENABLE
	// Enable ADC for checking battery voltage
	adc_sync_enable_channel(&ADC_0, 1);
	#endif
	
	#ifdef EWL_ENABLE
	I2C_BB_init();
	#endif

	timerInit();
	
	irqInit();
	
	#ifdef PYTHON480_ENABLE
	PCCLinkedListInit();
	#endif
	
	#if defined(DMA_TO_SPI_ENABLE) || defined(DMA_TO_USART_ENABLE)
	TXLinkedListInit();
	sdo_dma_irq_setup();
	#endif
	
	#ifdef DMA_TO_SD_ENABLE
	SDCardInit();
	#endif

	imageSensorInit();

	#ifdef EWL_ENABLE
	setEWL(getPropFromHeader(HEADER_EWL_POS));
	//setExcitationLED(getPropFromHeader(HEADER_LED_POS), 1);
	#endif
	
	#if defined(DMA_TO_USART_ENABLE) && defined(USART_SERCOM5_ENABLE)
	SERCOM5->USART.CTRLA.bit.ENABLE = 0; // Disable UART
	while (SERCOM5->USART.SYNCBUSY.bit.ENABLE)
	;                                 // Wait for disable
	//hri_sercomusart_write_BAUD_reg(SERCOM5, USART_BAUD_MS);
	//SERCOM5->USART.CTRLC.bit.DATA32B = 1; // Enable 32-bit mode. Still packet structure is 8-bit
	SERCOM5->USART.CTRLA.bit.ENABLE = 1;  // Re-enable USART
	while (SERCOM5->USART.SYNCBUSY.bit.ENABLE)
	;                                 // Wait for disable
	#endif

	#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
	SERCOM0->SPI.CTRLA.bit.ENABLE = 0; // Disable SPI
	while (SERCOM0->SPI.SYNCBUSY.bit.ENABLE)
	;                                 // Wait for disable
	hri_sercomspi_set_CTRLC_ICSPACE_bf(SERCOM0, SPI_ICSPACE_MS);
	hri_sercomspi_write_BAUD_reg(SERCOM0, SPI_BAUD_MS);
	SERCOM0->SPI.CTRLC.bit.DATA32B = 1; // Enable 32-bit mode
	SERCOM0->SPI.CTRLA.bit.ENABLE = 1;  // Re-enable SPI
	while (SERCOM0->SPI.SYNCBUSY.bit.ENABLE)
	;                                 // Wait for disable
	#endif
	
	#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM7_ENABLE)
	hri_sercomspi_set_CTRLC_ICSPACE_bf(SERCOM7, SPI_ICSPACE_MS);
	hri_sercomspi_write_BAUD_reg(SERCOM7, SPI_BAUD_MS);
	spi_m_sync_enable(&SPI_0);
	#endif
	
	#if defined(NODMA_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
	SERCOM0->SPI.CTRLA.bit.ENABLE = 0x01;
	SERCOM0->SPI.DATA.reg = (uint32_t) dataBuffer[0][1];
	#endif
}

void configPropInit(void){
	// Set some parameters in config buffer to be written to SD card at end of recording
	// TODO: Add additional info in the config block to cover everything needed for offline processing

}

#ifdef DMA_TO_SD_ENABLE
void SDCardInit(void){
	// Wait for SD Card and then load config from it
	while (SD_MMC_OK != sd_mmc_check(0)) {}
	if (loadSDCardHeader() == MS_SUCCESS)
	deviceState |= DEVICE_STATE_CONFIG_LOADED;
	else
	deviceState |= DEVICE_STATE_ERROR;
	
	// Give capabilities info of sd card
	tempPCC[0] = SDHC0->CA0R.reg;
	tempPCC[1] = SDHC0->CA1R.reg;
	tempPCC[2] = SDHC0->HC1R.reg;
	
	// Select ADMA as the DMA to use. This should be moved to where other bits of HC1R get set.
	SDHC0->HC1R.reg |= 1<<4;
	
	// Set some parameters in config buffer to be written to SD card at end of recording
	// TODO: Add additional info in the config block to cover everything needed for offline processing

	setConfigBlockProp(CONFIG_BLOCK_WIDTH_POS, WIDTH / BINNING);
	setConfigBlockProp(CONFIG_BLOCK_HEIGHT_POS, HEIGHT / BINNING);
	setConfigBlockProp(CONFIG_BLOCK_FRAME_RATE_POS, getPropFromHeader(HEADER_FRAME_RATE_POS));
	setConfigBlockProp(CONFIG_BLOCK_BUFFER_SIZE_POS, BUFFER_BLOCK_LENGTH * SD_BLOCK_SIZE);
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_RECORDED_POS, 0);
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_DROPPED_POS,0);
	
	sd_mmc_init_write_blocks(0, CONFIG_BLOCK, 1);
	sd_mmc_start_write_blocks(configBlock, 1); // We will re-write this block at the end of recording too
	sd_mmc_wait_end_of_write_blocks(false);
}


void setSDDescriptor(uint32_t *address, uint16_t length, uint8_t attribute)
// address holds the pointer location to the front of a data buffer
// Length is in bytes
// attribute holds the lower 6 bits of the descriptor table
{
	uint64_t temp = address;
	temp = temp<<32;
	SDTransferDescriptor = (temp)|attribute|SD_DESCRIPTOR_LENGTH(length);
}

#endif // DMA_TO_SD_ENABLE

void irqInit(void){
	// Setup callbacks for interrupts
	#ifdef IR_TRIGGER_ENABLE
	ext_irq_register(PIN_PB22, irReceive_cb);
	#endif
	
	#ifdef BATTERY_ENABLE
	ext_irq_register(PIN_PB23, battCharging_cb);
	#endif
	
	#ifdef PYTHON480_ENABLE
	ext_irq_register(PIN_PB14, frameValid_cb);
	#endif
	
	#ifdef PUSH_BUT_ENABLE
	ext_irq_register(PIN_PA25, pushButton_cb);
	#endif
	
	
}

void setConfigBlockProp(uint8_t position, uint32_t value) {
	uint32_t *configBlock32bit = (uint32_t *)configBlock;
	
	configBlock32bit[position] = value;
}

#if defined(PYTHON480_ENABLE)
void setBufferHeader(uint32_t dataWordLength) {
	uint32_t numBuffer = bufferCount % NUM_BUFFERS;
	#ifdef DEV_MODE
	dataBuffer[numBuffer][BUFFER_HEADER_HEADER_LENGTH_POS] = 0x12345678;
	#else
	dataBuffer[numBuffer][BUFFER_HEADER_HEADER_LENGTH_POS] = BUFFER_HEADER_LENGTH;
	#endif
	dataBuffer[numBuffer][BUFFER_HEADER_LINKED_LIST_POS] = bufferCount % NUM_BUFFERS;
	dataBuffer[numBuffer][BUFFER_HEADER_FRAME_NUM_POS] = frameNum;
	dataBuffer[numBuffer][BUFFER_HEADER_BUFFER_COUNT_POS] = bufferCount;
	dataBuffer[numBuffer][BUFFER_HEADER_FRAME_BUFFER_COUNT_POS] = frameBufferCount;
	dataBuffer[numBuffer][BUFFER_HEADER_WRITE_BUFFER_COUNT_POS] = writeBufferCount;
	dataBuffer[numBuffer][BUFFER_HEADER_DROPPED_BUFFER_COUNT_POS] = droppedBufferCount;
	dataBuffer[numBuffer][BUFFER_HEADER_TIMESTAMP_POS] = getCurrentTimeMS() - startTimeMS;
	
	// TODO: Put the correct value for data length. This will change if it is a partially filled buffer
	// UBLEN in XDMAC_CUBC gets decremented by MBSIZE or CSIZE for each memory or chunk transfer. We can calculate from this
	dataBuffer[numBuffer][BUFFER_HEADER_DATA_LENGTH_POS] = dataWordLength * 4; // In bytes
}
#endif

#ifdef EXLED_PWM_ENABLE
void setExcitationLED(uint32_t value, bool enable)
{
	// Value is a percentage of brightness from 0 to 100.
	// PWM runs at 1ms period using 16bit MAX counter and a ~60MHz clock
	if (value > 100)
	value = 100;
	
	value = (0xFFFF * value ) /100;
	
	pwm_set_parameters(&PWM_0, value, 0); // value sets duty cycle out of 2^16. We aren't using CC1 so just send it 0
	pwm_enable(&PWM_0); //Only actually needs to be done once. Consider moving to init stuff at top of main()
	
	gpio_set_pin_level(ENT_LED, enable);
}
#endif

#ifdef EWL_ENABLE
void setEWL(uint32_t value)
{
	I2C_BB_write(EWL_I2C_ADDR,value);
}
#endif

#ifdef STATUS_LED_ENABLE
void setStatusLED(bool value)
{
	gpio_set_pin_level(LED_STATUS, value);
}
#endif