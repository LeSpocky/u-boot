/*
 * © 2020 Thorsis Technologies GmbH
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <common.h>
#include <debug_uart.h>
#include <init.h>
#include <asm/io.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/atmel_pio4.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpio.h>
#include <asm/arch/sama5d2.h>
#include <asm/arch/sama5d2_smc.h>

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
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 22, ATMEL_PIO_DRVSTR_ME);	/* D0 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 23, ATMEL_PIO_DRVSTR_ME);	/* D1 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 24, ATMEL_PIO_DRVSTR_ME);	/* D2 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 25, ATMEL_PIO_DRVSTR_ME);	/* D3 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 26, ATMEL_PIO_DRVSTR_ME);	/* D4 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 27, ATMEL_PIO_DRVSTR_ME);	/* D5 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 28, ATMEL_PIO_DRVSTR_ME);	/* D6 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 29, ATMEL_PIO_DRVSTR_ME);	/* D7 */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 2, 0);	/* RE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 30, 0);	/* WE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 31, ATMEL_PIO_PUEN_MASK);	/* NCS */
	atmel_pio4_set_b_periph(AT91_PIO_PORTC, 8, ATMEL_PIO_PUEN_MASK);	/* RDY */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 0, ATMEL_PIO_PUEN_MASK);	/* ALE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 1, ATMEL_PIO_PUEN_MASK);	/* CLE */

	/* Configure SMC CS3 for NAND */
	writel(AT91_SMC_SETUP_NWE(1) | AT91_SMC_SETUP_NCS_WR(1) |
	       AT91_SMC_SETUP_NRD(1) | AT91_SMC_SETUP_NCS_RD(1),
	       &smc->cs[3].setup);
	writel(AT91_SMC_PULSE_NWE(2) | AT91_SMC_PULSE_NCS_WR(4) |
	       AT91_SMC_PULSE_NRD(2) | AT91_SMC_PULSE_NCS_RD(3),
	       &smc->cs[3].pulse);
	writel(AT91_SMC_CYCLE_NWE(6) | AT91_SMC_CYCLE_NRD(5),
	       &smc->cs[3].cycle);
	writel(AT91_SMC_TIMINGS_TCLR(2) | AT91_SMC_TIMINGS_TADL(7) |
	       AT91_SMC_TIMINGS_TAR(2)  | AT91_SMC_TIMINGS_TRR(3)   |
	       AT91_SMC_TIMINGS_TWB(7)  | AT91_SMC_TIMINGS_RBNSEL(3) |
	       AT91_SMC_TIMINGS_NFSEL(1), &smc->cs[3].timings);
	writel(AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
	       AT91_SMC_MODE_EXNW_DISABLE |
	       AT91_SMC_MODE_DBW_8 |
	       AT91_SMC_MODE_TDF_CYCLE(3),
	       &smc->cs[3].mode);
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
