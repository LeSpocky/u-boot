// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 Alexander Dahl <post@lespocky.de>
 */

#include <dm.h>

U_BOOT_DRIVER(sandbox_fpga) = {
	.name	= "sandbox_fpga",
	.id	= UCLASS_FPGA,
};
