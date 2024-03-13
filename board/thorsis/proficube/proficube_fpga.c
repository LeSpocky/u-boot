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

#define LOG_CATEGORY UCLASS_FPGA

#include <common.h>
#include <asm/arch/atmel_pio4.h>
#include <ACEX1K.h>
#include <altera.h>
#include <console.h>
#include <dm.h>
#include <efinix.h>
#include <env.h>
#include <errno.h>
#include <fpga.h>
#include <log.h>
#include <malloc.h>

#include "../common/tt_fpga.h"
#include "proficube_fpga.h"

struct proficube_fpga_info {
	const char	*fpgaimg;
	Altera_desc	*fpgadesc;
};

static Altera_desc *proficube_get_fpga_desc(void);

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

/**
 * Set state of nCONFIG pin.
 */
static int proficube_fpga_config(int assert_config, int flush, int cookie)
{
	log_debug("%s: called\n", __func__);

	if (assert_config) {
		log_debug("Setting nCONFIG to HIGH.\n");
		atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_nCONFIG, 1);
	} else {
		log_debug("Setting nCONFIG to LOW.\n");
		atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_nCONFIG, 0);
	}

	return FPGA_SUCCESS;
}

/**
 * Read state of CONF_DONE pin.
 */
static int proficube_fpga_done(int cookie)
{
	log_debug("%s: called\n", __func__);

	return atmel_pio4_get_pio_input(PROFICUBE_FPGA_PIN_CONF_DONE);
}

/**
 * Read state of nSTATUS pin.
 */
static int proficube_fpga_status(int cookie)
{
	log_debug("%s: called\n", __func__);

	return atmel_pio4_get_pio_input(PROFICUBE_FPGA_PIN_nSTATUS);
}

#if PROFICUBE_USE_SPI
static int proficube_fpga_write(const void *buf, size_t len, int flush, int cookie)
{
	struct spi_slave *slave;
	Altera_desc *fpga_desc;
	uint8_t *data = NULL;
	int ret = 0;
	size_t i;

	log_debug("%s: called (%s)\n", __func__,
		  PROFICUBE_USE_SPI ? "spi" : "gpio");

	fpga_desc = proficube_get_fpga_desc();
	if (fpga_desc) {
		if (len > fpga_desc->size) {
			log_err("FPGA image too big (%zu > %zu)!\n",
				len, fpga_desc->size);
			return FPGA_FAIL;
		}
	} else {
		log_err("Could not determine FPGA type!\n");
		return FPGA_FAIL;
	}

	data = malloc(len);
	if (!data) {
		log_err("Could not allocate memory!\n");
		return FPGA_FAIL;
	}

#ifdef CONFIG_DM_SPI
	struct udevice *dev;

	log_debug("Using SPI CS%u.\n", PROFICUBE_FPGA_SPI_CS);
	ret = _spi_get_bus_and_cs(PROFICUBE_FPGA_SPI_BUS,
				  PROFICUBE_FPGA_SPI_CS,
				  PROFICUBE_FPGA_SPI_SPEED, SPI_MODE_0,
				  "spi_generic_drv",
				  PROFICUBE_FPGA_SPI_DEV_NAME, &dev,
				  &slave);
	if (ret) {
		log_err("spi_get_bus_and_cs() failed: %d!\n", ret);
		goto err_data_allocated;
	}
#else
#error "Not supported!"
#endif

	ret = spi_claim_bus(slave);
	if (ret) {
		log_debug("spi_claim_bus(%p) failed: %d!\n", slave, ret);
		goto err_data_allocated;
	}

	for (i = 0; i < len; i++) {
		data[i] = bitrev8( *((const uint8_t *)buf + i) );
	}

	ret = spi_xfer(slave, len * 8, data, NULL, SPI_XFER_ONCE);
	if (ret) {
		log_err("Error %d during SPI transaction\n", ret);
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
	Altera_desc *fpga_desc;
	size_t bytecount = 0;

	log_debug("%s: called (%s)\n", __func__,
		  PROFICUBE_USE_SPI ? "spi" : "gpio");

	fpga_desc = proficube_get_fpga_desc();
	if (fpga_desc) {
		if (len > fpga_desc->size) {
			log_err("FPGA image too big (%zu > %zu)!\n",
				len, fpga_desc->size);
			return FPGA_FAIL;
		}
	} else {
		log_err("Could not determine FPGA type!\n");
		return FPGA_FAIL;
	}

	if (fpga_desc->family == ALTERA_FAMILY_EFINIX_TRION) {
		log_debug("Claim SPI CS%u (%x, %u)\n", PROFICUBE_FPGA_SPI_CS,
			  PROFICUBE_FPGA_PIN_SPI_CS(PROFICUBE_FPGA_SPI_CS));
		atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_SPI_CS(PROFICUBE_FPGA_SPI_CS), 1);
	}

	while (bytecount < len) {
		uint8_t val = data[bytecount++];
		uint8_t i = 8;

		do {
			switch (fpga_desc->family) {
			case ALTERA_FAMILY_EFINIX_TRION:
#if 0
				/* Unmodified board rev v1.2 had this wrong. */
				atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_ASDO,
							  (val & 0x01) ? 1 : 0);
				break;
#endif
			case Altera_CYC2:
				atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_DATA0,
							  (val & 0x01) ? 1 : 0);
				break;
			default:
				return FPGA_FAIL;
			}

			/* set clk to 1 */
			atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_DCLK, 1);

			/* set clk to 0 */
			atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_DCLK, 0);

			val >>= 1;
			i--;
		} while (i > 0);

