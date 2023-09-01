/*
 * MS_definitions.h
 *
 * Created: 5/20/2023 1:39:43 PM
 *  Author: Takuya
 */ 
#ifndef MS_DEFINITIONS_H_
#define MS_DEFINITIONS_H_

#include <atmel_start.h>
#include <utils.h>

#include "MS_config.h"

// SPI
#define SPI_ICSPACE_MS				0 // Clock cycle between word
#define SPI_BAUD_MS					0 // f_baud = f_ref / (2*(BAUD + 1))

// USART
#define USART_ICSPACE_MS				1 // Clock cycle between word
#define USART_BAUD_MS					23 // f_baud = f_ref / (2*(BAUD + 1))

// Peripheral address
#define EWL_I2C_ADDR					0x23  //7 bit address!


// ------ OTHER STUFF ------------------------
#define MS_SUCCESS						1
#define MS_ERROR						0
// -------------------------------------------

// ---------- Device State Definitions -------
#define DEVICE_STATE_IDLE				1<<1
#define DEVICE_STATE_START_RECORDING	1<<2
#define DEVICE_STATE_RECORDING			1<<3
#define DEVICE_STATE_STOP_RECORDING		1<<4
#define DEVICE_STATE_CHARGING			1<<5
#define DEVICE_STATE_CONFIG_LOADED		1<<6
#define DEVICE_STATE_ERROR				1<<7
#define DEVICE_STATE_LOW_VOLTAGE		1<<8
#define DEVICE_STATE_START_RECORDING_WAITING	1<<9
#define DEVICE_STATE_SDCARD_WRITE_ERROR			1<<10
#define DEVICE_STATE_SDCARD_INIT_WRITE_ERROR	1<<11
// -------------------------------------------

// ----------- Buffer Definitions ------------
#define BUFFER_BLOCK_LENGTH		40 // can be edited by user to optimize speed
#define NUM_BUFFERS				8  // can be edited by user to optimize speed
#define PCC_BLOCK_SIZE_IN_WORDS		128
#ifdef SDO_32BIT_ENABLE
#define SDO_BLOCK_SIZE_IN_WORDS		128 // (512 bytes) / (4 byte word size)
#endif
#ifdef SDO_8BIT_ENABLE
#define SDO_BLOCK_SIZE_IN_WORDS		512 // (512 bytes) / (1 byte word size)
#endif

// Buffer Header position definitions
#define BUFFER_HEADER_LENGTH					11

#define BUFFER_HEADER_HEADER_LENGTH_POS			0
#define BUFFER_HEADER_LINKED_LIST_POS			1
#define BUFFER_HEADER_FRAME_NUM_POS				2
#define BUFFER_HEADER_BUFFER_COUNT_POS			3
#define BUFFER_HEADER_FRAME_BUFFER_COUNT_POS	4
#define BUFFER_HEADER_WRITE_BUFFER_COUNT_POS	5
#define BUFFER_HEADER_DROPPED_BUFFER_COUNT_POS	6
#define BUFFER_HEADER_TIMESTAMP_POS				7
#define BUFFER_HEADER_DATA_LENGTH_POS			8
#define BUFFER_HEADER_BATTERY_VOLTAGE_POS		9
#define BUFFER_HEADER_WRITE_TIMESTAMP_POS		10

// -------------------------------------------

// ------------ Config Block Definitions -----
#define CONFIG_BLOCK_WIDTH_POS					0
#define CONFIG_BLOCK_HEIGHT_POS					1
#define CONFIG_BLOCK_FRAME_RATE_POS				2
#define CONFIG_BLOCK_BUFFER_SIZE_POS			3
#define CONFIG_BLOCK_NUM_BUFFERS_RECORDED_POS	4
#define CONFIG_BLOCK_NUM_BUFFERS_DROPPED_POS	5
// -------------------------------------------

// ------------ ADC channel Definitions -----
#define ADC_CHANNEL_BATTERY						0
#define ADC_CHANNEL_POWERBUS					2
// -------------------------------------------

// -------------------------------------------
// -------------- SD Definitions -------------
#define STARTING_BLOCK				1024
// This is only used in non-DMA operation
#define NB_BUFFER_WRITES_PER_CHUNK	20 // Can be edited by user to optimize speed

//#define HEADER_BLOCK				STARTING_BLOCK - 2  //1022
#define HEADER_BLOCK				1022  //1022
#define CONFIG_BLOCK				STARTING_BLOCK - 1  //1023

#define SD_BLOCK_SIZE					512 //Number of bytes in a single block (sector)

#define SD_DESCRIPTOR_ATT_VALID			1
#define SD_DESCRIPTOR_ATT_END			1<<1
#define SD_DESCRIPTOR_ATT_Int			1<<2
#define SD_DESCRIPTOR_LENGTH(val)		(val<<16)
#define SD_DESCRIPTOR_ATT_TRANSFER		(1<<5)
#define SD_DESCRIPTOR_ATT_LINK			(3<<4)

// Write Key Protection Values
#define WRITE_KEY0					0x0D7CBA17
#define WRITE_KEY1					0x0D7CBA17
#define WRITE_KEY2					0x0D7CBA17
#define WRITE_KEY3					0x0D7CBA17

// SD Header Definitions
#define HEADER_GAIN_POS				4
#define HEADER_LED_POS				5
#define HEADER_EWL_POS				6
#define HEADER_RECORD_LENGTH_POS	7
#define HEADER_FRAME_RATE_POS		8
#define HEADER_DELAY_START_POS		9
#define HEADER_BATT_CUTOFF_POS		10
// -------------------------------------------

