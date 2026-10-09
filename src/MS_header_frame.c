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

static void finishFrame(uint8_t *area, uint32_t areaBytes, uint32_t length)
{
	const uint32_t crcAt = areaBytes - HEADER_FRAME_CRC_BYTES;

	area[0] = (uint8_t)length;
	for (uint32_t i = HEADER_FRAME_LENGTH_BYTES + length; i < crcAt; i++) {
		area[i] = 0;
	}

	uint32_t crc = headerCRC32(area, crcAt);
	area[crcAt]     = (uint8_t)(crc);
	area[crcAt + 1] = (uint8_t)(crc >> 8);
	area[crcAt + 2] = (uint8_t)(crc >> 16);
	area[crcAt + 3] = (uint8_t)(crc >> 24);
}

bool headerFrameEncode(uint8_t *area, uint32_t areaBytes, const wlms_BufferHeader *header)
{
	const uint32_t maxMessage = areaBytes - HEADER_FRAME_LENGTH_BYTES - HEADER_FRAME_CRC_BYTES;
	pb_ostream_t stream = pb_ostream_from_buffer(area + HEADER_FRAME_LENGTH_BYTES, maxMessage);
	bool ok = pb_encode(&stream, wlms_BufferHeader_fields, header);

	finishFrame(area, areaBytes, ok ? (uint32_t)stream.bytes_written : 0);
	return ok;
}

// ---- hand-written protobuf encoder ---------------------------------------------------------
// proto3 rules as nanopb applies them: a scalar equal to 0 and an empty bytes field are not sent,
// an `optional` field is sent when its has_ flag is set. Fields go out in field-number order.

static inline uint8_t *putVarint(uint8_t *p, uint32_t v)
{
	while (v >= 0x80) {
		*p++ = (uint8_t)(v | 0x80);
		v >>= 7;
	}
	*p++ = (uint8_t)v;
	return p;
}

static inline uint8_t *putTag(uint8_t *p, uint32_t field, uint32_t wireType)
{
	return putVarint(p, (field << 3) | wireType);
}

static inline uint8_t *putU32(uint8_t *p, uint32_t field, uint32_t v)
{
	if (v == 0) return p;
	return putVarint(putTag(p, field, 0), v);
}

static inline uint8_t *putS32(uint8_t *p, uint32_t field, int32_t v) // sint32: zigzag
{
	uint32_t u = ((uint32_t)v << 1) ^ (uint32_t)(v >> 31);
	return putVarint(putTag(p, field, 0), u);
}

static inline uint8_t *putBytes(uint8_t *p, uint32_t field, const uint8_t *bytes, uint32_t n)
{
	if (n == 0) return p;
	p = putVarint(putTag(p, field, 2), n);
	for (uint32_t i = 0; i < n; i++) p[i] = bytes[i];
	return p + n;
}

// upper bound of the encoded size: every uint32 as 5 bytes plus its tag
#define FAST_MAX_MESSAGE (16 * (2 + 5) + 2 * (2 + 1 + 4))

bool headerFrameEncodeFast(uint8_t *area, uint32_t areaBytes, const wlms_BufferHeader *h)
{
	const uint32_t maxMessage = areaBytes - HEADER_FRAME_LENGTH_BYTES - HEADER_FRAME_CRC_BYTES;
	uint8_t  buf[FAST_MAX_MESSAGE];
	uint8_t *p = buf;

	p = putU32(p, wlms_BufferHeader_firmware_id_tag, h->firmware_id);
	p = putU32(p, wlms_BufferHeader_packet_type_tag, (uint32_t)h->packet_type);
	p = putU32(p, wlms_BufferHeader_device_id_tag, h->device_id);
	p = putU32(p, wlms_BufferHeader_flags_tag, h->flags);
	p = putU32(p, wlms_BufferHeader_frame_number_tag, h->frame_number);
	p = putU32(p, wlms_BufferHeader_buffer_count_tag, h->buffer_count);
	p = putU32(p, wlms_BufferHeader_buffer_in_frame_tag, h->buffer_in_frame);
	p = putU32(p, wlms_BufferHeader_tx_backlog_tag, h->tx_backlog);
	p = putU32(p, wlms_BufferHeader_buffers_lost_tag, h->buffers_lost);
	p = putU32(p, wlms_BufferHeader_timestamp_ms_tag, h->timestamp_ms);
	p = putU32(p, wlms_BufferHeader_pixel_bytes_tag, h->pixel_bytes);
	p = putU32(p, wlms_BufferHeader_battery_adc_tag, h->battery_adc);
	p = putU32(p, wlms_BufferHeader_input_adc_tag, h->input_adc);
	if (h->has_mcu_temp_centi_c) p = putS32(p, wlms_BufferHeader_mcu_temp_centi_c_tag, h->mcu_temp_centi_c);
	p = putU32(p, wlms_BufferHeader_sensor_temp_raw_tag, h->sensor_temp_raw);
	p = putBytes(p, wlms_BufferHeader_black_ref_tag, h->black_ref.bytes, h->black_ref.size);
	p = putBytes(p, wlms_BufferHeader_tx_diag_tag, h->tx_diag.bytes, h->tx_diag.size);
	p = putU32(p, wlms_BufferHeader_encode_cycles_tag, h->encode_cycles);
	#ifdef wlms_BufferHeader_timing_tag
	p = putBytes(p, wlms_BufferHeader_timing_tag, h->timing.bytes, h->timing.size);
	#endif

	uint32_t length = (uint32_t)(p - buf);
	bool ok = length <= maxMessage;
	if (ok) {
		for (uint32_t i = 0; i < length; i++) area[HEADER_FRAME_LENGTH_BYTES + i] = buf[i];
	}
	finishFrame(area, areaBytes, ok ? length : 0);
	return ok;
}
