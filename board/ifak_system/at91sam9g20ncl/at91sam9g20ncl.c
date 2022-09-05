/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian@popies.net>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * © 2010 ifak system GmbH
 * © 2019 Thorsis Technologies GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/at91sam9260_matrix.h>
#include <asm/arch/at91sam9_smc.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpio.h>

#if defined(CONFIG_RESET_PHY_R) && defined(CONFIG_MACB)
# include <net.h>
#endif
#include <netdev.h>

#include "button.h"
#include <status_led.h>

DECLARE_GLOBAL_DATA_PTR;

/* ------------------------------------------------------------------------- */
/*
 * Miscelaneous platform dependent initialisations
 */

#ifdef CONFIG_FPGA
int ncl_fpga_init( void );
#endif

#ifdef CONFIG_CMD_NAND
static void at91sam9g20ncl_nand_hw_init(void)
{
	struct at91_smc *smc = (struct at91_smc *)ATMEL_BASE_SMC;
	struct at91_matrix *matrix = (struct at91_matrix *)ATMEL_BASE_MATRIX;
	unsigned long csa;

	/* Assign CS3 to NAND/SmartMedia Interface */
	csa = readl(&matrix->ebicsa);
	csa |= AT91_MATRIX_CS3A_SMC_SMARTMEDIA;
	writel(csa, &matrix->ebicsa);

	/* Configure SMC CS3 for NAND/SmartMedia */
	writel(AT91_SMC_SETUP_NWE(1) | AT91_SMC_SETUP_NCS_WR(0) |
		AT91_SMC_SETUP_NRD(1) | AT91_SMC_SETUP_NCS_RD(0),
		&smc->cs[3].setup);
	writel(AT91_SMC_PULSE_NWE(3) | AT91_SMC_PULSE_NCS_WR(3) |
		AT91_SMC_PULSE_NRD(3) | AT91_SMC_PULSE_NCS_RD(3),
		&smc->cs[3].pulse);
	writel(AT91_SMC_CYCLE_NWE(5) | AT91_SMC_CYCLE_NRD(5),
		&smc->cs[3].cycle);
	writel(AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
		AT91_SMC_MODE_EXNW_DISABLE |
#ifdef CONFIG_SYS_NAND_DBW_16
		AT91_SMC_MODE_DBW_16 |
#else /* CONFIG_SYS_NAND_DBW_8 */
		AT91_SMC_MODE_DBW_8 |
#endif
		AT91_SMC_MODE_TDF_CYCLE(2),
		&smc->cs[3].mode);

	/* Configure RDY/BSY */
	at91_set_gpio_input(CONFIG_SYS_NAND_READY_PIN, 1);

	/* Enable NandFlash */
	at91_set_gpio_output(CONFIG_SYS_NAND_ENABLE_PIN, 1);
}
#endif

#ifdef CONFIG_NCL_REG_SET
static void at91sam9g20ncl_reg_hw_init(void)
{
	struct at91_smc *smc = (struct at91_smc *) ATMEL_BASE_SMC;

	/*
	 *	Set the PIO Register
	 *
	 *	AT91SAM9G20 hardware manual says:
	 *
	 *	> The pins used for interfacing the Static Memory Controller may
	 *	> be multiplexed with the PIO lines. The programmer must first
	 *	> program the PIO controller to assign the Static Memory
	 *	> Controller pins to their peripheral function. If I/O Lines of
	 *	> the SMC are not used by the application, they can be used for
	 *	> other purposes by the PIO Controller.
	 */
	at91_set_a_periph( AT91_PIO_PORTC, 9, 1 );	/*	enable CS5		*/
	at91_set_b_periph( AT91_PIO_PORTC, 12, 1 );	/*	enable CS7		*/
	at91_set_a_periph( AT91_PIO_PORTC, 15, 1 );	/*	enable NWAIT	*/

	/* Configure SMC CS5 for BP_RAM, Timing ... */
	writel( AT91_SMC_SETUP_NWE(0x01) | AT91_SMC_SETUP_NCS_WR(0x01) |
			AT91_SMC_SETUP_NRD(0x01) | AT91_SMC_SETUP_NCS_RD(0x01),
			&smc->cs[5].setup );

	// e.g. SMC Pulse Register on offset address (0x0454): 0x6F010403
	writel( AT91_SMC_PULSE_NWE(0x08) | AT91_SMC_PULSE_NCS_WR(0x08) |
			AT91_SMC_PULSE_NRD(0x08) | AT91_SMC_PULSE_NCS_RD(0x08),
			&smc->cs[5].pulse );

	writel( AT91_SMC_CYCLE_NWE(0x0B) | AT91_SMC_CYCLE_NRD(0x0B),
			&smc->cs[5].cycle );

	writel( AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
			AT91_SMC_MODE_EXNW_READY | AT91_SMC_MODE_DBW_8 |
			AT91_SMC_MODE_PS_4, &smc->cs[5].mode );

	/* Configure SMC CS7 for BP_CTRL, Timing ... */
	writel( AT91_SMC_SETUP_NWE(0x0) | AT91_SMC_SETUP_NCS_WR(0x0) |
			AT91_SMC_SETUP_NRD(0x0) | AT91_SMC_SETUP_NCS_RD(0x0),
			&smc->cs[7].setup );

	writel( AT91_SMC_PULSE_NWE(0x09) | AT91_SMC_PULSE_NCS_WR(0x09) |
			AT91_SMC_PULSE_NRD(0x09) | AT91_SMC_PULSE_NCS_RD(0x09),
			&smc->cs[7].pulse );

	writel( AT91_SMC_CYCLE_NWE(0x0A) | AT91_SMC_CYCLE_NRD(0x0A),
			&smc->cs[7].cycle );

	writel( AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
			AT91_SMC_MODE_EXNW_READY | AT91_SMC_MODE_DBW_8 |
			AT91_SMC_MODE_PS_4, &smc->cs[7].mode );
}
#endif

