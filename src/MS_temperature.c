/**
@file
@brief MCU die temperature readout using the SAMD51 internal temperature sensor.
The sensor lives in the Supply Controller (SUPC) and is sampled through ADC0 on the
internal PTAT / CTAT inputs. The result is converted to degrees Celsius with the
two-point factory calibration stored in the NVM Temperature Log Row.
@author Marcel
*/

#include "MS_config.h"
#include "MS_definitions.h"

#ifdef MCU_TEMP_ENABLE

// ------------ NVM Temperature Log Row --------------------------------------
// SAM D5x/E5x datasheet, "NVM Software Calibration Area Mapping", Temperature Log Row.
// Factory calibration: 12-bit PTAT/CTAT ADC readings at a low ("room", TL) and a
// high ("hot", TH) temperature.
#define MS_NVM_TEMP_LOG_BASE		0x00800100UL
#define MS_NVM_TEMP_LOG_WORD(n)		(*(volatile const uint32_t *)(MS_NVM_TEMP_LOG_BASE + 4UL * (n)))

#define MS_TEMP_CAL_TLI()	((MS_NVM_TEMP_LOG_WORD(0) >> 0) & 0xFF)		// TL integer part [degC]
#define MS_TEMP_CAL_TLD()	((MS_NVM_TEMP_LOG_WORD(0) >> 8) & 0xF)		// TL decimal part [1/10 degC]
#define MS_TEMP_CAL_THI()	((MS_NVM_TEMP_LOG_WORD(0) >> 12) & 0xFF)	// TH integer part [degC]
#define MS_TEMP_CAL_THD()	((MS_NVM_TEMP_LOG_WORD(0) >> 20) & 0xF)		// TH decimal part [1/10 degC]
#define MS_TEMP_CAL_VPL()	((MS_NVM_TEMP_LOG_WORD(1) >> 8) & 0xFFF)	// PTAT reading at TL
#define MS_TEMP_CAL_VPH()	((MS_NVM_TEMP_LOG_WORD(1) >> 20) & 0xFFF)	// PTAT reading at TH
#define MS_TEMP_CAL_VCL()	((MS_NVM_TEMP_LOG_WORD(2) >> 0) & 0xFFF)	// CTAT reading at TL
#define MS_TEMP_CAL_VCH()	((MS_NVM_TEMP_LOG_WORD(2) >> 12) & 0xFFF)	// CTAT reading at TH
// ---------------------------------------------------------------------------

// ADC_0 channel index used by the ASF driver. The SAMD51 has no per-channel state,
// so this is only bookkeeping; reuse the battery channel.
#define MCU_TEMP_ADC_CHANNEL		ADC_CHANNEL_BATTERY

// Upper bound on the RESRDY busy-wait. readMCUTemperature() runs inside
// checkBattVoltage_cb, so a conversion that never completes would hang that ISR
// permanently, so every conversion in this file goes through convertADC(). One
// averaged 12-bit conversion takes ~75 us, so this leaves a wide margin while still
// guaranteeing the loop terminates.
#define MCU_TEMP_RESRDY_TIMEOUT		100000UL

/**
@brief Busy-wait for an ADC0 conversion, giving up instead of spinning forever.
@return true if RESRDY came up, false if the wait timed out
*/
static bool waitADCResultReady(void)
{
	for (uint32_t spins = 0; spins < MCU_TEMP_RESRDY_TIMEOUT; spins++) {
		if (hri_adc_get_INTFLAG_RESRDY_bit(ADC0)) {
			return true;
		}
	}

	return false;
}

/**
@brief Run one ADC0 conversion with a bounded wait. Used instead of adc_sync_read_channel(),
which spins on RESRDY forever.
On timeout the in-flight conversion is flushed and a late RESRDY is cleared. Otherwise the
next ASF battery read would see RESRDY already set, return the stale result, and every
battery reading after that would lag one conversion behind.
@param result Raw ADC result, or NULL to discard it
@return true if the conversion completed, false if it timed out
*/
static bool convertADC(uint16_t *result)
{
	uint16_t value;

	hri_adc_clear_INTFLAG_RESRDY_bit(ADC0);
	hri_adc_set_SWTRIG_START_bit(ADC0);

	if (!waitADCResultReady()) {
		// Plain write, not read-modify-write, so a START bit that still reads back as set
		// is not re-triggered together with the flush.
		hri_adc_write_SWTRIG_reg(ADC0, ADC_SWTRIG_FLUSH);	// abort the in-flight conversion
		hri_adc_clear_INTFLAG_RESRDY_bit(ADC0);				// drop a result that landed between timeout and flush
		return false;
	}

	value = hri_adc_read_RESULT_reg(ADC0);	// reading RESULT also clears RESRDY
	if (result != NULL) {
		*result = value;
	}

	return true;
}

