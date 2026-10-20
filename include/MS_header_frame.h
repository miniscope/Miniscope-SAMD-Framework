/**
@file
@brief Framing of the protobuf buffer header: length byte, message, zero padding, CRC-32.
Plain C99 with no MCU dependencies so it can be compiled and checked on a host as well.
@author Marcel
*/

#ifndef MS_HEADER_FRAME_H_
#define MS_HEADER_FRAME_H_

#include <stdbool.h>
#include <stdint.h>
#include "wlms_header.pb.h"

#define HEADER_FRAME_LENGTH_BYTES	1
#define HEADER_FRAME_CRC_BYTES		4

/** CRC-32 (IEEE 802.3, reflected 0xEDB88320, the same as zlib.crc32) over @p length bytes. */
uint32_t headerCRC32(const uint8_t *bytes, uint32_t length);

/**
@brief Encode @p header into the fixed header area @p area of @p areaBytes bytes.
Layout: area[0] = encoded message length n, area[1..n] = message, zero padding,
area[areaBytes-4..] = CRC-32 little endian over area[0..areaBytes-5].
@return true if the message fit; otherwise the length byte is 0, the CRC still valid, and the
host sees an empty message.
*/
bool headerFrameEncode(uint8_t *area, uint32_t areaBytes, const wlms_BufferHeader *header);

#endif /* MS_HEADER_FRAME_H_ */
