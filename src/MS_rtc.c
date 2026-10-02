/**
@file
@brief Crystal-accurate header timestamps from the RTC.
The SAM D51 runs its 48 MHz DFLL in open loop, so the TC1 millisecond tick that stamps every
buffer drifts by a few thousand ppm and differs from board to board (0.19 % slow on the rig
board, 2026-09-30). Closing the DFLL loop was tried and dithers the optical SPI rate, so the
CPU clock stays as it is. Instead the RTC counts the 32.768 kHz crystal (XOSC32K, about
20 ppm) in 32-bit mode and getCurrentTimeMS() converts that count to milliseconds. TC1 keeps
scheduling the periodic tasks.
@author Marcel
*/

#include "MS_config.h"
#include "MS_definitions.h"

#ifdef RTC_TIMESTAMP_ENABLE

static bool rtcRunning = false;

/**
@brief Make XOSC32K free-running and wait for it to report ready, bounded.
Atmel START enables XOSC32K with ONDEMAND set, so it stops whenever no peripheral requests it.
STARTUP 0 means about 62 ms of start-up time after the request.
@return true if XOSC32K is ready, false after RTC_XOSC32K_READY_TIMEOUT_US
*/
static bool startXOSC32K(void)
{
	hri_osc32kctrl_clear_XOSC32K_ONDEMAND_bit(OSC32KCTRL);

	for (uint32_t waited = 0; waited < RTC_XOSC32K_READY_TIMEOUT_US; waited += RTC_XOSC32K_POLL_US) {
		if (hri_osc32kctrl_get_STATUS_XOSC32KRDY_bit(OSC32KCTRL)) {
			return true;
		}
		delay_us(RTC_XOSC32K_POLL_US);
	}

	return false;
}

void rtcInit(void)
{
	rtcRunning = false;

	if (!startXOSC32K()) {
		// No crystal: getCurrentTimeMS() keeps the TC1 tick and header slot 11 bit 31 stays 0.
		return;
	}

	hri_mclk_set_APBAMASK_RTC_bit(MCLK);
	hri_osc32kctrl_write_RTCCTRL_reg(OSC32KCTRL, OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K);

	// Reset, then a 32-bit counter at the full 32.768 kHz. COUNTSYNC keeps COUNT continuously
	// synchronised to the bus so a read in an ISR never has to wait.
	hri_rtcmode0_set_CTRLA_SWRST_bit(RTC);
	hri_rtcmode0_write_CTRLA_reg(RTC, RTC_MODE0_CTRLA_MODE_COUNT32 | RTC_MODE0_CTRLA_PRESCALER_DIV1
	                                     | RTC_MODE0_CTRLA_COUNTSYNC);
	hri_rtcmode0_set_CTRLA_ENABLE_bit(RTC);
	hri_rtcmode0_wait_for_sync(RTC, RTC_MODE0_SYNCBUSY_ENABLE | RTC_MODE0_SYNCBUSY_COUNTSYNC);

	// The datasheet warns that the first COUNT read after enabling COUNTSYNC can be stale.
	(void)hri_rtcmode0_read_COUNT_reg(RTC);
	delay_us(RTC_XOSC32K_POLL_US);

	rtcRunning = true;
}

bool rtcIsRunning(void)
{
	return rtcRunning;
}

uint32_t rtcTimeMS(void)
{
	// count * 1000 / 32768 in 64 bit so the product cannot overflow. The 32-bit count wraps
	// after 36.4 h and the returned ms wrap with it; recordings are shorter than that.
	uint32_t count = hri_rtcmode0_read_COUNT_reg(RTC);

	return (uint32_t)(((uint64_t)count * 1000ULL) >> 15);
}

#endif // RTC_TIMESTAMP_ENABLE
