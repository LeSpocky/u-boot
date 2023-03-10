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

static int tt_ledtest(void)
{
	const char *labels[9] = { "green:fault", "green:rx",
							  "green:status", "green:tx",
							  "red:fault", "red:rx",
							  "red:status", "red:tx", "red:debug" };
	struct udevice *leds[9];
	int ec, i;

	for (i = 0; i < 9; i++) {
		ec = led_get_by_label(labels[i], &leds[i]);
		if (ec != 0) {
			pr_err("Error (%d) getting LED '%s'!\n", ec, labels[i]);
			return ec;
		}
	}

	/* green leds */
	for (i = 0; i < 4; i++)
		led_set_state(leds[i], LEDST_ON);

	mdelay(TT_LED_DELAY);

	for (i = 0; i < 4; i++)
		led_set_state(leds[i], LEDST_OFF);

	mdelay(TT_LED_DELAY);

	/* red leds */
	for (i = 4; i < 9; i++)
		led_set_state(leds[i], LEDST_ON);

	mdelay(TT_LED_DELAY);

	for (i = 4; i < 9; i++)
		led_set_state(leds[i], LEDST_OFF);

	mdelay(TT_LED_DELAY);

	return 0;
}

int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[])
{
	int ec;

	ec = tt_ledtest();
	if (ec != 0) {
		pr_err("Error (%d) testing LEDs!\n", ec);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}
