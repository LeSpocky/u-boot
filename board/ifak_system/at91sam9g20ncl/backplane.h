/*******************************************************************//**
 *  @file	backplane.c
 *
 *  @author	Norman Rädke <nra@ifak-system.com>
 *  @author	Alexander Dahl <ada@ifak-system.com>
 *
 *  Copyright 2010,2012 ifak system GmbH
 **********************************************************************/
#ifndef	_BACKPLANE_H_
#define	_BACKPLANE_H_

#include <common.h>
#include <asm/types.h>

typedef unsigned char BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#define FALSE	0
#define TRUE	(~FALSE)

#define BP_BASE_ADDR		0x60000000
#define BP_RAM_START_ADDR	BP_BASE_ADDR
#define BP_CMD_MOD_REG_ADDR	0x80000000
#define BP_PORT_REG_ADDR	(BP_CMD_MOD_REG_ADDR + 1)
#define BP_ENABLE_REG_ADDR	(BP_CMD_MOD_REG_ADDR + 3)

//#define BP_WAIT_DELAY 100
#define BP_WAIT_DELAY		1

#define BP_READ_WAIT_CYCLES	0
#define BP_WRITE_WAIT_CYCLES	0

u8 atoi( uchar *string );

uint8_t rand( void );

int msleep( int time );

BYTE init_modul( BYTE byModuleNr );

BYTE BpRead( DWORD ulAddr );

void BpWrite( DWORD ulAddr, BYTE byData );

#endif /* _BACKPLANE_H_ */

/* vim: set noet sts=0 ts=8 sw=8 sr: */
