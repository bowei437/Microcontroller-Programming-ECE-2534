////////////////////////////////////////////////////////////////////////////////////

// ECE 2534:        HW3 - debouncing and FSMs

// File name:       main.c

// Description:     Update the LED (increment or complement)

//                    based on pressing PB1 or PB2

// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.

//					delay.c uses Timer1 to provide delays with increments of 1 ms.

//					PmodOLED.c uses SPI1 for communication with the OLED.

// Written by:      ALA ( modified By Mudabir Kabir)

// Last modified:   9/29/2015

#define _PLIB_DISABLE_LEGACY

#include <plib.h>

#include "PmodOLED.h"

#include "OledChar.h"

#include "OledGrph.h"

#include "delay.h"



// Implementation-dependent features

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



// Global variables

unsigned int  Sec1000;                  // Millisecond counter

unsigned char Button1History = 0x00;    // Last eight samples of BTN1

unsigned char Button2History = 0x00;    // Last eight samples of BTN1



// Interrupt handler - respond to timer-generated interrupt

#pragma interrupt InterruptHandler ipl1 vector 0

void InterruptHandler(void)

{

    if(INTGetFlag(INT_T2))              // Is TMR2 the interrupt source?

    {

        Sec1000++;                      // Update millisecond counter



        Button1History <<= 1;           // Update button history

        if(PORTG & 0x40)                // Read current position of BTN1

            Button1History |= 0x01;



        Button2History <<= 1;           // Update button history

        if(PORTG & 0x80)                // Read current position of BTN2

            Button2History |= 0x01;



        INTClearFlag(INT_T2);           // Acknowledge TMR2 interrupt

    }

    return;

}



int main()

{

   //char buf[17];        // Temp string for OLED display



   enum States {

       ST_READY, ST_BTN1_TRANSITION, ST_BTN1_DOWN, ST_BTN2_TRANSITION, ST_BTN2_DOWN

   } sysState = ST_READY;



   unsigned int count = 0;	// Display least-significant 4 bits on LEDs



   // Initialize GPIO for BTN1 and LED1

   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input

   TRISGSET = 0x80;     // For BTN2: configure PortG bit for input

   TRISGCLR = 0xf000;   // For LEDs: configure PortG pins for output

   ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)

   LATGCLR  = 0xf000;   // Initialize LEDs to 0000



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

   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);

   INTClearFlag(INT_T2);

   INTEnable(INT_T2, INT_ENABLED);

   INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);

   INTEnableInterrupts();



   // Use of the OLED was not required for this assignment

   OledClearBuffer();

   OledSetCursor(0, 0);

   OledPutString("ECE 2534");

   OledSetCursor(0, 2);

   OledPutString("Homework 3");

   OledUpdate();



   // Main processing loop

   while (1)

   {

      switch(sysState)

      {

      case ST_READY:

         if(Button1History == 0xFF)

             sysState = ST_BTN1_TRANSITION; // BTN1 transition detected

		 else if(Button2History == 0xFF)

             sysState = ST_BTN2_TRANSITION; // BTN2 transition detected

         break;

      case ST_BTN1_TRANSITION:

         count++;                           // Update display

         LATG = (count & 0xff) << 12;

         sysState = ST_BTN1_DOWN;

         break;

      case ST_BTN1_DOWN:

         if(Button1History == 0x00)

             sysState = ST_READY;           // Release transition detected

         break;

      case ST_BTN2_TRANSITION:

         count = count - 1;                    // Update display

         LATG = (count & 0xff) << 12;

         sysState = ST_BTN2_DOWN;

         break;

      case ST_BTN2_DOWN:

         if(Button2History == 0x00)

             sysState = ST_READY;           // Release transition detected

         break;

      default:                              // Should never occur

         LATG = 0xF << 12;

         while(1)

         { /* do nothing */ }               // Trap for debugging

      }

   }

   return 0;

}
