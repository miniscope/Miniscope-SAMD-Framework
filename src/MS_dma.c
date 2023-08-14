/*
 * MS_dma.c
 *
 * Created: 5/20/2023 3:47:16 PM
 *  Author: Takuya
 */ 

#include <hpl_dmac_config.h>
#include <hpl_pcc_config.h>
#include <hpl_dma.h>
#include "MS_definitions.h"
#include "dma_custom_driver.h"

COMPILER_ALIGNED(16)
volatile DmacDescriptor TXLinkedList[NUM_BUFFERS];

COMPILER_ALIGNED(16)
volatile DmacDescriptor PCCLinkedList[NUM_BUFFERS];

void dmaEnable(void){
	#ifdef PYTHON480_ENABLE
	// Enables DMA Transfer complete interrupt. Should be put in better place
	DMAC->Channel[CONF_PCC_DMA_CHANNEL].CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
	
	// Sets the callback for when each DMA buffer is full
	camera_async_register_callback(&CAMERA_0, pcc_dma_cb);

	// This should already be done in init but trying here as well
	PCC->MR.reg = PCC_MR_CID(0x3) | PCC_MR_ISIZE(CONF_PCC_ISIZE) | CONF_PCC_FRSTS << PCC_MR_FRSTS_Pos
	| CONF_PCC_HALFS << PCC_MR_HALFS_Pos | CONF_PCC_ALWYS << PCC_MR_ALWYS_Pos
	| CONF_PCC_SCALE << PCC_MR_SCALE_Pos | PCC_MR_DSIZE(CONF_PCC_DSIZE);
	#endif

	#if defined(DMA_TO_SPI_ENABLE) || defined(DMA_TO_USART_ENABLE)
	//NVIC_SetPriority(DMAC_1_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for DMAC Channel 1
	//NVIC_EnableIRQ(DMAC_1_IRQn);         // Connect DMAC Channel 1 to Nested Vector Interrupt Controller (NVIC)
	DMAC->Channel[SDO_DMA_CHANNEL].CHINTENSET.reg = DMAC_CHINTENSET_TCMPL;
	//DMAC->Channel[SDO_DMA_CHANNEL].CHINTENSET.reg = DMAC_CHINTENSET_SUSP;
	//DMAC->Channel[SDO_DMA_CHANNEL].CHINTENSET.reg = DMAC_CHINTENSET_TERR;
	//DMAC->Channel[SDO_DMA_CHANNEL].CHINTENCLR.reg = 0;                    // Activate the transfer complete (TCMPL) interrupt on DMAC channel 0
	//DMAC->Channel[SDO_DMA_CHANNEL].CHPRILVL.reg = DMAC_CHPRILVL_PRILVL_LVL0;
	dmac_register_callback(SDO_DMA_CHANNEL, sdo_dma_transfer_complete_cb);
	#endif
}

#if defined(DMA_TO_SPI_ENABLE) || defined(DMA_TO_USART_ENABLE)
void TXLinkedListInit(void)
{
	for (uint8_t i = 0; i < NUM_BUFFERS; i++) {
		if (i == (NUM_BUFFERS - 1)) TXLinkedList[i].DESCADDR.reg = (uint32_t)&TXLinkedList[0];
		// Last buffer in list. Need to loop back
		else TXLinkedList[i].DESCADDR.reg = (uint32_t)&TXLinkedList[i + 1];
		
		
		TXLinkedList[i].BTCNT.reg = BUFFER_BLOCK_LENGTH * SDO_BLOCK_SIZE_IN_WORDS;

		// We aren't actually using the STEPSIZE part of incrementing the source address.
		TXLinkedList[i].BTCTRL.reg = DMAC_BTCTRL_STEPSIZE(0) | (CONF_DMAC_STEPSEL_1 << DMAC_BTCTRL_STEPSEL_Pos)\
		| (CONF_DMAC_DSTINC_1 << DMAC_BTCTRL_DSTINC_Pos) | (CONF_DMAC_SRCINC_1 << DMAC_BTCTRL_SRCINC_Pos)\
		| DMAC_BTCTRL_BEATSIZE(CONF_DMAC_BEATSIZE_1) | DMAC_BTCTRL_BLOCKACT(CONF_DMAC_BLOCKACT_1 | 0x01)\
		| DMAC_BTCTRL_EVOSEL(CONF_DMAC_EVOSEL_1) | DMAC_BTCTRL_VALID;
		
		// For sending out data
		#ifdef SDO_32BIT_ENABLE
		TXLinkedList[i].SRCADDR.reg = (uint32_t)(&dataBuffer[i][0]) + TXLinkedList[i].BTCNT.reg * 4;
		#endif
		#ifdef SDO_8BIT_ENABLE
		TXLinkedList[i].SRCADDR.reg = (uint32_t)(&dataBuffer[i][0]) + TXLinkedList[i].BTCNT.reg;
		#endif
		// Destination address when incrementing address needs to be the end address and not the start address.
		// I think the last scale multiplication needs to be either 3 or 5 but _dma_set_data_amount() uses a 4.
		
		#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
		TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM0->SPI.DATA.reg;
		#endif
		#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM7_ENABLE)
		TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM7->SPI.DATA.reg;
		#endif
		#if defined(DMA_TO_USART_ENABLE) && defined(USART_SERCOM5_ENABLE)
		TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM5->USART.DATA.reg;
		#endif
	}
	setTXLinkedListPosition(0);
}

