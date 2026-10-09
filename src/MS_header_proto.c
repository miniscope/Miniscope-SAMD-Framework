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
volatile uint32_t headerEncodeMismatches = 0; // buffers where the fast encoder differed from nanopb

// the packed-struct layout from issue #39, timed for comparison only (never sent)
typedef struct __attribute__((packed)) {
	uint16_t firmware_id; uint16_t packet_type; uint8_t header_bytes; uint8_t device_id;
	uint8_t battery_adc; uint8_t input_adc;
	uint32_t frame_number; uint32_t buffer_count; uint32_t buffers_sent; uint32_t buffers_lost;
	uint32_t timestamp_ms; uint32_t pixel_bytes; uint8_t buffer_in_frame; uint8_t diag[4];
	uint8_t sensor_temp_raw; int16_t mcu_temp_centi_c; uint8_t black_ref[4];
	uint32_t crc32;
} wlmsStructHeader;
_Static_assert(sizeof(wlmsStructHeader) == 48, "struct layout");

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
	if (mcuTempCentiC != MCU_TEMP_INVALID) {
		h.has_mcu_temp_centi_c = true;
		h.mcu_temp_centi_c     = mcuTempCentiC;
	}
	#ifdef PYTHON480_SENSOR_ENABLE
	h.sensor_temp_raw = sensorTempRaw;
	// Byte p = reference-line pixels with index % 4 == p, captured at the start of this frame
	uint32_t ref = blackRefLine[0];
	h.black_ref.size = 4;
	h.black_ref.bytes[0] = (uint8_t)(ref);
	h.black_ref.bytes[1] = (uint8_t)(ref >> 8);
	h.black_ref.bytes[2] = (uint8_t)(ref >> 16);
	h.black_ref.bytes[3] = (uint8_t)(ref >> 24);
	#endif

	#ifdef TX_SLIP_TELEMETRY_ENABLE
	h.tx_diag.size = 4;
	h.tx_diag.bytes[0] = (uint8_t)sdoSlipCount;     // slips
	h.tx_diag.bytes[1] = (uint8_t)sdoPhaseErr;      // slot out of order
	h.tx_diag.bytes[2] = (uint8_t)sdoSkippedResume; // restarts skipped
	h.tx_diag.bytes[3] = (uint8_t)sdoMaxBacklog;    // max waiting
	#endif
	h.encode_cycles = headerEncodeCycles;
	// previous buffer's timing and whether the two encoders disagreed (flag bit 7)
	static uint16_t prevNano = 0, prevFast = 0, prevStruct = 0, prevCrc = 0;
	static bool prevMismatch = false;
	if (prevMismatch) h.flags |= 0x80;
	h.timing.size = 8;
	h.timing.bytes[0] = (uint8_t)prevNano;   h.timing.bytes[1] = (uint8_t)(prevNano >> 8);
	h.timing.bytes[2] = (uint8_t)prevFast;   h.timing.bytes[3] = (uint8_t)(prevFast >> 8);
	h.timing.bytes[4] = (uint8_t)prevStruct; h.timing.bytes[5] = (uint8_t)(prevStruct >> 8);
	h.timing.bytes[6] = (uint8_t)prevCrc;    h.timing.bytes[7] = (uint8_t)(prevCrc >> 8);

	// 1. nanopb into a scratch area (reference)
	static uint8_t scratch[HEADER_PB_AREA_BYTES];
	uint32_t tA = DWT->CYCCNT;
	headerFrameEncode(scratch, HEADER_PB_AREA_BYTES, &h);
	uint32_t tB = DWT->CYCCNT;
	// 2. hand-written encoder onto the wire
	uint8_t *area = (uint8_t *)areaWords;
	uint32_t tC = DWT->CYCCNT;
	if (!headerFrameEncodeFast(area, HEADER_PB_AREA_BYTES, &h)) {
		headerEncodeFailures++;
	}
	uint32_t tD = DWT->CYCCNT;
	// 3. packed struct of the same content (the layout discussed in issue #39) plus its CRC
	static volatile uint8_t structArea[56];
	uint32_t tE = DWT->CYCCNT;
	{
		wlmsStructHeader s;
		s.firmware_id = FW_ID; s.packet_type = 1; s.header_bytes = 56; s.device_id = (uint8_t)DEVICE_ID;
		s.battery_adc = (uint8_t)battVolt; s.input_adc = wptVolt;
		s.frame_number = frameNum; s.buffer_count = bufferCount; s.buffers_sent = writeBufferCount;
		s.buffers_lost = h.buffers_lost; s.timestamp_ms = h.timestamp_ms; s.pixel_bytes = dataBytes;
		s.buffer_in_frame = (uint8_t)frameBufferCount;
		s.diag[0] = h.tx_diag.bytes[0]; s.diag[1] = h.tx_diag.bytes[1]; s.diag[2] = h.tx_diag.bytes[2]; s.diag[3] = h.tx_diag.bytes[3];
		s.sensor_temp_raw = h.sensor_temp_raw; s.mcu_temp_centi_c = (int16_t)mcuTempCentiC;
		s.black_ref[0] = h.black_ref.bytes[0]; s.black_ref[1] = h.black_ref.bytes[1]; s.black_ref[2] = h.black_ref.bytes[2]; s.black_ref[3] = h.black_ref.bytes[3];
		s.crc32 = headerCRC32((const uint8_t *)&s, sizeof(s) - 4);
		const uint8_t *sb = (const uint8_t *)&s;
		for (uint32_t i = 0; i < sizeof(s); i++) structArea[i] = sb[i];
	}
	uint32_t tF = DWT->CYCCNT;
	// 4. CRC alone over the framed area
	volatile uint32_t crcAgain = headerCRC32(area, HEADER_PB_AREA_BYTES - 4);
	uint32_t tG = DWT->CYCCNT;
	(void)crcAgain;

	bool same = true;
	for (uint32_t i = 0; i < HEADER_PB_AREA_BYTES; i++) {
		if (scratch[i] != area[i]) { same = false; break; }
	}
	if (!same) headerEncodeMismatches++;
	prevMismatch = !same;
	prevNano = (uint16_t)(tB - tA); prevFast = (uint16_t)(tD - tC);
	prevStruct = (uint16_t)(tF - tE); prevCrc = (uint16_t)(tG - tF);

	headerEncodeCycles = DWT->CYCCNT - t0;
	if (headerEncodeCycles > headerEncodeCyclesMax) headerEncodeCyclesMax = headerEncodeCycles;
}

#endif // HEADER_PROTOBUF_ENABLE
