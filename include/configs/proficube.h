/*
 * Configuration file for the Thorsis ProfiCube board.
 *
 * Copyright 2019 Thorsis Technologies GmbH
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_AT91_SLOW_CLOCK      32768
#define CONFIG_SYS_AT91_MAIN_CLOCK      24000000 /* from 24 MHz crystal */

#define CONFIG_ARCH_CPU_INIT
#define CONFIG_SKIP_LOWLEVEL_INIT

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		SZ_4M

/* SDRAM */
#define CONFIG_SYS_SDRAM_BASE		0x20000000
#define CONFIG_SYS_SDRAM_SIZE		0x8000000

#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_SDRAM_BASE + SZ_16K - GENERATED_GBL_DATA_SIZE)

#define CONFIG_SYS_LOAD_ADDR		0x22000000

/* NAND flash */
#ifdef CONFIG_NAND_BOOT

#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		ATMEL_BASE_CS3
#define CONFIG_SYS_NAND_MASK_ALE	BIT(21)
#define CONFIG_SYS_NAND_MASK_CLE	BIT(22)
/*
 * as of U-Boot v2019.1 using the RDY pin does not work with atmel nand
 * driver and the pio4 controller on SAMA5D2 …
 */
#undef CONFIG_SYS_NAND_READY_PIN
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SYS_NAND_USE_FLASH_BBT

#define CONFIG_ATMEL_NAND_HWECC
#define CONFIG_ATMEL_NAND_HW_PMECC

#define CONFIG_PMECC_CAP		8
#define CONFIG_PMECC_SECTOR_SIZE	512

#define CONFIG_ENV_OFFSET		0x100000
#define CONFIG_ENV_OFFSET_REDUND	0x140000
#define CONFIG_ENV_SIZE			SZ_256K

#endif

/* Environment */
#define MEM_LAYOUT_ENV_SETTINGS \
	"kernel_addr_r=0x22000000\0" \
	"fdt_addr_r=0x23000000\0" \
	"scriptaddr=0x23100000\0" \
	"pxefile_addr_r=0x23200000\0" \
	"ramdisk_addr_r=0x23300000\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	MEM_LAYOUT_ENV_SETTINGS \
	"fdtfile=" CONFIG_DEFAULT_FDT_FILE "\0"

#endif
