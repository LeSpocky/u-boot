/*
 * Copyright 2019 Thorsis Technologies GmbH
 *
 * NOTE	This is using the ancient FPGA infrastructure. A modern approach
 * 	would use driver model and device tree. For the configuration
 * 	part the FPGA is a SPI slave, so it would be a child node of
 * 	some SPI node in DT. After configuration FPGA is a child of some
 * 	not yet existing EBI node. SPI bus including child nodes do not
 * 	need to be defined in DT at the moment, we must still hardcode
 * 	bus and cs here.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <asm/arch/atmel_pio4.h>
#include <altera.h>
#include <console.h>
#include <dm.h>
#include <errno.h>
#include <fpga.h>
#include <malloc.h>

#include "proficube_fpga.h"

#if PROFICUBE_USE_SPI
#include <spi.h>
#ifdef CONFIG_BITREVERSE
#include <linux/bitrev.h>
#else
#include <limits.h>
static uint8_t bitrev8(uint8_t v)
{
	unsigned int r = v; /* r will be reversed bits of v; first get LSB of v	*/
	int s = sizeof(v) * CHAR_BIT - 1; /* extra shift needed at end */

	for (v >>= 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}

	r <<= s; /* shift when v's highest bits are zero	*/

	return r;
}
#endif
#endif

static int proficube_fpga_abort(int cookie)
{
	pr_debug("%s: called\n", __func__);

	return FPGA_SUCCESS;
}

/**
 * Set state of nCONFIG pin.
 */
static int proficube_fpga_config(int assert_config, int flush, int cookie)
{
	pr_debug("%s: called\n", __func__);

	if (assert_config)
	{
		pr_debug("Setting nCONFIG to HIGH.\n");
		atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_nCONFIG, 1);
	}
	else
	{
		pr_debug("Setting nCONFIG to LOW.\n");
		atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_nCONFIG, 0);
	}

	return FPGA_SUCCESS;
}

/**
 * Read state of CONF_DONE pin.
 */
static int proficube_fpga_done(int cookie)
{
	pr_debug("%s: called\n", __func__);

	return atmel_pio4_get_pio_input(PROFICUBE_FPGA_PIN_CONF_DONE);
}

/**
 * Read state of nSTATUS pin.
 */
static int proficube_fpga_status(int cookie)
{
	pr_debug("%s: called\n", __func__);

	return atmel_pio4_get_pio_input(PROFICUBE_FPGA_PIN_nSTATUS);
}

#if PROFICUBE_USE_SPI
static int proficube_fpga_write(const void *buf, size_t len, int flush, int cookie)
{
	struct spi_slave *slave;
	uint8_t *data = NULL;
	int ret = 0;
	size_t i;

	pr_debug("%s: called (%s)\n", __func__,
		 PROFICUBE_USE_SPI ? "spi" : "gpio");

	if (len > PROFICUBE_FPGA_SIZE)
	{
		pr_err("FPGA image too big (%zu > %zu)!", len, PROFICUBE_FPGA_SIZE);
		return FPGA_FAIL;
	}

	data = malloc(len);
	if (!data)
	{
		pr_err("Could not allocate memory!\n");
		return FPGA_FAIL;
	}

#ifdef CONFIG_DM_SPI
	struct udevice *dev;
	ret = _spi_get_bus_and_cs(PROFICUBE_FPGA_SPI_BUS,
				  PROFICUBE_FPGA_SPI_CS,
				  PROFICUBE_FPGA_SPI_SPEED, SPI_MODE_0,
				  "spi_generic_drv",
				  PROFICUBE_FPGA_SPI_DEV_NAME, &dev,
				  &slave);
	if (ret)
	{
		pr_err("spi_get_bus_and_cs() failed: %d!\n", ret);
		goto err_data_allocated;
	}
#else
#error "Not supported!"
#endif

	ret = spi_claim_bus(slave);
	if (ret)
	{
		pr_debug("spi_claim_bus(%p) failed: %d!\n", slave, ret);
		goto err_data_allocated;
	}

	for (i = 0; i < len; i++)
	{
		data[i] = bitrev8( *((const uint8_t *)buf + i) );
	}

	ret = spi_xfer(slave, len * 8, data, NULL, SPI_XFER_ONCE);
	if(ret)
	{
		pr_err("Error %d during SPI transaction\n", ret);
	}

	spi_release_bus(slave);

err_data_allocated:
	free(data);

	return ret;
}
#else
static int proficube_fpga_write(const void *buf, size_t len, int flush, int cookie)
{
#ifdef CONFIG_SYS_FPGA_PROG_FEEDBACK
	const size_t len_40 = len / 40;
#endif
	const uint8_t *data = buf;
	size_t bytecount = 0;

	pr_debug("%s: called (%s)\n", __func__,
		 PROFICUBE_USE_SPI ? "spi" : "gpio");

	if (len > PROFICUBE_FPGA_SIZE)
	{
		pr_err("FPGA image too big (%zu > %zu)!", len, PROFICUBE_FPGA_SIZE);
		return FPGA_FAIL;
	}

	while (bytecount < len)
	{
		uint8_t val = data[bytecount++];
		uint8_t i = 8;

		do
		{
			if (val & 0x01)
			{
				/* set data to 1 */
				atmel_pio4_set_pio_output(
					PROFICUBE_FPGA_PIN_DATA0, 1);
			}
			else
			{
				/* set data to 0 */
				atmel_pio4_set_pio_output(
					PROFICUBE_FPGA_PIN_DATA0, 0);
			}

			/* set clk to 1 */
			atmel_pio4_set_pio_output(
				PROFICUBE_FPGA_PIN_DCLK, 1);

			/* set clk to 0 */
			atmel_pio4_set_pio_output(
				PROFICUBE_FPGA_PIN_DCLK, 0);

			val >>= 1;
			i--;
		} while (i > 0);

#ifdef CONFIG_SYS_FPGA_PROG_FEEDBACK
		if (bytecount % len_40 == 0)
		{
			putc ('.');		/* let them know we are alive */
#ifdef CONFIG_SYS_FPGA_CHECK_CTRLC
			if (ctrlc ()) return FPGA_FAIL;
#endif
		}
#endif
	}

	return FPGA_SUCCESS;
}
#endif

static Altera_CYC2_Passive_Serial_fns proficube_fns = {
	.pre = NULL,
	.config = proficube_fpga_config,
	.status = proficube_fpga_status,
	.done = proficube_fpga_done,
	.write = proficube_fpga_write,
	.abort = proficube_fpga_abort,
	.post = NULL
};

static Altera_desc proficube_fpga = {
	.family = Altera_CYC2,
	.iface = passive_serial,
	.size = PROFICUBE_FPGA_SIZE,
	.iface_fns = &proficube_fns
};

void board_fpga_init(void)
{
	pr_debug("%s: called\n", __func__);

	fpga_init();
	fpga_add(fpga_altera, &proficube_fpga);
}
