/*
 * MS_dma.c
 *
 * Created: 5/20/2023 3:47:16 PM
 *  Author: Takuya
 */ 

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

	#ifdef DMA_TO_SPI_ENABLE
	_dma_enable_transaction(SPI_DMA_CHANNEL, false);
	#endif
}

void TXLinkedListInit(void)
{
	for (uint8_t i = 0; i < NUM_BUFFERS; i++) {
		if (i == (NUM_BUFFERS - 1)) TXLinkedList[i].DESCADDR.reg = (uint32_t)&TXLinkedList[0];
		// Last buffer in list. Need to loop back
		else TXLinkedList[i].DESCADDR.reg = (uint32_t)&TXLinkedList[i + 1];
		
		TXLinkedList[i].BTCNT.reg = (BUFFER_BLOCK_LENGTH * BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH);

		// We aren't actually using the STEPSIZE part of incrementing the destination address.
		TXLinkedList[i].BTCTRL.reg = DMAC_BTCTRL_STEPSIZE(0) | (CONF_DMAC_STEPSEL_1 << DMAC_BTCTRL_STEPSEL_Pos)\
		| (CONF_DMAC_DSTINC_1 << DMAC_BTCTRL_DSTINC_Pos) | (CONF_DMAC_SRCINC_1 << DMAC_BTCTRL_SRCINC_Pos)\
		| DMAC_BTCTRL_BEATSIZE(CONF_DMAC_BEATSIZE_1) | DMAC_BTCTRL_BLOCKACT(CONF_DMAC_BLOCKACT_1 | 0x01)\
		| DMAC_BTCTRL_EVOSEL(CONF_DMAC_EVOSEL_1) | DMAC_BTCTRL_VALID;
		
		
		//For sending out data
		TXLinkedList[i].SRCADDR.reg = (uint32_t)(&dataBuffer[i][BUFFER_HEADER_LENGTH]) + TXLinkedList[i].BTCNT.reg * 4;
		
		//ignore buffer
		//TXLinkedList[i].SRCADDR.reg = (uint32_t)(&dataBuffer[i][0]) + TXLinkedList[i].BTCNT.reg * 4;		
		// Destination address when incrementing address needs to be the end address and not the start address.
		// I think the last scale multiplication needs to be either 3 or 5 but _dma_set_data_amount() uses a 4.

		#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM0_ENABLE)
		TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM0->SPI.DATA.reg;
		#endif
		#if defined(DMA_TO_SPI_ENABLE) && defined(SPI_SERCOM7_ENABLE)
		TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM7->SPI.DATA.reg;
		#endif
		#ifdef DMA_TO_USART_ENABLE
		TXLinkedList[i].DSTADDR.reg = (uint32_t) &SERCOM5->USART.DATA.reg;
		#endif
	}
	setTXLinkedListPosition(0);
}

void DataBufferInit(void)
{
	for (uint32_t i = 0; i<NUM_BUFFERS; i++)
	{
		for (uint32_t j = 0; j<BUFFER_BLOCK_LENGTH * BLOCK_SIZE_IN_WORDS; j++)
		{
			#ifdef TEST_BUFFER_ENABLE
			dataBuffer[i][j] = 1*(j+1);
			#else
			dataBuffer[i][j] = 0;
			#endif
		}
	}
}

void setTXLinkedListPosition(uint8_t pos)
{
	_dma_set_source_address(SPI_DMA_CHANNEL, (void *)TXLinkedList[pos].SRCADDR.reg);
	_dma_set_destination_address(SPI_DMA_CHANNEL, (void *)TXLinkedList[pos].DSTADDR.reg);
	_dma_set_data_amount(SPI_DMA_CHANNEL, TXLinkedList[pos].BTCNT.reg);
	_dma_set_BTCTRL(SPI_DMA_CHANNEL, (void *)TXLinkedList[pos].BTCTRL.reg);//block transfer control
	_dma_set_source_address(SPI_DMA_CHANNEL, (void *)TXLinkedList[pos].SRCADDR.reg); // Overwrite source address since set_data_amount function modifies this
	
	_dma_set_DESCADDR(SPI_DMA_CHANNEL, TXLinkedList[pos].DESCADDR.reg);
}

void PCCLinkedListInit(void)
{
	for (uint8_t i = 0; i < NUM_BUFFERS; i++) {
		if (i == (NUM_BUFFERS - 1))
		// Last buffer in list. Need to loop back
		PCCLinkedList[i].DESCADDR.reg = (uint32_t)&PCCLinkedList[0];
		else
		PCCLinkedList[i].DESCADDR.reg = (uint32_t)&PCCLinkedList[i + 1];
		
		PCCLinkedList[i].BTCNT.reg = (BUFFER_BLOCK_LENGTH * BLOCK_SIZE_IN_WORDS - BUFFER_HEADER_LENGTH);
		// We aren't actually using the STEPSIZE part of incrementing the destination address.
		PCCLinkedList[i].BTCTRL.reg = DMAC_BTCTRL_STEPSIZE(0) | (CONF_DMAC_STEPSEL_0 << DMAC_BTCTRL_STEPSEL_Pos)						\
		| (CONF_DMAC_DSTINC_0 << DMAC_BTCTRL_DSTINC_Pos) | (CONF_DMAC_SRCINC_0 << DMAC_BTCTRL_SRCINC_Pos)	\
		| DMAC_BTCTRL_BEATSIZE(CONF_DMAC_BEATSIZE_0) | DMAC_BTCTRL_BLOCKACT(CONF_DMAC_BLOCKACT_0 | 0x01)            \
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
	_dma_set_destination_address(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].DSTADDR.reg);
	_dma_set_data_amount(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].BTCNT.reg);
	_dma_set_BTCTRL(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].BTCTRL.reg);
	_dma_set_destination_address(CONF_PCC_DMA_CHANNEL, (void *)PCCLinkedList[pos].DSTADDR.reg); // Overwrite destination address since set_data_amount function modifies this

	_dma_set_DESCADDR(CONF_PCC_DMA_CHANNEL, PCCLinkedList[pos].DESCADDR.reg);
}
#endif
