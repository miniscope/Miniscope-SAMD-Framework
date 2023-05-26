#include <atmel_start.h>
#include <hpl_dmac_config.h>
#include <hpl_dma.h>

#include "MS_definitions.h"
#include "driver_init_ms.h"
#include "dma_custom_driver.h"
#include "i2c_bb.h"
#include "sd_mmc_start_ms.h"

#ifdef PYTHON480_ENABLE
#include "python480.h"
#endif



int main(void)
{
	#ifdef PYTHON480_ENABLE
	getBuffersPerFrame();
	#endif
	
	/* Initializes MCU, drivers and middleware */
	system_init();
	IO_BUS_init();
	sd_mmc_stack_init();
	peripheralInit();
	
	timerInit();
	irqInit();

	#ifdef DMA_TO_SPI_ENABLE
	TXLinkedListInit();
	#endif

	#ifdef PYTHON480_ENABLE
	PCCLinkedListInit();
	imageSensorInit();
	#endif

	#ifdef DMA_TO_SD_ENABLE
	debugHeaderProp();
	#endif
	
	DataBufferInit(); // For testing buffer to SERCOM DMA
	dmaEnable();
	
	while (1) {
		#if defined(PYTHON480_ENABLE)
		if (deviceState & DEVICE_STATE_START_RECORDING) {
			startRecording();
		}
		if (deviceState & DEVICE_STATE_RECORDING) {
			recording();
		}
		if (deviceState & DEVICE_STATE_STOP_RECORDING) {
			stopRecording();
		}
		#endif
	}
}
