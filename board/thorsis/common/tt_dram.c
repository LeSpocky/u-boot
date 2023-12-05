/*
 * © 2023 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/arch/sama5d2.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

int tt_dram_init_sama5_sip(void)
{
	unsigned int extension_id;
	long size;

	pr_debug("%s: entered\n", __func__);

	extension_id = get_extension_chip_id();
	switch (extension_id) {
	case ARCH_EXID_SAMA5D27C_D5M:
		size = SZ_64M;
		break;
	case ARCH_EXID_SAMA5D27C_D1G:
		size = SZ_128M;
		break;
	default:
		size = SZ_64M;
		break;
	}

	pr_debug("exid: 0x%08x\n", extension_id);
	pr_debug("size: 0x%08lx (%ld)\n", size, size);
	pr_debug("CFG_SYS_SDRAM_BASE: %p\n", (void *) CFG_SYS_SDRAM_BASE);

	gd->ram_size = get_ram_size((void *)CFG_SYS_SDRAM_BASE, size);

	pr_debug("gd->ram_size: 0x%08lx (%lu)\n", gd->ram_size, gd->ram_size);

	pr_debug("%s: leaving\n", __func__);

	return 0;
}
