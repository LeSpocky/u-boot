/*******************************************************************//**
 *	@file	hrttest.c
 *
 *	@author	Norman Rädke <nra@ifak-system.com>
 *	@author	Alexander Dahl <ada@thorsis.com>
 *
 *	@copyright	2010 ifak system GmbH
 *	@copyright	2019 Thorsis Technologies GmbH
 **********************************************************************/

#include <command.h>
#include <console.h>
#include <stdio.h>

/*
#include <asm/arch/at91sam9260_matrix.h>
#include <asm/arch/at91sam9_smc.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/at91_rstc.h>
#include <asm/arch/gpio.h>
*/

#include <linux/delay.h>

#include "backplane.h"

#define DPRAM_CHANNEL0		0x0100
#define DPRAM_CHANNEL_SIZE	0x0200
#define DPRAM_BUFFER_SIZE	0x00FF
#define DPRAM_SEM_OFFSET	0x00FF
#define DPRAM_RESP_OFFSET	0x0100

#define HART_NO_ERROR			0	/* success */
#define HART_INVALID_PARAMETER		4
#define HART_STATE_ERROR		5	/* operation not allowed in current state */
#define HART_TRANSMIT_BUFFER_BUSY	7	/* device is transmitting a message */
#define HART_RECEIVE_BUFFER_EMPTY	8	/* no message received */
#define HART_BUFFER_OVERRUN		11
#define HART_DEVICE_NOT_INITIALIZED	12
#define HART_INVALID_REQUEST		22

#define UDELAY(us)	udelay(us)

#define ESC		27
#define cls()		printf("%c[2J",ESC)
#define cursor(a,b)	printf("%c[%d;%dH",ESC,a,b)
#define CHANNEL_DY		6
#define TDEL_INIT		1000000

static void DPRAM_WRITE( DWORD saddr, BYTE *buf, int len ) {
	int i;
	for(i=0;i<len;i++){
		BpWrite(saddr++,*buf++);
	}
}

static void DPRAM_READ( DWORD saddr, BYTE *buf, int len ) {
	int i;
	for(i=0;i<len;i++){
		*buf++ = BpRead(saddr++);
	}
}

static BYTE hrttest_setRequest( int module, BYTE channel, BYTE *req, int len ) {
	DWORD semaddr=0;
	DWORD chaddr=0;
	BYTE sem=0;
	int t=1000;

	//calculate semaphore address
	chaddr = DPRAM_CHANNEL0 + channel*DPRAM_CHANNEL_SIZE + 0x10000*(module-1);
	semaddr = chaddr + DPRAM_SEM_OFFSET;
	//read request semaphore
	//valid data?
	//printf("\nRead ReqSem:");
	while((sem & 0x1)&(t)){
		DPRAM_READ(semaddr,&sem,1);
	//	printf(".");
		UDELAY(1000);
		t--;
	}
	if(t==0)return 0;
	//printf(" ok\n");
	//set request data
	DPRAM_WRITE(chaddr,req,len);
	//set semaphore
	sem=1;
	DPRAM_WRITE(semaddr,&sem,1);
	return 1;
}

static BYTE hrttest_getResponse( int module, BYTE channel, BYTE *resp, int len ) {
	DWORD semaddr=0;
	DWORD chaddr=0;
	int t=1000;
	BYTE sem=1;

	chaddr = DPRAM_CHANNEL0 + channel*DPRAM_CHANNEL_SIZE + DPRAM_RESP_OFFSET + 0x10000*(module-1);
	semaddr = chaddr + DPRAM_SEM_OFFSET;
	//read semaphore
	//printf("\r\nRead RespSem:");
	while((!sem)&(t)){
		DPRAM_READ(semaddr,&sem,1);
		//printf(".");
		UDELAY(1000);
		t--;
	}
	if(t==0)return 0;
	//printf("ok\n");
	//get response data
	DPRAM_READ(chaddr,resp,len);
	//clear response semaphore
	sem=0;
	DPRAM_WRITE(semaddr,&sem,1);
	return 1;
}

static BYTE hrttest_initChannel( int module, BYTE channel, BYTE pmaster ) {
	BYTE req[2];
	BYTE resp=0;

	req[0] = 0xE0;
	req[1] = pmaster;

	//set request
	if(hrttest_setRequest(module,channel,req,2)){
		if(hrttest_getResponse(module,channel,&resp,1)){
		return resp;
		}
	}
	return 0xFF;
}

static BYTE hrttest_closeChannel( int module, BYTE channel ) {
	BYTE req[2];
	BYTE resp;

	req[0] = 0xE1;
	if(hrttest_setRequest(module,channel,req,1)){
		if(hrttest_getResponse(module,channel,&resp,1)){
		//printf("CloseChannel = %d",(int)resp);
		return resp;
		}//else{printf("ERROR: Response\n");}
	}//else {printf("ERROR: Request");}
	return 0xFF;
}

static int hrttest_send( int module, BYTE channel, BYTE *hrtMessage, int len ) {
	BYTE req[255];
	BYTE resp;
	int i;
	if(len > 252) len = 252;
	req[0] = 0xE2;
	req[1] = 10; //# preambles
	req[2] = len;
	for(i=0;i<len;i++){
		req[i+3]=hrtMessage[i];
	}
	//printf("Send to channel %d:\n",channel);
	if(hrttest_setRequest(module,channel,req,len+3)){
		if(hrttest_getResponse(module,channel,&resp,1)){
		//printf("Retval=%d\n",(int)resp);
		return resp;
		}//else{printf("ERROR: Response\n");}
	}//else {printf("ERROR: Request");}
	return 0xFF;
}

