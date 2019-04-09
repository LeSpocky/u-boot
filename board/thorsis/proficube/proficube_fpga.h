/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#ifndef PROFICUBE_FPGA
#define PROFICUBE_FPGA

#define PROFICUBE_FPGA_PIN_nCONFIG	AT91_PIO_PORTA, 8
#define PROFICUBE_FPGA_PIN_CONF_DONE	AT91_PIO_PORTA, 9
#define PROFICUBE_FPGA_PIN_nSTATUS	AT91_PIO_PORTA, 10

void board_fpga_init(void);

#endif
