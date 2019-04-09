/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <asm/arch/atmel_pio4.h>
#include <ACEX1K.h>
#include <altera.h>
#include <fpga.h>

#include "proficube_fpga.h"

static int proficube_fpga_abort(int cookie)
{
	pr_debug("%s: called\n", __func__);

	return 0;
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

	return 0;
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
	pr_debug("%s: called\n", __func__);

	return -ENOSYS;
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
