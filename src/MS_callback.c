/*
 * MS_cb.c
 *
 * Created: 5/20/2023 7:41:53 PM
 *  Author: Takuya
 */ 

#include "MS_definitions.h"
#include "dma_custom_driver.h"

//#include <hpl_pcc_config.h>
#include <hpl_dma.h>

#ifdef PYTHON480_ENABLE
void millisecondTimer_cb(const struct timer_task *const timer_task)
{
	timeMS++;
}
#endif

#ifdef BATTERY_ENABLE
void checkBattVoltage_cb(const struct timer_task *const timer_task)
{
	uint8_t adcValue;
	// Uses ADC0 to check battery voltage
	adc_sync_read_channel(&ADC_0, 0, &adcValue, 1);
	battVolt = adcValue;
	
	// If under voltage, set device state to ...
	// Compare to 1.1V band gap
	// Battery voltage goes through a 1/5x voltage divider
	
	// Raise issue if voltage is under 3.4V
	// 3.4V = 158
	// 3.3V = 148
	if (adcValue < getPropFromHeader(HEADER_RECORD_LENGTH_POS)) {
		// Low voltage problem
		deviceState |= DEVICE_STATE_LOW_VOLTAGE;
		deviceState |= DEVICE_STATE_STOP_RECORDING;
	}
}
#endif

#if defined(IR_TRIGGER_ENABLE) || defined(IR_UART_ENABLE)
void irReceive_cb(void)
{
	bool pinState = gpio_get_pin_level(IR_REC);
	if (pinState == true) {
		delay_ms(13); //13 ms after the pin gets high we check if there is a 0
		if (gpio_get_pin_level(IR_REC) == false) {
			for( uint8_t i = 0; i < delayvalue; i++ ){
				gpio_toggle_pin_level(LED_STATUS);
				delay_ms(1000);
			}
			deviceState = DEVICE_STATE_START_RECORDING;
		}
		
		else{}
		
	}
	else {
		
	}
}
#endif

#ifdef PUSH_BUT_ENABLE
void pushButton_cb(void)
{
	bool pinState = gpio_get_pin_level(PUSH_BUT_MCU);
	if (pinState == true) {
		
	}
	else {
		
	}
}
#endif



#ifdef PYTHON480_ENABLE
void frameValid_cb(void)
{
	bool pinState = gpio_get_pin_level(FrameValid);
	
	//if (gpio_get_pin_level(LED_STATUS) == 1) {
	//setStatusLED(0);
	//
	//}
	//else {
	//setStatusLED(1);
	//}
	
	if (pinState == true) {	// beginning of new frame acquisition
	}
	else { // Handles end of frame
		
		if (deviceState & (DEVICE_STATE_RECORDING | DEVICE_STATE_STOP_RECORDING)) {
			// At the end of frame the current buffer is likely only partially filled.
			// Disable DMA to flush DMA FIFO then start DMA again but with the next linked list
			
			PCC->MR.reg &= ~(PCC_MR_PCEN); // Disables PCC
			DMAC->Channel[CONF_PCC_DMA_CHANNEL].CHCTRLA.reg &= ~(DMAC_CHCTRLA_ENABLE); // Disables PCC DMA
			
			// Some debugging stuff here
			//tempHeader[tempCount][0] = frameNum;
			//tempHeader[tempCount][1] = bufferCount;
			//tempHeader[tempCount][2] = 4 * dma_get_WRB_data(CONF_PCC_DMA_CHANNEL);
			//tempHeader[tempCount][3] = PCC->ISR.reg;;
			//if (tempCount < 99)
			//tempCount++;
			
			setBufferHeader((BUFFER_BLOCK_LENGTH * PCC_BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH) - _dma_get_WRB_data(CONF_PCC_DMA_CHANNEL)); // This should get total beats transferred through DMA
			
			frameBufferCount = 0;
			bufferCount++; // A buffer has been filled (likely partially) and is ready for writing to SD card
			frameNum++; // Zero-Indexed
			
			if (deviceState & DEVICE_STATE_RECORDING) { // Keep recording
				// Update Linked List
				setPCCLinkedListPosition(bufferCount % NUM_BUFFERS); // Moves to next buffer/linked list element
				#if 0 // this part is probably not needed because the SDO linked list is independent of end of frame
				if (bufferCount % NUM_BUFFERS == 0)
				{
					setTXLinkedListPosition(NUM_BUFFERS - 1); // Moves to next buffer/linked list element
				}
				else{
					setTXLinkedListPosition(bufferCount % NUM_BUFFERS - 1); // Moves to next buffer/linked list element
				}
				#endif
				_dma_enable_transaction(CONF_PCC_DMA_CHANNEL, false); // Should enable DMA transfer
				
				PCC->MR.reg |= PCC_MR_PCEN; // Enables PCC
			}
			if (deviceState & DEVICE_STATE_STOP_RECORDING) {
				// Reset linked lists so we will be ready to start recording again in the future
				
				//deviceState &= ~(DEVICE_STATE_RECORDING);
				//deviceState &= ~(DEVICE_STATE_STOP_RECORDING);
				//deviceState |= DEVICE_STATE_IDLE;
			}
		}
		else if (deviceState & DEVICE_STATE_START_RECORDING_WAITING) {
			// We wait till !FV to enable recording so the first buffer starts at the beginning and not middle of a frame
			
			frameNum = 0;
			bufferCount = 0;
			frameBufferCount = 0;
			
			TXLinkedListInit();
			PCCLinkedListInit();
			setPCCLinkedListPosition(0); // Moves to next buffer/linked list element
			_dma_enable_transaction(CONF_PCC_DMA_CHANNEL, false); // Should enable DMA transfer
			
			PCC->MR.reg |= PCC_MR_PCEN; // Enables PCC
			
			deviceState &= ~(DEVICE_STATE_START_RECORDING_WAITING);
			deviceState |= DEVICE_STATE_RECORDING;
		}
	}
}
#endif

