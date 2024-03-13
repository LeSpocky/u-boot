/*
 * Copyright 2019 Thorsis Technologies GmbH
 */

#ifndef PROFICUBE_FPGA
#define PROFICUBE_FPGA

#define PROFICUBE_FPGA_PIN_nCONFIG	AT91_PIO_PORTA, 8
#define PROFICUBE_FPGA_PIN_CONF_DONE	AT91_PIO_PORTA, 9
#define PROFICUBE_FPGA_PIN_nSTATUS	AT91_PIO_PORTA, 10
#define PROFICUBE_FPGA_PIN_DCLK		AT91_PIO_PORTA, 14
#define PROFICUBE_FPGA_PIN_DATA0	AT91_PIO_PORTA, 15
#define PROFICUBE_FPGA_PIN_ASDO		AT91_PIO_PORTA, 16
#define PROFICUBE_FPGA_PIN_SPI_CS0	AT91_PIO_PORTA, 17
#define PROFICUBE_FPGA_PIN_SPI_CS1	AT91_PIO_PORTA, 18

#define PROFICUBE_USE_SPI		1

#define PROFICUBE_FPGA_SPI_BUS		0
/*
 * SPI controller needs a CS number when calling spi_get_bus_and_cs().
 * This is not stricly necessary for the hardware on boards with Altera
 * Cyclone which ignore SPI CS and need/use nCE instead (which is set in
 * board setup code).
 * Boards with Efinix Trion however need a correctly setup SPI interface
 * with the right CS line.
 */
#define PROFICUBE_FPGA_SPI_CS		0
#define PROFICUBE_FPGA_SPI_DEV_NAME	"fpga@0"
#define PROFICUBE_FPGA_SPI_SPEED	50000000 /* from 83 MHz / 255 = 325490 Hz to 83 MHz */

#define _PROFICUBE_FPGA_PIN_SPI_CS(x)	PROFICUBE_FPGA_PIN_SPI_CS##x
#define PROFICUBE_FPGA_PIN_SPI_CS(x)	_PROFICUBE_FPGA_PIN_SPI_CS(x)

#endif