static BYTE hrttest_receive( int module, BYTE channel, BYTE *hrtMessage ) {
	BYTE req = 0xE3;
	BYTE resp[255]={0};
	int i;

	if(hrttest_setRequest(module,channel,&req,1)){
		if(hrttest_getResponse(module,channel,resp,255)){
		if(resp[0] == 0){
			for(i=0;i<resp[2]+2;i++){
			hrtMessage[i]=resp[i+1];
			}
		}
		return resp[0];
		}//else{printf("ERROR: Response\n");}
	}//else {printf("ERROR: Request");}
	return 0xFF;
}

static int hrttest_setParam( void ) {
	return 0;
}

static int hrttest_getParam( void ) {
	return 0;
}

int do_hrt_test(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[] ) {
	int i=0;
	int module=0;
	BYTE channel=0;
	BYTE message0[5]={0x02,0x80,0x00,0x00,0x82};//
	BYTE message1[9]={0x82, 0x96, 0x08, 0x0B, 0x9E, 0x28, 0x0D, 0x00, 0xAC};//
	BYTE response[255]={0};
	/* int t=5; */
	int state[4]={-1,-1,-1,-1};
	DWORD  tdelay[4]={TDEL_INIT,TDEL_INIT,TDEL_INIT,TDEL_INIT};
	BYTE err;
	/* BYTE key=0; */

	switch (argc) {
	case 2:
		module = atoi((uchar *)argv[1]);
		break;
	default:
		printf("Usage:\n%s\n", cmdtp->help);
		return 1;
	}
	if(module <= 0){printf("ERROR: Module <= 0\n"); return 0;}

	cls();

	disable_ctrlc (0);	/* 1 to disable, 0 to enable Control-C detect */
	clear_ctrlc();	/* clear the Control-C condition */

	while(!had_ctrlc()){ /* have we had a Control-C since last clear? */
//        key=getc();
		for(channel=0;channel<4;channel++){
			switch(state[channel]){
			case -1:
				cursor(channel*CHANNEL_DY+1,1);
				printf("###################### CHANNEL %d #######################\r\n",channel);
				printf("STATUS:\r\n");
				printf("Init:\r\nSend:\r\nReceive:\r\nSlave:");
				state[channel]=0;
				break;
			case 0://init channel
				err=hrttest_initChannel(module,channel,1);
				if(err == HART_NO_ERROR){
					cursor(channel*CHANNEL_DY+3,10);
					printf("ok          ");
					state[channel] = 1;
				}else{
					cursor(channel*CHANNEL_DY+3,10);
					printf("ERROR %d  ",err);
				}
			break;
			case 1://send telegramm0
				err=hrttest_send(module, channel,message0,5);
				if(err == 0){
					cursor(channel*CHANNEL_DY+4,10);
					printf("ok          ");
					state[channel]=2;
				}else{
					cursor(channel*CHANNEL_DY+4,10);
					printf("ERROR %d  ",err);
				}
			break;
			case 2://receive telegramm
				err=hrttest_receive(module, channel,response);
				if(err == 0){//telegramm received
					cursor(channel*CHANNEL_DY+5,10);
					printf("ok          ");
					cursor(channel*CHANNEL_DY+6,10);
					printf("                                                              \r");
					cursor(channel*CHANNEL_DY+6,10);
					for(i=0;i<response[1]+2;i++){
						printf("%x ",response[i]);
					}
					state[channel]=3;
				}else{
					cursor(channel*CHANNEL_DY+5,10);
					printf("ERROR %d  ",err);
				}
				break;
			case 3:
				tdelay[channel]--;
				if(tdelay[channel] <= 0){
					tdelay[channel]=TDEL_INIT;
					state[channel]=4;
				}
				break;
			case 4://send telegramm1
				err=hrttest_send(module, channel,message1,9);
				if(err == 0){
					cursor(channel*CHANNEL_DY+4,10);
					printf("ok          ");
					state[channel]=5;
				}else{
					cursor(channel*CHANNEL_DY+4,10);
					printf("ERROR %d  ",err);
				}
				break;
			case 5://receive telegramm
				err = hrttest_receive(module, channel, response);
				if (err == 0)
				{//telegramm received
					cursor(channel*CHANNEL_DY+5,10);
					printf("ok          ");
					cursor(channel*CHANNEL_DY+6,10);
					printf( "                                                              \r");
					cursor(channel*CHANNEL_DY+6,10);
					for (i = 0; i < response[1] + 2; i++)
					{
						printf("%x ", response[i]);
					}
					state[channel] = 6;
				}
				else
				{
					cursor(channel*CHANNEL_DY+5,10);
					printf("ERROR %d  ", err);
				}
				break;
			case 6:
				tdelay[channel]--;
				if (tdelay[channel] <= 0)
				{
					tdelay[channel] = TDEL_INIT;
					state[channel] = 1;
				}
				break;
			default:break;
			}//switch
		}//for
	}//while

	clear_ctrlc();	/* clear the Control-C condition */
	return 1;
}

U_BOOT_CMD(
	hrttest, 2, 1, do_hrt_test,
	"Hart Request test",
	"hrttest moduleNr "
);

/* vim: set noet sts=0 ts=8 sw=8 sr: */
