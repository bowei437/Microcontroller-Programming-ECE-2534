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
unsigned int timer;
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

   // initialize the LED
   OledClearBuffer();
   OledSetCursor(0, 0);
   OledPutString("HW4: StopWatch");
   OledSetCursor(0, 4);
   OledPutString("00:00");
   OledUpdate();

    enum States {init_state, read, start, stop, done};
    enum States state = init_state;
    int button, prevButton;
    int stop_int = 0;
    unsigned int temp, min_r, min_l, sec_l;
   // Main processing loop
   while (1)
   {
      // Display millisecond count value
     // sprintf(buf, "%16d", sec1000);
      //OledSetCursor(0, 3);
      //OledPutString(buf);
      //OledUpdate();
       switch(state)
       {
           case init_state:

               if(btn1_hist == 0xFF)
               {
                   DelayMs(130);
                   state = start;
                   min_r = 0;
                   min_l = 0;
                   sec_l = 0;
                   sec1000 = 0;
               }
               break;
           case start:
               //while(stop_int == 0)
              // {
               timer = sec1000/1000;
               temp = timer;
               char zero = '0';
 
                 sprintf(buf, "%d%d:%2d",min_r, min_l, timer);
                 OledSetCursor(0,4);
    
                   
                   OledPutString(buf);
                   OledUpdate();
                   if (temp == 59)
                   {
                       sec1000 = 0;
                       min_l++;
                   }
                   if(min_l == 9)
                   {
                       if (min_r = 5)
                       {
                           state = init_state;
                       }
                       else
                       {
                           min_l = 0;
                           min_r++;
                       }
                       
                   }

                   if (PORTG & 0x40)
                   {
                       state = stop;
                   }
                   break;
               //}
           case stop:
               sprintf(buf, "%d%d:%2d",min_r, min_l, temp);
               OledSetCursor(0,4);
               OledPutString(buf);
               OledUpdate();
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
