/*
 * MS_record.c
 *
 * Created: 5/20/2023 7:26:08 PM
 *  Author: Takuya
 */

#include "MS_definitions.h"

#ifdef DMA_TO_SD_ENABLE
#include "sd_mmc_ms.h"
#endif

#ifdef PYTHON480_ENABLE
void startRecording()
{
	writeFrameNum = 0;
	writeBufferCount = 0;
	droppedBufferCount = 0;
	droppedFrameCount = 0;
	framesToDrop = 0;
	
	// This gets the next set of blocks ready to be written into
	#ifdef DMA_TO_SD_ENABLE
	#ifndef ADMA_ENABLE
	sd_mmc_init_write_blocks(0, currentBlock, BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK);
	#endif // DMA_TO_SD_ENABLE
	initBlocksRemaining = BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK;
	#endif // not ADMA_ENABLE

	startTimeMS = getCurrentTimeMS();
	
	setEWL(getPropFromHeader(HEADER_EWL_POS));
	setExcitationLED(getPropFromHeader(HEADER_LED_POS), 1);
	python480SetGain(getPropFromHeader(HEADER_GAIN_POS));
	python480SetFPS(getPropFromHeader(HEADER_FRAME_RATE_POS));
	python480SetFPS(FRAME_RATE);
	setStatusLED(1);
	
	deviceState &= ~(DEVICE_STATE_IDLE);
	deviceState &= ~(DEVICE_STATE_START_RECORDING);
	deviceState |= DEVICE_STATE_START_RECORDING_WAITING;
	
	
}

void stopRecording()
{
	
	deviceState &= ~(DEVICE_STATE_STOP_RECORDING);
	deviceState &= ~(DEVICE_STATE_RECORDING);
	deviceState |= DEVICE_STATE_IDLE;
	
	// Must be a better way of doing this. This finishes up the remaining init blocks so we can then write to the config block
	
	#ifndef ADMA_ENABLE
	while (initBlocksRemaining > BUFFER_BLOCK_LENGTH) {
		#ifdef DMA_TO_SD_ENABLE
		if (sd_mmc_start_write_blocks(dataBuffer[0], BUFFER_BLOCK_LENGTH) != SD_MMC_OK)
		deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
		initBlocksRemaining -= BUFFER_BLOCK_LENGTH;
		sd_mmc_wait_end_of_write_blocks(false);
		#endif // DMA_TO_SD_ENABLE
	}
	if (initBlocksRemaining > 0) {
		#ifdef DMA_TO_SD_ENABLE
		if (sd_mmc_start_write_blocks(dataBuffer[0], initBlocksRemaining) != SD_MMC_OK)
		deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
		initBlocksRemaining = 0;
		sd_mmc_wait_end_of_write_blocks(false);
		#endif // DMA_TO_SD_ENABLE
	}
	#endif // not ADMA_ENABLE
	
	//sd_mmc_wait_end_of_write_blocks(true); // Abort any initalized write blocks
	// TODO: Change status LEDs
	
	// TODO: Update currentBlock maybe to get ready for next recording??
	
	// Write end of recording info to a block
	// TODO: Add more meta data to this (frames dropped?, blocks written?, overall time, data starting block?)!
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_RECORDED_POS, writeBufferCount);
	setConfigBlockProp(CONFIG_BLOCK_NUM_BUFFERS_DROPPED_POS, droppedBufferCount);
	
	#ifdef DMA_TO_SD_ENABLE
	// Currently not using ADMA. Might consider switching everything over to ADMA to be consistent
	sd_mmc_init_write_blocks(0,CONFIG_BLOCK, 1);
	sd_mmc_start_write_blocks(configBlock, 1);
	sd_mmc_wait_end_of_write_blocks(false);
	#endif // DMA_TO_SD_ENABLE
	
	#ifdef EXLED_PWM_ENABLE
	setExcitationLED(0, false);
	#endif
	
	#ifdef EWL_ENABLE
	setEWL(0x00);	//Sets the EWL to standby mode
	#endif
	
	#ifdef STATUS_LED_ENABLE
	setStatusLED(0);
	#endif
	
}

