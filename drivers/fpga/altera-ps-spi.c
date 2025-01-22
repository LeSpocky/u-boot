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
#include <linux/string.h>

struct altera_ps_spi_caps {
	enum altera_family	family;
	size_t			dummy_bytes;
};

struct altera_ps_spi_priv {
	struct gpio_desc	*nconfig;
	struct gpio_desc	*nstatus;
	struct gpio_desc	*conf_done;
	struct gpio_desc	*nce;
	Altera_desc		desc;
	size_t			dummy_bytes;
};

static int altera_ps_spi_pre(int cookie)
{
	struct udevice *dev = (struct udevice *)cookie;
	struct altera_ps_spi_priv *priv = dev_get_priv(dev);

	if (priv->nce) {
		int ret;

		ret = dm_gpio_set_value(priv->nce, 1);
		return log_ret(ret) ? FPGA_FAIL : FPGA_SUCCESS;
	}

	return FPGA_SUCCESS;
}

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
	ret = dm_gpio_set_value(priv->nconfig, !assert_config);
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

	if (!priv->nstatus) {
		dev_dbg(dev, "Optional nSTATUS pin not defined\n");
		return 1;
	}

	/*
	 * nSTATUS pin might be defined on Trion nonetheless, but we
	 * don't query it for FPGA configuration here.
	 */
	if (priv->desc.family == ALTERA_FAMILY_EFINIX_TRION) {
		dev_dbg(dev, "Skipping nSTATUS read on Trion\n");
		return 1;
	}

	ret = dm_gpio_get_value(priv->nstatus);
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

	/*
	 * SPI Passive Mode (x1) Timing Sequence suggests there are one
	 * or two SPI clock cycles between end of data and CDONE asserted.
	 */
	if (priv->desc.family == ALTERA_FAMILY_EFINIX_TRION)
		udelay(2);

	ret = dm_gpio_get_value(priv->conf_done);
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

	if ((priv->desc.size != (size_t)-1) && len > priv->desc.size) {
		dev_err(dev, "FPGA image too big (%zu > %zu)!\n",
			len, priv->desc.size);
		return log_ret(-ENOSPC);
	}

	/*
	 * From 'AN 006: Configuring Trion FPGAs':
	 *
	 * > Important: To ensure a successful configuration,
	 * > the microprocessor must continue to supply
	 * > configuration clock to the Trion® FPGA for at least
	 * > 100 cycles after sending the last configuration data.
	 */

	data = malloc(len + priv->dummy_bytes);
	if (!data)
		return log_ret(-ENOMEM);

	if (priv->dummy_bytes > 0) {
		dev_dbg(dev,
			"Writing %zu dummy bytes to keep clock supplied.\n",
			priv->dummy_bytes);
		memset(data + len, 0xFF, priv->dummy_bytes);
	}

	ret = dm_spi_claim_bus(dev);
	if (ret)
		goto free_data;

	/* reverse bitstream */
	for (int i = 0; i < len; i++)
		data[i] = bitrev8(*((const u8 *)buf + i));

	/* transfer data */
	ret = dm_spi_xfer(dev, (len + priv->dummy_bytes) * 8, data,
			  NULL, SPI_XFER_ONCE);
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
	.pre = altera_ps_spi_pre,
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
	struct altera_ps_spi_caps *caps;

	caps = (struct altera_ps_spi_caps *)dev_get_driver_data(dev);

	priv->desc.family = caps->family;
	priv->desc.iface = passive_serial;
	priv->desc.iface_fns = &altera_ps_spi_fns;
	priv->desc.base = NULL;
	priv->desc.cookie = (uintptr_t)dev;
	priv->dummy_bytes = caps->dummy_bytes;

	/* nCONFIG pin */
	priv->nconfig = devm_gpiod_get(dev, "nconfig",
				       GPIOD_IS_OUT | GPIOD_ACTIVE_LOW);
	if (IS_ERR(priv->nconfig))
		return log_ret(PTR_ERR(priv->nconfig));

	/* nSTATUS pin */
	priv->nstatus = devm_gpiod_get_optional(dev, "nstat",
						GPIOD_IS_IN | GPIOD_ACTIVE_LOW);

	/* CONF_DONE pin */
	priv->conf_done = devm_gpiod_get(dev, "confd", GPIOD_IS_IN);
	if (IS_ERR(priv->conf_done))
		return log_ret(PTR_ERR(priv->conf_done));

	/* nCE pin */
	priv->nce = devm_gpiod_get_optional(dev, "enable",
					    GPIOD_IS_OUT | GPIOD_ACTIVE_LOW);

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
		priv->desc.size = (size_t)-1;
	}

	return 0;
}

static const struct altera_ps_spi_caps altr_caps = {
	.family = Altera_CYC2,
	.dummy_bytes = 0,
};

static const struct altera_ps_spi_caps efinix_caps = {
	.family = ALTERA_FAMILY_EFINIX_TRION,
	.dummy_bytes = 120,
};

static const struct udevice_id altera_ps_spi_match[] = {
	{
		.compatible = "altr,fpga-passive-serial",
		.data = (ulong)&altr_caps,
	},
	{
		.compatible = "efinix,fpga-passive-serial",
		.data = (ulong)&efinix_caps,
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
