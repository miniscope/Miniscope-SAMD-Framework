/**
@file
@brief Header integrity check for the host.
HEADER_CRC_ENABLE: CRC-32 (IEEE, as zlib.crc32) over the 12 header words as little-endian bytes,
with slot 10 bits 31:8 zero, truncated to its low 24 bits and stored in slot 10 bits 31:8. A header
whose CRC does not match was corrupted on the optical link and the host should drop that buffer.
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
