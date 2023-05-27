/*
 * MS_global_variable.c
 *
 * Created: 5/24/2023 10:51:24 AM
 *  Author: Takuya
 */ 

#include "MS_definitions.h"


COMPILER_ALIGNED(4)
volatile uint32_t dataBuffer[NUM_BUFFERS][BUFFER_BLOCK_LENGTH * BLOCK_SIZE_IN_WORDS]; //Allocate memory for DMA image buffers

COMPILER_ALIGNED(16)
volatile DmacDescriptor TXLinkedList[NUM_BUFFERS];

COMPILER_ALIGNED(16)
volatile DmacDescriptor PCCLinkedList[NUM_BUFFERS];



volatile uint8_t headerBlock[SD_BLOCK_SIZE] = {0}; // Will hold the 512 bytes from the header block of sd card
volatile uint8_t configBlock[SD_BLOCK_SIZE]; // Will hold the device config information to be written to the starting block
volatile uint32_t currentBlock = STARTING_BLOCK;
volatile uint32_t initBlocksRemaining;

volatile uint32_t deviceState = DEVICE_STATE_IDLE;
volatile uint8_t battVolt;

volatile uint32_t startTimeMS;
volatile uint32_t timeMS = 0;
volatile uint32_t frameBufferCount = 0;
volatile uint32_t frameNum = 0;
volatile uint32_t bufferCount = 0;
volatile uint32_t frameBufferCount;

// used for tracking recording and inc. DMA buffers
volatile uint32_t writeFrameNum;
volatile uint32_t writeBufferCount;
volatile uint32_t droppedBufferCount;
volatile uint32_t droppedFrameCount;
volatile uint32_t framesToDrop;
volatile uint32_t *bufferToWrite;
volatile uint32_t numBlocks = BUFFER_BLOCK_LENGTH;
volatile uint32_t numBuffersPerFrame = 0;

// Debugging and checking stuff
volatile uint16_t chip_id; // Reads the chip id from Python480 to make sure we can talk to it
volatile uint32_t ewlvalue;
volatile uint32_t batteryvalue;
volatile uint32_t ledvalue;
volatile uint32_t frameratevalue;
volatile uint32_t delayvalue;
volatile uint32_t reclengthvalue;

volatile uint16_t regValue[2];
volatile uint32_t tempPCC[4];
volatile uint32_t tempHeader[100][4];
volatile uint32_t tempCount = 0;
volatile uint32_t tempTimestamp[100];
volatile uint8_t timerIndex = 0;

struct timer_task TIMER_0_task1;
struct timer_task TIMER_0_task2;
