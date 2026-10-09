/**
@file
@brief Framing of the protobuf buffer header, see MS_header_frame.h.
@author Marcel
*/

#include "MS_header_frame.h"
#include "pb_encode.h"

// CRC-32, reflected polynomial 0xEDB88320, one nibble per lookup (16 entries instead of 256).
static const uint32_t crc32NibbleTable[16] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

uint32_t headerCRC32(const uint8_t *bytes, uint32_t length)
{
	uint32_t crc = 0xFFFFFFFFUL;

	for (uint32_t i = 0; i < length; i++) {
		crc = crc32NibbleTable[(crc ^ bytes[i]) & 0x0F] ^ (crc >> 4);
		crc = crc32NibbleTable[(crc ^ (bytes[i] >> 4)) & 0x0F] ^ (crc >> 4);
	}

	return crc ^ 0xFFFFFFFFUL;
}

bool headerFrameEncode(uint8_t *area, uint32_t areaBytes, const wlms_BufferHeader *header)
{
	const uint32_t maxMessage = areaBytes - HEADER_FRAME_LENGTH_BYTES - HEADER_FRAME_CRC_BYTES;
	const uint32_t crcAt      = areaBytes - HEADER_FRAME_CRC_BYTES;
	pb_ostream_t stream = pb_ostream_from_buffer(area + HEADER_FRAME_LENGTH_BYTES, maxMessage);
	bool ok = pb_encode(&stream, wlms_BufferHeader_fields, header);
	uint32_t length = ok ? (uint32_t)stream.bytes_written : 0;

	area[0] = (uint8_t)length;
	for (uint32_t i = HEADER_FRAME_LENGTH_BYTES + length; i < crcAt; i++) {
		area[i] = 0;
	}

	uint32_t crc = headerCRC32(area, crcAt);
	area[crcAt]     = (uint8_t)(crc);
	area[crcAt + 1] = (uint8_t)(crc >> 8);
	area[crcAt + 2] = (uint8_t)(crc >> 16);
	area[crcAt + 3] = (uint8_t)(crc >> 24);

	return ok;
}
