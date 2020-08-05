/*
 * © 2020 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <command.h>
#include <led.h>
#include <linux/delay.h>
#include <vsprintf.h>

#include "../common/tt_led.h"

static int tt_ledtest_color(const char *color)
{
	struct udevice *led[4];
	int ec, i;

	if (!color)
		return -EINVAL;

	for (i = 0; i < 4; i++) {
		char buf[30];

		snprintf(buf, sizeof(buf), "%s:indicator-%d", color, i + 1);

		ec = led_get_by_label(buf, &led[i]);
		if (ec != 0) {
			pr_err("Error (%d) getting LED '%s'!\n", ec, buf);
			return ec;
		}
	}

	for (i = 0; i < 4; i++) {
		led_set_state(led[i], LEDST_ON);
	}

	mdelay(TT_LED_DELAY);

	for (i = 0; i < 4; i++) {
		led_set_state(led[i], LEDST_OFF);
	}

	mdelay(TT_LED_DELAY);

	return 0;
}

int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[])
{
	int ec;

	ec = tt_ledtest_color("green");
	if (ec != 0) {
		pr_err("Error (%d) testing %s LEDs!\n", ec, "green");
		return CMD_RET_FAILURE;
	}

	ec = tt_ledtest_color("red");
	if (ec != 0) {
		pr_err("Error (%d) testing %s LEDs!\n", ec, "red");
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}
