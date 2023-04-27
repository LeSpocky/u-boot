/*
 * © 2020 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <debug_uart.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/atmel_pio4.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpio.h>
#include <asm/arch/sama5d2.h>
#include <asm/arch/sama5d2_smc.h>

#ifdef CONFIG_NAND_ATMEL
#include "../common/tt_nand.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_NAND_ATMEL
static void board_nand_hw_init(void)
{
	struct at91_smc *smc = (struct at91_smc *)ATMEL_BASE_SMC;

	pr_debug("%s: called\n", __func__);

	at91_periph_clk_enable(ATMEL_ID_HSMC);

	/*
	 * > 36.7.1 I/O Lines
	 * > The pins used for interfacing the Static Memory Controller are
	 * > multiplexed with the PIO lines. The programmer must first
	 * > program the PIO controller to assign the Static Memory
	 * > Controller pins to their peripheral function. If I/O lines of
	 * > the SMC are not used by the application, they can be used for
	 * > other purposes by the PIO controller.
	 */

	/* Configure data, NRD, and NWE lines (also needed for NAND!) */
	tt_nand_configure_pio_datarw();

	tt_nand_configure_pio_misc();

	/* Configure SMC CS3 for NAND */
	tt_nand_configure_smc(smc);
}
#endif

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
static void board_uart1_hw_init(void)
{
	atmel_pio4_set_a_periph(AT91_PIO_PORTD, 2, ATMEL_PIO_PUEN_MASK);	/* URXD1 */
	atmel_pio4_set_a_periph(AT91_PIO_PORTD, 3, 0);				/* UTXD1 */

	at91_periph_clk_enable(ATMEL_ID_UART1);
}

void board_debug_uart_init(void)
{
	board_uart1_hw_init();
}
#endif

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
#ifdef CONFIG_DEBUG_UART
	debug_uart_init();
#endif

	return 0;
}
#endif

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

#ifdef CONFIG_NAND_ATMEL
	board_nand_hw_init();
#endif

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				    CONFIG_SYS_SDRAM_SIZE);
	return 0;
}