void setTXLinkedListPosition(uint8_t pos)
{
	_dma_set_destination_address(SDO_DMA_CHANNEL, (void *)TXLinkedList[pos].DSTADDR.reg);
	_dma_set_data_amount(SDO_DMA_CHANNEL, TXLinkedList[pos].BTCNT.reg);
	_dma_set_BTCTRL(SDO_DMA_CHANNEL, (void *)TXLinkedList[pos].BTCTRL.reg); //block transfer control
	_dma_set_DESCADDR(SDO_DMA_CHANNEL, TXLinkedList[pos].DESCADDR.reg);
	_dma_set_source_address(SDO_DMA_CHANNEL, (void *)TXLinkedList[pos].SRCADDR.reg); // Overwrite source address since set_data_amount function modifies this
}

void sdo_dma_transfer_trigger(void)
{
	DMAC->SWTRIGCTRL.reg = 0x2;
}

void sdo_dma_transfer_complete_cb(void)
{
	// call count for debug
	sdo_dma_cb_calls++;
	
	//increment if appropriate
	sdo_dma_transfer_control(true);
}

void sdo_dma_transfer_control(bool callback_flag) // flag if called via callback
{
	// for first call; if not enabled
	if (DMAC->Channel[SDO_DMA_CHANNEL].CHCTRLA.bit.ENABLE == 0)
	{
		_dma_enable_transaction(SDO_DMA_CHANNEL, false);
		writeBufferCount++; // not sure if this should be counted
		return;
	}

	#ifdef PYTHON480_ENABLE
	// send out pending bits and return
	if(DMAC->Channel[SDO_DMA_CHANNEL].CHSTATUS.bit.PEND == 1){
		sdo_dma_transfer_resume();
		return;
	}

	// if coming from TRCMP callback and buffer is left just resume
	if(callback_flag == 1 && bufferCount - (writeBufferCount + droppedBufferCount) > 0){
		sdo_dma_transfer_resume();
		return;
	}

	//end if middle of transfer
	if(DMAC->Channel[SDO_DMA_CHANNEL].CHSTATUS.bit.BUSY == 1){
		return;
	}

	#else //ifdef PYTHON480_ENABLE just send out bit
	if(DMAC->Channel[SDO_DMA_CHANNEL].CHCTRLA.bit.ENABLE == 0)
	{
		_dma_enable_transaction(SDO_DMA_CHANNEL, false);
	}
	sdo_dma_transfer_resume();
	writeBufferCount++;
	#endif
}
void sdo_dma_transfer_resume(void)
{
	writeBufferCount++;
	DMAC->Channel[SDO_DMA_CHANNEL].CHCTRLB.reg = 0x2;
	//sdo_dma_transfer_trigger(); // SERCOM 5 is triggering so not necessary
}

void sdo_dma_transfer_suspend(void)
{
	DMAC->Channel[SDO_DMA_CHANNEL].CHCTRLB.reg = 0x1;
}
#endif

