/*
 * © 2020 Thorsis Technologies GmbH
 */

#include <common.h>
#include <command.h>

#include "tt_led.h"

static int do_tt_hello(struct cmd_tbl *cmdtp, int flag,
		       int argc, char * const argv[])
{
	printf("Hello TT!\n");
	return CMD_RET_SUCCESS;
}

static char tt_help_text[] =
	"hello    - Print hello world\n"
	"tt ledtest  - Turn on and off all LEDs once\n"
	;

U_BOOT_CMD_WITH_SUBCMDS(tt, "tt common commands", tt_help_text,
			U_BOOT_SUBCMD_MKENT(ledtest, 1, 1, do_tt_ledtest),
			U_BOOT_SUBCMD_MKENT(hello, 1, 1, do_tt_hello));