/**
@brief Read one internal temperature sensor input (PTAT or CTAT) on ADC0.
@param input ADC_INPUTCTRL_MUXPOS_PTAT_Val or ADC_INPUTCTRL_MUXPOS_CTAT_Val
@param value Raw ADC result (ADC0 must be configured for 12-bit results)
@return true on success, false if a conversion timed out
*/
static bool readTempSensorInput(adc_pos_input_t input, uint16_t *value)
{
	adc_sync_set_inputs(&ADC_0, input, ADC_INPUTCTRL_MUXNEG_GND_Val, MCU_TEMP_ADC_CHANNEL);
	// The first conversion after changing the input mux / reference is discarded.
	return convertADC(NULL) && convertADC(value);
}

/**
@brief Convert PTAT / CTAT readings to temperature using the NVM calibration values.
The formula (datasheet, ADC "Device Temperature Measurement") only depends on the
ratio TP/TC, so the ADC reference and averaging settings cancel out as long as both
inputs are sampled with the same configuration.
@param TP PTAT reading
@param TC CTAT reading
@return Temperature in 0.01 degC, or MCU_TEMP_INVALID if the calibration row is unusable
*/
static int32_t calcTemperatureCentiC(uint16_t TP, uint16_t TC)
{
	float TL  = (float)MS_TEMP_CAL_TLI() + (float)MS_TEMP_CAL_TLD() / 10.0f;
	float TH  = (float)MS_TEMP_CAL_THI() + (float)MS_TEMP_CAL_THD() / 10.0f;
	float VPL = (float)MS_TEMP_CAL_VPL();
	float VPH = (float)MS_TEMP_CAL_VPH();
	float VCL = (float)MS_TEMP_CAL_VCL();
	float VCH = (float)MS_TEMP_CAL_VCH();

	float num = TL * VPH * TC - VPL * TH * TC - TL * VCH * TP + TH * VCL * TP;
	float den = VCL * TP - VCH * TP - VPL * TC + VPH * TC;

	if (den == 0.0f) {
		return MCU_TEMP_INVALID;
	}

	float tempCentiC = (num / den) * 100.0f;
	return (int32_t)(tempCentiC + (tempCentiC >= 0.0f ? 0.5f : -0.5f));
}

int32_t readMCUTemperature(void)
{
	// ADC0 is shared with the battery measurement: save its state and restore it afterwards.
	hri_adc_ctrlb_reg_t     ctrlb     = hri_adc_read_CTRLB_reg(ADC0);
	hri_adc_refctrl_reg_t   refctrl   = hri_adc_read_REFCTRL_reg(ADC0);
	hri_adc_inputctrl_reg_t inputctrl = hri_adc_read_INPUTCTRL_reg(ADC0);
	hri_supc_vref_reg_t     vref      = hri_supc_read_VREF_reg(SUPC);
	uint16_t ptat = 0, ctat = 0;
	bool sampled;

	// CTRLB / REFCTRL are enable-protected
	adc_sync_disable_channel(&ADC_0, MCU_TEMP_ADC_CHANNEL);

	// Route the temperature sensor to the ADC
	hri_supc_set_VREF_ONDEMAND_bit(SUPC);
	hri_supc_set_VREF_TSEN_bit(SUPC);
	hri_supc_set_VREF_VREFOE_bit(SUPC);

	// 12-bit results; 1/2 VDDANA reference keeps PTAT/CTAT (~0.6-0.8 V) well inside the range
	adc_sync_set_resolution(&ADC_0, ADC_CTRLB_RESSEL_12BIT_Val);
	adc_sync_set_reference(&ADC_0, ADC_REFCTRL_REFSEL_INTVCC0_Val);
	adc_sync_enable_channel(&ADC_0, MCU_TEMP_ADC_CHANNEL);

	// No early return on a timeout: the battery configuration below must always be restored.
	sampled = readTempSensorInput(ADC_INPUTCTRL_MUXPOS_PTAT_Val, &ptat)
	       && readTempSensorInput(ADC_INPUTCTRL_MUXPOS_CTAT_Val, &ctat);

	// Restore battery measurement configuration
	adc_sync_disable_channel(&ADC_0, MCU_TEMP_ADC_CHANNEL);
	hri_supc_write_VREF_reg(SUPC, vref);
	hri_adc_write_CTRLB_reg(ADC0, ctrlb);
	hri_adc_write_REFCTRL_reg(ADC0, refctrl);
	hri_adc_write_INPUTCTRL_reg(ADC0, inputctrl);
	adc_sync_enable_channel(&ADC_0, MCU_TEMP_ADC_CHANNEL);

	// Throw away the first conversion after switching the reference back. The
	// temperature is already sampled at this point, so a timeout here does not
	// invalidate it; convertADC() has flushed the ADC for the next battery read.
	(void)convertADC(NULL);

	if (!sampled) {
		return MCU_TEMP_INVALID;
	}

	return calcTemperatureCentiC(ptat, ctat);
}

#endif // MCU_TEMP_ENABLE
