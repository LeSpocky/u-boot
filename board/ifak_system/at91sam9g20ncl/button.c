/*******************************************************************//**
 *	@file		button.c
 *
 *	@author		Peter Schulz <psu@ifak-system.com>
 *	@author		Alexander Dahl <ada@thorsis.com>
 *
 *	@copyright	2010,2012 ifak system GmbH
 *	@copyright	2019 Thorsis Technologies GmbH
 **********************************************************************/

#include "button.h"

#include <common.h>
#include <command.h>
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/gpio.h>
#include <mach/clk.h>

/*
 * Hardcoded for now, this should go to dts later, maybe when switching
 * to generic (gpio) button support with v2020.10?
 */
#define	NCL_PIN_FACTORYRESET	AT91_PIN_PA11

void ncl_button_init(void)
{
	debug("%s: entered\n", __func__);

	at91_periph_clk_enable(ATMEL_ID_PIOA);
	at91_set_gpio_input(NCL_PIN_FACTORYRESET, 1);
}

int do_button(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[] ) {
	debug("%s: entered\n", __func__);

	return at91_get_gpio_value(NCL_PIN_FACTORYRESET);
}

U_BOOT_CMD( button, 1, 1, do_button,
	"status of FACTORY_RESET button",
	"returns the status of the FACTORY_RESET button\n"
	"example: if button; then echo pressed; else echo released; fi"
);

/* vim: set noet sts=0 ts=8 sw=8 sr: */
