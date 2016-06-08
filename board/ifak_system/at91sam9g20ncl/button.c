/*******************************************************************//**
 *  @file   button.c
 *
 *  @author Peter Schulz <psu@ifak-system.com>
 *  @author Alexander Dahl <ada@ifak-system.com>
 *
 *  Copyright 2010,2012 ifak system GmbH
 **********************************************************************/

#include <common.h>
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/gpio.h>

void BUTTON_init(void)
{
	at91_set_gpio_input(CONFIG_FACTORYRESET_BUTTON, 1);
}

int do_button( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	return at91_get_gpio_value(CONFIG_FACTORYRESET_BUTTON);
}

U_BOOT_CMD( button, 1, 1, do_button,
	"status of FACTORY_RESET button",
	"returns the status of the FACTORY_RESET button\n"
	"example: if button; then echo pressed; else echo released; fi"
);

/* vim: set noet sts=0 ts=8 sw=8 sr: */
