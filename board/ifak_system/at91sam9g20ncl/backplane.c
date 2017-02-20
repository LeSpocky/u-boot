/*******************************************************************//**
 *  @file	backplane.c
 *
 *  @author	Norman Rädke <nra@ifak-system.com>
 *  @author	Alexander Dahl <ada@ifak-system.com>
 *
 *  Copyright 2010,2012 ifak system GmbH
 **********************************************************************/

#include "backplane.h"

/*
#include <common.h>
#include <command.h>
*/

#include <asm/io.h>

/*
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91sam9260_matrix.h>
#include <asm/arch/at91sam9_smc.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/at91_rstc.h>
#include <asm/arch/gpio.h>
*/

#define led_pin (1 << 4)

//#define DEBUG
#undef DEBUG
#ifdef DEBUG
	#define DEBUG_BP_READ_WRITE
	#define DEBUG_RANDOM_TEST
	#define DEBUG_SHORT_READ_WRITE_TEST
	#define DEBUG_PRINTF(string,args...) printf(string, ##args)
#else
	#define DEBUG_PRINTF(string,args...) do{ } while(0)
#endif

#define DEBUG_BPT_4B
//#undef DEBUG_BPT_4B

#define DEBUG_TESTS
//#undef DEBUG_TESTS

/* global vars */
BYTE byTestPattern[]	= {0x53,0xa5,0xC2,0x18,0x81,0xef,0x01};
BYTE byModuleInfo[8]	= {0,0,0,0,0,0,0,0};
BYTE byModuleCount	= 0;
BYTE g_byCmpBuffer[sizeof(byModuleInfo)][0x8000];
WORD g_wDprSize = sizeof(g_byCmpBuffer[0]);
/* TODO	warning: large integer implicitly truncated to unsigned type */
WORD g_wDprStartAddr = BP_RAM_START_ADDR;

uint16_t random_startwert = 0x0AA;

u8 atoi( uchar *string ) {
	u8 res = 0;
	while (*string >= '0' && *string <= '9')
	{
		res *= 10;
		res += *string - '0';
		string++;
	}
	return res;
}

uint8_t rand( void ) {
	//FIXME-SME        static uint16_t startwert=0x0AA;

	uint16_t temp;
	uint8_t n;

	for (n = 1; n < 8; n++)
	{
		temp = random_startwert;
		random_startwert = random_startwert << 1;

		temp ^= random_startwert;
		if ((temp & 0x4000) == 0x4000)
			random_startwert |= 1;
	}

	return (random_startwert);
}

int msleep( int time ) {
	ulong start = get_timer(0);
	ulong delay;

	delay = time * CONFIG_SYS_HZ;

	while (get_timer(start) < delay)
	{
		//printf("while(%d < %d);\n", get_timer(start), delay);
		if (ctrlc())
		{
			printf("Backplane sleep error\n!");
			return (-1);
		}
		udelay(100);
	}
	//printf("Sleep end for %d ms.", time);
	return 0;
}

BYTE BpRead( DWORD ulAddr ) {
#ifdef DEBUG_BP_READ_WRITE
	printf("BpRead at address: 0x%.8X \n", (ulAddr + BP_BASE_ADDR));
#endif

#if BP_READ_WAIT_CYCLES > 0
	{ int i; for (i=0; i<BP_READ_WAIT_CYCLES; i++) asm("nop"); }
#endif
	return readb(ulAddr + BP_BASE_ADDR);
}

void BpWrite( DWORD ulAddr, BYTE byData ) {
#ifdef DEBUG_BP_READ_WRITE
	printf("BpWrite 0x%.2X at address: 0x%.8X \n", byData, (ulAddr + BP_BASE_ADDR));
#endif
	writeb(byData, (ulAddr + BP_BASE_ADDR));

#if BP_WRITE_WAIT_CYCLES > 0
	{ int i; for (i=0; i<BP_WRITE_WAIT_CYCLES; i++) asm("nop"); }
#endif
}

