#include <plib.h>
#include <stdlib.h>
#include <stdio.h>
#include "PmodACL.h"
#include "SPI.h"
#include "Clock.h"
#include "LCD.h"
#include "i2c.h"
#include "tiltGame.h"

// Configuration Bit settings
//
// SYSCLK = 80 MHz (8MHz Crystal/ FPLLIDIV * FPLLMUL / FPLLODIV)
// PBCLK = 10 MHz
// WDT OFF

#ifndef OVERRIDE_CONFIG_BITS
        
    #pragma config FPLLMUL  = MUL_20        // PLL Multiplier
    #pragma config FPLLIDIV = DIV_2         // PLL Input Divider
    #pragma config FPLLODIV = DIV_1         // PLL Output Divider
    #pragma config FPBDIV   = DIV_8         // Peripheral Clock divisor
    #pragma config FWDTEN   = OFF           // Watchdog Timer 
    #pragma config WDTPS    = PS1           // Watchdog Timer Postscale
    #pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
    #pragma config OSCIOFNC = OFF           // CLKO Enable
    #pragma config POSCMOD  = HS            // Primary Oscillator
    #pragma config IESO     = OFF           // Internal/External Switch-over
    #pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
    #pragma config FNOSC    = PRIPLL        // Oscillator Selection
    #pragma config CP       = OFF           // Code Protect
    #pragma config BWP      = OFF           // Boot Flash Write Protect
    #pragma config PWP      = OFF           // Program Flash Write Protect
    #pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
    #pragma config DEBUG    = OFF           // Debugger Disabled for Starter Kit
            
#endif // OVERRIDE_CONFIG_BITS


int main()
{
  const char clearDisplay[] = "j";

//	char theNum[5];
	int time=0;
	
	// Enable interrupts after setting up the LCD and timer peripherals.
	initI2C();
	initLCD();
	initClock();
	initSPI();
	initPmodACL();
	INTEnableSystemMultiVectoredInt();

	initTiltGame();

	PmodACLdata data;

	INT8 testID;
	testID = PmodACLreadReg(DEVID);

	lcdInstruction("0;24;24;0;0;0;0;0;1d"); //ball 1
	lcdInstruction("0;3;3;0;0;0;0;0;2d"); //ball 2
	lcdInstruction("0;0;0;0;0;24;24;0;3d"); //ball 3
	lcdInstruction("0;0;0;0;0;3;3;0;4d"); //ball 4
	lcdInstruction("0;4;14;14;14;4;0;0;5d"); //hole

	lcdInstruction (clearDisplay);
	lcdInstruction("3p");

	lcdInstruction ("1;0H");
	lcdString("\x05"); //hole
	lcdInstruction ("0;15H");
	lcdString("\x02"); //ball 2 @ start

	while(!secondTick()); //brief pause for lcd

	int x1 = getX();
	int y1 = getY();
	int ballStat1 = getBallStatus();
	int supfresh1 = 1;

	while (1) {
	while(secondTick()){
		time++;
		PmodACLreadData(&data);

		int x = data.x;
		int y = data.y;

		if(x > 75){
			tilted(1,0);
		}else if(x < -75){
			tilted(-1,0);
		}

		if(y > 75){
			tilted(0,1);
		}else if(y < -75){
			tilted(0,-1);
		}

		int ballStat;
		if(wasTilted()){
			x = getX();
			y = getY();
			ballStat = getBallStatus();

			char inst[10];
			char str[10];
			sprintf(inst,"%d;%dH",y,x);
			lcdInstruction(clearDisplay);

			lcdInstruction ("1;0H");
			lcdString("\x05");

			lcdInstruction(inst);

			if(ballStat==1){
				lcdString("\x01");
			}else if(ballStat==2){
				lcdString("\x02");
			}else if(ballStat==3){
				lcdString("\x03");
			}else{
				lcdString("\x04");
			}
		}

		if(hasWon()){
			char timeElapsed[10];
			lcdInstruction(clearDisplay);
			lcdInstruction("0;0H");
			sprintf(timeElapsed,"Game in:%ds",time/10);
			lcdString(timeElapsed);

			PmodACLreadReg(INT_SOURCE);

			while(1){
				if(PmodACLsingleTap()){
					break;
				}
			}

			initTiltGame();
			lcdInstruction(clearDisplay);
			time = 0;
			
			lcdInstruction ("1;0H");
			lcdString("\x05"); //hole
			lcdInstruction ("0;15H");
			lcdString("\x02"); //ball 2 @ start
		}

	}
	}

	return 0;
}
