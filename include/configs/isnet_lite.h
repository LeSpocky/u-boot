/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian@popies.net>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * © 2010 ifak system GmbH
 * © 2019 Thorsis Technologies GmbH
 *
 * Based on configuation settings for the AT91SAM9260EK & AT91SAM9G20EK
 * boards for this AT91SAM9G20NCL configuration.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * SoC must be defined first, before hardware.h is included.
 * In this case SoC is defined in boards.cfg.
 */
#include <asm/hardware.h>

#include <linux/sizes.h>

/* ARM asynchronous clock */
#define CONFIG_SYS_AT91_SLOW_CLOCK	32768		/* slow clock xtal */
#define CONFIG_SYS_AT91_MAIN_CLOCK	18432000	/* main clock xtal */

/*
 * SDRAM: 1 bank, min 32, max 128 MB
 * Initialized before u-boot gets started.
 */
#define CONFIG_SYS_SDRAM_BASE		ATMEL_BASE_CS1
#define CONFIG_SYS_SDRAM_SIZE		SZ_32M

#define CONFIG_SYS_INIT_RAM_SIZE	SZ_16K
#ifdef CONFIG_AT91SAM9XE
# define CONFIG_SYS_INIT_RAM_ADDR	ATMEL_BASE_SRAM
#else
# define CONFIG_SYS_INIT_RAM_ADDR	ATMEL_BASE_SRAM1
#endif

/* NAND flash */
#ifdef CONFIG_CMD_NAND
#define CONFIG_SYS_NAND_BASE		ATMEL_BASE_CS3
#define CONFIG_SYS_NAND_MASK_ALE	(1 << 21)
#define CONFIG_SYS_NAND_MASK_CLE	(1 << 22)
#define CONFIG_SYS_NAND_ENABLE_PIN	AT91_PIN_PC14
#define CONFIG_SYS_NAND_READY_PIN	AT91_PIN_PC13
#endif

#endif /* __CONFIG_H */

/* vim: set noet sts=0 ts=8 sw=8 sr: */
