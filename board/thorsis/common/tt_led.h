/*
 * © 2019 Thorsis Technologies GmbH
 */

#ifndef TT_LED
#define TT_LED

#include <command.h>

#define TT_LED_DELAY	250

int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[]);

#endif /* TT_LED */
