/*******************************************************************//**
 *	@file		led.c
 *
 *	@author		Peter Schulz <psu@ifak-system.com>
 *	@author		Alexander Dahl <ada@thorsis.com>
 *
 *	@copyright	2010,2012 ifak system GmbH
 *	@copyright	2019 Thorsis Technologies GmbH
 **********************************************************************/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <status_led.h>

#define	NCL_PIN_LED_STATUS		AT91_PIN_PA22	/* this is the user led */

void coloured_LED_init( void ) {
	/* Clock is enabled in board_early_init_f() */

	/*
	 * switch on the power led, if linux and
	 * all applications started the power led
	 * switched off over led_kernel_driver under linux.
	 */
	at91_set_gpio_output(NCL_PIN_LED_STATUS, 1);
}

int do_led_set( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	/* need at least one arguments app + arg */
	if ( argc < 2 ) {
		puts( "Few arguments given.\n\n" );
		goto usage;
	}

	if ( strcmp(argv[1], "on") == 0 ) {
		at91_set_gpio_value(NCL_PIN_LED_STATUS, 1);
		return 0;
	} else if ( strcmp (argv[1], "off") == 0 ) {
		at91_set_gpio_value(NCL_PIN_LED_STATUS, 0);
		return 0;
	} else goto usage;

usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	led, 2, 1, do_led_set,
	"set status of led",
	"<on|off>"
);

/* vim: set noet sts=0 ts=8 sw=8 sr: */
