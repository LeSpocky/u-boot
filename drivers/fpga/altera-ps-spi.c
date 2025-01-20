// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Alexander Dahl <ada@thorsis.com>
 */

#include <ACEX1K.h>
#include <altera.h>
#include <linux/bitrev.h>
#include <linux/delay.h>
#include <dm/device_compat.h>
#include <dm.h>
#include <efinix.h>
#include <errno.h>
#include <fpga.h>
#include <asm/gpio.h>
#include <log.h>
#include <malloc.h>
#include <spi.h>

struct altera_ps_spi_priv {
	struct gpio_desc	nconfig;
	struct gpio_desc	nstatus;
	struct gpio_desc	conf_done;
	Altera_desc		desc;
};

/*
 * CYC2_ps_load() wants this false, waits 100 us, wants this true.
 * Altera needs the nCONFIG line be set to low and then high again,
 * Efinix has that wired to CRESET_N and wants that line low and then
 * high again, so the pin is actually low active in both cases.
 * However CYC2_ps_load() calls ->config(false, …) first and
 * ->config(true) after, which ignores gpio subsystem handling low
 * active pins by itself (you give dm_gpio the logical value only).
 * So to be compatible to old code we need to invert the boolean here.
 */
static int altera_ps_spi_config(int assert_config, int flush, int cookie)
{
	struct udevice *dev = (struct udevice *)cookie;
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);
	int ret;

	/* low active */
	ret = dm_gpio_set_value(&priv->nconfig, !assert_config);
	return log_ret(ret) ? FPGA_FAIL : FPGA_SUCCESS;
}

/*
 * CYC2_ps_load() waits for fn->status returning 1 to proceed.
 * The line will go high, but the signal is marked as low active,
 * so we need to invert it.
 */
static int altera_ps_spi_status(int cookie)
{
	struct udevice *dev = (struct udevice *)cookie;
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->desc.family == ALTERA_FAMILY_EFINIX_TRION) {
		dev_dbg(dev, "Skipping nSTATUS read on Trion\n");
		return 1;
	}

	ret = dm_gpio_get_value(&priv->nstatus);
	if (ret < 0)
		return log_ret(ret);

	/* low active */
	return !ret;
}

static int altera_ps_spi_done(int cookie)
{
	struct udevice *dev = (struct udevice *)cookie;
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);
	int ret;

	/* SPI Passive Mode (x1) Timing Sequence suggests there are one
	 * or two SPI clock cycles between end of data and CDONE asserted. */
	if (priv->desc.family == ALTERA_FAMILY_EFINIX_TRION)
		udelay(2);

	ret = dm_gpio_get_value(&priv->conf_done);
	if (ret < 0)
		return log_ret(ret);

	return ret;
}

static int altera_ps_spi_write(const void *buf, size_t len, int flush, int cookie)
{
	struct udevice *dev = (struct udevice *)cookie;
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);
	u8 *data = NULL;
	int ret;

	if ((priv->desc.size != (size_t) -1) && (len > priv->desc.size)) {
		dev_err(dev, "FPGA image too big (%zu > %zu)!\n",
			len, priv->desc.size);
		return log_ret(-ENOSPC);
	}

	data = malloc(len);
	if (!data)
		return log_ret(-ENOMEM);

	ret = dm_spi_claim_bus(dev);
	if (ret)
		goto free_data;

	/* reverse bitstream */
	for (int i = 0; i < len; i++)
		data[i] = bitrev8(*((const u8 *)buf + i));

	// transfer data
	ret = dm_spi_xfer(dev, len * 8, data, NULL, SPI_XFER_ONCE);
	if (ret)
		dev_err(dev, "Error %d during SPI transaction\n", ret);

	dm_spi_release_bus(dev);

free_data:
	free(data);

	return log_ret(ret);
}

static int altera_ps_spi_abort(int cookie)
{
	return FPGA_SUCCESS;
}

static Altera_CYC2_Passive_Serial_fns altera_ps_spi_fns = {
	.pre = NULL,
	.config = altera_ps_spi_config,
	.status = altera_ps_spi_status,
	.done = altera_ps_spi_done,
	.write = altera_ps_spi_write,
	.abort = altera_ps_spi_abort,
	.post = NULL,
};

static int altera_ps_spi_probe(struct udevice *dev)
{
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);
	int ret;

	priv->desc.family = dev_get_driver_data(dev);
	priv->desc.iface = passive_serial;
	priv->desc.iface_fns = &altera_ps_spi_fns;
	priv->desc.base = NULL;
	priv->desc.cookie = (uintptr_t)dev;

	/* nCONFIG pin */
	ret = gpio_request_by_name(dev, "nconfig-gpios", 0, &priv->nconfig,
				   GPIOD_IS_OUT | GPIOD_ACTIVE_LOW);
	if (ret)
		return log_ret(ret);

	/* nSTATUS pin */
	if (priv->desc.family != ALTERA_FAMILY_EFINIX_TRION) {
		ret = gpio_request_by_name(dev, "nstat-gpios", 0, &priv->nstatus,
					   GPIOD_IS_IN | GPIOD_ACTIVE_LOW);
		if (ret)
			return log_ret(ret);
	}

	/* CONF_DONE pin */
	ret = gpio_request_by_name(dev, "confd-gpios", 0, &priv->conf_done,
				   GPIOD_IS_IN);
	if (ret)
		return log_ret(ret);

	fpga_add(fpga_altera, &priv->desc);

	return 0;
}

static int altera_ps_spi_of_to_plat(struct udevice *dev)
{
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);
	int ret;

	/* Altera Size */
	ret = dev_read_u32(dev, "altr,size", &priv->desc.size);
	if (ret) {
		dev_dbg(dev,
			"Property \"altr,size\" not set, driver won't check size on write!\n");
		priv->desc.size = (size_t) -1;
	}

	return 0;
}

static const struct udevice_id altera_ps_spi_match[] = {
	{
		.compatible = "altr,fpga-passive-serial",
		.data = Altera_CYC2
	},
	{
		.compatible = "efinix,fpga-passive-serial",
		.data = ALTERA_FAMILY_EFINIX_TRION
	},
	{ /* sentinel */ }
};

U_BOOT_DRIVER(altera_ps_spi) = {
	.name		= "altera-ps-spi",
	.id		= UCLASS_FPGA,
	.of_match	= altera_ps_spi_match,
	.probe		= altera_ps_spi_probe,
	.of_to_plat	= altera_ps_spi_of_to_plat,
	.priv_auto	= sizeof(struct altera_ps_spi_priv),
};
