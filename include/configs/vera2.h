/*
 * Configuration file for the Thorsis Technologies Vera2 Board.
 *
 * © 2021 Thorsis Technologies GmbH
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>
#include <asm/arch/sama5d2.h>

#define CONFIG_SYS_AT91_SLOW_CLOCK      32768
#define CONFIG_SYS_AT91_MAIN_CLOCK      24000000 /* from 24 MHz crystal */

#define CONFIG_SKIP_LOWLEVEL_INIT

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		SZ_4M

/* SDRAM */
#define CONFIG_SYS_SDRAM_BASE		0x20000000
#define CONFIG_SYS_SDRAM_SIZE		SZ_64M

#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_SDRAM_BASE + SZ_16K - GENERATED_GBL_DATA_SIZE)

#define CONFIG_SYS_LOAD_ADDR		0x22000000 /* load address */

/* NAND flash */
#ifdef CONFIG_NAND_BOOT

#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		ATMEL_BASE_CS3
#define CONFIG_SYS_NAND_MASK_ALE	BIT(21)
#define CONFIG_SYS_NAND_MASK_CLE	BIT(22)
/*
 * as of U-Boot v2019.01 using the RDY pin does not work with atmel nand
 * driver and the pio4 controller on SAMA5D2 …
 */
#undef CONFIG_SYS_NAND_READY_PIN
#define CONFIG_SYS_NAND_ONFI_DETECTION

#endif

#endif