#ifdef CONFIG_MACB
static void at91sam9g20ncl_macb_hw_init(void)
{
	struct at91_port *pioa = (struct at91_port *)ATMEL_BASE_PIOA;

	/*
	 * Disable pull-up on:
	 *	RXDV (PA17) => PHY normal mode (not Test mode)
	 *	ERX0 (PA14) => PHY ADDR0
	 *	ERX1 (PA15) => PHY ADDR1
	 *	ERX2 (PA25) => PHY ADDR2
	 *	ERX3 (PA26) => PHY ADDR3
	 *	ECRS (PA28) => PHY ADDR4  => PHYADDR = 0x0
	 *
	 * PHY has internal pull-down
	 */
	writel(pin_to_mask(AT91_PIN_PA14) |
		pin_to_mask(AT91_PIN_PA15) |
		pin_to_mask(AT91_PIN_PA17) |
		pin_to_mask(AT91_PIN_PA25) |
		pin_to_mask(AT91_PIN_PA26) |
		pin_to_mask(AT91_PIN_PA28),
		&pioa->pudr);

	at91_phy_reset();

	/* Re-enable pull-up */
	writel(pin_to_mask(AT91_PIN_PA14) |
		pin_to_mask(AT91_PIN_PA15) |
		pin_to_mask(AT91_PIN_PA17) |
		pin_to_mask(AT91_PIN_PA25) |
		pin_to_mask(AT91_PIN_PA26) |
		pin_to_mask(AT91_PIN_PA28),
		&pioa->puer);

	/* Initialize EMAC=MACB hardware */
	at91_macb_hw_init();
}
#endif

int board_early_init_f(void)
{
	at91_periph_clk_enable(ATMEL_ID_PIOA);
	at91_periph_clk_enable(ATMEL_ID_PIOB);
	at91_periph_clk_enable(ATMEL_ID_PIOC);

	at91_seriald_hw_init();

	return 0;
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

#ifdef CONFIG_CMD_NAND
	at91sam9g20ncl_nand_hw_init();
#endif

#ifdef CONFIG_AT91_LED
	coloured_LED_init();	/* init LED */
#endif

#ifdef CONFIG_TT_NCL_BUTTON
	ncl_button_init();	/* init BUTTON */
#endif

#ifdef CONFIG_HAS_DATAFLASH
	at91_spi0_hw_init((1 << 0) | (1 << 1));
#endif
#ifdef CONFIG_MACB
	at91sam9g20ncl_macb_hw_init();
#endif

#ifdef CONFIG_FPGA
	ncl_fpga_init();
#endif

#ifdef CONFIG_NCL_REG_SET
	at91sam9g20ncl_reg_hw_init();
#endif

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size(
		(void *)CONFIG_SYS_SDRAM_BASE,
		CONFIG_SYS_SDRAM_SIZE);
	return 0;
}

#ifdef CONFIG_RESET_PHY_R
void reset_phy(void)
{
}
#endif

int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_MACB
	rc = macb_eth_initialize(0, (void *)ATMEL_BASE_EMAC0, 0x00);
#endif
	return rc;
}

/* vim: set noet sts=0 ts=8 sw=8 sr: */
