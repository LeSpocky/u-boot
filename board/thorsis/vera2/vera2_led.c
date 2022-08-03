/*
 * © 2021 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <command.h>
#include <led.h>
#include <linux/delay.h>

#include "../common/tt_led.h"

static int tt_ledtest(void)
{
	const char *labels[2] = { "green:status", "green:debug" };
	struct udevice *leds[2];
	int ec, i;

	for (i = 0; i < 2; i++) {
		ec = led_get_by_label(labels[i], &leds[i]);
		if (ec != 0) {
			pr_err("Error (%d) getting LED '%s'!\n", ec, labels[i]);
			return ec;
		}
	}

	for (i = 0; i < 2; i++) {
		led_set_state(leds[i], LEDST_ON);
	}

	mdelay(TT_LED_DELAY);

	for (i = 0; i < 2; i++) {
		led_set_state(leds[i], LEDST_OFF);
	}

	mdelay(TT_LED_DELAY);

	return 0;
}

int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[])
{
	int ec;

	ec = tt_ledtest();
	if (ec != 0) {
		pr_err("Error (%d) on first LED test!\n", ec);
		return CMD_RET_FAILURE;
	}

	ec = tt_ledtest();
	if (ec != 0) {
		pr_err("Error (%d) on second LED test!\n", ec);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}
