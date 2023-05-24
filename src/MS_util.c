/*
* MS_util.c
*
* Created: 5/20/2023 7:16:23 PM
*  Author: Takuya
*/

#include <atmel_start.h>
#include "MS_definitions.h"

#ifdef DMA_TO_SD_ENABLE
#include "sd_mmc_ms.h"
#endif

#ifdef EWL_ENABLE
#include "i2c_bb.h"
#endif

// ----------- GLOBAL VARIABLES -----------
volatile uint8_t headerBlock[SD_BLOCK_SIZE] = {0}; // Will hold the 512 bytes from the header block of sd card
volatile uint8_t configBlock[SD_BLOCK_SIZE]; // Will hold the device config information to be written to the starting block
volatile uint32_t currentBlock = STARTING_BLOCK;
volatile uint32_t initBlocksRemaining;

volatile uint32_t deviceState = DEVICE_STATE_IDLE;
volatile uint8_t battVolt;

volatile uint32_t startTimeMS;
volatile uint32_t timeMS = 0;
volatile uint32_t frameBufferCount = 0;
volatile uint32_t frameNum = 0;
volatile uint32_t bufferCount = 0;
volatile uint32_t frameBufferCount;

// used for tracking recording and inc. DMA buffers
volatile uint32_t writeFrameNum;
volatile uint32_t writeBufferCount;
volatile uint32_t droppedBufferCount;
volatile uint32_t droppedFrameCount;
volatile uint32_t framesToDrop;
volatile uint32_t *bufferToWrite;
volatile uint32_t numBlocks = BUFFER_BLOCK_LENGTH;
volatile uint32_t numBuffersPerFrame = 0;

// Debugging and checking stuff
volatile uint16_t chip_id; // Reads the chip id from Python480 to make sure we can talk to it
volatile uint32_t ewlvalue;
volatile uint32_t batteryvalue;
volatile uint32_t ledvalue;
volatile uint32_t frameratevalue;
volatile uint32_t delayvalue;
volatile uint32_t reclengthvalue;

volatile uint16_t regValue[2];
volatile uint32_t tempPCC[4];
volatile uint32_t tempHeader[100][4];
volatile uint32_t tempCount = 0;
volatile uint32_t tempTimestamp[100];
volatile uint8_t timerIndex = 0;

struct timer_task TIMER_0_task1;
struct timer_task TIMER_0_task2;

#ifdef DMA_TO_SD_ENABLE
volatile uint32_t initBlocksRemaining = 0;
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
	if((WIDTH * HEIGHT) % (BUFFER_BLOCK_LENGTH * SD_BLOCK_SIZE - (BUFFER_HEADER_LENGTH * 4)) != 0) numBuffersPerFrame += 1;
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
	I2C_BB_init();
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
	setEWL(getPropFromHeader(HEADER_EWL_POS));
	//setExcitationLED(getPropFromHeader(HEADER_LED_POS), 1);
	#endif
	
	#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
	hri_sercomspi_set_CTRLC_ICSPACE_bf(SERCOM0, SPI_ICSPACE_MS);
	hri_sercomspi_write_BAUD_reg(SERCOM0, SPI_BAUD_MS);
	spi_m_sync_enable(&SPI_0);
	#endif
	
	#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM7_ENABLE)
	hri_sercomspi_set_CTRLC_ICSPACE_bf(SERCOM7, SPI_ICSPACE_MS);
	hri_sercomspi_write_BAUD_reg(SERCOM7, SPI_BAUD_MS);
	spi_m_sync_enable(&SPI_0);
	#endif
	
	#ifdef DMA_TO_USART
	usart_async_enable(&USART_0);
	#endif
	
	#ifdef DMA_TO_SD_ENABLE
	SDCardInit();
	#endif // DMA_TO_SD_ENABLE

	//Test for no DMA
	/*
	SERCOM0->SPI.CTRLA.bit.ENABLE = 0x01;
	SERCOM0->SPI.DATA.reg = (uint32_t) dataBuffer[0][1];
	*/

}

void configPropInit(void){
	// Set some parameters in config buffer to be written to SD card at end of recording
	// TODO: Add additional info in the config block to cover everything needed for offline processing
	setConfigBlockProp(CONFIG_BLOCK_WIDTH_POS, WIDTH / BINNING);
	setConfigBlockProp(CONFIG_BLOCK_HEIGHT_POS, HEIGHT / BINNING);
	setConfigBlockProp(CONFIG_BLOCK_FRAME_RATE_POS, getPropFromHeader(HEADER_FRAME_RATE_POS));
	setConfigBlockProp(CONFIG_BLOCK_BUFFER_SIZE_POS, BUFFER_BLOCK_LENGTH * SD_BLOCK_SIZE);
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_RECORDED_POS, 0);
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_DROPPED_POS,0);
}

