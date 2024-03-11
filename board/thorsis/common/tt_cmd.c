/*
 * © 2020 Thorsis Technologies GmbH
 */

#include <common.h>
#include <command.h>

#ifdef CONFIG_LED
#include "tt_led.h"
#endif

static int do_tt_hello(struct cmd_tbl *cmdtp, int flag,
		       int argc, char * const argv[])
{
	printf("Hello TT!\n");
	return CMD_RET_SUCCESS;
}

__weak int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[])
{
	pr_err("LED test not implemented for this board!\n");
	return CMD_RET_FAILURE;
}

#ifdef CONFIG_SYS_LONGHELP
static char tt_help_text[] =
	"hello    - Print hello world\n"
#ifdef CONFIG_LED
	"tt ledtest  - Turn on and off all LEDs once\n"
#endif
	;
#endif

U_BOOT_CMD_WITH_SUBCMDS(tt, "tt common commands", tt_help_text,
#ifdef CONFIG_LED
			U_BOOT_SUBCMD_MKENT(ledtest, 1, 1, do_tt_ledtest),
#endif
			U_BOOT_SUBCMD_MKENT(hello, 1, 1, do_tt_hello));
