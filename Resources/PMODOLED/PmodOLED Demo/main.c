/************************************************************************/
/*																		*/
/*	main.c	--	PmodOled Library Demo Program							*/
/*																		*/
/************************************************************************/
/*	Author: 	Gene Apperson											*/
/*	Copyright 2011, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/*																		*/
/*	07/06/2011(GeneA): created											*/
/*																		*/
/************************************************************************/


/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#define	CEREBOT_32MX4

#include <p32xxxx.h>
#include <plib.h>

#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

/* ------------------------------------------------------------ */
/*				PIC32 Configuration Settings					*/
/* ------------------------------------------------------------ */

/* Oscillator Settings
*/
#pragma config FNOSC		= PRIPLL	// Oscillator selection
#pragma config POSCMOD		= XT		// Primary oscillator mode
#pragma config FPLLIDIV 	= DIV_2		// PLL input divider
#pragma config FPLLMUL		= MUL_20	// PLL multiplier
#pragma config FPLLODIV 	= DIV_1		// PLL output divider
#pragma config FPBDIV		= DIV_1		// Peripheral bus clock divider
#pragma config FSOSCEN		= OFF		// Secondary oscillator enable

/* Clock control settings
*/
#pragma config IESO			= OFF		// Internal/external clock switchover
#pragma config FCKSM		= CSDCMD	// Clock switching (CSx)/Clock monitor (CMx)
#pragma config OSCIOFNC		= OFF		// Clock output on OSCO pin enable

/* USB Settings
*/
#pragma config UPLLEN		= OFF		// USB PLL enable
#pragma config UPLLIDIV		= DIV_2		// USB PLL input divider

/* Other Peripheral Device settings
*/
#pragma config FWDTEN		= OFF		// Watchdog timer enable
#pragma config WDTPS		= PS1024	// Watchdog timer postscaler

/* Code Protection settings
*/
#pragma config CP			= OFF		// Code protection
#pragma config BWP			= OFF		// Boot flash write protect
#pragma config PWP			= OFF		// Program flash write protect

/* Debug settings
*/
#pragma config ICESEL		= ICS_PGx1	// ICE pin selection
//#pragma config DEBUG		= ON		// Debug mode select

/* ------------------------------------------------------------ */
/*				Local Type Definitions							*/
/* ------------------------------------------------------------ */

#define	fTrue	1
#define fFalse	0

#define cbPageMax	256

/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

extern BYTE	rgbOledBmp[];

BYTE rgbBmp0[] = {
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81,
	0x18, 0x24, 0x42, 0x81, 0x81, 0x42, 0x24, 0x18
};

BYTE rgbBmp1[32];

BYTE rgbUserFont[cbOledFontUser] = {
	0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E,	// 0x00
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81,	// 0x01
	0x7E, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,	// 0x02
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E,	// 0x03
	0x7E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,	// 0x04
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x7E,	// 0x05
	0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E,	// 0x06
	0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, // 0x07
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	// 0x08
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,	// 0x09
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 0x0A
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, // 0x0B
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, // 0x0C
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, // 0x0D
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,	// 0x0E
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,	// 0x0F
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x10
	0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x11
	0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x12
	0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, // 0x13
	0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, // 0x14
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, // 0x15
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,	// 0x16
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF	// 0x17
};

/* ------------------------------------------------------------ */
/*				Local Variables									*/
/* ------------------------------------------------------------ */

BYTE rgbCmd[cbPageMax];
BYTE rgbBuf[cbPageMax];

/* ------------------------------------------------------------ */
/*				Forward Declarations							*/
/* ------------------------------------------------------------ */

void	DeviceInit();
void	AppInit();
void	AppTask();

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */
/***	main
**
**	Parameters:
**		none
**
**	Return Value:
**		not meaningful
**
**	Errors:
**		none
**
**	Description:
**		Top level main program entry point
*/

int
main()
	{

	DeviceInit();
	AppInit();

	while(fTrue) {
		AppTask();
	}

}

/* ------------------------------------------------------------ */
/***	DeviceInit
**
**	Parameters:
**		none
**
**	Return Value:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Perform basic hardware initialization of the board.
*/

void
DeviceInit()
	{

	DelayInit();
	OledInit();
}

/* ------------------------------------------------------------ */
/***	AppInit
**
**	Parameters:
**		none
**
**	Return Value:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Initialize the application.
*/

void
AppInit()
	{
	char	ch;

	OledInit();
	OledClear();

	/* Define the user definable characters
	*/
	for (ch = 0; ch < 0x18; ch++) {
		OledDefUserChar(ch, &rgbUserFont[ch*cbOledChar]);
	}

}

/* ------------------------------------------------------------ */
/***	AppTask
**
**	Parameters:
**		none
**
**	Return Value:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Perform the next iteration of the embedded application
*/

