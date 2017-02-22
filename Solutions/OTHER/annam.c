////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Hw 4, Anna Zhou
// File name:       main.c
// Description:     Incrementing a number one by one from 0-15 by pushing button 1.
//                  We compliment the displayed number back and forth by pressing button 2.
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//					delay.c uses Timer1 to provide delays with increments of 1 ms.
//					PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/25/2014
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

// Global variables
unsigned int sec1000;
unsigned int minute = 0;
unsigned int second = 0; // This is updated 1000 times per second by the interrupt handler
unsigned char Button1History = 0x00; // Update button history
unsigned char Button2History = 0x00; // Update button history
// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0

void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      sec1000++;                        // Update global variable
      Button1History <<= 1; // Update button history
      Button2History <<= 1; // Update button history

      if(PORTG & 0x40) // Read current position of BTN1
      Button1History |= 0x01;
      if(PORTG & 0x80) // Read current position of BTN1
      Button2History |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}

int main()
{
   char buf[17];        // Temp string for OLED display

   // Initialize GPIO for BTN1, BTN2 and LED1
   TRISGSET = 0xC0;     // For BTN1: configure PortG bit for input
   TRISGCLR = 0xF000;   // For LED1: configure PortG pin for output
   ODCGCLR  = 0xF000;   // For LED1: configure as normal output (not open drain)
    TRISGSET = 0x40;     // For BTN1: configure PortG bit for input


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

   OpenTimer3(T3_ON         |
            T3_IDLE_CON    |
            T3_SOURCE_INT  |
            T3_PS_1_256     |
            T3_GATE_OFF,
            39061);   // freq = 10MHz/16/625 = 1 kHz

   // Set up CPU to respond to interrupts from Timer2
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);
   INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
   INTEnableInterrupts();

   // Send a welcome message to the OLED display
   OledClearBuffer();
   OledSetCursor(0, 0);
   OledPutString("ECE CLOCK");
   OledSetCursor(0, 2);
   OledPutString("00:00");
   OledUpdate();

 //intitializing all of variables and enum
   int prevB1 = 0; //0 = pressed
   int currB1 = 0; //0 = pressed


  enum buttonstate{pressedState,countState,stopState,resetState,defaultState};
  enum buttonstate var = pressedState;//declaring a initial state

   while (1)
   {
       //prevB1 = currB1;//updating previous state of b1
  
       //checking is the button 1 is pressed
       if (PORTG & 0x40 && Button1History == 0xFF)//setting current is button one is pressed
       {
          DelayMs(100);
           currB1 = currB1 + 1; 
       }
      
      // Display millisecond count value

       if(currB1 == 1)//makes sure that the current state is saved
       {
           switch(var)
           {
               case pressedState:
                  while(currB1 == 1)
                  { 
                       OledSetCursor(0, 0);
                       OledPutString("ANNERS #1");
                       OledSetCursor(0, 2);
                       //OledPutString("00:00");
                       OledUpdate();
                    if( INTGetFlag(INT_T3) )             // Verify source of interrupt
                    {
                       second++;
                       INTClearFlag(INT_T3);             // Acknowledge interrupt
                    }
                    if (PORTG & 0x40 && Button1History == 0xFF)//setting current is button one is pressed
                    {
                         currB1 = currB1 + 1; // set from 1 to 2
                         var = stopState;
                         break;
                    }
                    else
                    {
                      sprintf(buf, "%2.2d:%2.2d", minute, second);
                      OledSetCursor(0, 2);
                      OledPutString(buf);
                      OledUpdate();
                    }



                  } 

               case stopState:
                  while(currB1 == 2)
                  {
                    if (PORTG & 0x40 && Button1History == 0xFF)//setting current is button one is pressed
                    {
                         currB1 = currB1 + 1; // set from 2 to 3
                         var = resetState;
                         break;
                    }

                  }
                   var = resetState;
                   break;
               case resetState:
                   var = defaultState;
                   break;
                   
           
           }
       }

      
    }

   return 0;
}

