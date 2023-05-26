/*
 * hpl_sdhc_ms.h
 *
 * Created: 5/24/2023 12:00:38 PM
 *  Author: Takuya
 */ 


#ifndef HPL_SDHC_MS_H_
#define HPL_SDHC_MS_H_

extern bool _mci_send_cmd_execute(const void *const hw, uint32_t cmdr, uint32_t cmd, uint32_t arg);

#endif /* HPL_SDHC_MS_H_ */