BYTE init_modul( BYTE byModuleNr ) {
	DWORD i;

	printf("Init module %d ...", byModuleNr);
	for (i=0x0; i<g_wDprSize; i++) {
		g_byCmpBuffer[byModuleNr][i] = 0xFF & rand();
		BpWrite(i+byModuleNr*0x10000,g_byCmpBuffer[byModuleNr][i]);
	}
	printf("done\n\r");

	return 0;
}

int do_bp_scan( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	int i,n;
	BOOL bModuleFound;

	byModuleCount = 0;

	DEBUG_PRINTF("disable BP bus \n");
	//msleep(BP_WAIT_DELAY);
	writeb(0, BP_ENABLE_REG_ADDR);
	msleep(BP_WAIT_DELAY);

	DEBUG_PRINTF("enable BP bus \n");
	writeb(1, BP_ENABLE_REG_ADDR);
	msleep(BP_WAIT_DELAY*2);

	for (i=0; i<sizeof(byModuleInfo); i++)
		byModuleInfo[i] = 0;

	DEBUG_PRINTF("enable modules \n");
	for (i=0; i<sizeof(byModuleInfo); i++)
	{
		bModuleFound = TRUE;
		BYTE byRead[sizeof(byTestPattern)];

		writeb(0x38 + i, BP_CMD_MOD_REG_ADDR );	/* Modul aktivieren */

#ifdef DEBUG
		printf("Write test pattern: ");
		for (n=0; n<sizeof(byTestPattern);n++)
			printf("0x%.2X ", byTestPattern[n]);
		printf("\n");
#endif
		for (n=0; n<sizeof(byTestPattern); n++)
			BpWrite(i * 0x10000 + n, byTestPattern[n]);

		for (n=0; n<sizeof(byRead);n++)
			byRead[n] = 0;

		for (n=0; n<sizeof(byTestPattern);n++)
		{
			byRead[n] = BpRead(i * 0x10000 + n);

			if(byRead[n] != byTestPattern[n])
			{
				bModuleFound=FALSE;
//				break;
			}
		}
#ifdef DEBUG
		printf("Read test pattern: ");
		for (n=0; n<sizeof(byTestPattern);n++)
			printf("0x%.2X ", byRead[n]);
		printf("\n");
#endif

		if (bModuleFound)
		{
			byModuleInfo[i] = BpRead(0xFFFF + i * 0x10000);

			writeb(0x80, BP_PORT_REG_ADDR ); /* Im Port-Register Bp_Next_Enable-Bit setzen */
			writeb(0x30 + i, BP_CMD_MOD_REG_ADDR ); /* SetPort-Kommando an das Modul senden */
			msleep(BP_WAIT_DELAY*2);
		}
		else
		{
			break;
		}
	}

	byModuleCount = i;

	for (i=0; i<sizeof(byModuleInfo); i++)
	{
		if (byModuleInfo[i] != 0)
		{
			printf("module type 0x%.2X",byModuleInfo[i]);
			printf(" at 0x%X-0x%X\n\r",BP_RAM_START_ADDR + i*0x10000,BP_RAM_START_ADDR + i*0x10000 + 0xFFFF);
		}
		else
		{
			if (i==0)
				printf("No connected modules found\n\r");
			break;
		}
	}

	return  0;
}

int do_bp_reset_random_generator( cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[] )
{
	random_startwert = 0x0AA;

	return  0;
}

