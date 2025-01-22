/*
 * © 2024 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <config.h>
#include <debug_uart.h>
#include <dm.h>
#include <fdtdec.h>
#include <fpga.h>
#include <init.h>
#include <led.h>
#include <asm/arch/at91_common.h>
#include <asm/global_data.h>
#include <linux/kconfig.h>
#include <linux/printk.h>

DECLARE_GLOBAL_DATA_PTR;

static void board_leds_init(void)
{
	if (IS_ENABLED(CONFIG_LED)) {
		const char *led_name;
		struct udevice *dev;
		int ret;

		led_name = ofnode_conf_read_str("u-boot,boot-led");
		if (!led_name)
			return;

		ret = led_get_by_label(led_name, &dev);
		if (ret)
			return;

		led_set_state(dev, LEDST_ON);
	} else {
		pr_warn("LED support disabled!\n");
	}
}

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
	at91_seriald_hw_init();
}
#endif

int board_init(void)
{
	pr_debug("%s: entered\n", __func__);

	/* address of boot parameters */
	gd->bd->bi_boot_params = gd->bd->bi_dram[0].start + 0x100;

	board_leds_init();

	pr_debug("%s: leaving\n", __func__);
	return 0;
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

int dram_init(void)
{
	return fdtdec_setup_mem_size_base();
}

#ifdef CONFIG_MISC_INIT_R
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
#endif