void recording()
{
	if (bufferCount > (writeBufferCount + droppedBufferCount)) {
		// This means there are filled buffer(s) ready to be written to SD card
		
		// We need to check if the writing to sd card of data buffers has fallen too far behind where we are at risk
		// of writing overwritten data. We need to detect this and decide what to do in this case
		if (bufferCount > (writeBufferCount + droppedBufferCount + NUM_BUFFERS)) {
			// We  are at risk of at least the current buffer that we want to write to SD card being overflown with new image data
			// We are going to just drop writing the rest of this frame
			
			// Let's figure out how many buffers need to be dropped
			// TODO: I think NUM_BUFFERS here should actually be number_of_buffers_per_frame
			droppedBufferCount += (numBuffersPerFrame - (writeBufferCount + droppedBufferCount) % numBuffersPerFrame);
		}
		else {
			// Actual writing of good buffers
			bufferToWrite = (uint32_t)(&dataBuffer[(writeBufferCount + droppedBufferCount) % NUM_BUFFERS]);
			numBlocks = (bufferToWrite[BUFFER_HEADER_DATA_LENGTH_POS] + (BUFFER_HEADER_LENGTH * 4) + (SD_BLOCK_SIZE - 1)) / SD_BLOCK_SIZE;
			
			// This if statement shouldn't be needed
			if (numBlocks > BUFFER_BLOCK_LENGTH)
			numBlocks = BUFFER_BLOCK_LENGTH;
			
			bufferToWrite[BUFFER_HEADER_WRITE_BUFFER_COUNT_POS] = writeBufferCount;
			bufferToWrite[BUFFER_HEADER_DROPPED_BUFFER_COUNT_POS] = droppedBufferCount;
			bufferToWrite[BUFFER_HEADER_WRITE_TIMESTAMP_POS] = getCurrentTimeMS() - startTimeMS;
			
			tempTimestamp[(writeBufferCount + droppedBufferCount) % 100] = getCurrentTimeMS() - startTimeMS;
			
			#ifdef ADMA_ENABLE
			// Sets up ADMA descriptor for writing 1 full buffer
			setSDDescriptor(bufferToWrite, numBlocks * SD_BLOCK_SIZE,
			SD_DESCRIPTOR_ATT_TRANSFER|SD_DESCRIPTOR_ATT_VALID|SD_DESCRIPTOR_ATT_END);
			
			#ifdef DMA_TO_SD_ENABLE
			sd_mmc_write_with_ADMA(0, currentBlock, (uint32_t)&SDTransferDescriptor, numBlocks);			
			sd_mmc_wait_end_of_ADMA_write(false);
			#endif // DMA_TO_SD_ENABLE
			
			currentBlock += numBlocks;
			
			#else // not ADMA_ENABLE
			if (numBlocks < initBlocksRemaining) {
				// There are enough init blocks for this write
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_start_write_blocks(bufferToWrite, numBlocks) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				#endif // DMA_TO_SD_ENABLE
				
				initBlocksRemaining -= numBlocks;
				currentBlock += numBlocks;
			}
			else if (numBlocks == initBlocksRemaining)
			{
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_start_write_blocks(bufferToWrite, numBlocks) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				#endif // DMA_TO_SD_ENABLE
				currentBlock += numBlocks;
				
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_init_write_blocks(0, currentBlock, BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				#endif // DMA_TO_SD_ENABLE
				
				initBlocksRemaining = (BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK);
			}
			else {
				// TODO: error checking with LED showing status
				
				// This finishes up the remaining blocks in the current set of initialized blocks
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_start_write_blocks(bufferToWrite, initBlocksRemaining) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				#endif // DMA_TO_SD_ENABLE
				currentBlock += initBlocksRemaining;
				
				// We now initialize the next set of blocks
				// TODO: Probably handle errors better here and don't go forward with writing if init fails
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_init_write_blocks(0, currentBlock, BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_INIT_WRITE_ERROR;
				#endif // DMA_TO_SD_ENABLE
				
				// And write remaining data from buffer
				#ifdef DMA_TO_SD_ENABLE
				if (sd_mmc_start_write_blocks((uint32_t)(&bufferToWrite[initBlocksRemaining * SD_BLOCK_SIZE / 4]), numBlocks - initBlocksRemaining) != SD_MMC_OK)
				deviceState |= DEVICE_STATE_SDCARD_WRITE_ERROR;
				sd_mmc_wait_end_of_write_blocks(false);
				#endif // DMA_TO_SD_ENABLE
				
				currentBlock += numBlocks - initBlocksRemaining;
				initBlocksRemaining = (BUFFER_BLOCK_LENGTH * NB_BUFFER_WRITES_PER_CHUNK) - (numBlocks - initBlocksRemaining);
				
			}
			#endif // not ADMA_ENABLE
			
			writeBufferCount++;
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