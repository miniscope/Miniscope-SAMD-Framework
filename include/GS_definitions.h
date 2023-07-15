/*
 * GS_definitions.h
 *
 * Created: 7/7/2023 3:19:28 PM
 *  Author: HSemwal
 */ 

// Adding NanEye configuration values here

#ifdef DMA_TO_SPI_GS_TESTMODE

// NanEyeC state machine
#define STATE_IDLE						1
#define STATE_INTERFACE					1<<0
#define STATE_SYNC_AND_DELAY			1<<1
#define STATE_READOUT					1<<2
// NanEyeC register values
#define NANEYE_REG0_DEFAULT_VALUE		0b1000000010010101
#define NANEYE_REG1_DEFAULT_VALUE		0b0000001101011010

#define NANEYE_LAST_INTERFACE_BYTE		0b000000010101

// DMA Transfer definitions, possibly also added in MS_definitions.h

#define INTERFACE_MODE_SIZE				972
#define SYNC_MODE_SIZE					984
#define READOUT_MODE_SIZE				984
#define READOUT_MODE_SIZE				157452


#define BUFFER_FRAME_SIZE				2// number of frames in the buffer

// here we are defining the SPI parameters for DMA Transfer
#define INTERFACE_MODE_TX_POS			0
#define INTERFACE_MODE_RX_POS			1
#define SYNC_AND_DELAY_MODE_TX_POS		2
#define SYNC_AND_DELAY_MODE_RX_POS		3
#define READOUT_MODE_TX_POS				4
#define READOUT_MODE_RX_POS				5


#define SPI_MASTER_BASE					SPI0
#define SPI_IRQn						SPI0_IRQN // what is this
#define SPI_ID							ID_SPI0

#define SPI_CHIP_SEL					0
#define SPI_CHIP_PCS					spi_get_pcs(SPI_CHIP_SEL)
#define SPI_DLYBS						0x0
#define SPI_DLYBCT						0x00
#define SPI_DLYBCS						0x00
#define SPI_CLK_RATE					16000000

// #define SPI_MISO_PIN					PB14
// #define SPI_MISO_MODE					IOPORT_MODE_MUX_B // this i dont understand
// #define SPI_MOSI_PIN					PB15
// #define SPI_MOSI_MODE					IOPORT_MODE_MUX_B
// #define SPI_SPCK_PIN					PB13
// #define SPI_SPCK_MODE					IOPORT_MODE_MUX_B
// #define SPI_NPCS_PIN					NOTSURE
// #define SPI_NPCS_MODE					IOPORT_MODE_MUX_D

void NanEyeInit(void);
// void startRecordingNE();
// void stopRecordingNE();

#endif


