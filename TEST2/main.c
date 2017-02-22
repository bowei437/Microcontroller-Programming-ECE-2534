////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 1
// STUDENT:         Bowei Zhao
// New Description: File to modify and test code on the Digilent MX board for Lab 1.
//                  Program will write messages to the OLED from this file
//                  and will engage LEDs to turn on with Button presses.
//
// File name:       main.c
// Description:     Test the Digilent board by writing a short message to the OLED.
//                  Also display a counter that updates every millisecond.
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//             delay.c uses Timer1 to provide delays with increments of 1 ms.
//             PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/1/2014
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

// Digilent board configuration
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

// Global variables


typedef struct _ElapsedTime
{
   unsigned short millisec;
   unsigned char seconds;
   unsigned char minutes;
   unsigned char hours;
   unsigned int days;
}EllapsedTime;

static EllapsedTime elapsed;


void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{  //int i,j,k,l;

    elapsed.millisec++;
    if(elapsed.millisec >= 998)
    {
      elapsed.seconds = i++;
      elapsed.millisec = 0;
    }
    if (elapsed.seconds >=60)
    {
      elapsed.minutes=j++;
      elapsed.seconds=0;
    }
    if(elapsed.minutes >=60)
    {
      elapsed.hours = k++;
      elapsed.minutes=0;
    }
    if(elapsed.hours >=24)
    {
      elapsed.days++;
      elapsed.hours=0;
    }
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

// Interrupt handler - respond to timer-generated interrupt


int main()
{
   char buf[17];

   DelayInit();
   OledInit();


   // Set up CPU to respond to interrupts from Timer2
   OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);

   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
   INTEnableInterrupts();

   // Send a welcome message to the OLED display
   OledClearBuffer();
   OledSetCursor(0, 0);
   //OledPutString("ECE 2534");
   OledSetCursor(0, 2);
   //OledPutString("Bowei Zhao");
   OledUpdate();

   // Main processing loop
   while (1)
   {

      OledSetCursor(0,0);
      sprintf(buf, "%2d : %2d : %3d", elapsed.minutes, elapsed.seconds, elapsed.millisec);
      OledPutString(buf);
      OledUpdate();

   }

   return 0;
}

