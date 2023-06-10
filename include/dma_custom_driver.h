/*
 * MS_custom_driver.h
 *
 * Created: 5/22/2023 6:56:33 PM
 *  Author: Takuya
 */ 


#ifndef MS_CUSTOM_DRIVER_H_
#define MS_CUSTOM_DRIVER_H_

#include <utils.h>

// probably should move to hpl_dmac.hsrc
int32_t _dma_set_BTCTRL(const uint8_t channel, uint32_t src);
uint32_t _dma_get_DESCADDR(const uint8_t channel);

#endif /* MS_CUSTOM_DRIVER_H_ */