int do_bp_test( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	DWORD i, n;
	BYTE byData;
	DWORD ulRdCnt = 0;
	DWORD ulWrCnt = 0;
	DWORD ulRdCntM = 0;
	DWORD ulWrCntM = 0;
	WORD uiLoopCnt = 0;

	/* power supply pcb led on */
	//writel(led_pin, SYS_OUTPUTSET);

	if(byModuleCount == 0) {
		printf("No connected modules found \n");
		printf ("Usage: bps or bpmod [number]\n");

		/* power supply pcb led off */
		//writel(led_pin, SYS_OUTPUTCLR);

		return 1;
	}

#ifdef DEBUG
	printf("test random function \n");
	for (n=0; n<64; n++)	{
		if ((n & 0xF) == 0 )
			printf("rand() =");

		printf(" %i", rand());

		if ((n & 0xF) == 0xF)
			printf("\n");
	}
	printf("\n");
#endif

	printf("starting random backplane test\n\r");

	for (n=0; n<byModuleCount; n++)	{
		printf("Init module %lu ...",n);
		for (i=0x0; i<g_wDprSize; i++) {
			g_byCmpBuffer[n][i] = 0xFF & rand();
		}
		memcpy( (volatile unsigned char *) (n * 0x10000 + BP_BASE_ADDR),
				&g_byCmpBuffer[n][0], g_wDprSize );
		printf("done\n\r");
	}

	for (n=0; n<byModuleCount; n++) {
		printf("Check init module %lu ...",n);
		for (i=0x0; i<g_wDprSize; i++) {
			byData = BpRead(i+n*0x10000);
			if (g_byCmpBuffer[n][i] != byData) {
				printf("Difference found\n\r");
				printf("0x%.2X read, 0x%.2X expected at addr 0x%.2lX", byData, g_byCmpBuffer[n][i], i+n*0x10000 );
				printf(" 2nd read->(0x%.2X)",BpRead(i+n*0x10000));
				printf("\n\r");
				break;
			}
		}
		if (i == g_wDprSize) {
			printf("done\n\r");
		}
		else {
			printf("Initialization failed\n\r");
			return(1);
		}
	}

	do {
		do {
			n=(rand() % byModuleCount );
			{
				WORD uiStartAddr ;
				WORD uiNumberOfBytes;

				uiStartAddr = rand() % g_wDprSize;
				uiNumberOfBytes = (rand()+1) % 256;

				if ((uiStartAddr + uiNumberOfBytes) > g_wDprSize) {
					uiNumberOfBytes = g_wDprSize - uiStartAddr;
				}

				if ((rand() & 1) ) {
					//write
					ulWrCnt += uiNumberOfBytes;
					for(i=0; i<uiNumberOfBytes; i++) {
						g_byCmpBuffer[n][uiStartAddr+i] = 0xFF & rand();
					}
					memcpy( (volatile unsigned char *) (uiStartAddr + n * 0x10000 + BP_BASE_ADDR),
							&g_byCmpBuffer[n][uiStartAddr], uiNumberOfBytes );
#ifdef DEBUG_RANDOM_TEST
					printf("  WE:%.7X...%.7X\n\r",uiStartAddr+n*0x10000,uiStartAddr+i+n*0x10000-1);
#endif
				}
				else {
					//read
					ulRdCnt += uiNumberOfBytes;
#ifdef DEBUG_RANDOM_TEST
					printf("  RD:%.7X...%.7X\n\r",uiStartAddr+n*0x10000,uiStartAddr+i+n*0x10000-1);
#endif
					for(i=0; i<uiNumberOfBytes; i++)
					{
						byData=BpRead(uiStartAddr+i+n*0x10000);
						if (g_byCmpBuffer[n][uiStartAddr+i] != byData) {
							BYTE byData2;

							/* power supply pcb led off */
							//writel(led_pin, SYS_OUTPUTCLR);

							ulWrCntM += ulWrCnt >> 20;
							ulRdCntM += ulRdCnt >> 20;

							ulWrCnt = ulWrCnt % 0xfffff;
							ulRdCnt = ulRdCnt % 0xfffff;


							//second read
							byData2=BpRead(uiStartAddr+i+n*0x10000);
							printf("\n\r");
							printf("Difference found\n\r");
							printf("%ld MB and %ld byte written ", ulWrCntM, ulWrCnt);
							printf("%ld MB and %ld byte read  \n\r", ulRdCntM, ulRdCnt);

							printf("i=%lu, 0x%.2X read, 0x%.2X expected at addr 0x%.5lX", i, byData, g_byCmpBuffer[n][uiStartAddr+i], uiStartAddr+i+n*0x10000);
							printf(" 2nd read->(0x%.2X)",byData2);
							printf("\n\r");

							return 1;
						}
					}
				}
				uiLoopCnt ++;
				if (uiLoopCnt >= 1000) {
					ulWrCntM += ulWrCnt >> 20;
					ulRdCntM += ulRdCnt >> 20;

					ulWrCnt = ulWrCnt % 0xfffff;
					ulRdCnt = ulRdCnt % 0xfffff;

					printf("\rinfo(%lu): ",n);
					printf("%ld MB and %ld byte written ", ulWrCntM, ulWrCnt);
					printf("%ld MB and %ld byte read  \r", ulRdCntM, ulRdCnt);
					uiLoopCnt=0;
				}
			}
		} while(uiLoopCnt);

	} while (! ctrlc());

	printf("\n");
	printf("random backplane test stopped \n");

	/* power supply pcb led off */
	//writel(led_pin, SYS_OUTPUTCLR);

	return  0;
}

