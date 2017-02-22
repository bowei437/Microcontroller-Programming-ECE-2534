#include <plib.h>

#include "PmodACL.h"
#include "SPI.h"


//ra07
#define INT_PORT IOPORT_A
#define INT_PIN BIT_7

void initPmodACL()
{
  PORTSetPinsDigitalIn(INT_PORT, INT_PIN);
 
	PmodACLwriteReg (INT_ENABLE, 0x40);
	PmodACLwriteReg (POWER_CTL, 0x08);

	PmodACLwriteReg (0x2A,0x01);
	PmodACLwriteReg (0x1D,0x30);
	PmodACLwriteReg (0x21,0x20);
	PmodACLwriteReg (0x22,0x00);

	PmodACLreadReg(INT_SOURCE);
}


UINT8 PmodACLreadReg (UINT8 reg)
{
	UINT8 value;

	reg = reg | 0x80;

	SPIio(SPI_CHN, 1, &reg, 1, &value, 1);	

	return value;
}


void PmodACLwriteReg(UINT8 reg, UINT8 value)
{
	UINT8 buffer[2];
	buffer [0] = reg;
	buffer [1] = value;

	SPIio (SPI_CHN, 2, buffer, 0, buffer, 0);
}


void PmodACLreadData(PmodACLdata* data)
{
	UINT8 sendBuff[1];
	UINT8 recvBuff[6];
	sendBuff[0] = 0xC0;
	sendBuff[0] |= DATAX0;
	SPIio (SPI_CHN, 1, sendBuff, 6, recvBuff, 1);
	INT16 cord = recvBuff[1];
	cord = cord << 8;
	cord |= recvBuff [0];
	data -> x = cord;
	cord = recvBuff[3];
	cord = cord << 8;
	cord |= recvBuff [2];
	data -> y = cord;
	cord = recvBuff[5];
	cord = cord << 8;
	cord |= recvBuff [4];
	data -> z = cord;
}


BOOL PmodACLsingleTap()
{
	UINT8 tap = 0;

	tap = PORTReadBits(INT_PORT, INT_PIN);

	if(tap > 0){
		PmodACLreadReg(INT_SOURCE);
		PORTClearBits(INT_PORT,INT_PIN);
		return TRUE;
	}
	return FALSE;
}
