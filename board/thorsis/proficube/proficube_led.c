/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#include <asm/io.h>
#include <command.h>
#include <linux/delay.h>

#include "../common/tt_led.h"

#define TT_FPGA_LEDOR	0x7000A000
#define TT_FPGA_LEDCR	0x7000A001

int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[])
{
	/* Acquire all LEDs */
	writeb(0xFF, TT_FPGA_LEDCR);

	/* Set all green LEDs */
	writeb(0xAA, TT_FPGA_LEDOR);
	mdelay(TT_LED_DELAY);
	writeb(0x00, TT_FPGA_LEDOR);
	mdelay(TT_LED_DELAY);

	/* Set all red LEDs */
	writeb(0x55, TT_FPGA_LEDOR);
	mdelay(TT_LED_DELAY);
	writeb(0x00, TT_FPGA_LEDOR);
	mdelay(TT_LED_DELAY);

	/* Acquire non-PB LEDs only */
	writeb(0x3F, TT_FPGA_LEDCR);

	return CMD_RET_SUCCESS;
}
