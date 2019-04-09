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
#include <ACEX1K.h>
#include <altera.h>
#include <dm.h>
#include <errno.h>
#include <fpga.h>
#include <spi.h>

#include "proficube_fpga.h"

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

static int proficube_fpga_write(const void *buf, size_t len, int flush, int cookie)
{
	struct spi_slave *slave;
	int ret = 0;

	pr_debug("%s: called\n", __func__);

#ifdef CONFIG_DM_SPI
	struct udevice *dev;
	ret = spi_get_bus_and_cs(PROFICUBE_FPGA_SPI_BUS,
				 PROFICUBE_FPGA_SPI_CS, 0, SPI_MODE_0,
				 "spi_generic_drv",
				 PROFICUBE_FPGA_SPI_DEV_NAME, &dev,
				 &slave);
	if (ret)
	{
		pr_err("spi_get_bus_and_cs() failed: %d!\n", ret);
		return ret;
	}
#else
	slave = spi_setup_slave(PROFICUBE_FPGA_SPI_BUS,
				PROFICUBE_FPGA_SPI_CS, 0, SPI_MODE_0);
	if (!slave)
	{
		pr_err("Invalid device %d:%d\n", bus, cs);
		return -EINVAL;
	}
#endif

	ret = spi_claim_bus(slave);
	if (ret)
	{
		pr_debug("spi_claim_bus(%p) failed: %d!\n", slave, ret);
		goto done;
	}

	/* TODO	Call spi_xfer() here! */
	pr_debug("Would call spi_xfer() now!\n");
	ret = -ENOSYS;

	if(ret)
	{
		pr_err("Error %d during SPI transaction\n", ret);
	}

done:
	spi_release_bus(slave);
#ifndef CONFIG_DM_SPI
	spi_free_slave(slave);
#endif

	return ret;
}

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
	.size = ALTERA_EP4CE6_SIZE,
	.iface_fns = &proficube_fns
};

void board_fpga_init(void)
{
	pr_debug("%s: called\n", __func__);

	fpga_init();
	fpga_add(fpga_altera, &proficube_fpga);
}
