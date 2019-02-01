/*
 * (C) Copyright 2006
 * Heiko Schocher, DENX Software Engineering, hs@denx.de
 * (C) Copyright 2010
 * psu@ifak-system.com
 * (C) Copyright 2019
 * Thorsis Technologies GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
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

#include <common.h>
#include <altera.h>
#include <ACEX1K.h>
#include <command.h>
#include <fpga.h>
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/gpio.h>

#if defined(CONFIG_FPGA)

#ifdef FPGA_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define	PRINTF(fmt,args...)
#endif

#define ERROR_FPGA_PRG_INIT_LOW  -1        /* Timeout after PRG* asserted   */
#define ERROR_FPGA_PRG_INIT_HIGH -2        /* Timeout after PRG* deasserted */
#define ERROR_FPGA_PRG_DONE      -3        /* Timeout after programming     */

#define FPGA_DONE_STATE (at91_get_gpio_value(CONFIG_SYS_GPIO_CON_DON))

#define FPGA_WRITE_1 {							\
		at91_set_gpio_output(CONFIG_SYS_GPIO_DATA,1);  /* set data to 1  */	\
		at91_set_gpio_output(CONFIG_SYS_GPIO_CLK,1);  /* set data to 1  */	\
		at91_set_gpio_output(CONFIG_SYS_GPIO_CLK,0);} /* set clk to 0  */

#define FPGA_WRITE_0 {							\
		at91_set_gpio_output(CONFIG_SYS_GPIO_DATA,0);  /* set data to 0  */	\
		at91_set_gpio_output(CONFIG_SYS_GPIO_CLK,1);  /* set data to 1  */	\
		at91_set_gpio_output(CONFIG_SYS_GPIO_CLK,0);} /* set clk to 0  */

/* Plattforminitializations */
int fpga_pre_fn (int cookie)
{
	/* initialize the GPIO Pins */
	/* output */
	at91_set_gpio_output(CONFIG_SYS_GPIO_CLK,0);
	at91_set_gpio_output(CONFIG_SYS_GPIO_DATA,0);

	/* output */
	at91_set_gpio_output(CONFIG_SYS_GPIO_CONFIG,0);

	/* First we set STATUS to 0 (reset state) */
	/* CONFIG = 0 STATUS = 0 -> FPGA in reset state */
	// at91_set_gpio_output(CONFIG_SYS_GPIO_STATUS,0);

	/* input */
	at91_set_gpio_input(CONFIG_SYS_GPIO_CON_DON,1);
	at91_set_gpio_input(CONFIG_SYS_GPIO_STATUS,0);

	PRINTF("FPGA IO configured\n");

	return FPGA_SUCCESS;
}

/* Set the state of CONFIG Pin */
int fpga_config_fn (int assert_config, int flush, int cookie)
{
	if (assert_config) {
		at91_set_gpio_output(CONFIG_SYS_GPIO_CONFIG,1);
		PRINTF("CONFIG set to HIGH\n");
	} else {
		at91_set_gpio_output(CONFIG_SYS_GPIO_CONFIG,0);
	}
	return FPGA_SUCCESS;
}

/* Returns the state of STATUS Pin */
int fpga_status_fn (int cookie)
{
	if (at91_get_gpio_value(CONFIG_SYS_GPIO_STATUS)) {
		PRINTF("STATUS = HIGH\n");
		return FPGA_FAIL;
	}
	PRINTF("STATUS = LOW\n");
	return FPGA_SUCCESS;
}

/* Returns the state of CONF_DONE Pin */
int fpga_done_fn (int cookie)
{
	if (at91_get_gpio_value(CONFIG_SYS_GPIO_CON_DON)) {
		PRINTF("CONF_DON = HIGH\n");
		return FPGA_FAIL;
	}
	PRINTF("CONF_DON = LOW\n");
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

/* called, when programming is aborted */
int fpga_abort_fn (int cookie)
{
	return FPGA_SUCCESS;
}

/* called, when programming was succesful */
int fpga_post_fn (int cookie)
{
	return fpga_abort_fn (cookie);
}

/* Note that these are pointers to code that is in Flash.  They will be
 * relocated at runtime.
 */
Altera_CYC2_Passive_Serial_fns fpga_fns = {
	fpga_pre_fn,
	fpga_config_fn,
	fpga_status_fn,
	fpga_done_fn,
	fpga_write_fn,
	fpga_abort_fn,
	fpga_post_fn
};

Altera_desc fpga[CONFIG_FPGA_COUNT] = {
	{Altera_CYC2,
	 passive_serial,
	 Altera_EP2C35_SIZE,
	 (void *) &fpga_fns,
	 NULL,
	 0}
};

/*
 *	Initialize the fpga.  Return 1 on success, 0 on failure.
 */
int ncl_fpga_init( void )
{
	int i;

	PRINTF( "%s:%d: Initialize FPGA interface\n", __FUNCTION__, __LINE__ );
	fpga_init();

	for ( i = 0; i < CONFIG_FPGA_COUNT; i++ )
	{
		PRINTF( "%s:%d: Adding fpga %d\n", __FUNCTION__, __LINE__, i );
		fpga_add( fpga_altera, &fpga[i] );
	}

	return 1;
}

#endif

/* vim: set noet sts=0 ts=8 sw=8 sr: */
