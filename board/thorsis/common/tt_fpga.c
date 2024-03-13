/*
 * © 2024 Thorsis Technologies GmbH
 */

#define LOG_CATEGORY UCLASS_FPGA

#include <fpga.h>
#include <log.h>

int tt_fpga_abort(int cookie)
{
	log_debug("%s: called\n", __func__);

	return FPGA_SUCCESS;
}
