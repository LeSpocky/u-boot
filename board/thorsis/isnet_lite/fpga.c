/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2006
 * Heiko Schocher, DENX Software Engineering, hs@denx.de
 *
 * (C) Copyright 2010
 * Peter Schulz <psu@ifak-system.com>
 *
 * (C) Copyright 2019
 * Thorsis Technologies GmbH
 */

/*
 * based on Altera FPGA configuration support for the ALPR computer from prodrive
 */

 /*
  * todo: testing
  * todo: add the correct Cyclone3 device with the correct description
  * todo: try to use SPI-functions of the AT91 instead of bitbanging (should increase the data rate of actually 1,250 MHz)
  * todo: add support for compressed uImages (WARNING: compressed images will not be decompressed!)
  */

#define LOG_CATEGORY LOGC_BOARD

#include <common.h>
#include <altera.h>
#include <ACEX1K.h>
#include <command.h>
#include <console.h>
#include <fpga.h>
#include <log.h>
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/gpio.h>
#include <mach/clk.h>

#include "../common/tt_fpga.h"


#define ERROR_FPGA_PRG_INIT_LOW  -1        /* Timeout after PRG* asserted   */
#define ERROR_FPGA_PRG_INIT_HIGH -2        /* Timeout after PRG* deasserted */
#define ERROR_FPGA_PRG_DONE      -3        /* Timeout after programming     */

#define NCL_PIN_CLK		AT91_PIN_PA2	/* FPGA clk pin (cpu output)		*/
#define NCL_PIN_DATA		AT91_PIN_PA1	/* FPGA data pin (cpu output)		*/
#define NCL_PIN_STATUS		AT91_PIN_PA27	/* FPGA status pin (cpu input)		*/
#define NCL_PIN_CONFIG		AT91_PIN_PA25	/* FPGA CONFIG pin (cpu output)		*/
#define NCL_PIN_CON_DON		AT91_PIN_PA26	/* FPGA CONFIG_DONE pin (cpu input)	*/

#define FPGA_DONE_STATE (at91_get_gpio_value(NCL_PIN_CON_DON))

#define FPGA_WRITE_1 {							\
		at91_set_gpio_output(NCL_PIN_DATA,1);  /* set data to 1  */	\
		at91_set_gpio_output(NCL_PIN_CLK,1);  /* set data to 1  */	\
		at91_set_gpio_output(NCL_PIN_CLK,0);} /* set clk to 0  */

#define FPGA_WRITE_0 {							\
		at91_set_gpio_output(NCL_PIN_DATA,0);  /* set data to 0  */	\
		at91_set_gpio_output(NCL_PIN_CLK,1);  /* set data to 1  */	\
		at91_set_gpio_output(NCL_PIN_CLK,0);} /* set clk to 0  */

/* Plattforminitializations */
int fpga_pre_fn (int cookie)
{
	/* initialize the GPIO Pins */
	/* output */
	at91_set_gpio_output(NCL_PIN_CLK,0);
	at91_set_gpio_output(NCL_PIN_DATA,0);

	/* output */
	at91_set_gpio_output(NCL_PIN_CONFIG,0);

	/* First we set STATUS to 0 (reset state) */
	/* CONFIG = 0 STATUS = 0 -> FPGA in reset state */
	// at91_set_gpio_output(NCL_PIN_STATUS,0);

	/* input */
	at91_set_gpio_input(NCL_PIN_CON_DON,1);
	at91_set_gpio_input(NCL_PIN_STATUS,0);

	log_debug("FPGA IO configured\n");

	return FPGA_SUCCESS;
}

/* Set the state of CONFIG Pin */
int fpga_config_fn (int assert_config, int flush, int cookie)
{
	if (assert_config) {
		at91_set_gpio_output(NCL_PIN_CONFIG,1);
		log_debug("CONFIG set to HIGH\n");
	} else {
		at91_set_gpio_output(NCL_PIN_CONFIG,0);
	}
	return FPGA_SUCCESS;
}

/* Returns the state of STATUS Pin */
int fpga_status_fn (int cookie)
{
	if (at91_get_gpio_value(NCL_PIN_STATUS)) {
		log_debug("STATUS = HIGH\n");
		return FPGA_FAIL;
	}
	log_debug("STATUS = LOW\n");
	return FPGA_SUCCESS;
}

/* Returns the state of CONF_DONE Pin */
int fpga_done_fn (int cookie)
{
	if (at91_get_gpio_value(NCL_PIN_CON_DON)) {
		log_debug("CONF_DON = HIGH\n");
		return FPGA_FAIL;
	}
	log_debug("CONF_DON = LOW\n");
	return FPGA_SUCCESS;
}

/* writes the complete buffer to the FPGA
   writing the complete buffer in one function is much faster,
   then calling it for every bit */
int fpga_write_fn( const void *buf, size_t len, int flush, int cookie )
{
	size_t bytecount = 0;
	unsigned char *data = (unsigned char *) buf;
	unsigned char val=0;
	int		i;
	int len_40 = len / 40;

	while (bytecount < len) {
		val = data[bytecount++];
		i = 8;
		do {
			if (val & 0x01) {
				FPGA_WRITE_1;
			} else {
				FPGA_WRITE_0;
			}
			val >>= 1;
			i --;
		} while (i > 0);

#ifdef CONFIG_SYS_FPGA_PROG_FEEDBACK
		if (bytecount % len_40 == 0) {
			putc ('.');		/* let them know we are alive */
#ifdef CONFIG_SYS_FPGA_CHECK_CTRLC
			if (ctrlc ())
				return FPGA_FAIL;
#endif
		}
#endif
	}
	return FPGA_SUCCESS;
}

static Altera_CYC2_Passive_Serial_fns isnet_lite_fns = {
	.pre = fpga_pre_fn,
	.config = fpga_config_fn,
	.status = fpga_status_fn,
	.done = fpga_done_fn,
	.write = fpga_write_fn,
	.abort = tt_fpga_abort,
	.post = NULL,
};

static Altera_desc isnet_lite_fpga = {
	.family = Altera_CYC2,
	.iface = passive_serial,
	.size = Altera_EP2C35_SIZE,
	.iface_fns = &isnet_lite_fns,
};

void board_fpga_init(void)
{
	at91_periph_clk_enable(ATMEL_ID_PIOA);

	log_debug("Initialize FPGA interface\n");
	fpga_init();

	log_debug("Adding fpga 0\n");
	fpga_add( fpga_altera, &isnet_lite_fpga );
}

/* vim: set noet sts=0 ts=8 sw=8 sr: */