int do_bp_test_short_read_write( cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[] )
{
	DWORD i;
	BYTE byData;
	BYTE byModule;
	WORD uiAddr = (WORD)0x69E3;
	DWORD ulRdCnt = 0;
	DWORD ulRdCntM = 0;
	WORD uiLoopCnt = 0;

	switch (argc) {
	case 2:
		byModule = atoi((uchar *)argv[1]);
		break;
	default:
		printf("Usage:\n%s\n", cmdtp->help);
		return 1;
	}

	if ((byModule + 1) > byModuleCount) {
		printf("Module not connected \n");
		printf("Usage: bps or bpmod [number]\n");
		return 1;
	}


	/* power supply pcb led on */
	//writel(led_pin, SYS_OUTPUTSET);

	if(byModuleCount == 0) {
		printf("No connected modules found \n");
		printf("Usage: bps or bpmod [number]\n");

		/* power supply pcb led off */
		//writel(led_pin, SYS_OUTPUTCLR);

		return 1;
	}

	printf("Init module %d ...", byModule);
	for (i=0x0; i<g_wDprSize; i++) {
		g_byCmpBuffer[byModule][i] = 0xFF & rand();
		BpWrite(i+byModule*0x10000,g_byCmpBuffer[byModule][i]);
	}
	printf("done\n\r");

	printf("Check init module %d...", byModule);
	for (i=0x0; i<g_wDprSize; i++) {
		byData = BpRead(i+byModule*0x10000);
		if (g_byCmpBuffer[byModule][i] != byData) {
			printf("Difference found\n\r");
			printf("0x%.2X read, 0x%.2X expected at addr 0x%.2lX",
					byData, g_byCmpBuffer[byModule][i], i+byModule*0x10000);
			printf(" 2nd read->(0x%.2X)",BpRead(i+byModule*0x10000));
			printf("\n\r");
			break;
		}
	}
	if (i == g_wDprSize) {
		printf("done\n\r");
	}
	else {
		printf("Initialization failed\n\r");

		/* power supply pcb led off */
		//writel(led_pin, SYS_OUTPUTCLR);

		return(1);
	}

	do {
		do {
			ulRdCnt++;

			g_byCmpBuffer[byModule][uiAddr] = 0xFF & rand();

#ifdef DEBUG_SHORT_READ_WRITE_TEST
			printf("ADR: 0x%.5X ", uiAddr+byModule*0x10000);
#endif /* DEBUG_SHORT_READ_WRITE_TEST */
			BpWrite(uiAddr+byModule*0x10000 ,g_byCmpBuffer[byModule][uiAddr]);
#ifdef DEBUG_SHORT_READ_WRITE_TEST
			printf("WRD: 0x%.2X ", g_byCmpBuffer[byModule][uiAddr]);
#endif /* DEBUG_SHORT_READ_WRITE_TEST */
			byData=BpRead(uiAddr+byModule*0x10000);
#ifdef DEBUG_SHORT_READ_WRITE_TEST
			printf("RDD: 0x%.2X\n", byData);
#endif /* DEBUG_SHORT_READ_WRITE_TEST */

			if (g_byCmpBuffer[byModule][uiAddr] != byData) {
				BYTE byData2;

				/* power supply pcb led off */
				//writel(led_pin, SYS_OUTPUTCLR);

				//second read
				byData2=BpRead(uiAddr+byModule*0x10000);

				printf("\n\r");
				printf("Difference found\n\r");
				printf("0x%.2X read, 0x%.2X expected at addr 0x%.5X",
						byData, g_byCmpBuffer[byModule][uiAddr], uiAddr+byModule*0x10000);
				printf(" 2nd read->(0x%.2X)",byData2);
				printf("\n\r");

				return 1;
			}

			uiLoopCnt ++;
			if (uiLoopCnt >= 10000) {
				ulRdCntM += ulRdCnt >> 20;

				ulRdCnt = ulRdCnt % 0xfffff;

				printf("\rinfo: ");
				printf("%ld MB and %ld byte written/read  \r", ulRdCntM, ulRdCnt);
				uiLoopCnt=0;
			}

		} while(uiLoopCnt);
	} while (! ctrlc());

	printf("\n");
	printf("backplane short write/read test stopped \n");

	/* power supply pcb led off */
	//writel(led_pin, SYS_OUTPUTCLR);

	return  0;
}

