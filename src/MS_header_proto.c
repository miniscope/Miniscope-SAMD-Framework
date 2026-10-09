/**
@file
@brief Fills the protobuf buffer header (HEADER_PROTOBUF_ENABLE) from the device state.
The header area after the preamble word holds one wlms.BufferHeader message, framed by
MS_header_frame.c (length byte, message, zero padding, CRC-32). Schema: proto/wlms_header.proto.
@author Marcel
*/

#include <atmel_start.h>
#include "MS_config.h"
#include "MS_definitions.h"

#ifdef HEADER_PROTOBUF_ENABLE

#include "MS_header_frame.h"

volatile uint32_t headerEncodeCycles    = 0; // CPU cycles the last header encode took (DWT)
volatile uint32_t headerEncodeCyclesMax = 0;
volatile uint32_t headerEncodeFailures  = 0; // messages that did not fit the header area

static void cycleCounterEnable(void)
{
	if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		DWT->CYCCNT = 0;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}

void setBufferHeaderProtobuf(volatile uint32_t *areaWords, uint32_t dataBytes)
{
	wlms_BufferHeader h = wlms_BufferHeader_init_zero;
	uint32_t t0;

	cycleCounterEnable();
	t0 = DWT->CYCCNT;

	h.firmware_id = FW_ID;
	h.packet_type = wlms_PacketType_PACKET_TYPE_IMAGE;
	h.device_id   = DEVICE_ID;
	#ifdef RTC_TIMESTAMP_ENABLE
	if (rtcIsRunning()) h.flags |= wlms_HeaderFlag_HEADER_FLAG_RTC_TIMESTAMP;
	#endif

	h.frame_number    = frameNum;
	h.buffer_count    = bufferCount;
	h.buffer_in_frame = frameBufferCount;
	h.tx_backlog      = (bufferCount >= writeBufferCount) ? bufferCount - writeBufferCount : 0;
	#if defined(DMA_TO_SPI_ENABLE) || defined(DMA_TO_USART_ENABLE)
	// The SD write path is compiled out on the optical link, so droppedBufferCount never
	// moves there; sdoOverrunCount carries the losses that path can actually suffer.
	h.buffers_lost = droppedBufferCount + sdoOverrunCount;
	#else
	h.buffers_lost = droppedBufferCount;
	#endif
	h.timestamp_ms = getCurrentTimeMS() - startTimeMS;
	h.pixel_bytes  = dataBytes;

	h.battery_adc = battVolt;
	h.input_adc   = wptVolt;


	h.ewl_value = ewlvalue;
	h.roi_x     = roi_x_shift;
	h.roi_y     = roi_y_shift;

	// The header area is CPU-owned until the transmitter picks the buffer up, so nanopb may write
	// it through a plain pointer.
	if (!headerFrameEncode((uint8_t *)areaWords, HEADER_PB_AREA_BYTES, &h)) {
		headerEncodeFailures++;
	}

	headerEncodeCycles = DWT->CYCCNT - t0;
	if (headerEncodeCycles > headerEncodeCyclesMax) headerEncodeCyclesMax = headerEncodeCycles;
}

#endif // HEADER_PROTOBUF_ENABLE
