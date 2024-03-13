/*
 * © 2024 Thorsis Technologies GmbH
 */

#define LOG_CATEGORY LOGC_BOARD

#include <ACEX1K.h>
#include <altera.h>
#include <efinix.h>
#include <fpga.h>
#include <linux/errno.h>
#include <log.h>

#include "../common/tt_fpga.h"

static int hartcube_fpga_config(int assert_config, int flush, int cookie)
{
	log_debug("%s: entered\n", __func__);
	return log_ret(-ENOSYS);
}

static int hartcube_fpga_status(int cookie)
{
	log_debug("%s: entered\n", __func__);
	return log_ret(-ENOSYS);
}

static int hartcube_fpga_done(int cookie)
{
	log_debug("%s: entered\n", __func__);
	return log_ret(-ENOSYS);
}

static int hartcube_fpga_write(const void *buf, size_t len, int flush, int cookie)
{
	log_debug("%s: entered\n", __func__);
	return log_ret(-ENOSYS);
}

static Altera_CYC2_Passive_Serial_fns hartcube_fns = {
	.pre = NULL,
	.config = hartcube_fpga_config,
	.status = hartcube_fpga_status,
	.done = hartcube_fpga_done,
	.write = hartcube_fpga_write,
	.abort = tt_fpga_abort,
	.post = NULL
};

static Altera_desc hartcube_fpga_t20q144 = {
	.family = ALTERA_FAMILY_EFINIX_TRION,
	.iface = passive_serial,
	.size = EFINIX_TRION_T20Q144_SIZE,
	.iface_fns = &hartcube_fns
};

void board_fpga_init(void)
{
	log_debug("%s: entered\n", __func__);

	fpga_init();
	fpga_add(fpga_altera, &hartcube_fpga_t20q144);

	log_debug("%s: leaving\n", __func__);
}