#ifdef DEBUG_BPT_4B
int do_bp_test_4byte_write_read( cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[] )
{
	DWORD i, n;
	BYTE byData;
	DWORD ulRdCnt = 0;
	DWORD ulWrCnt = 0;
	DWORD ulRdCntM = 0;
	DWORD ulWrCntM = 0;
	WORD uiLoopCnt = 0;

	/* power supply pcb led on */
	//writel(led_pin, SYS_OUTPUTSET);

	if(byModuleCount == 0) {
		printf("No connected modules found \n");
		printf ("Usage: bps or bpmod [number]\n");

		/* power supply pcb led off */
		//writel(led_pin, SYS_OUTPUTCLR);

		return 1;
	}

	printf("starting random backplane test\n\r");

#if 0
	for (n=0; n<byModuleCount; n++)	{
		printf("Init module %d ...",n);
		for (i=0x0; i<g_wDprSize; i++) {
			g_byCmpBuffer[n][i] = 0xFF & rand();
			BpWrite(i+n*0x10000,g_byCmpBuffer[n][i]);
		}
		printf("done\n\r");
	}

	for (n=0; n<byModuleCount; n++) {
		printf("Check init module %d...",n);
		for (i=0x0; i<g_wDprSize; i++) {
			byData = BpRead(i+n*0x10000);
			if (g_byCmpBuffer[n][i] != byData) {
				printf("Difference found\n\r");
				printf("0x%.2X read, 0x%.2X expected at addr 0x%.2X", byData, g_byCmpBuffer[n][i], i+n*0x10000);
				printf(" 2nd read->(0x%.2X)",BpRead(i+n*0x10000));
				printf("\n\r");
				break;
			}
		}
		if (i == g_wDprSize) {
			printf("done\n\r");
		}
		else {
			printf("Initialization failed\n\r");
			return(1);
		}
	}
#endif

	do {
		do {
			n=(rand() % byModuleCount );
			{
				WORD uiStartAddr ;
				WORD uiNumberOfBytes;

				uiStartAddr = rand() % g_wDprSize;
				uiNumberOfBytes = 4 % 256;

				if ((uiStartAddr + uiNumberOfBytes) > g_wDprSize) {
					uiNumberOfBytes = g_wDprSize - uiStartAddr;
				}

				//write
				ulWrCnt += uiNumberOfBytes;
				for(i=0; i<uiNumberOfBytes; i++) {
					g_byCmpBuffer[n][uiStartAddr+i] = 0xFF & rand();
					BpWrite(uiStartAddr+i+n*0x10000,g_byCmpBuffer[n][uiStartAddr+i]);
				}
#ifdef DEBUG_RANDOM_TEST
				printf("  WE:%.7X...%.7X\n\r",uiStartAddr+n*0x10000,uiStartAddr+i+n*0x10000-1);
#endif


				//read
				ulRdCnt += uiNumberOfBytes;
#ifdef DEBUG_RANDOM_TEST
				printf("  RD:%.7X...%.7X\n\r",uiStartAddr+n*0x10000,uiStartAddr+i+n*0x10000-1);
#endif
				for(i=0; i<uiNumberOfBytes; i++)
				{
					byData=BpRead(uiStartAddr+i+n*0x10000);
					if (g_byCmpBuffer[n][uiStartAddr+i] != byData) {
						BYTE byData2;

						/* power supply pcb led off */
						//writel(led_pin, SYS_OUTPUTCLR);

						ulWrCntM += ulWrCnt >> 20;
						ulRdCntM += ulRdCnt >> 20;

						ulWrCnt = ulWrCnt % 0xfffff;
						ulRdCnt = ulRdCnt % 0xfffff;

						//second read
						byData2=BpRead(uiStartAddr+i+n*0x10000);
						printf("\n\r");
						printf("Difference found\n\r");
						printf("%ld MB and %ld byte written ", ulWrCntM, ulWrCnt);
						printf("%ld MB and %ld byte read  \n\r", ulRdCntM, ulRdCnt);

						printf("i=%lu, 0x%.2X read, 0x%.2X expected at addr 0x%.5lX", i, byData, g_byCmpBuffer[n][uiStartAddr+i], uiStartAddr+i+n*0x10000);
						printf(" 2nd read->(0x%.2X)",byData2);
						printf("\n\r");

						return 1;
					}
				}

				uiLoopCnt ++;
				if (uiLoopCnt >= 1000) {
					ulWrCntM += ulWrCnt >> 20;
					ulRdCntM += ulRdCnt >> 20;

					ulWrCnt = ulWrCnt % 0xfffff;
					ulRdCnt = ulRdCnt % 0xfffff;

					printf("\rinfo(%lu): ",n);
					printf("%ld MB and %ld byte written ", ulWrCntM, ulWrCnt);
					printf("%ld MB and %ld byte read  \r", ulRdCntM, ulRdCnt);
					uiLoopCnt=0;
				}
			}
		} while(uiLoopCnt);

	} while (! ctrlc());

	printf("\n");
	printf("random backplane test stopped \n");

	/* power supply pcb led off */
	//writel(led_pin, SYS_OUTPUTCLR);

	return  0;
}
#endif /* DEBUG_BPT_4B */

