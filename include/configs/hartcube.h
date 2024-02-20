/*
 * Configuration file for the Thorsis Technologies HartCube board.
 *
 * © 2024 Thorsis Technologies GmbH
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <linux/sizes.h>

#define CFG_SYS_AT91_SLOW_CLOCK	32768
#define CFG_SYS_AT91_MAIN_CLOCK	24000000

/* SDRAM */
#define CFG_SYS_SDRAM_BASE		0x20000000
#define CFG_SYS_SDRAM_SIZE		SZ_64M

#endif
