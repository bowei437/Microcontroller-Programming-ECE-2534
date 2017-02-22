////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        HW3
// STUDENT:         Bowei Zhao
// Description: File Program to test code on the Digilent MX board for HW3.
//                  Program will write states to LEDs in accordance to BITs 0,1
//                  and will engage LEDs to turn on with Button presses for Increment and Decrement
//
// File name:       main.c
// the main file change log is set here
// 
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//          delay.c uses Timer1 to provide delays with increments of 1 ms.
//          PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/1/2014
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL  // Oscillator selection
#pragma config POSCMOD      = XT  // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2 // PLL input divider
#pragma config FPLLMUL      = MUL_20  // PLL multiplier
#pragma config FPLLODIV     = DIV_1 // PLL output divider
#pragma config FPBDIV       = DIV_8 // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF // Secondary oscillator enable

// Global variables
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2

// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      sec1000++;                        // Update global variable

      Button1History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x40) // Record the latest BTN1 sample
      Button1History |= 0x01;

      Button2History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x80) // Record the latest BTN1 sample
      Button2History |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}

int main()
{
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

   //Initialize LED's to be off

   LATGCLR = 1 << 12;   // LED1 is cleared/off
   LATGCLR = 1 << 13;   // LED2 is cleared/oof
   LATGCLR = 1 << 14;   // LED3 is cleared/off
   LATGCLR = 1 << 15;   // LED4 is cleared/off

   //Initialize output and button position variables

   int B1prev=0; // previous state of Button 1
   int B1cur=0; // Current state of Button 1

   int B2prev=0; // previous state of Button 2
   int B2cur=0; // current state of button 2


   enum case_var{V0,V1,V2,V3,V4,V5,V6,V7,V8,V9,V10,V11,V12,V13,V14,V15};
   enum case_var cur_var = V0;
   enum case_var cur_var2 = V0;


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


   // Main processing loop
   while (1)
   {
      // First state is to read up on what the status of the buttons are
      // if Button 1 is pressed, then B1cur is given value of 1, otherwise it is set to 0 and vice versa for Button 2
      // This establishes the 'read' state

      // DELAYS are used because debouncing doesn't fix the whole problem of the code cycling fast. 
      // debouncing only fixes issues with the state of the value registered being high after a press
      // or release. Thus delays are implemented to slow the processing down so that it doesn't register
      // a single button press in human reaction time as multiples.

      if(PORTG & 0x40)
      {
         DelayMs(50);
         B1cur = 1;
      }
      else
      {
         B1cur = 0;
      }
      if(PORTG & 0x80)
      {
         DelayMs(50);
         B2cur = 1;
      }
      else
      {
         B2cur = 0;
      }

      DelayMs(40);

      // If the button has been pressed and the debouncer verifies it is sampled 8 times
      // and they are all the same value, then run the code below.

      if (B1cur ==1 && Button1History == 0xff)
      {
         DelayMs(50);
         // the code below checks if Button 2 was pressed previous
         // this has special cases for exceptions and statements that adds enums
         // or sets enums to give the correct output.
         if (B2prev == 1 && B2cur == 0)
         {
            if(cur_var2 == V14)
            {
               cur_var = V0;
            }
            else if(cur_var2 == V15)
            {
               cur_var = V1;
            }
            else
            {
               cur_var = cur_var2+2;
            }
            
         }
         // B1prev=1 and B2prev=0 is used to keep track of what happened 'before' and after.

         B1prev = 1;
         B2prev = 0;
         // switch takes in variables of cur_var for buton 1

         // the following code are hard coded variables to turning on select cases of the LED
         // all while using 'case' statements and the setting cur to a different value
         // break is then used to 'break' from the code

         switch(cur_var)
         {
            case V0:
               LATGCLR = 1 << 12;   // LED1 is cleared/off
               LATGCLR = 1 << 13;   // LED2 is cleared/oof
               LATGCLR = 1 << 14;   // LED3 is cleared/off
               LATGCLR = 1 << 15;   // LED4 is cleared/off
               cur_var = V1;
               break;
            case V1:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGCLR = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var = V2;
               break;
            case V2:
               LATGCLR = 1 << 12;   // LED1 is cleared/off
               LATGSET = 1 << 13;   // LED2 is cleared/oof
               LATGCLR = 1 << 14;   // LED3 is cleared/off
               LATGCLR = 1 << 15;   // LED4 is cleared/off
               cur_var = V3;
               break;
            case V3:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGCLR = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var = V4;
               break;
            case V4:
               LATGCLR = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var = V5;
               break;
            case V5:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var = V6;
               break;
            case V6:
               LATGCLR = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var = V7;
               break;
            case V7:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var = V8;
               break;
            case V8:
               LATGCLR = 1 << 12;
               LATGCLR = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V9;
               break;
            case V9:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V10;
               break;
            case V10:
               LATGCLR = 1 << 12;
               LATGSET = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V11;
               break;
            case V11:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V12;
               break;
            case V12:
               LATGCLR = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V13;
               break;
            case V13:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V14;
               break;
            case V14:
               LATGCLR = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V15;
               break;
            case V15:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var = V0;
               break;

         }
      }

      DelayMs(10);

      // the code here operates the same as above with exceptions for how special cases are handled
      // differently. 

      if (B2cur ==1 && Button2History == 0xff)
      {
         DelayMs(10);
         if (B1prev == 1 && B1cur == 0)
         {
            if (cur_var == V0)
            {
               cur_var2 = V14;
            }
            else if(cur_var == V1)
            {
               cur_var2 = V15;
            }
            else
            {
               cur_var2 = cur_var-2 ;
            }
            
         }
         B1prev = 0;
         B2prev = 1;

         // the following code are hard coded variables to turning on select cases of the LED
         // all while using 'case' statements and the setting cur to a different value
         // break is then used to 'break' from the code
         
         // the switch here takes seperate variables of cur_var2 instead of the usual
         switch(cur_var2)
         {
            case V0:
               LATGCLR = 1 << 12;   // LED1 is cleared/off
               LATGCLR = 1 << 13;   // LED2 is cleared/oof
               LATGCLR = 1 << 14;   // LED3 is cleared/off
               LATGCLR = 1 << 15;   // LED4 is cleared/off
               cur_var2 = V15;
               break;
            case V1:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGCLR = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var2 = V0;
               break;
            case V2:
               LATGCLR = 1 << 12;   // LED1 is cleared/off
               LATGSET = 1 << 13;   // LED2 is cleared/oof
               LATGCLR = 1 << 14;   // LED3 is cleared/off
               LATGCLR = 1 << 15;   // LED4 is cleared/off
               cur_var2 = V1;
               break;
            case V3:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGCLR = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var2 = V2;
               break;
            case V4:
               LATGCLR = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var2 = V3;
               break;
            case V5:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var2 = V4;
               break;
            case V6:
               LATGCLR = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var2 = V5;
               break;
            case V7:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGCLR = 1 << 15;
               cur_var2 = V6;
               break;
            case V8:
               LATGCLR = 1 << 12;
               LATGCLR = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V7;
               break;
            case V9:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V8;
               break;
            case V10:
               LATGCLR = 1 << 12;
               LATGSET = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V9;
               break;
            case V11:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGCLR = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V10;
               break;
            case V12:
               LATGCLR = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V11;
               break;
            case V13:
               LATGSET = 1 << 12;
               LATGCLR = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V12;
               break;
            case V14:
               LATGCLR = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V13;
               break;
            case V15:
               LATGSET = 1 << 12;
               LATGSET = 1 << 13;
               LATGSET = 1 << 14;
               LATGSET = 1 << 15;
               cur_var2 = V14;
               break;

         }
      }
   }

   return 0;
}