void dmaEnable(void){
		#ifdef PYTHON480_ENABLE
		// Enables DMA Transfer complete interrupt. Should be put in better place
		DMAC->Channel[CONF_PCC_DMA_CHANNEL].CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
		
		// Sets the callback for when each DMA buffer is full
		camera_async_register_callback(&CAMERA_0, pcc_dma_cb);

		// This should already be done in init but trying here as well
		PCC->MR.reg = PCC_MR_CID(0x3) | PCC_MR_ISIZE(CONF_PCC_ISIZE) | CONF_PCC_FRSTS << PCC_MR_FRSTS_Pos
		| CONF_PCC_HALFS << PCC_MR_HALFS_Pos | CONF_PCC_ALWYS << PCC_MR_ALWYS_Pos
		| CONF_PCC_SCALE << PCC_MR_SCALE_Pos | PCC_MR_DSIZE(CONF_PCC_DSIZE);
		#endif

		#ifdef DMA_TO_SPI_ENABLE 
		_dma_enable_transaction(SPI_DMA_CHANNEL, false);
		#endif
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
	
	sd_mmc_init_write_blocks(0, CONFIG_BLOCK, 1);
	sd_mmc_start_write_blocks(configBlock, 1); // We will re-write this block at the end of recording too
	sd_mmc_wait_end_of_write_blocks(false);

}
#endif // DMA_TO_SD_ENABLE

#ifdef PYTHON480_ENABLE
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
	python480SetGain(getPropFromHeader(HEADER_GAIN_POS));
	python480SetFPS(getPropFromHeader(HEADER_FRAME_RATE_POS));
	python480SetFPS(FRAME_RATE);
}
#endif


void timerInit(void)
{
	#if defined(PYTHON480_ENABLE)
	// Setup a timer to count in milliseconds
	TIMER_0_task1.interval	= 1; // Need to check this value
	TIMER_0_task1.cb		= millisecondTimer_cb;
	TIMER_0_task1.mode		= TIMER_TASK_REPEAT;
	timer_add_task(&TIMER_0, &TIMER_0_task1);
	#endif
	
	#if defined(BATTERY_ENABLE) || defined(WPT_ADC_ENABLE)
	TIMER_0_task2.interval = 1000; // Units are in ms so 1000 should check every 1 second
	TIMER_0_task2.cb       = checkBattVoltage_cb;
	TIMER_0_task2.mode     = TIMER_TASK_REPEAT;
	timer_add_task(&TIMER_0, &TIMER_0_task2);
	#endif
	
	#if defined(PYTHON480_ENABLE) || defined(BATTERY_ENABLE) || defined(WPT_ADC_ENABLE)
	timer_start(&TIMER_0);
	#endif
}

void irqInit(void){
	// Setup callbacks for external interrupts
	#ifdef IR_RX_ENABLE
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

uint32_t getCurrentTimeMS(void)
{
	return timeMS;
}

#if defined(PYTHON480_ENABLE)
void setBufferHeader(uint32_t dataWordLength) {
	uint32_t numBuffer = bufferCount % NUM_BUFFERS;
	dataBuffer[numBuffer][BUFFER_HEADER_HEADER_LENGTH_POS] = BUFFER_HEADER_LENGTH;
	dataBuffer[numBuffer][BUFFER_HEADER_LINKED_LIST_POS] = bufferCount % NUM_BUFFERS;
	dataBuffer[numBuffer][BUFFER_HEADER_FRAME_NUM_POS] = frameNum;
	dataBuffer[numBuffer][BUFFER_HEADER_BUFFER_COUNT_POS] = bufferCount;
	dataBuffer[numBuffer][BUFFER_HEADER_FRAME_BUFFER_COUNT_POS] = frameBufferCount;
	//dataBuffer[numBuffer][BUFFER_HEADER_WRITE_BUFFER_COUNT_POS] = writeBufferCount;
	//dataBuffer[numBuffer][BUFFER_HEADER_DROPPED_BUFFER_COUNT_POS] = droppedBufferCount;
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