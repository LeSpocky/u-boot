/*
 * © 2021 Thorsis Technologies GmbH
 */

#include <asm/io.h>
#include <asm/arch/atmel_pio4.h>
#include <asm/arch/sama5d2_smc.h>

void tt_nand_configure_pio_datarw(void)
{
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
}

void tt_nand_configure_pio_misc(void)
{
	atmel_pio4_set_b_periph(AT91_PIO_PORTA, 31, ATMEL_PIO_PUEN_MASK);	/* NCS */
	atmel_pio4_set_b_periph(AT91_PIO_PORTC, 8, ATMEL_PIO_PUEN_MASK);	/* RDY */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 0, ATMEL_PIO_PUEN_MASK);	/* ALE */
	atmel_pio4_set_b_periph(AT91_PIO_PORTB, 1, ATMEL_PIO_PUEN_MASK);	/* CLE */
}

void tt_nand_configure_smc(struct at91_smc *smc)
{
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
