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

//Added by DAharoni
bool mci_send_cmd_execute(Sdhc *mci, uint32_t cmdr, uint32_t cmd, uint32_t arg)
{
	return _mci_send_cmd_execute(mci, cmdr, cmd, arg);
}

#ifdef DMA_TO_SD_ENABLE
sd_mmc_err_t sd_mmc_write_with_ADMA(uint8_t slot, uint32_t start, uint32_t *descAdd, uint16_t nb_block)
// Added by DAharoni
{
	sd_mmc_err_t sd_mmc_err;
	uint32_t     cmd, arg, resp, tmr;
	
	sd_mmc_err = sd_mmc_select_slot(slot);
	if (sd_mmc_err != SD_MMC_OK) {
		return sd_mmc_err;
	}
	if (sd_mmc_is_write_protected(slot)) {
		sd_mmc_deselect_slot();
		return SD_MMC_ERR_WP;
	}

	// Figures out initial command value
	if (nb_block > 1) {
		cmd = SDMMC_CMD25_WRITE_MULTIPLE_BLOCK;
	} else {
		cmd = SDMMC_CMD24_WRITE_BLOCK;
	}
	
	/*
	 * SDSC Card (CCS=0) uses byte unit address,
	 * SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
	 */
	if (sd_mmc_card->type & CARD_TYPE_HC) {
		arg = start;
	} else {
		arg = (start * SD_MMC_BLOCK_SIZE);
	}
	
	// Sets ADMA System address register
	SDHC0->ASAR[0].reg = (uint32_t)descAdd;
	//hri_sdhc_write_ASAR_reg(SDHC0, 0, descAdd); // ASAR is an array but only has 1 entry (0)
	// Set block size which should always be 512 for memory
	hri_sdhc_write_BSR_reg(SDHC0, SDHC_BSR_BLOCKSIZE(SD_MMC_BLOCK_SIZE) | SDHC_BSR_BOUNDARY_4K);
	// Set block count
	hri_sdhc_write_BCR_reg(SDHC0, SDHC_BCR_BCNT(nb_block));
	
	/* Check Command Inhibit (CMD/DAT) in the Present State register */
	if (hri_sdhc_get_PSR_CMDINHC_bit(SDHC0) || hri_sdhc_get_PSR_CMDINHD_bit(SDHC0)) {
		return false;
	}	
	
	// Figures out tmr and based on cmd
	if (cmd & MCI_CMD_WRITE) {
		tmr = SDHC_TMR_DTDSEL_WRITE;
	} else {
		tmr = SDHC_TMR_DTDSEL_READ;
	}

	if (cmd & MCI_CMD_SDIO_BYTE) {
		tmr |= SDHC_TMR_MSBSEL_SINGLE;
		} else if (cmd & MCI_CMD_SDIO_BLOCK) {
		tmr |= SDHC_TMR_BCEN | SDHC_TMR_MSBSEL_MULTIPLE;
		} else if (cmd & MCI_CMD_SINGLE_BLOCK) {
		tmr |= SDHC_TMR_MSBSEL_SINGLE;
		} else if (cmd & MCI_CMD_MULTI_BLOCK) {
		tmr |= SDHC_TMR_BCEN | SDHC_TMR_MSBSEL_MULTIPLE;
		} else {
		return false;
	}
	
	// Sets TMR and make sure DMA is enabled. Not sure if we need to enable auto 12 and 23
	hri_sdhc_write_TMR_reg(SDHC0, tmr|SDHC_TMR_ACMDEN_CMD12|SDHC_TMR_DMAEN);
	//hri_sdhc_write_TMR_reg(SDHC0, tmr|SDHC_TMR_ACMDEN_CMD12|SDHC_TMR_ACMDEN_CMD23|SDHC_TMR_DMAEN);
	
	// This sets ARG1 along with setting up and sending command and waiting for command complete int
	if (mci_send_cmd_execute(SDHC0, SDHC_CR_DPSEL_DATA, cmd, arg) == false) {
		sd_mmc_deselect_slot();
		return SD_MMC_ERR_COMM;
	}
	
	// Get response
	/* Check response */
	resp = driver_get_response(sd_mmc_hal);
	if (resp & CARD_STATUS_ERR_RD_WR) {
		sd_mmc_deselect_slot();
		return SD_MMC_ERR_COMM;
	}
	
	return SD_MMC_OK;
}

// Added by DAharoni
sd_mmc_err_t sd_mmc_wait_end_of_ADMA_write(bool abort)
{
	// Currently we don't do anything with the "abort" option
	uint32_t sr;
	
	do {
		sr = hri_sdhc_read_EISTR_reg(SDHC0);

		if (sr & (SDHC_EISTR_DATTEO | SDHC_EISTR_DATCRC | SDHC_EISTR_DATEND)) {
			// TODO: We should reset here but I got lazy and didn't want to make mci_reset to access _mci_reset
			//_mci_reset(SDHC0);
			return false;
		}
	} while (!hri_sdhc_get_NISTR_TRFC_bit(SDHC0));
	hri_sdhc_set_NISTR_TRFC_bit(SDHC0);
	
	
	// If we don't do auto CMD12 then I think we need to do the following:
	
	//if (!driver_adtc_stop(sd_mmc_hal, SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
		//sd_mmc_deselect_slot();
		//return SD_MMC_ERR_COMM;
	//}
	
	sd_mmc_deselect_slot();
	
	return SD_MMC_OK;
}

#endif