int do_bp_debug( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	int i,n;

	if(byModuleCount == 0) {
		printf("No connected modules found \n");
		printf ("Usage: bps or bpmod [number]\n");
		return 1;
	}


	for (i=0; i<byModuleCount; i++)
	{
		BYTE byRead[sizeof(byTestPattern)];

		printf("Write test pattern: ");
		for (n=0; n<sizeof(byTestPattern);n++)
		{
			printf("0x%.2X ", byTestPattern[n]);
		}
		printf("\n");

		for (n=0; n<sizeof(byRead);n++)
			byRead[n] = 0;

		BpWrite(0x12345, 0x67);

#ifdef DEBUG
		for (n=0; n<sizeof(byTestPattern);n++)
			BpWrite(i * 0x10000 + n, byTestPattern[n]);
#else
		BpWrite(i * 0x10000 + 0, byTestPattern[0]);
		BpWrite(i * 0x10000 + 1, byTestPattern[1]);
		BpWrite(i * 0x10000 + 2, byTestPattern[2]);
		BpWrite(i * 0x10000 + 3, byTestPattern[3]);
		BpWrite(i * 0x10000 + 4, byTestPattern[4]);
		BpWrite(i * 0x10000 + 5, byTestPattern[5]);
		BpWrite(i * 0x10000 + 6, byTestPattern[6]);
#endif

		BpWrite(0x12345, 0x67);

#if 1
		for (n=0; n<sizeof(byTestPattern);n++)
			byRead[n] = BpRead(i * 0x10000 + n);
#else
		byRead[0] = BpRead(i * 0x10000 + 0);
		byRead[1] = BpRead(i * 0x10000 + 1);
		byRead[2] = BpRead(i * 0x10000 + 2);
		byRead[3] = BpRead(i * 0x10000 + 3);
		byRead[4] = BpRead(i * 0x10000 + 4);
		byRead[5] = BpRead(i * 0x10000 + 5);
		byRead[6] = BpRead(i * 0x10000 + 6);
#endif
		printf("Read test pattern: ");
		for (n=0; n<sizeof(byTestPattern);n++)
			printf("0x%.2X ", byRead[n]);

		printf("\n");

	}

	return  0;
}