void DataBufferInit(void)
{
	for (uint32_t i = 0; i<NUM_BUFFERS; i++)
	{
		dataBuffer[i][0] = 0x12345678;
		for (uint32_t j = 1; j<BUFFER_BLOCK_LENGTH * PCC_BLOCK_SIZE_IN_WORDS; j++)
		{
			#ifdef TEST_BUFFER_ENABLE // hard coding test buffers for 304 * 304 px. Should be a defined better.
			dataBuffer[i][BUFFER_HEADER_LINKED_LIST_POS] = i;
			dataBuffer[i][BUFFER_HEADER_FRAME_NUM_POS] = 1;
			if (i < 5){
				dataBuffer[i][BUFFER_HEADER_FRAME_BUFFER_COUNT_POS] = i;
			}
			if (i < 4) dataBuffer[i][BUFFER_HEADER_DATA_LENGTH_POS] = 20480;
			if (i == 4) dataBuffer[i][BUFFER_HEADER_DATA_LENGTH_POS] = 10496;
			if (j >= 9){
			dataBuffer[i][j] = ((i+1) * (j-9)*4) % 0x100 * 0x01000000 + ((i+1) * ((j-9)*4+1)) % 0x100 * 0x00010000 + ((i+1) * ((j-9)*4+2)) % 0x100 * 0x00000100 + ((i+1) * ((j-9)*4+3)) % 0x100 * 0x00000001;
			}
			#else
			dataBuffer[i][j] = 0;
			#endif
		}
	}
}

void PCCLinkedListInit(void)
{
	for (uint8_t i = 0; i < NUM_BUFFERS; i++) {
		if (i == (NUM_BUFFERS - 1))
		// Last buffer in list. Need to loop back
		PCCLinkedList[i].DESCADDR.reg = (uint32_t)&PCCLinkedList[0];
		else
		PCCLinkedList[i].DESCADDR.reg = (uint32_t)&PCCLinkedList[i + 1];
		
		PCCLinkedList[i].BTCNT.reg = (BUFFER_BLOCK_LENGTH * PCC_BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH);
		// We aren't actually using the STEPSIZE part of incrementing the destination address.
		PCCLinkedList[i].BTCTRL.reg = DMAC_BTCTRL_STEPSIZE(0) | (CONF_DMAC_STEPSEL_0 << DMAC_BTCTRL_STEPSEL_Pos)\
		| (CONF_DMAC_DSTINC_0 << DMAC_BTCTRL_DSTINC_Pos) | (CONF_DMAC_SRCINC_0 << DMAC_BTCTRL_SRCINC_Pos)\
		| DMAC_BTCTRL_BEATSIZE(CONF_DMAC_BEATSIZE_0) | DMAC_BTCTRL_BLOCKACT(CONF_DMAC_BLOCKACT_0 | 0x01)\
		| DMAC_BTCTRL_EVOSEL(CONF_DMAC_EVOSEL_0) | DMAC_BTCTRL_VALID;
		
		PCCLinkedList[i].SRCADDR.reg = (uint32_t)(&PCC->RHR.reg); //(void *)&(((Pcc *)device->hw)->RHR.reg)
		
		// Destination address when incrementing address needs to be the end address and not the start address.
		// I think the last scale multiplication needs to be either 3 or 5 but _dma_set_data_amount() uses a 4.
		PCCLinkedList[i].DSTADDR.reg = (uint32_t)(&dataBuffer[i][BUFFER_HEADER_LENGTH]) + PCCLinkedList[i].BTCNT.reg * 4;
	}
	setPCCLinkedListPosition(0);
}

#ifdef PYTHON480_ENABLE
void setPCCLinkedListPosition(uint8_t pos)
{
	// Set up initial DMA descriptor for DMA channel handling PCC. BTCNT is already setup in DMA init step
	_dma_set_source_address(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].SRCADDR.reg);
	//_dma_set_destination_address(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].DSTADDR.reg);
	_dma_set_data_amount(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].BTCNT.reg);
	_dma_set_BTCTRL(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].BTCTRL.reg);
	_dma_set_destination_address(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].DSTADDR.reg); // Overwrite destination address since set_data_amount function modifies this

	_dma_set_DESCADDR(CONF_PCC_DMA_CHANNEL, PCCLinkedList[pos].DESCADDR.reg);
}
#endif
