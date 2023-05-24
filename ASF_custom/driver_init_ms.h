/*
 * driver_init_ms.h
 *
 * Created: 5/23/2023 6:22:39 PM
 *  Author: Takuya
 */ 


#ifndef DRIVER_INIT_MS_H_
#define DRIVER_INIT_MS_H_

extern struct mci_sync_desc IO_BUS;

void IO_BUS_PORT_init(void);
void IO_BUS_CLOCK_init(void);
void IO_BUS_init(void);

#endif /* DRIVER_INIT_MS_H_ */