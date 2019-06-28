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

static cmd_tbl_t tt_cmds[] = {
	U_BOOT_CMD_MKENT(hello, 0, 1, do_tt_hello, "", ""),
#ifdef CONFIG_FPGA
	U_BOOT_CMD_MKENT(ledtest, 0, 1, do_tt_ledtest, "", ""),
#endif
};

static int do_tt_cmd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *tt_cmd;

	if (argc < 2)
		return CMD_RET_USAGE;

	/* drop sub-command argument */
	argc--;
	argv++;

	tt_cmd = find_cmd_tbl(argv[0], tt_cmds, ARRAY_SIZE(tt_cmds));

	if (tt_cmd)
		return tt_cmd->cmd(tt_cmd, flag, argc, argv);

	return CMD_RET_USAGE;
}

static char tt_help_text[] =
	"hello    - Print hello world\n"
#ifdef CONFIG_FPGA
	"tt ledtest  - Turn on and off all LEDs once\n"
#endif
	;

U_BOOT_CMD(tt, 2, 1, do_tt_cmd, "Board specific commands", tt_help_text);
