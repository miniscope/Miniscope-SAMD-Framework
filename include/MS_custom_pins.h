/*
 * MS_custom_pins.h
 *
 * Created: 5/23/2023 6:03:24 PM
 *  Author: Takuya
 */ 


#ifndef MS_CUSTOM_PINS_H_
#define MS_CUSTOM_PINS_H_

#define SDCMD GPIO(GPIO_PORTA, 8)
#define SDDAT0 GPIO(GPIO_PORTA, 9)
#define SDDAT1 GPIO(GPIO_PORTA, 10)
#define SDDAT2 GPIO(GPIO_PORTA, 11)
#define CARD_DETECT_CUSTOM GPIO(GPIO_PORTB, 5)
#define SDDAT3 GPIO(GPIO_PORTB, 10)
#define SDCK GPIO(GPIO_PORTB, 11)

#endif /* MS_CUSTOM_PINS_H_ */