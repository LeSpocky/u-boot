/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#include <common.h>
#include <command.h>

#ifdef CONFIG_FPGA
#include "proficube_led.h"
#endif

static int do_tt_hello(cmd_tbl_t *cmdtp, int flag,
		       int argc, char * const argv[])
{
	printf("Hello TT!\n");
	return CMD_RET_SUCCESS;
}

static char tt_help_text[] =
	"hello    - Print hello world\n"
#ifdef CONFIG_FPGA
	"tt ledtest  - Turn on and off all LEDs once\n"
#endif
	;

U_BOOT_CMD_WITH_SUBCMDS(tt, "Board specific commands", tt_help_text,
#ifdef CONFIG_FPGA
	U_BOOT_SUBCMD_MKENT(ledtest, 1, 1, do_tt_ledtest),
#endif
	U_BOOT_SUBCMD_MKENT(hello, 1, 1, do_tt_hello));
