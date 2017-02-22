////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Homeowrk 3
// File name:       main.c
// Description:     Have my LED show the characteristics of binary arithmetic(increment, decrement)
//                  based on btn1 or btn2
//                  Also display a counter that updates every millisecond.
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//					delay.c uses Timer1 to provide delays with increments of 1 ms.
//					PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/25/2014
// Modified by: Joshua Chung
// Description: Changed the LED settings so that LED 1-4 will turn on when either
//              BTN1 is pressed or BTN2 is pressedbut not both
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL	// Oscillator selection
#pragma config POSCMOD      = XT	// Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2	// PLL input divider
#pragma config FPLLMUL      = MUL_20	// PLL multiplier
#pragma config FPLLODIV     = DIV_1	// PLL output divider
#pragma config FPBDIV       = DIV_8	// Peripheral bus clock divider
#pragma config FSOSCEN      = OFF	// Secondary oscillator enable

int btn1_hist, btn2_hist;
// Global variables
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler

// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      sec1000++;                        // Update global variable

      btn1_hist <<= 1;
      btn2_hist <<= 1;

      if(PORTG & 0x40)
          btn1_hist |= 0x01;

      if(PORTG & 0x80)
          btn2_hist |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}

int main()
{
   char buf[17];        // Temp string for OLED display

   // Initialize GPIO for BTN1 and LED1
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGSET = 0x80;     //   For BTN2: configured for inpupt
   TRISGCLR = 0x1000;   // For LED1: configure PortG pin for output
   TRISGCLR = 0x2000;    // For LED2: configure PortG pin for output
   TRISGCLR = 0x4000;   // For LED3: configure PortG pin for output
   TRISGCLR = 0x8000;   // For LED4: configure PortG pin for output
   ODCGCLR  = 0x1000;   // For LED1: configure as normal output (not open drain)
    ODCGCLR  = 0x2000;    // For LED2: configure as normal output (not open drain)
     ODCGCLR  = 0x4000;  // For LED3: configure as normal output (not open drain)
      ODCGCLR  = 0x8000;     // For LED4: configure as normal output (not open drain)

   
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

   // Send a welcome message to the OLED display
   OledClearBuffer();
   OledSetCursor(0, 0);
   OledPutString("ECE 2534");
   OledSetCursor(0, 2);
   OledPutString("Joshua Chung");
   OledUpdate();

    enum States {init_state, read, incr, decr, done};
    enum States state = read;
    int button, prevButton;
    int count_value = 0x0;
   
   // Main processing loop
   while (1)
   {
      // Display millisecond count value
      sprintf(buf, "%16d", sec1000);
      OledSetCursor(0, 3);
      OledPutString(buf);
      OledUpdate();
      LATGCLR = 0xF << 12;
      LATGSET = count_value << 12;
      //start of hw 3 code
      
          switch(state)
          {
              //read is my condition I will always come back to.
              //i just read if the btn is either btn1->incr or btn2->decr -- assume both is never pressed
              case read:
                  if (PORTG & 0x40)
                      state = incr;
                  if(PORTG & 0x80)
                      state = decr;
                  break;
              //this case runs when i press btn1.
                  //FIRST: I check if my count_Value is 1111 because if so I clear all the bits to reset the LED to 0
                  //IF NOT: then i shift left my count_value 12 positions to the enabling bits for the LED
                  //Finally i go back to the read state for the next btn
              case incr:
                  if (count_value == 0xf)
                  {
                      LATGCLR = 0xF << 12;
                      count_value = 0x0;
                  }
                      
                  else
                  {
                      count_value += 1;
                      LATGSET = count_value << 12;
                  }
                      
                  state = read;
                  break;
              //This case runs when I press btn2.
                  //FIRST: I check whether or not my count_value is 0x0 (0) because if so then I want to SET all of my leds on and I also want to reinitialize my count_Value to 0 0xF because
                  //        in my method i'm using arithmetic and subtracting from 0 gives me a negative.
                  //IF NOT: I do simple arithmetic and shift the coun_value to the enabling bits for the LED
                  //Finally I go back to my read state
              case decr:
                  if (count_value == 0x0)
                  {
                      LATGSET = 0xF << 12;
                      count_value = 0xF;
                  }
                      
                  else
                  {
                     count_value -= 1;
                     LATGSET = count_value << 12;
                  }
                  state = read;
                  break;
              //I dont quite know when to call this or when I'm supposed to end this switch statement
                  //***MAYBE YOU DONT NEED***
              case done:
                  break;
              default:
                  break;
          }

        //***NOTES ON LED***
         //[PORTG & 0x40] = BTN1
         //[PORTG & 0x80] = BTN2
         //The bits for the LED are in bits 15-12 [LD4(15), LD3(14), LD2(13), LD1(12)]
         //To *SET* an LED use *LATGSET* and shift left 12 times [i.e. LATGSET = 0x1 << 12 ---> will turn on LD1]
         //To *CLEAR* an LED use *LATGCLR* and shift left 12 times [i.e. LATGCLEAR = 0x1 << 12 --> will clear LD1]

   }
   return 0;
}
