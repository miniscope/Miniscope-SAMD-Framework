/*
 * MS_custom_driver.h
 *
 * Created: 5/22/2023 6:56:33 PM
 *  Author: Takuya
 */ 


#ifndef MS_CUSTOM_DRIVER_H_
#define MS_CUSTOM_DRIVER_H_

#include <utils.h>
#include <sd_mmc.h>
#include <sd_mmc_protocol.h>

int32_t _dma_set_BTCTRL(const uint8_t channel, uint32_t src);
int32_t _dma_set_DESCADDR(const uint8_t channel, uint32_t src);
uint32_t _dma_get_DESCADDR(const uint8_t channel);
uint16_t _dma_get_WRB_data(uint8_t channel);

#ifdef DMA_TO_SD_ENABLE
// Added by DAharoni.
// This should handling 1 multiblock ADMA transfer
sd_mmc_err_t sd_mmc_write_with_ADMA(uint8_t slot, uint32_t start, uint32_t *descAdd, uint16_t nb_block);

//Added by DAharoni
sd_mmc_err_t sd_mmc_wait_end_of_ADMA_write(bool abort);
#endif /* DMA_TO_SD_ENABLE */




#endif /* MS_CUSTOM_DRIVER_H_ */