/**
@file
@brief PYTHON480 black-level calibration status and die temperature for the buffer header.
The computed black offset is not a readable register, so this exposes what is: the per-channel
black-calibration error flags (reg 136), the sensor die temperature (reg 97) and a readback check
of the black-calibration configuration (regs 96, 128, 129). It also applies the compile-time
BLACKCAL_MODE used for the auto vs frozen/manual A/B test under wireless power.
@author Marcel
*/

#include "MS_config.h"
#include "MS_definitions.h"
#include "python480.h"

#ifdef SENSOR_STATUS_ENABLE

// ------------ PYTHON480 registers ------------------------------------------
#define PY_REG_TEMP_CONFIG			96	// [0] enable, [13:8] signed offset
#define PY_REG_TEMP					97	// [7:0] temperature readout, ~0.75 degC/LSB, uncalibrated
#define PY_REG_BLACKCAL				128	// [7:0] target black level, [10:8] log2(samples)
#define PY_REG_GENERAL_CONFIG		129	// [0] auto_blackcal_enable, [9:1] blackcal_offset, [10] offset sign
#define PY_REG_BLACKCAL_ERROR		136	// [1:0] one error bit per data channel

#define PY_TEMP_CONFIG_VALUE		0x0001
#define PY_BLACKCAL_VALUE			0x470A	// Must match RequiredUploads()
#define PY_GENERAL_CONFIG_AUTO		0x8001	// Must match RequiredUploads()
// ---------------------------------------------------------------------------

static uint32_t sensorTemp = 0;	// last reg 97 readout, reused between temperature reads
static uint8_t tempFrame = 0;	// frames since the last temperature read

/**
@brief Value written to reg 129 for the configured BLACKCAL_MODE.
*/
static uint16_t blackCalGeneralConfig(void)
{
	#if BLACKCAL_MODE == BLACKCAL_MODE_FREEZE
	return 0x8000 | (0x1FF << 1) | 0x0001;
	#elif BLACKCAL_MODE == BLACKCAL_MODE_MANUAL
	return 0x8000 | ((BLACKCAL_MANUAL_OFFSET & 0x1FF) << 1) | ((BLACKCAL_MANUAL_OFFSET_DEC & 0x1) << 10);
	#else
	return PY_GENERAL_CONFIG_AUTO;
	#endif
}

/**
@brief Bit-banged register read with a short, configurable clock half period.
spi_BB_Read() waits 10 us per edge (~0.6 ms per read), too slow for the frame ISR.
The sensor's SPI limit scales with its input clock (f_in / f_spi >= 6..30 per datasheet),
so SENSOR_SPI_HALF_PERIOD_US is kept conservative. Same framing as spi_BB_Read().
@param address PYTHON480 register address (9 bit)
@return register value
*/
static uint16_t spiBBReadFast(uint16_t address)
{
	uint16_t value = 0;

	gpio_set_pin_level(SPI_BB_NSS, 1);
	delay_us(SENSOR_SPI_HALF_PERIOD_US);
	gpio_set_pin_level(SPI_BB_NSS, 0);
	delay_us(SENSOR_SPI_HALF_PERIOD_US);

	for (int8_t i = 8; i >= 0; i--) { // 9 bit address
		gpio_set_pin_level(SPI_BB_SCK, 0);
		gpio_set_pin_level(SPI_BB_MOSI, (address >> i) & 0x0001);
		delay_us(SENSOR_SPI_HALF_PERIOD_US);
		gpio_set_pin_level(SPI_BB_SCK, 1);
		delay_us(SENSOR_SPI_HALF_PERIOD_US);
	}
	// Read bit
	gpio_set_pin_level(SPI_BB_SCK, 0);
	gpio_set_pin_level(SPI_BB_MOSI, 0);
	delay_us(SENSOR_SPI_HALF_PERIOD_US);
	gpio_set_pin_level(SPI_BB_SCK, 1);
	delay_us(SENSOR_SPI_HALF_PERIOD_US);
	gpio_set_pin_level(SPI_BB_SCK, 0);
	delay_us(SENSOR_SPI_HALF_PERIOD_US);

	for (int8_t i = 15; i >= 0; i--) {
		gpio_set_pin_level(SPI_BB_SCK, 1);
		delay_us(SENSOR_SPI_HALF_PERIOD_US);
		gpio_set_pin_level(SPI_BB_SCK, 0);
		value |= (gpio_get_pin_level(SPI_BB_MISO) << i);
		delay_us(SENSOR_SPI_HALF_PERIOD_US);
	}
	delay_us(SENSOR_SPI_HALF_PERIOD_US);
	gpio_set_pin_level(SPI_BB_NSS, 1);

	return value;
}

void applyBlackCalMode(void)
{
	#if BLACKCAL_MODE != BLACKCAL_MODE_AUTO
	// Written at recording start so auto calibration has converged on the frames since
	// boot before FREEZE holds it. Expect a few transient frames after this write.
	spi_BB_Write(PY_REG_GENERAL_CONFIG, blackCalGeneralConfig());
	#endif

	bool mismatch = (spi_BB_Read(PY_REG_TEMP_CONFIG) != PY_TEMP_CONFIG_VALUE)
		|| (spi_BB_Read(PY_REG_BLACKCAL) != PY_BLACKCAL_VALUE)
		|| (spi_BB_Read(PY_REG_GENERAL_CONFIG) != blackCalGeneralConfig());

	sensorTemp = 0;
	tempFrame = 0;
	sensorStatus = SENSOR_STATUS_VALID
		| ((uint32_t)(BLACKCAL_MODE & 0x3) << SENSOR_STATUS_MODE_SHIFT)
		| (mismatch ? SENSOR_STATUS_CFG_MISMATCH : 0);
}

void readSensorStatus(void)
{
	uint32_t blackCalError = spiBBReadFast(PY_REG_BLACKCAL_ERROR) & 0x3;

	// The die temperature drifts slowly; read it every SENSOR_TEMP_READ_PERIOD_FRAMES
	// frames so the per-frame SPI cost stays at one register.
	if (tempFrame == 0) {
		sensorTemp = spiBBReadFast(PY_REG_TEMP) & 0xFF;
	}
	if (++tempFrame >= SENSOR_TEMP_READ_PERIOD_FRAMES) {
		tempFrame = 0;
	}

	sensorStatus = (sensorStatus & (SENSOR_STATUS_VALID | SENSOR_STATUS_CFG_MISMATCH | SENSOR_STATUS_MODE_MASK))
		| (blackCalError << SENSOR_STATUS_BLACKCAL_ERR_SHIFT)
		| (sensorTemp << SENSOR_STATUS_TEMP_SHIFT);
}

#endif // SENSOR_STATUS_ENABLE