void
AppTask()
	{
	int		ipag;
	int		icol;
	int		irow;
	BYTE	bVal;
	BYTE *	pb;
	int		ib;
	int		xco1;
	int		xco2;
	int		yco1;
	int		yco2;

	OledClearBuffer();

	OledSetFillPattern(OledGetStdPattern(0));
	OledSetCharUpdate(0);

	for (irow = 0; irow < crowOledMax; irow++) {
		OledClearBuffer();
		OledSetCursor(0, 0);
		OledPutString("PmodOLED");
		OledSetCursor(0, 1);
		OledPutString("OLED Display");
		OledSetCursor(0, 2);
		OledPutString("by Digilent");

		OledMoveTo(0, irow);
		OledFillRect(127,31);
		OledMoveTo(0,irow);
		OledLineTo(127,irow);
		OledUpdate();
		DelayMs(100);

	}

	DelayMs(2000);

	for (irow = crowOledMax-1; irow >= 0; irow--) {
		OledSetDrawColor(1);
		OledSetDrawMode(modOledSet);
		OledMoveTo(0,irow);
		OledLineTo(127,irow);
		OledUpdate();
		DelayMs(25);
		OledSetDrawMode(modOledXor);
		OledMoveTo(0, irow);
		OledLineTo(127, irow);
		OledUpdate();
	}

	DelayMs(1000);

	OledSetDrawColor(1);
	OledSetCharUpdate(1);
	OledSetDrawMode(modOledSet);

	for (icol = 0; icol < ccolOledMax/2; icol++) {
		OledClearBuffer();
		OledMoveTo(ccolOledMax/2 - icol, 0);
		OledLineTo(ccolOledMax/2 - icol, 31);
		OledMoveTo(ccolOledMax/2 + icol, 0);
		OledLineTo(ccolOledMax/2 + icol, 31);
		OledUpdate();
		DelayMs(10);
	}

	for (icol = ccolOledMax/2; icol >= 0; icol--) {
		OledClearBuffer();
		OledMoveTo(ccolOledMax/2 - icol, 0);
		OledLineTo(ccolOledMax/2 - icol, 31);
		OledMoveTo(ccolOledMax/2 + icol, 0);
		OledLineTo(ccolOledMax/2 + icol, 31);
		OledUpdate();
		DelayMs(10);
	}

	/* Test the OledDrawPixel function.
	** This draws diagonal lines from left to right
	*/
	OledClearBuffer();
	for (icol = 0; icol < 128; icol++) {
		if (icol < 32) {
			irow = icol;
		}
		else if (icol < 64) {
			irow = 31 - (icol & 0x1F);
		}
		else if (icol < 96) {
			irow = icol & 0x1F;
		}
		else {
			irow = 31 - (icol & 0x1F);
		}
		OledMoveTo(icol, irow);
		OledDrawPixel();
	}

	OledUpdate();
	DelayMs(3000);

	/* Test the OledLineTo function
	*/
	OledClearBuffer();
	for (icol = 0; icol < 128; icol += 16) {
		OledMoveTo(ccolOledMax/2, crowOledMax/2);
		OledLineTo(icol, 0);
	}

	for (irow = 0; irow < 32; irow += 8) {
		OledMoveTo(ccolOledMax/2, crowOledMax/2);
		OledLineTo(127, irow);
	}

	for (icol = 0; icol < 128; icol += 16) {
		OledMoveTo(ccolOledMax/2, crowOledMax/2);
		OledLineTo(icol, 31);
	}

	for (irow = 0; irow < 32; irow += 8) {
		OledMoveTo(ccolOledMax/2, crowOledMax/2);
		OledLineTo(0, irow);
	}

	OledUpdate();
	DelayMs(3000);

	/* Test the OledFillRect function
	*/
	for (ib = 1; ib < 8; ib++) {
		OledClearBuffer();
	
		OledSetFillPattern(OledGetStdPattern(ib));
		OledMoveTo(5, 1);
		OledFillRect(25, 5);
		OledDrawRect(25,5);
		OledUpdate();
	
		OledMoveTo(30, 1);
		OledFillRect(50,10);
		OledDrawRect(50,10);
		OledUpdate();
	
		OledMoveTo(55, 1);
		OledFillRect(75, 27);
		OledDrawRect(75,27);
		OledUpdate();
	
		OledMoveTo(80, 8);
		OledFillRect(105, 23);
		OledDrawRect(105,23);
		OledUpdate();

		DelayMs(1000);
	}

	/* Test the OledDrawRect Function
	*/
	xco1 = 5;
	yco1 = 5;

	xco2 = 122;
	yco2 = 26;

	while (xco1 < xco2) {
		OledClearBuffer();
	
		OledMoveTo(xco1, yco1);
		OledDrawRect(xco1+15, yco1+5);
	
		OledMoveTo(xco2, yco1);
		OledDrawRect(xco2-15, yco1+5);
	
		OledMoveTo(xco1, yco2);
		OledDrawRect(xco1+15, yco2-5);
	
		OledMoveTo(xco2, yco2);
		OledDrawRect(xco2-15, yco2-5);
	
		OledUpdate();
		DelayMs(10);

		xco1 += 1;
		xco2 -= 1;
	}

	DelayMs(3000);

	/* Test the OledPutBmp function
	*/
	yco1 = 0;
	for (ib = 0; ib < 30; ib++) {
		OledClearBuffer();

		OledMoveTo(8+ib, yco1);
		OledPutBmp(8, 4, rgbBmp0);

		OledMoveTo(18+ib, yco1);
		OledPutBmp(8, 6, rgbBmp0);
	
		OledMoveTo(28+ib, yco1);
		OledPutBmp(8, 8, rgbBmp0);
	
		OledMoveTo(38+ib, yco1);
		OledPutBmp(8, 10, rgbBmp0);
	
		OledMoveTo(48+ib, yco1);
		OledPutBmp(8, 12, rgbBmp0);

		OledMoveTo(58+ib, yco1);
		OledPutBmp(8, 14, rgbBmp0);

		OledMoveTo(68+ib, yco1);
		OledPutBmp(8, 16, rgbBmp0);

		OledUpdate();

		if (ib <= 15) {
			yco1 += 1;
		}
		else {
			yco1 -= 1;
		}

		DelayMs(100);
	}

	/* Test moving the bitmap off the screen to the right.
	*/
	for (xco1 = ccolOledMax-16; xco1 <ccolOledMax; xco1++) {
		OledClearBuffer();
		OledMoveTo(xco1, 8);
		OledPutBmp(8, 16, rgbBmp0);
		OledUpdate();
		DelayMs(100);
	}

	/* Test moving the bitmap off the screen to the bottom.
	*/
	for (yco1 = 8; yco1 < crowOledMax; yco1++) {
		OledClearBuffer();
		OledMoveTo(40, yco1);
		OledPutBmp(8, 16, rgbBmp0);
		OledUpdate();
		DelayMs(100);
	}

	/* Test OledGetBmp
	/* This loop tests getting an 8 pixel high bitmap at each
	** possible bit offset.
	*/
	OledClearBuffer();
	for (ib = 0; ib < 8; ib++) {
		OledMoveTo(8*ib+4, ib);
		OledPutBmp(8, 8, rgbBmp0);
		OledMoveTo(8*ib+4, ib);
		OledGetBmp(8, 8, rgbBmp1);
		OledMoveTo(8*ib+4, 16+ib);
		OledPutBmp(8, 8, rgbBmp1);
		OledUpdate();
		DelayMs(100);
	}

	DelayMs(3000);

	/* This loop tests getting various sized bitmaps.
	*/
	OledClearBuffer();
	OledMoveTo(1, 3);
	OledPutBmp(8, 16, rgbBmp0);
	for (ib = 4; ib < 17; ib++) {
		OledMoveTo(1, 3);
		OledGetBmp(8, ib, rgbBmp1);
		OledMoveTo(8*ib-16, 8);
		OledPutBmp(8, ib, rgbBmp1);
		OledUpdate();
		DelayMs(100);
	}

	DelayMs(3000);	

	/* Test the OledPutString function
	*/
	OledClear();

	OledSetCursor(0, 0);
	OledPutString("First Line");
	OledSetCursor(1, 1);
	OledPutString("Second Line");
	OledSetCursor(2, 2);
	OledPutString("Third Line");
	OledSetCursor(3, 3);
	OledPutString("Fourth Line");

	DelayMs(3000);

	/* Test the OledPutChar function.
	*/
	OledSetCursor(0, 0);
	for (ib = 0; ib < 64; ib ++) {
		OledPutChar('A'+ib);
		DelayMs(100);
	}

	DelayMs(3000);

	/* Test using OledPutChar to draw user defined characters
	*/
	OledClear();

	OledSetCursor(0, 0);
	OledPutString("User Defined");
	OledSetCursor(0, 1);
	OledPutString("Characters");

	OledSetCursor(0, 2);
	for (ib = 0; ib < 32; ib++) {
		OledPutChar(ib);
		DelayMs(100);
	}

	DelayMs(3000);

	/* Test drawing characters using graphics functions
	*/
	xco1 = 0;
	yco1 = 0;
	xco2 = 64;
	yco2 = 23;

	for (ib = 0; ib < 16; ib++) {
		OledClearBuffer();
		OledMoveTo(xco1+ib, yco1+ib);
		OledDrawString("ABC");
		OledMoveTo(xco2-ib, yco2-ib);
		OledDrawString("DEF");
		OledUpdate();
		DelayMs(100);
	}

	DelayMs(3000);
}

/* ------------------------------------------------------------ */
/***	ProcName
**
**	Parameters:
**
**	Return Value:
**
**	Errors:
**
**	Description:
**
*/

/* ------------------------------------------------------------ */

/************************************************************************/