// ADC Level Definition
#define ADC_BATTERY_LOW		177 // 3.8 V (x5 attenuation, 8-bit)
#define ADC_BATTERY_HIGH	191 // 4.1 V (x5 attenuation, 8-bit)
#define ADC_WPT_LOW			89 // 4.2 V (x11 attenuation, 8-bit)
#define ADC_WPT_HIGH		169 // 8 V (x11 attenuation, 8-bit)

// ------- Image Sensor Definitions ----------
// This should be defined from SD card header. Temporary
#ifdef FRAMERATE_20FPS
#define FRAME_RATE					20 // 1, 5, 10, 20, 0: 0.5 FPS
#elif defined(FRAMERATE_1FPS)
#define FRAME_RATE					1 // 1, 5, 10, 20, 0: 0.5 FPS
#endif

#define WIDTH						608
#define HEIGHT						608
#define BINNING						2

#define NUM_PIXELS					((WIDTH * HEIGHT) / (BINNING * BINNING))

// -------------------------------------------


// ----------- GLOBAL VARIABLES -----------


extern volatile uint32_t dataBuffer[][BUFFER_BLOCK_LENGTH * PCC_BLOCK_SIZE_IN_WORDS]; //Allocate memory for DMA image buffers
extern volatile DmacDescriptor PCCLinkedList[];
extern volatile DmacDescriptor TXLinkedList[];

extern volatile uint8_t headerBlock[]; // Will hold the 512 bytes from the header block of sd card
extern volatile uint8_t configBlock[]; // Will hold the device config information to be written to the starting block
extern volatile uint32_t currentBlock;
extern volatile uint32_t initBlocksRemaining;

extern volatile uint32_t deviceState;
extern volatile uint8_t battVolt;
extern volatile uint8_t wptVolt;
extern volatile uint32_t startTimeMS;
extern volatile uint32_t timeMS;
extern volatile uint32_t frameBufferCount;
extern volatile uint32_t frameNum;
extern volatile uint32_t bufferCount;
extern volatile uint32_t frameBufferCount;

// used for tracking recording and inc. DMA buffers
extern volatile uint32_t writeFrameNum;
extern volatile uint32_t writeBufferCount;
extern volatile uint32_t droppedBufferCount;
extern volatile uint32_t droppedFrameCount;
extern volatile uint32_t framesToDrop;
extern volatile uint32_t *bufferToWrite;
extern volatile uint32_t numBlocks;
extern volatile uint32_t numBuffersPerFrame;

// Debugging and checking stuff
extern volatile uint16_t chip_id; // Reads the chip id from Python480 to make sure we can talk to it
extern volatile uint32_t ewlvalue;
extern volatile uint32_t batteryvalue;
extern volatile uint32_t ledvalue;
extern volatile uint32_t frameratevalue;
extern volatile uint32_t delayvalue;
extern volatile uint32_t reclengthvalue;
extern volatile uint32_t sdo_dma_cb_calls;
extern volatile uint32_t pcc_dma_cb_calls;

extern volatile uint16_t regValue[];
extern volatile uint32_t tempPCC[];
extern volatile uint32_t tempHeader[][4];
extern volatile uint32_t tempCount;
extern volatile uint32_t tempTimestamp[];
extern volatile uint8_t timerIndex;

extern volatile uint16_t serialCommand;
extern volatile uint8_t uartBuffer;

extern volatile uint8_t headerBlock[]; // Will hold the 512 bytes from the header block of sd card
extern volatile uint8_t configBlock[]; // Will hold the device config information to be written to the starting block

// ----------- FUNCTIONS ----------------

void getBuffersPerFrame(void);
void peripheralInit(void);
void timerInit(void);
void irqInit(void);
uint8_t loadSDCardHeader(void);
uint32_t getPropFromHeader(uint8_t headerPos);
void setConfigBlockProp(uint8_t position, uint32_t value);
void setBufferHeader(uint32_t dataWordLength);

void setSDDescriptor(uint32_t *address, uint16_t length, uint8_t attribute);
volatile uint64_t SDTransferDescriptor; // I think we will only use 1 of these for now. Each descriptor is 64bits long


void imageSensorInit(void);
void SDCardInit(void);
void dmaEnable(void);
void setPCCCurrentLinkedListPosition(uint8_t pos);
void debugHeaderProp(void);

void setExcitationLED(uint32_t value, bool enable);
void setEWL(uint32_t value);
void setStatusLED(bool value);

void startRecording(void);
void stopRecording(void);
void recording_cb(const struct timer_task *const timer_task);
uint32_t getCurrentTimeMS(void);
void resetGlobalVar(void);

// callbacks
void millisecondTimer_cb(const struct timer_task *const timer_task);
void checkBattVoltage_cb(const struct timer_task *const timer_task);
void sdo_dma_transfer_complete_cb(void);
void battCharging_cb(void);
void irReceive_cb(void);
void pushButton_cb(void);
void frameValid_cb(void);
void pcc_dma_cb(struct camera_async_descriptor *const descr, uint32_t ch);
void usart_rx_cb(void);
// --------------------------------------

// DMA
#define SDO_DMA_CHANNEL 0x1
extern volatile DmacDescriptor PCCLinkedList[];
extern volatile DmacDescriptor TXLinkedList[];

extern void sdo_dma_transfer_trigger(void);
extern void sdo_dma_transfer_resume(void);
extern void sdo_dma_transfer_suspend(void);
extern void sdo_dma_irq_setup(void);
void setTXLinkedListPosition(uint8_t pos);
void setPCCLinkedListPosition(uint8_t pos);
void TXLinkedListInit(void);
void PCCLinkedListInit(void);
void sdo_dma_transfer_control(bool callback_flag);
void sdmmc_dma_transfer_control(void);
void sdo_dma_setup(void);
void DataBufferInit(void);

#endif /* MS_DEFINITIONS_H_ */