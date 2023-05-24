/*
 * driver_init_ms.c
 *
 * Created: 5/23/2023 6:05:12 PM
 *  Author: Takuya
 */ 
#include <atmel_start_pins.h>
#include <MS_custom_peripheral_clock.h>
#include <MS_custom_pins.h>
#include "hal_mci_sync_ms.h"

struct mci_sync_desc IO_BUS;

void IO_BUS_PORT_init(void)
{

	gpio_set_pin_direction(SDCK,
	// <y> Pin direction
	// <id> pad_direction
	// <GPIO_DIRECTION_OFF"> Off
	// <GPIO_DIRECTION_IN"> In
	// <GPIO_DIRECTION_OUT"> Out
	GPIO_DIRECTION_OUT);

	gpio_set_pin_level(SDCK,
	// <y> Initial level
	// <id> pad_initial_level
	// <false"> Low
	// <true"> High
	false);

	gpio_set_pin_pull_mode(SDCK,
	// <y> Pull configuration
	// <id> pad_pull_config
	// <GPIO_PULL_OFF"> Off
	// <GPIO_PULL_UP"> Pull-up
	// <GPIO_PULL_DOWN"> Pull-down
	GPIO_PULL_OFF);

	gpio_set_pin_function(SDCK,
	// <y> Pin function
	// <id> pad_function
	// <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
	// <PINMUX_PB11I_SDHC0_SDCK"> Auto
	// <GPIO_PIN_FUNCTION_OFF"> Off
	// <GPIO_PIN_FUNCTION_A"> A
	// <GPIO_PIN_FUNCTION_B"> B
	// <GPIO_PIN_FUNCTION_C"> C
	// <GPIO_PIN_FUNCTION_D"> D
	// <GPIO_PIN_FUNCTION_E"> E
	// <GPIO_PIN_FUNCTION_F"> F
	// <GPIO_PIN_FUNCTION_G"> G
	// <GPIO_PIN_FUNCTION_H"> H
	// <GPIO_PIN_FUNCTION_I"> I
	// <GPIO_PIN_FUNCTION_J"> J
	// <GPIO_PIN_FUNCTION_K"> K
	// <GPIO_PIN_FUNCTION_L"> L
	// <GPIO_PIN_FUNCTION_M"> M
	// <GPIO_PIN_FUNCTION_N"> N
	PINMUX_PB11I_SDHC0_SDCK);

	gpio_set_pin_direction(SDCMD,
	// <y> Pin direction
	// <id> pad_direction
	// <GPIO_DIRECTION_OFF"> Off
	// <GPIO_DIRECTION_IN"> In
	// <GPIO_DIRECTION_OUT"> Out
	GPIO_DIRECTION_OUT);

	gpio_set_pin_level(SDCMD,
	// <y> Initial level
	// <id> pad_initial_level
	// <false"> Low
	// <true"> High
	false);

	gpio_set_pin_pull_mode(SDCMD,
	// <y> Pull configuration
	// <id> pad_pull_config
	// <GPIO_PULL_OFF"> Off
	// <GPIO_PULL_UP"> Pull-up
	// <GPIO_PULL_DOWN"> Pull-down
	GPIO_PULL_OFF);

	gpio_set_pin_function(SDCMD,
	// <y> Pin function
	// <id> pad_function
	// <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
	// <PINMUX_PA08I_SDHC0_SDCMD"> Auto
	// <GPIO_PIN_FUNCTION_OFF"> Off
	// <GPIO_PIN_FUNCTION_A"> A
	// <GPIO_PIN_FUNCTION_B"> B
	// <GPIO_PIN_FUNCTION_C"> C
	// <GPIO_PIN_FUNCTION_D"> D
	// <GPIO_PIN_FUNCTION_E"> E
	// <GPIO_PIN_FUNCTION_F"> F
	// <GPIO_PIN_FUNCTION_G"> G
	// <GPIO_PIN_FUNCTION_H"> H
	// <GPIO_PIN_FUNCTION_I"> I
	// <GPIO_PIN_FUNCTION_J"> J
	// <GPIO_PIN_FUNCTION_K"> K
	// <GPIO_PIN_FUNCTION_L"> L
	// <GPIO_PIN_FUNCTION_M"> M
	// <GPIO_PIN_FUNCTION_N"> N
	PINMUX_PA08I_SDHC0_SDCMD);

	gpio_set_pin_direction(SDDAT0,
	// <y> Pin direction
	// <id> pad_direction
	// <GPIO_DIRECTION_OFF"> Off
	// <GPIO_DIRECTION_IN"> In
	// <GPIO_DIRECTION_OUT"> Out
	GPIO_DIRECTION_OUT);

	gpio_set_pin_level(SDDAT0,
	// <y> Initial level
	// <id> pad_initial_level
	// <false"> Low
	// <true"> High
	false);

	gpio_set_pin_pull_mode(SDDAT0,
	// <y> Pull configuration
	// <id> pad_pull_config
	// <GPIO_PULL_OFF"> Off
	// <GPIO_PULL_UP"> Pull-up
	// <GPIO_PULL_DOWN"> Pull-down
	GPIO_PULL_OFF);

	gpio_set_pin_function(SDDAT0,
	// <y> Pin function
	// <id> pad_function
	// <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
	// <PINMUX_PA09I_SDHC0_SDDAT0"> Auto
	// <GPIO_PIN_FUNCTION_OFF"> Off
	// <GPIO_PIN_FUNCTION_A"> A
	// <GPIO_PIN_FUNCTION_B"> B
	// <GPIO_PIN_FUNCTION_C"> C
	// <GPIO_PIN_FUNCTION_D"> D
	// <GPIO_PIN_FUNCTION_E"> E
	// <GPIO_PIN_FUNCTION_F"> F
	// <GPIO_PIN_FUNCTION_G"> G
	// <GPIO_PIN_FUNCTION_H"> H
	// <GPIO_PIN_FUNCTION_I"> I
	// <GPIO_PIN_FUNCTION_J"> J
	// <GPIO_PIN_FUNCTION_K"> K
	// <GPIO_PIN_FUNCTION_L"> L
	// <GPIO_PIN_FUNCTION_M"> M
	// <GPIO_PIN_FUNCTION_N"> N
	PINMUX_PA09I_SDHC0_SDDAT0);

	gpio_set_pin_direction(SDDAT1,
	// <y> Pin direction
	// <id> pad_direction
	// <GPIO_DIRECTION_OFF"> Off
	// <GPIO_DIRECTION_IN"> In
	// <GPIO_DIRECTION_OUT"> Out
	GPIO_DIRECTION_OUT);

	gpio_set_pin_level(SDDAT1,
	// <y> Initial level
	// <id> pad_initial_level
	// <false"> Low
	// <true"> High
	false);

	gpio_set_pin_pull_mode(SDDAT1,
	// <y> Pull configuration
	// <id> pad_pull_config
	// <GPIO_PULL_OFF"> Off
	// <GPIO_PULL_UP"> Pull-up
	// <GPIO_PULL_DOWN"> Pull-down
	GPIO_PULL_OFF);

	gpio_set_pin_function(SDDAT1,
	// <y> Pin function
	// <id> pad_function
	// <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
	// <PINMUX_PA10I_SDHC0_SDDAT1"> Auto
	// <GPIO_PIN_FUNCTION_OFF"> Off
	// <GPIO_PIN_FUNCTION_A"> A
	// <GPIO_PIN_FUNCTION_B"> B
	// <GPIO_PIN_FUNCTION_C"> C
	// <GPIO_PIN_FUNCTION_D"> D
	// <GPIO_PIN_FUNCTION_E"> E
	// <GPIO_PIN_FUNCTION_F"> F
	// <GPIO_PIN_FUNCTION_G"> G
	// <GPIO_PIN_FUNCTION_H"> H
	// <GPIO_PIN_FUNCTION_I"> I
	// <GPIO_PIN_FUNCTION_J"> J
	// <GPIO_PIN_FUNCTION_K"> K
	// <GPIO_PIN_FUNCTION_L"> L
	// <GPIO_PIN_FUNCTION_M"> M
	// <GPIO_PIN_FUNCTION_N"> N
	PINMUX_PA10I_SDHC0_SDDAT1);

	gpio_set_pin_direction(SDDAT2,
	// <y> Pin direction
	// <id> pad_direction
	// <GPIO_DIRECTION_OFF"> Off
	// <GPIO_DIRECTION_IN"> In
	// <GPIO_DIRECTION_OUT"> Out
	GPIO_DIRECTION_OUT);

	gpio_set_pin_level(SDDAT2,
	// <y> Initial level
	// <id> pad_initial_level
	// <false"> Low
	// <true"> High
	false);

	gpio_set_pin_pull_mode(SDDAT2,
	// <y> Pull configuration
	// <id> pad_pull_config
	// <GPIO_PULL_OFF"> Off
	// <GPIO_PULL_UP"> Pull-up
	// <GPIO_PULL_DOWN"> Pull-down
	GPIO_PULL_OFF);

	gpio_set_pin_function(SDDAT2,
	// <y> Pin function
	// <id> pad_function
	// <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
	// <PINMUX_PA11I_SDHC0_SDDAT2"> Auto
	// <GPIO_PIN_FUNCTION_OFF"> Off
	// <GPIO_PIN_FUNCTION_A"> A
	// <GPIO_PIN_FUNCTION_B"> B
	// <GPIO_PIN_FUNCTION_C"> C
	// <GPIO_PIN_FUNCTION_D"> D
	// <GPIO_PIN_FUNCTION_E"> E
	// <GPIO_PIN_FUNCTION_F"> F
	// <GPIO_PIN_FUNCTION_G"> G
	// <GPIO_PIN_FUNCTION_H"> H
	// <GPIO_PIN_FUNCTION_I"> I
	// <GPIO_PIN_FUNCTION_J"> J
	// <GPIO_PIN_FUNCTION_K"> K
	// <GPIO_PIN_FUNCTION_L"> L
	// <GPIO_PIN_FUNCTION_M"> M
	// <GPIO_PIN_FUNCTION_N"> N
	PINMUX_PA11I_SDHC0_SDDAT2);

	gpio_set_pin_direction(SDDAT3,
	// <y> Pin direction
	// <id> pad_direction
	// <GPIO_DIRECTION_OFF"> Off
	// <GPIO_DIRECTION_IN"> In
	// <GPIO_DIRECTION_OUT"> Out
	GPIO_DIRECTION_OUT);

	gpio_set_pin_level(SDDAT3,
	// <y> Initial level
	// <id> pad_initial_level
	// <false"> Low
	// <true"> High
	false);

	gpio_set_pin_pull_mode(SDDAT3,
	// <y> Pull configuration
	// <id> pad_pull_config
	// <GPIO_PULL_OFF"> Off
	// <GPIO_PULL_UP"> Pull-up
	// <GPIO_PULL_DOWN"> Pull-down
	GPIO_PULL_OFF);

	gpio_set_pin_function(SDDAT3,
	// <y> Pin function
	// <id> pad_function
	// <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
	// <PINMUX_PB10I_SDHC0_SDDAT3"> Auto
	// <GPIO_PIN_FUNCTION_OFF"> Off
	// <GPIO_PIN_FUNCTION_A"> A
	// <GPIO_PIN_FUNCTION_B"> B
	// <GPIO_PIN_FUNCTION_C"> C
	// <GPIO_PIN_FUNCTION_D"> D
	// <GPIO_PIN_FUNCTION_E"> E
	// <GPIO_PIN_FUNCTION_F"> F
	// <GPIO_PIN_FUNCTION_G"> G
	// <GPIO_PIN_FUNCTION_H"> H
	// <GPIO_PIN_FUNCTION_I"> I
	// <GPIO_PIN_FUNCTION_J"> J
	// <GPIO_PIN_FUNCTION_K"> K
	// <GPIO_PIN_FUNCTION_L"> L
	// <GPIO_PIN_FUNCTION_M"> M
	// <GPIO_PIN_FUNCTION_N"> N
	PINMUX_PB10I_SDHC0_SDDAT3);
}

void IO_BUS_CLOCK_init(void)
{
	hri_mclk_set_AHBMASK_SDHC0_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, SDHC0_GCLK_ID, CONF_GCLK_SDHC0_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SDHC0_GCLK_ID_SLOW, CONF_GCLK_SDHC0_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
}

void IO_BUS_init(void)
{
	IO_BUS_CLOCK_init();
	mci_sync_init(&IO_BUS, SDHC0);
	IO_BUS_PORT_init();
}
