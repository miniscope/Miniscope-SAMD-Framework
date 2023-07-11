/*
 * NanEye.c
 *
 * Created: 7/7/2023 3:17:10 PM
 *  Author: HSemwal
 */ 

#include "GS_definitions.h"
#include <utils.h>
#include <atmel_start.h>

// send data 
void NanEyeInit(uint32_t data)
{
	SERCOM4->SPI.DATA.reg = data; 
}