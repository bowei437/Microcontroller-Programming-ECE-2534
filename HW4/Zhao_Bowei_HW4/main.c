///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        HW4
// STUDENT:         Bowei Zhao
// Description:     File Program to test code on the Digilent MX board for HW4.
//                  Program will write a stopwatch to the OLED Display and will
//                  engage Button's for starting, stopping, and resetting count.
//
// File name:       main.c
// the main file change log is set here
// 
// Resources:       main.c uses Timer2, and Timer 3 to generate interrupts at intervals of 1 ms and 1s.
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
 // initialize time1s to be seconds. Will run later in code
unsigned int minutes = 0; // initialize minutes to be minutes. Will run later in code.
unsigned int buttondelay = 0; // Buttondelay is used so that the next 'press' won't be registered after switch state.
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2

// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      Button1History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x40) // Record the latest BTN1 sample
      Button1History |= 0x01;
      Button2History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x80) // Record the latest BTN2 sample
      Button2History |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}
int main()
{
   ///////////////////////////////// REAL MAIN CODE /////////////////////////////////////////////////////////
   char buf[17];        // Temp string for OLED display
   // Initialize GPIO for BTN1 and LED1

   // All variables are initialized just because....
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGSET = 0x80;     // For BTN2: configure PortG bit for input
   TRISGCLR = 0xf000;   // For LEDs: configure PortG pins for output
   ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)
   LATGCLR  = 0xf000;   // Initialize LEDs to 0000

   // Enumeration states below are of type enum named case_var. 
   // They have the states of being READ, BTN1_START, BTN1_STOP, and BTN1_RESET. 
   // the current case variable will be hard set to READ. 
   // ENUM Default will be used just in case but setting current state is better.
   enum case_var{READ,BTN1_START,BTN1_STOP,BTN1_RESET};
   enum case_var cur_var = READ;
   unsigned int time1s =0;
   unsigned int timekeep;

   // BTN_PRESS is set to 0. BTN press is used as the tertiary check for a button press on BTN1
   // it is used so that a while loop can run continuously during each switch and to also keep track of states
   int BTN_PRESS = 0;
   int saved = 0;