#ifdef CONFIG_SYS_FPGA_PROG_FEEDBACK
		if (bytecount % len_40 == 0) {
			putc('.');		/* let them know we are alive */
#ifdef CONFIG_SYS_FPGA_CHECK_CTRLC
			if (ctrlc()) return FPGA_FAIL;
#endif
		}
#endif
	}

	if (fpga_desc->family == ALTERA_FAMILY_EFINIX_TRION) {
		log_debug("Release SPI CS%u (%x, %u)\n", PROFICUBE_FPGA_SPI_CS,
			  PROFICUBE_FPGA_PIN_SPI_CS(PROFICUBE_FPGA_SPI_CS));
		atmel_pio4_set_pio_output(PROFICUBE_FPGA_PIN_SPI_CS(PROFICUBE_FPGA_SPI_CS), 1);
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
	.abort = tt_fpga_abort,
	.post = NULL
};

static Altera_desc proficube_fpga_ep3c5 = {
	.family = Altera_CYC2,
	.iface = passive_serial,
	.size = Altera_EP3C5_SIZE,
	.iface_fns = &proficube_fns
};

static Altera_desc proficube_fpga_ep4ce6 = {
	.family = Altera_CYC2,
	.iface = passive_serial,
	.size = ALTERA_EP4CE6_SIZE,
	.iface_fns = &proficube_fns
};

static Altera_desc proficube_fpga_t20q144 = {
	.family = ALTERA_FAMILY_EFINIX_TRION,
	.iface = passive_serial,
	.size = EFINIX_TRION_T20Q144_SIZE,
	.iface_fns = &proficube_fns
};

static const struct proficube_fpga_info proficube_fpgas[] = {
	{
		.fpgaimg = "fpga_ep3c5.uimg",
		.fpgadesc = &proficube_fpga_ep3c5
	},
	{
		.fpgaimg = "fpga_ep4ce6.uimg",
		.fpgadesc = &proficube_fpga_ep4ce6
	},
	{
		.fpgaimg = "fpga_t20q144.uimg",
		.fpgadesc = &proficube_fpga_t20q144
	},
	{
		.fpgaimg = "fpga_t20q144-v1dot4.uimg",
		.fpgadesc = &proficube_fpga_t20q144
	},
	{
		.fpgaimg = NULL,
		.fpgadesc = NULL
	}
};

void board_fpga_init(void)
{
	Altera_desc *fpga_desc;

	log_debug("%s: called\n", __func__);

	fpga_desc = proficube_get_fpga_desc();
	if (fpga_desc) {
		fpga_init();
		fpga_add(fpga_altera, fpga_desc);
	} else {
		log_err("Could not determine FPGA type!\n");
	}
}

Altera_desc *proficube_get_fpga_desc(void)
{
	const struct proficube_fpga_info *p;
	char *fpgaimg;

	/*
	 * If the env variable is missing we assume the default FPGA
	 * type, the one we started with in the previous hardware
	 * revisions.
	 * We do the same in our U-Boot scripts.
	 */
	fpgaimg = env_get("fpgaimg");
	if (!fpgaimg)
		fpgaimg = "fpga_ep4ce6.uimg";

	for (p = proficube_fpgas; p->fpgaimg; p++) {
		if (!strcmp(fpgaimg, p->fpgaimg))
			return p->fpgadesc;
	}

	return NULL;
}
