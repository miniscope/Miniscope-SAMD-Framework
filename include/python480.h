/*
 * python480.h
 *
 * Created: 5/29/2021 1:33:16 PM
 *  Author: dbaha
 */ 


#ifndef PYTHON480_H_
#define PYTHON480_H_

#include <driver_init.h>
#include <utils.h>

#define DISABLE_PLL

void python480Init(void);
void python480SetGain(uint32_t value);
void python480SetFPS(uint32_t value);
void spi_BB_Write(uint16_t address, uint16_t value);
uint16_t spi_BB_Read(uint16_t address);
void EnableClockMngmnt1(void);
void EnableClockMngmnt2(void);
void RequiredUploads(void);
void SoftPowerUp(void);
void NoTransfer(void);
void Transfer(void);
void Eblack(void);
void Egray(void);
void Enable_Subsample(void);
void DisableE(void);
void EnableSeq (void);
void DisableSeq (void);

#endif /* PYTHON480_H_ */