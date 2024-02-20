#include <common.h>
#include <debug_uart.h>
#include <fdtdec.h>
#include <init.h>
#include <led.h>
#include <log.h>
#include <asm/arch/at91_common.h>
#include <asm/global_data.h>
#include <dm/ofnode.h>

DECLARE_GLOBAL_DATA_PTR;

static void board_leds_init(void)
{
#if CONFIG_IS_ENABLED(LED)
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
#else
	pr_warn("LED support disabled!\n");
#endif
}

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
	at91_seriald_hw_init();
}
#endif

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = gd->bd->bi_dram[0].start + 0x100;

	board_leds_init();

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