#ifdef PYTHON480_ENABLE
void pcc_dma_cb(struct camera_async_descriptor *const descr, uint32_t ch)
{
	if (ch == CONF_PCC_DMA_CHANNEL) {
		pcc_dma_cb_calls++;
			
		// add header to current buffer
		
		// Some debugging stuff here
		//tempHeader[tempCount][0] = frameNum;
		//tempHeader[tempCount][1] = bufferCount;
		//tempHeader[tempCount][2] = 4 * dma_get_WRB_data(CONF_PCC_DMA_CHANNEL);
		//tempHeader[tempCount][3] = PCC->ISR.reg;
		//if (tempCount < 99)
		//tempCount++;
		
		setBufferHeader(BUFFER_BLOCK_LENGTH * PCC_BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH);
		bufferCount++;// increment counters
		frameBufferCount++;
		#if 1
		//#if defined(DMA_TO_SPI_ENABLE) || defined(DMA_TO_USART_ENABLE)
		sdo_dma_transfer_control(false);		
		#endif
	}
}
#endif

#if defined(PYTHON480_ENABLE)
void recording_cb(const struct timer_task *const timer_task)
{
	// not sure if the bufferCount > 1 is needed.
	if (bufferCount > (writeBufferCount + droppedBufferCount) && bufferCount > 1) { // when camera data is ahead
		// This means there are filled buffer(s) ready to be written to SD card


		// We need to check if the writing to sd card of data buffers has fallen too far behind where we are at risk
		// of writing overwritten data. We need to detect this and decide what to do in this case
		if (bufferCount > (writeBufferCount + droppedBufferCount + NUM_BUFFERS)) { // when data transfer isn't catching up
			// We  are at risk of at least the current buffer that we want to write to SD card being overflown with new image data
			// We are going to just drop writing the rest of this frame
			
			// Let's figure out how many buffers need to be dropped
			// TODO: I think NUM_BUFFERS here should actually be number_of_buffers_per_frame
			//droppedBufferCount += (numBuffersPerFrame - (writeBufferCount + droppedBufferCount) % numBuffersPerFrame);
			//droppedBufferCount += bufferCount - writeBufferCount + droppedBufferCount + NUM_BUFFERS;
		}
		else { // Actual writing of good buffers
			
			bufferToWrite = (uint32_t)(&dataBuffer[(writeBufferCount + droppedBufferCount) % NUM_BUFFERS]);
			numBlocks = (bufferToWrite[BUFFER_HEADER_DATA_LENGTH_POS] + (BUFFER_HEADER_LENGTH * 4) + (SD_BLOCK_SIZE - 1)) / SD_BLOCK_SIZE;
			
			// This if statement shouldn't be needed
			//if (numBlocks > BUFFER_BLOCK_LENGTH)
			//numBlocks = BUFFER_BLOCK_LENGTH;
			
			bufferToWrite[BUFFER_HEADER_WRITE_BUFFER_COUNT_POS] = writeBufferCount;
			bufferToWrite[BUFFER_HEADER_DROPPED_BUFFER_COUNT_POS] = droppedBufferCount;
			bufferToWrite[BUFFER_HEADER_WRITE_TIMESTAMP_POS] = getCurrentTimeMS() - startTimeMS;
			
			tempTimestamp[(writeBufferCount + droppedBufferCount) % 100] = getCurrentTimeMS() - startTimeMS;
			
			#ifdef DMA_TO_SD_ENABLE
			#ifdef ADMA_ENABLE
			// Sets up ADMA descriptor for writing 1 full buffer
			setSDDescriptor(bufferToWrite, numBlocks * SD_BLOCK_SIZE,
			SD_DESCRIPTOR_ATT_TRANSFER|SD_DESCRIPTOR_ATT_VALID|SD_DESCRIPTOR_ATT_END);
			sd_mmc_write_with_ADMA(0, currentBlock, (uint32_t)&SDTransferDescriptor, numBlocks);
			sd_mmc_wait_end_of_ADMA_write(false);

			currentBlock += numBlocks;
			
			
			
			#else // not ADMA_ENABLE
			if (numBlocks < initBlocksRemaining) {
				
				#ifdef DMA_TO_SD_ENABLE
				// There are enough init blocks for this write
				if (sd_mmc_start_write_blocks(bufferToWrite, numBlocks) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				#endif
				
				initBlocksRemaining -= numBlocks;
				currentBlock += numBlocks;
			}
			else if (numBlocks == initBlocksRemaining)
			{
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_start_write_blocks(bufferToWrite, numBlocks) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				#endif
				
				currentBlock += numBlocks;
				
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_init_write_blocks(0, currentBlock, BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				#endif
				
				initBlocksRemaining = (BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK);
			}
			else {
				// TODO: error checking with LED showing status
				
				// This finishes up the remaining blocks in the current set of initialized blocks
				if (sd_mmc_start_write_blocks(bufferToWrite, initBlocksRemaining) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);

				currentBlock += initBlocksRemaining;
				
				// We now initialize the next set of blocks
				// TODO: Probably handle errors better here and don't go forward with writing if init fails
				if (sd_mmc_init_write_blocks(0, currentBlock, BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_INIT_WRITE_ERROR;
				
				// And write remaining data from buffer
				if (sd_mmc_start_write_blocks((uint32_t)(&bufferToWrite[initBlocksRemaining * SD_BLOCK_SIZE / 4]), numBlocks - initBlocksRemaining) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				
				currentBlock += numBlocks - initBlocksRemaining;
				initBlocksRemaining = (BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK) - (numBlocks - initBlocksRemaining);
			}
			#endif // not ADMA_ENABLE
			#endif // DMA_TO_SD_ENABLE
			//writeBufferCount++; // Probably shouldn't be here?
		}
		//Code for demonstration
		//I jump through three planes using the EWL and different LED values
		//if (((getCurrentTimeMS() - startTimeMS) < 6*1000*10
		//)){
		//if((getCurrentTimeMS() - startTimeMS)<5*1000*2.5)
		//{
		//setExcitationLED(1,1);
		//setEWL(0x20);
		//}
		////num >= lower && num <= upper
		////else if(2*1000*2.5<=(getCurrentTimeMS() - startTimeMS)<2*1000*5)
		//else if((getCurrentTimeMS() - startTimeMS)>=5*1000*2.5 && (getCurrentTimeMS() - startTimeMS)<=5*1000*5)
		//{
		////setExcitationLED(5,1);
		//setExcitationLED(2,1);
		//setEWL(0x50);
		//}
		//else if((getCurrentTimeMS() - startTimeMS)>5*1000*5)
		//{
		//setExcitationLED(4,1);
		//setEWL(0xFE);
		//}
		//}
		if (((getCurrentTimeMS() - startTimeMS) >= getPropFromHeader(HEADER_RECORD_LENGTH_POS) * 1000) & (getPropFromHeader(HEADER_RECORD_LENGTH_POS) != 0)){
			//if (((getCurrentTimeMS() - startTimeMS) >= 10 * 1000*60) & (getPropFromHeader(HEADER_RECORD_LENGTH_POS) != 0)){
			deviceState |= DEVICE_STATE_STOP_RECORDING; // Sets the flag to want to end current recording
		}
		
		// Code used during testing to record for a fixed, hard-coded lengths
		//if (((getCurrentTimeMS() - startTimeMS) >= 1000*30000))
		//{

		//// Recording time has elapsed
		//deviceState |= DEVICE_STATE_STOP_RECORDING; // Sets the flag to want to end current recording
		//}
		
	}
	
}
#endif

#ifdef BATTERY_ENABLE
void battCharging_cb(void)
{
	bool pinState = gpio_get_pin_level(nCHRG);
	if (pinState == true) {
		// Not charging
		while(deviceState &= ~(DEVICE_STATE_CHARGING)){
			gpio_toggle_pin_level(LED_STATUS);
			delay_ms(5000);
		}
	}

	else {
		// charging
		while(deviceState |= DEVICE_STATE_CHARGING){
			gpio_toggle_pin_level(LED_STATUS);
			delay_ms(250);
		}
	}
}
#endif
