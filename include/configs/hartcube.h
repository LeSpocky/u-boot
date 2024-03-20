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

/* Environment */
#define MEM_LAYOUT_ENV_SETTINGS \
	"kernel_addr_r=0x22000000\0" \
	"fdt_addr_r=0x23000000\0" \
	"scriptaddr=0x23100000\0" \
	"pxefile_addr_r=0x23200000\0" \
	"ramdisk_addr_r=0x23300000\0"

#define BOOT_TARGET_DEVICES(func) \
	func(UBIFS, ubifs, 0, UBI, boot)
#include <config_distro_bootcmd.h>

#define CFG_EXTRA_ENV_SETTINGS \
	MEM_LAYOUT_ENV_SETTINGS \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	BOOTENV

#endif