int do_bp_mod( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	switch (argc) {
	case 2:			/* modul number */
		byModuleCount = atoi((uchar *)argv[1]);
		/* FALL TROUGH */
	case 1:			/* get status */
		printf ("byModuleCount = %i\n", byModuleCount);
		return 0;
	default:
		printf ("Usage:\n%s\n", cmdtp->help);
		return 1;
	}

	return  0;
}

int do_bp_dpr_size( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[] ) {
	switch (argc) {
	case 2:			/* modul number */
		g_wDprSize = simple_strtoul((char *)argv[1], NULL, 16);
		/* FALL TROUGH */
	case 1:			/* get status */
		printf ("g_wDprSize = %X\n", g_wDprSize);
		return 0;
	default:
		printf ("Usage:\n%s\n", cmdtp->help);
		return 1;
	}

	return  0;
}

/**
 *  @deprecated	not used
 */
int do_bp_dpr_startaddress( cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[] )
{
	switch (argc) {
	case 2:			/* modul number */
		g_wDprStartAddr = simple_strtoul((char *)argv[1], NULL, 16);
		/* FALL TROUGH */
	case 1:			/* get status */
		printf ("g_wDprStartAddr = %X\n", g_wDprStartAddr);
		return 0;
	default:
		printf ("Usage:\n%s\n", cmdtp->help);
		return 1;
	}

	return  0;
}

#ifdef DEBUG
int do_bp_debug_random_function( cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[] )
{
	int n;
	printf("test random function \n");
	for (n=0; n<64; n++)	{
		if ((n & 0xF) == 0 )
			printf("rand() =");

		printf(" %i", rand());

		if ((n & 0xF) == 0xF)
			printf("\n");
	}
	printf("\n");

	return  0;
}
#endif

//defined in hrttest.c
/*int hrt_test( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] ){

}*/

U_BOOT_CMD(
	bprrg, 1, 1, do_bp_reset_random_generator,
	"reset backplane random generator",
	"reset the random generator for backplane tests"
);


U_BOOT_CMD(
	bps, 1, 1, do_bp_scan,
	"backplane scan",
	"scan backplane bus for connected modules"
);


U_BOOT_CMD(
	bpt, 1, 1, do_bp_test,
	"backplane test",
	"random test of DPR via backplane"
);


U_BOOT_CMD(
	bpmod, 2, 1, do_bp_mod,
	"backplane modules ",
	"[number]\n"
	"backplane get or set number of connected modules"
);


U_BOOT_CMD(
	bpdpr, 2, 1, do_bp_dpr_size,
	"DP-RAM size",
	"[size]\n"
	"get or set the DP-RAM size of the backplane bus"
);


U_BOOT_CMD(
	bpadd, 2, 1, do_bp_dpr_startaddress,
	"DP-RAM start address",
	"[address]\n"
	"get or set the DP-RAM startaddress of the backplane bus"
);


U_BOOT_CMD(
	bpt_swr, 2, 1, do_bp_test_short_read_write,
	"backplane test short write/read",
	"[number]\n"
	"short address offset write/read test of DPR via backplane"
);


#ifdef DEBUG_BPT_4B
U_BOOT_CMD(
	bpt_4b, 1, 1, do_bp_test_4byte_write_read,
	"backplane test write 4 random Bytes and verify them",
	"4 random Byte write/read test of DPR via backplane"
);
#endif /* DEBUG_BPT_4B */


#ifdef DEBUG
U_BOOT_CMD(
	bpd_rf, 1, 1, do_bp_debug_random_function,
	"backplane debug random function",
	"debug random function (only for internal use)"
);
#endif

#ifdef DEBUG
U_BOOT_CMD(
	bpd, 1, 1, do_bp_debug,
	"backplane debug",
	"debug backplane bus (only for internal use)"
);
#endif

/* vim: set noet sts=0 ts=8 sw=8 sr: */
