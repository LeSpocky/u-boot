/*
 * © 2022 Thorsis Technologies GmbH
 */

#define LOG_CATEGORY LOGC_BOARD

#include <command.h>
#include <dm.h>
#include <led.h>
#include <log.h>
#include <linux/delay.h>

#include "../common/tt_led.h"

#define TT_LED_LABEL	"status"

static int tt_ledtest(void)
{
	struct udevice *led;
	int ec;

	ec = led_get_by_label(TT_LED_LABEL, &led);
	if (ec != 0) {
		log_err("Error (%d) getting LED '%s'!\n", ec, TT_LED_LABEL);
		return ec;
	}

	led_set_state(led, LEDST_OFF);

	mdelay(TT_LED_DELAY);

	led_set_state(led, LEDST_ON);

	mdelay(TT_LED_DELAY);

	return 0;
}

int do_tt_ledtest(struct cmd_tbl *cmdtp, int flag,
		  int argc, char * const argv[])
{
	int ec;

	ec = tt_ledtest();
	if (ec != 0) {
		log_err("Error (%d) on first LED test!\n", ec);
		return CMD_RET_FAILURE;
	}

	ec = tt_ledtest();
	if (ec != 0) {
		log_err("Error (%d) on second LED test!\n", ec);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}