//////////////////////////////////////// TIMER CODE //////////////////////////////////////////////////////////// 
   // Initialize Timer1, Timer 2, Time3 and OLED
   DelayInit();
   OledInit();

   // Set up Timer2 to roll over every ms
   OpenTimer2(T2_ON         |
             T2_IDLE_CON    |
             T2_SOURCE_INT  |
             T2_PS_1_16     |
             T2_GATE_OFF,
             624);  // freq = 10MHz/16/625 = 1 kHz

   // New Timer 3 is initialized to roll over every second. 
   // it is given a FPBCLK of 10MHz, Prescale of 256, and Period Register of 39061 to give T
   // MRx=Frequency Rollover of 1Hz = 1 second.
   OpenTimer3(T3_ON         |
             T3_IDLE_CON    |
             T3_SOURCE_INT  |
             T3_PS_1_256     |
             T3_GATE_OFF,
             39061); // freq = 10MHz/265/39061 = 1 Hz == 1 second 


   // Set up CPU to respond to interrupts from Timer2 and Timer 3
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);
   INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
   INTEnableInterrupts();

   // Initialize message to the OLED display
   // LED Disiplay will state StopWatch on it and be in the format of 00:00 going by minutes and seconds. 
   OledClearBuffer();
   OledSetCursor(0, 0);
   OledPutString("StopWatch");
   OledSetCursor(0, 2);
   OledPutString("00:00");
   OledUpdate();

   ///////////////////////////// Main processing loop ////////////////////////////////////////////////////
   while (1)
   {
      // SWITCH Variable using cur_var to switch CASES and determining a DEFAULT State. 
      switch(cur_var)
      {
         case READ:
            if(Button1History == 0xFF)
            {
               // Since BTN_PRESS is always 0 at first, this wil run first. 
               if(BTN_PRESS == 0)
               {
                  // A delay of 200mS is used so that the secondary 0xFF in BTN1_START won't initiate to change states
                  DelayMs(200);
                  // Time and ButtonDelay are re/ Initialized to 0 for every loop through START. 
                  time1s = 0;
                  buttondelay = 0;
                  // We are going to now move over to CASE BTN1_START thanks to break
                  cur_var = BTN1_START;
                  break;
               }
               // this code runs when the code breaks in BTN1_START. It will increment BTN_Press in that code 
               else if (BTN_PRESS == 1)
               { // delay and buttondelay is used to prevent accidental state changes
                  //DelayMs(100);
                  buttondelay = 0;
                  // we move cur_Var to BTN1_STOP so the code stops and then breaks
                  cur_var = BTN1_STOP;
                  break;
               }
               // this code runs when the code breaks in BTN1_STOP. It will increment BTN_Press in that code.
               else if (BTN_PRESS == 2)
               {  // delay and buttondelay are used to prevent accidental   
                  DelayMs(100);
                  buttondelay = 0;
                  cur_var = BTN1_RESET;
                  break;
               }
            }
            // this should never run, but just in case, we should put this here for problems.
            //BTN_PRESS = 0;
            cur_var = READ;
            break;

         case BTN1_START:
            while(BTN_PRESS == 0)
            {  
                  // Polls IntGetFlag which will poll through once every second.
                  // Therefore time1s is going to be equal time in seconds thanks to this poll
                  // time1s is set to 0 before this runs always
      
                  // This is used to move onto the next state. 
                  // if a button is pressed, and button delay is greater/equal to 10
                  // then move onto the next state. Cur_Var isn't auto set to the next state but READ
                  // because I have initialization variables in READ and also because of BTN_PRESS which is used
                  // for loops. 
                  // Buttondelay makes it so that it must run through my LED output 10 times. This prevents the first 
                  // press of the button to start time from accidentally going onto the next state. 
               if(Button1History == 0xFF)
               {
                  // BTN PRess is incremented so that next state can be chossen.
                  BTN_PRESS = BTN_PRESS + 1;
                  cur_var = READ;
                  break;
               }
               else // else will run every other time if the button isn't pressed.
               {
                  // if the entire clock/stopwatch is full given 59min and 59sec, then we will reset
                  // all time variables. The clock will still tick though.
                  if (minutes == 59 && time1s == 60)
                  {
                     time1s = 0;
                     minutes = 0; // reset all variables
                  } 
                  // else if in every other case. If the seconds reach 59, then we will reset seconds to 0 and
                  // increment minutes so that it goes from default 0 to 1 and so on. 
                  else if(time1s == 60)
                  {
                     time1s = 0;
                     minutes++; //increment minute
                  }
                  else
                  {
                     while(saved == time1s)
                     {
                        timekeep = ReadTimer3();
                        if(timekeep == 39061)
                        {
                           time1s++;

                        }
                        if(Button1History == 0xFF)
                        {
                           // BTN PRess is incremented so that next state can be chossen.
                           BTN_PRESS = BTN_PRESS + 1;
                           cur_var = READ;
                           break;
                        }
                     }
                          
                     // it will run this code most of the time. Where it will display the time
                     // buf stores the specific sprintf syntax for showing time given minutes and seconds(time1s)
                     sprintf(buf, "%2.2d:%2.2d",minutes, time1s);
                     OledSetCursor(0, 2);
                     // set cursor is used to put it on the second row/line
                     OledPutString(buf); //buf is loaded into the OLED
                     OledUpdate(); // OLED display is updated
                     saved = time1s;
                     //buttondelay++; // buttondelay is incremented here from default 0 making it required
                     // to run through this loop at least ten times so to prevent accidental presses of the next
                     // state value
                     // It's better than delay because as delay stops it, buttondelay still runs through so there isn't
                     // any time delay on the stopwatch.

                  }

               }
            }
            // this code should never run, but again, for safety, a cur_var = read and break are included
            //BTN_PRESS = 0;
            cur_var = READ;
            break;

         case BTN1_STOP:
         // while loop will always run as BTN_Press is hard coded to 1 before this runs
            while(BTN_PRESS == 1)
            { 
               // a long delay isn't required as this loop doesn't do much but stop the OLED
               // and check if the next button is pressed. 
               DelayMs(50);
               // if the button is pressed and it ran through this loop 10 times
               // we can then allow the button to increment so that the RESET state is reached. 
               if(Button1History == 0xFF && buttondelay >=10)
               {  // BTN_Press is incremented like BTN_PRESS++; Next Reset state can then be reached.
                  BTN_PRESS = BTN_PRESS + 1;
                  // we set it back to READ instead of the next state to re-initialize values. 
                  cur_var = READ;
                  break;
               }
               // just do nothing to OLED. 
               OledSetCursor(0, 2);
               OledPutString(buf);
               buttondelay++;
            }
            // should never run. But just in case the code ends up here. Whole loop will reset.
            //BTN_PRESS = 0;
            cur_var = READ;
            break;

         case BTN1_RESET:
         // not much in RESET switch case except to reset all time values to 0
               time1s = 0;
               minutes = 0;
               // OLED buffer is hard set to show 00:00 
               OledClearBuffer(); // clear all values off so that it won't show two states or flip
               OledSetCursor(0, 0);
               OledPutString("StopWatch");
               OledSetCursor(0, 2);
               OledPutString("00:00");
               OledUpdate(); // update the OLED

               // again, BTN_PRESS is hard set to 2 when it arrives here. The only purpose of this while loop
               // is to check to see if I pressed the button again to go back to READ state.
            while(BTN_PRESS == 2)
            {
               // extra delay here so that it won't double incrememnt by accident. 
               DelayMs(50);
               if(Button1History == 0xFF && buttondelay >=10)
               {
                  // this time, BTN_Press is reset to 0 so it will restart the loop at stopwatch start.
                  BTN_PRESS = 0;
                  cur_var = READ;
                  break;
               }
               // buttondelay is used in loop to prevent double increment. 
               buttondelay++;
            }
            // should never run, but just in case there are issues with main code, there is a safety
            // net for it to fall to. 
            //BTN_PRESS = 0;
            cur_var = READ;
            break;

            // default state should never run as Switch was given a starter variable.
            // it is used here to catch all for a safety net.
         default:
         // everything is cleared to 0, the OLED is re-initialized
            LATGCLR  = 0xf000;
            time1s = 0;
            minutes = 0;
            OledClearBuffer();
            OledSetCursor(0, 0);
            OledPutString("StopWatch");
            OledSetCursor(0, 2);
            OledPutString("00:00");
            OledUpdate();
            // we give it a real state to go back into so that it can READ from
            // BTN_PRESS = 0
            //BTN_PRESS = 0;
            cur_var = READ;
            break;
      }
   }
   return 0; // c code requires this as it is older.
}

