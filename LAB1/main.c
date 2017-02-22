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
//					delay.c uses Timer1 to provide delays with increments of 1 ms.
//					PmodOLED.c uses SPI1 for communication with the OLED.
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
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler


typedef struct _ElapsedTime
{
   unsigned short millisec;
   unsigned char seconds;
   unsigned char minutes;
   unsigned char hours;
   unsigned int days;
}EllapsedTime;

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{
    sec1000++; // Increment the millisecond counter.
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      //sec1000++;                        // Update global variable
      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}

int main()
{
   char buf[17];        // Temp string for OLED display

   // The buttons and LEDs below are initiaized following their port-bit locations where
   // PORT BIT 6 is 0x40 in HEX due to the '1' bit in the 6th bit location including 0 as a location
   // PORT BIT 7 is 0x80 in HEX due to the '1' bit in the 7th bit location including 0 as a location
   // PORT BIT 12 is 0x1000 in HEX due to the '1' bit in the 12th bit location including 0 as a location
   // PORT BIT 13 is 0x2000 in HEX due to the '1' bit in the 13th bit location including 0 as a location
   // PORT BIT 14 is 0x4000 in HEX due to the '1' bit in the 14th bit location including 0 as a location
   // PORT BIT 15 is 0x8000 in HEX due to the '1' bit in the 15th bit location including 0 as a location

   // They are initialized using the TRIGSET and ODCGCLR followed by the HEX code. 

   // Initialize GPIO for BTN1 and LED1
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGSET = 0x80;     // For BTN2: configuring BTN2 for input


   TRISGCLR = 0x1000;   // For LED1: configure PortG pin for output
   ODCGCLR  = 0x1000;   // For LED1: configure as normal output (not open drain)

   TRISGCLR = 0x2000;   // For LED2: configuring PortG LED2 for output
   ODCGCLR  = 0x2000;   // for LED2: configure as normal output

   TRISGCLR = 0x4000;   // for LED3: configuring PortG LED3 for output
   ODCGCLR  = 0x4000;   // for LED3: configure as normal output

   TRISGCLR = 0x8000;   // for LED4: configuring PortG LED4 for output
   ODCGCLR  = 0x8000;   // for LED4: configure as normal output

   // Initialize Timer1 and OLED
   DelayInit();
   OledInit();

   // Set up Timer2 to roll over every ms
   OpenTimer2(T2_ON         |
             T2_IDLE_CON    |
             T2_SOURCE_INT  |
             T2_PS_1_16     |
             T2_GATE_OFF,
             624);  // freq = 10MHz/16/625 = 1 kHz

   // Set up CPU to respond to interrupts from Timer2
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);
   INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
   INTEnableInterrupts();

   // Send a welcome message to the OLED display
   OledClearBuffer();
   OledSetCursor(0, 0);
   OledPutString("ECE 2534");
   OledSetCursor(0, 2);
   OledPutString("Bowei Zhao");
   OledUpdate();

   // Main processing loop
   while (1)
   {
      // Display millisecond count value
      sprintf(buf, "%16d", sec1000);
      OledSetCursor(0, 3);
      OledPutString(buf);
      OledUpdate();

      // The defining Functions below use a triple if loop with a concluding else to run 
      // the required function. BTN1 && BTN2's states have to be checked together first
      // as any other combination will prevent additional code from running
      // else if cases are thus used to allow certain button states that will
      // thus activate the LEDs.

      // They are initialized using LATGCLR and LATGSET to store values in registers and to turn
      // the LEDs on/off


      if(PORTG & 0x40 && PORTG & 0x80) // Checks FIRST to see if both buttons
                                       // are pressed
      {
         LATGCLR = 1 << 12;   // LED1 is cleared/off
         LATGCLR = 1 << 13;   // LED2 is cleared/oof
         LATGCLR = 1 << 14;   // LED3 is cleared/off
         LATGCLR = 1 << 15;   // LED4 is cleared/off
      }
      else if (PORTG & 0x40) //elseif follows to check if BTN1 is pressed
      {
         LATGSET = 1 << 12; // LED1 is turned on
         LATGSET = 1 << 13; // LED2 is turned on
         LATGSET = 1 << 14; // LED3 is turned on
         LATGSET = 1 << 15; // LED4 is turned on
      }
      else if(PORTG & 0x80) //elseif follows to check if BTN2 is pressed
      {
         LATGSET = 1 << 12; // LED1 is turned on
         LATGSET = 1 << 13; // LED2 is turned on
         LATGSET = 1 << 14; // LED3 is turned on
         LATGSET = 1 << 15; // LED4 is turned on
      }
      else //else is used to keep LEDS turned off when nothing is pressed
      {
         LATGCLR = 1 << 12;   // LED1 is cleared/off
         LATGCLR = 1 << 13;   // LED2 is cleared/oof
         LATGCLR = 1 << 14;   // LED3 is cleared/off
         LATGCLR = 1 << 15;   // LED4 is cleared/off
      }


   }

   return 0;
}

