/*
 * © 2021 Thorsis Technologies GmbH
 */

#ifndef TT_NAND
#define TT_NAND

/** Configure data, NRD, and NWE lines (also needed for NAND!) */
void tt_nand_configure_pio_datarw(void);

void tt_nand_configure_pio_misc(void);

/** Configure SMC CS3 for NAND */
void tt_nand_configure_smc(struct at91_smc *smc);

#endif /* TT_NAND */
