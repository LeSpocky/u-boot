/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#include <common.h>
#include <command.h>

#ifdef CONFIG_FPGA
#include "proficube_bp.h"

static char bp_help_text[] =
	"test - Write random patterns and read them back\n"
	"bp dpr [size]\n"
	"        - Get or set DP-RAM size to be used for testing\n"
	;

U_BOOT_CMD_WITH_SUBCMDS(bp, "tt backplane handling", bp_help_text,
			U_BOOT_SUBCMD_MKENT(test, 1, 0, do_bp_test),
			U_BOOT_SUBCMD_MKENT(dpr, 2, 1, do_bp_dpr));
#endif
