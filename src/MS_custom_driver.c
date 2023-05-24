/*
 * WLSM_custom_driver.c
 *
 * Created: 5/20/2023 1:42:40 PM
 *  Author: Takuya
 */ 
#include "MS_definitions.h"
#include "MS_custom_driver.h"
#include <hpl_dma.h>
#include <hpl_dmac_config.h>
#

#ifdef DMA_TO_SD_ENABLE
#endif

int32_t dma_set_BTCTRL(const uint8_t channel, uint32_t src)
// Added by DAharoni
{
	DMAC_CRITICAL_SECTION_ENTER();
	_descriptor_section[channel].BTCTRL.reg = src;
	DMAC_CRITICAL_SECTION_LEAVE();
	return ERR_NONE;
}

int32_t dma_set_DESCADDR(const uint8_t channel, uint32_t src)
// Added by DAharoni
{
	DMAC_CRITICAL_SECTION_ENTER();
	_descriptor_section[channel].DESCADDR.reg = src;
	DMAC_CRITICAL_SECTION_LEAVE();
	return ERR_NONE;
}

uint32_t dma_get_DESCADDR(const uint8_t channel)
// Added by DAharoni
{
	return _write_back_section[channel].DESCADDR.reg;
}

uint16_t dma_get_WRB_data(uint8_t channel)
// Added by DAharoni to get beat transfer count after disabling DMA transfer
{
	return _write_back_section[channel].BTCNT.reg;
}
