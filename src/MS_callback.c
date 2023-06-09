/*
 * MS_cb.c
 *
 * Created: 5/20/2023 7:41:53 PM
 *  Author: Takuya
 */ 

#include "MS_definitions.h"
#include "dma_custom_driver.h"

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
	
	if (pinState == true) {
		// beginning of new frame acquisition
		
	}
	else {
		// Handles end of frame
		
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
			
			setBufferHeader((BUFFER_BLOCK_LENGTH * BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH) - _dma_get_WRB_data(CONF_PCC_DMA_CHANNEL)); // This should get total beats transferred through DMA
			
			frameBufferCount = 0;
			bufferCount++; // A buffer has been filled (likely partially) and is ready for writing to SD card
			frameNum++; // Zero-Indexed
			
			if (deviceState & DEVICE_STATE_RECORDING) { // Keep recording
				// Update Linked List
				setPCCLinkedListPosition(bufferCount % NUM_BUFFERS); // Moves to next buffer/linked list element
				setTXLinkedListPosition(bufferCount % NUM_BUFFERS); // Moves to next buffer/linked list element
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
		
		// add header to current buffer
		
		// Some debugging stuff here
		//tempHeader[tempCount][0] = frameNum;
		//tempHeader[tempCount][1] = bufferCount;
		//tempHeader[tempCount][2] = 4 * dma_get_WRB_data(CONF_PCC_DMA_CHANNEL);
		//tempHeader[tempCount][3] = PCC->ISR.reg;
		//if (tempCount < 99)
		//tempCount++;
		
		setBufferHeader(BUFFER_BLOCK_LENGTH * BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH);
		bufferCount++;// increment counters
		frameBufferCount++;
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
