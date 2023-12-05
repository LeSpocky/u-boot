/*
 * Configuration file for the Thorsis Technologies MUXgate board.
 *
 * © 2020 Thorsis Technologies GmbH
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_AT91_SLOW_CLOCK      32768
#define CONFIG_SYS_AT91_MAIN_CLOCK      24000000 /* from 24 MHz crystal */

/* SDRAM */
#define CONFIG_SYS_SDRAM_BASE		0x20000000
#define CONFIG_SYS_SDRAM_SIZE		SZ_128M

/* NAND flash */
#ifdef CONFIG_NAND_BOOT

#define CFG_SYS_NAND_BASE		ATMEL_BASE_CS3
#define CFG_SYS_NAND_MASK_ALE	BIT(21)
#define CFG_SYS_NAND_MASK_CLE	BIT(22)
/*
 * as of U-Boot v2019.01 using the RDY pin does not work with atmel nand
 * driver and the pio4 controller on SAMA5D2 …
 */
#undef CFG_SYS_NAND_READY_PIN

#endif

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

#define CONFIG_EXTRA_ENV_SETTINGS \
	MEM_LAYOUT_ENV_SETTINGS \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0" \
	BOOTENV

#endif
