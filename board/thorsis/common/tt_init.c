/*
 * © 2025 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <dm.h>
#include <fpga.h>
#include <init.h>
#include <linux/kconfig.h>
#include <linux/printk.h>

int misc_init_r(void)
{
	pr_debug("%s: entered\n", __func__);

	if (IS_ENABLED(CONFIG_FPGA)) {
		fpga_init();

		if (IS_ENABLED(CONFIG_DM_FPGA)) {
			struct udevice *udev;
			int ret;

			ret = uclass_get_device(UCLASS_FPGA, 0, &udev);
			if (ret)
				pr_err("Error (%d) finding FPGA!\n", ret);
		}
	}

	pr_debug("%s: leaving\n", __func__);
	return 0;
}
