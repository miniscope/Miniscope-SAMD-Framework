/*
 * MS_config.h
 *
 * Created: 8/23/2023 5:18:12 PM
 *  Author: Takuya
 */ 


#ifndef MS_CONFIG_H_
#define MS_CONFIG_H_


// ------ DATA MODE ------------------------
//#define DEV_MODE

#ifdef DEV_MODE
#define TEST_PATTERN_ENABLE // For dev
#define TEST_BUFFER_ENABLE
#endif


// ------ HARDWARE MODE ------------------------
//#define V4WF_MODE
#define WLMS_SPI_MODE
//#define WLMS_USART_MODE
//#define WLMS_SD_MODE
//#define DMA_TO_SPI_TESTMODE
//#define DMA_TO_SPI_METRO_TESTMODE

// Peripheral enable based on mode
// PYTHON480_ENABLE: enables python480 to dataBuffer storage via PCC/DMA
// DMA_TO_SD_ENABLE: enables DMA to SD card ADMA
// DMA_TO_SPI_ENABLE: enables DMA to SPI via DMA

// ------ HARDWARE PERIPHERAL ENABLE ------------------------
#ifdef WLMS_SPI_MODE
#define TEST_PATTERN_ENABLE // For dev
#define PYTHON480_ENABLE
#define DMA_TO_SPI_ENABLE
#define EXLED_PWM_ENABLE
#define BATTERY_ENABLE
#define EWL_ENABLE
#define PUSH_BUT_ENABLE
#define STATUS_LED_ENABLE
#define IR_UART_ENABLE
#define IR_TRIGGER_ENABLE
#define SPI_SERCOM0_ENABLE
#define SDO_32BIT_ENABLE
#define PREAMBLE_ENABLE
#define PRESET_HEADER_ENABLE
#define AUTOSTART_ENABLE
#define FRAMERATE_1FPS
//#define FRAMERATE_5FPS
//#define FRAMERATE_10FPS
//#define FRAMERATE_20FPS
#endif

#ifdef WLMS_USART_MODE
#define PYTHON480_ENABLE
#define DMA_TO_USART_ENABLE
#define EXLED_PWM_ENABLE
#define BATTERY_ENABLE
//#define WPT_ADC_ENABLE
#define EWL_ENABLE
#define PUSH_BUT_ENABLE
#define STATUS_LED_ENABLE
#define IR_UART_ENABLE
#define USART_SERCOM5_ENABLE
#define SDO_8BIT_ENABLE
#define PREAMBLE_ENABLE
#define PRESET_HEADER_ENABLE
#define AUTOSTART_ENABLE
#define FRAMERATE_1FPS
#define DEBUGLED_ENABLE
#endif

#ifdef WLMS_SD_MODE
#define PYTHON480_ENABLE
#define DMA_TO_SD_ENABLE
#define EXLED_PWM_ENABLE
#define BATTERY_ENABLE
//#define WPT_ADC_ENABLE
#define EWL_ENABLE
#define PUSH_BUT_ENABLE
#define STATUS_LED_ENABLE
//#define IR_UART_ENABLE
//#define IR_TRIGGER_ENABLE
#define ADMA_ENABLE
#define STOP_ENABLE
#define FRAMERATE_20FPS
//#define PRESET_HEADER_ENABLE
//#define DEBUGLED_ENABLE
#define AUTOSTART_ENABLE
//#define RECORDTIME_DISABLE
#endif

#ifdef DMA_TO_SPI_TESTMODE
#define DMA_TO_SPI_ENABLE
#define EXLED_PWM_ENABLE
#define BATTERY_ENABLE
#define WPT_ADC_ENABLE
#define EWL_ENABLE
#define PUSH_BUT_ENABLE
#define STATUS_LED_ENABLE
#define IR_UART_ENABLE
#define IR_TRIGGER_ENABLE
#define SPI_SERCOM0_ENABLE
#define ADMA_ENABLE
#define SPI_LUT_ENABLE
#define SDO_32BIT_ENABLE
#define PREAMBLE_ENABLE
#define PRESET_HEADER_ENABLE
#endif

#ifdef DMA_TO_SPI_METRO_TESTMODE
#define DMA_TO_SPI_ENABLE
#define HEADER_DISABLE
#define	TEST_BUFFER_ENABLE
#define SPI_SERCOM0_ENABLE
#endif




#endif /* MS_CONFIG_H_ */