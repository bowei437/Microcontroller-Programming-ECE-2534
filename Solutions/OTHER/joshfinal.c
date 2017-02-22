////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Homeowrk 4
// File name:       main.c
// Description:     I created a stop watch using timer 3
//                  Also display a counter that updates every millisecond.
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//					delay.c uses Timer1 to provide delays with increments of 1 ms.
//					PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/25/2014
// Modified by: Joshua Chung
// Description: create a stop watch using btn1 and using the seconds as a timer
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

   // set up Timer3 to roll over ever second
   OpenTimer3(T3_ON         |
             T3_IDLE_CON    |
             T3_SOURCE_INT  |
             T3_PS_1_256     |
             T3_GATE_OFF,
             39061); //change your frequency

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
   OledSetCursor(0, 2);
   OledPutString("00:00");
   OledUpdate();

   //my states for my state machine
    enum States {init_state, read, start, stop, reset, no};
    enum States state = init_state;

    //stop int is used to control my switch statement
    int stop_int = 0;
    unsigned int temp, min_r, min_l, temp1;
    unsigned int timer =0, sec;

     

   // Main processing loop
   while (1)
   {
      //my timer...i increment timer every time there is a rollover in my timer 3
      temp1 = ReadTimer3();
       switch(state)
       {
           case init_state: //In this state I initialize all variables to zero and then check three conditions
                            //First I check if I pressed the button -- if i did then I want to go ahead and step into my start state
               if(btn1_hist == 0xFF)
               {
                   DelayMs(130);
                   state = start;
                   min_r = 0;
                   min_l = 0;
                   sec = 0;
               }
               if (stop_int == 2)//second I check if stop_int == 2 which means that I pressed
               {                //BTN1 three times and i want to reset everything to 0 until I press the btn again
                   OledSetCursor(0, 2);
                   OledPutString("00:00");
                   OledUpdate();
                   if (PORTG & 0x40)//if i press the btn i step into my start function again
                   {
                       DelayMs(130);
                       min_r = 0;
                       min_l = 0;
                       sec = 0;
                       state = start;
                   }
               }
               if(stop_int == 1) //If my stop_int == 1 then it means that I've reached 59:59
               {                //and I just want to start the stopwatch over at 00:00 and continue counting
                   min_r = 0;
                   min_l = 0;
                   sec = 0;
                   state = start;
               }
               break;

           case start: //in my start state I update my LED with the right time
             if (temp1 == 39061)
             {
                sec++;
                temp = sec; //i save a instance of the current second just in case i want to hold it
                if (sec == 60) //If my timer(seconds) reaches 59 it means I need to increment my min variable
               {
                   if(min_r == 9) //This checks if my right min is 9 then I need to increment my left min
                   {
                       if (min_l == 5) //This checks if my timer is at 59:59 in which i want to
                       {                //reset everything and continue the clock
                           state = init_state;
                           stop_int = 1;
                       }
                       else //if not then I just want to incerment my lefthand minute
                       {        //and reinitialize my right-side min to 0.
                           min_l++;
                           min_r = 0;
                       }
                    }
                   else //if my righthand min is not a 9 then I just increment my righthand min and then
                    {       //reset my timer
                         sec = 0;
                         min_r++;
                    }

                }
                sprintf(buf, "%d%d:%02d",min_l, min_r, sec); //I update my timer again with my new
                OledSetCursor(0,2);                              //timer variable.
                OledPutString(buf);
                OledUpdate();
             }

              if (PORTG & 0x40) //lastly I check if during any time in my start state if I press the button.
               {                //if so then I change states to stop -- only condition when i change it.
                   DelayMs(130);
                   state = stop;
               }
        
                   break;

           case stop: //in my stop state I take all the current values and display it onto the led
               sprintf(buf, "%d%d:%02d",min_l, min_r, temp);
               OledSetCursor(0,2);
               OledPutString(buf);
               OledUpdate();
               if(PORTG & 0x40) //if i press the btn again during anytime then i will change my state to reset
               {
                   DelayMs(130);
                   state = reset;
               }

               break;
           case reset: //In this case the btn was pressedd 3 times so I reset my whole clock and wait till
               state = init_state;  //the button is pressed
               stop_int = 2;
               break;
           case no:
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
