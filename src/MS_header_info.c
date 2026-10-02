/**
@file
@brief Header integrity check and firmware version record for the host.
Two things the host (miniscope-io) can use without knowing anything about this firmware beyond
the 12-word header layout that has been stable since the first release:
- HEADER_CRC_ENABLE: CRC-32 (IEEE, as zlib.crc32) over the 12 header words as little-endian
  bytes, with slot 10 bits 31:8 zero, truncated to its low 24 bits and stored in slot 10 bits
  31:8. A header whose CRC does not match was corrupted on the optical link and should be dropped.
- VERSION_SIDEBAND_ENABLE: a 32-byte version record sent one byte per buffer in slot 11 bits
  31:24, byte index = bufferCount % 32. The device streams from power-on, so the record repeats
  forever and the host can assemble it from any 32 consecutive buffers (4 frames, 0.2 s).
@author Marcel
*/

#include "MS_config.h"
#include "MS_definitions.h"

#ifdef HEADER_CRC_ENABLE

// CRC-32, reflected polynomial 0xEDB88320, one nibble per lookup (16 entries instead of 256).
static const uint32_t crc32NibbleTable[16] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

static inline uint32_t crc32Byte(uint32_t crc, uint8_t byte)
{
	crc = crc32NibbleTable[(crc ^ byte) & 0x0F] ^ (crc >> 4);
	crc = crc32NibbleTable[(crc ^ (byte >> 4)) & 0x0F] ^ (crc >> 4);
	return crc;
}

uint32_t headerCRC24(volatile uint32_t *header)
{
	uint32_t crc = 0xFFFFFFFFUL;

	for (uint32_t i = 0; i < BUFFER_HEADER_LENGTH; i++) {
		uint32_t word = header[i];
		if (i == BUFFER_HEADER_BATTERY_VOLTAGE_POS) {
			word &= ~HEADER_CRC_MASK; // the CRC field itself counts as zero
		}
		crc = crc32Byte(crc, (uint8_t)(word));
		crc = crc32Byte(crc, (uint8_t)(word >> 8));
		crc = crc32Byte(crc, (uint8_t)(word >> 16));
		crc = crc32Byte(crc, (uint8_t)(word >> 24));
	}

	return (crc ^ 0xFFFFFFFFUL) & 0x00FFFFFFUL;
}

#endif // HEADER_CRC_ENABLE

#ifdef VERSION_SIDEBAND_ENABLE

static uint8_t versionRecord[VERSION_RECORD_LENGTH];

static void putLE16(uint32_t offset, uint16_t value)
{
	versionRecord[offset]     = (uint8_t)(value);
	versionRecord[offset + 1] = (uint8_t)(value >> 8);
}

static void putLE32(uint32_t offset, uint32_t value)
{
	putLE16(offset, (uint16_t)value);
	putLE16(offset + 2, (uint16_t)(value >> 16));
}

void buildVersionRecord(void)
{
	uint8_t flags = 0;
	uint8_t sum   = 0;

	for (uint32_t i = 0; i < VERSION_RECORD_LENGTH; i++) {
		versionRecord[i] = 0;
	}

	#ifdef RTC_TIMESTAMP_ENABLE
	if (rtcIsRunning()) flags |= VR_FLAG_RTC_TIMESTAMP;
	#endif
	#ifdef BLACKREF_LINE_ENABLE
	flags |= VR_FLAG_BLACKREF_LINE;
	#endif
	#ifdef TX_SLIP_TELEMETRY_ENABLE
	flags |= VR_FLAG_TX_SLIP_TELEMETRY;
	#endif
	#ifdef SENSOR_STATUS_ENABLE
	flags |= VR_FLAG_SENSOR_STATUS;
	flags |= (BLACKCAL_MODE & 0x3) << VR_FLAG_BLACKCAL_MODE_SHIFT;
	#endif
	#ifdef MCU_TEMP_ENABLE
	flags |= VR_FLAG_MCU_TEMP;
	#endif
	#ifdef HEADER_CRC_ENABLE
	flags |= VR_FLAG_HEADER_CRC;
	#endif

	versionRecord[VR_MAGIC]         = VERSION_RECORD_MAGIC;
	versionRecord[VR_FORMAT]        = VERSION_RECORD_FORMAT;
	versionRecord[VR_FW_MAJOR]      = FW_VERSION_MAJOR;
	versionRecord[VR_FW_MINOR]      = FW_VERSION_MINOR;
	versionRecord[VR_FW_PATCH]      = FW_VERSION_PATCH;
	putLE32(VR_GIT_HASH, FW_GIT_HASH);
	versionRecord[VR_HEADER_LAYOUT] = BUFFER_HEADER_LAYOUT_VERSION;
	versionRecord[VR_FLAGS]         = flags;
	versionRecord[VR_DEVICE_ID]     = (uint8_t)DEVICE_ID;
	putLE16(VR_IMAGE_WIDTH, image_width);
	putLE16(VR_IMAGE_HEIGHT, image_height);
	putLE16(VR_BLACKREF_PX, BLACKREF_PIXELS_PER_FRAME);
	versionRecord[VR_FRAME_RATE]    = FRAME_RATE;
	versionRecord[VR_NUM_BUFFERS]   = NUM_BUFFERS;
	versionRecord[VR_BUFFER_BLOCK_LENGTH] = BUFFER_BLOCK_LENGTH;
	versionRecord[VR_GIT_DIRTY]     = FW_GIT_DIRTY;

	// Two's complement of the byte sum, so all 32 bytes add up to 0 modulo 256.
	for (uint32_t i = 0; i < VERSION_RECORD_LENGTH - 1; i++) {
		sum += versionRecord[i];
	}
	versionRecord[VR_CHECKSUM] = (uint8_t)(0 - sum);
}

uint8_t versionRecordByte(uint32_t index)
{
	return versionRecord[index % VERSION_RECORD_LENGTH];
}

#endif // VERSION_SIDEBAND_ENABLE
