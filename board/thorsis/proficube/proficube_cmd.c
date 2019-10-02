/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#include <common.h>
#include <command.h>

#ifdef CONFIG_FPGA
#include "proficube_bp.h"
#include "proficube_led.h"
#endif

static int do_tt_hello(cmd_tbl_t *cmdtp, int flag,
		       int argc, char * const argv[])
{
	printf("Hello TT!\n");
	return CMD_RET_SUCCESS;
}

#ifdef CONFIG_FPGA
static char bp_help_text[] =
	"test - Write random patterns and read them back\n"
	"bp dpr [size]\n"
	"        - Get or set DP-RAM size to be used for testing\n"
	;

U_BOOT_CMD_WITH_SUBCMDS(bp, "tt backplane handling", bp_help_text,
			U_BOOT_SUBCMD_MKENT(test, 1, 0, do_bp_test),
			U_BOOT_SUBCMD_MKENT(dpr, 2, 1, do_bp_dpr));
#endif

static char tt_help_text[] =
	"hello    - Print hello world\n"
#ifdef CONFIG_FPGA
	"tt ledtest  - Turn on and off all LEDs once\n"
#endif
	;

U_BOOT_CMD_WITH_SUBCMDS(tt, "tt board specific commands", tt_help_text,
#ifdef CONFIG_FPGA
			U_BOOT_SUBCMD_MKENT(ledtest, 1, 1, do_tt_ledtest),
#endif
			U_BOOT_SUBCMD_MKENT(hello, 1, 1, do_tt_hello));
