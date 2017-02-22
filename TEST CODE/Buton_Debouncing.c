#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include <stdlib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include <GenericTypeDefs.h>

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
#define GetPGClock() 10000000 // define FPGclock. It is already defined above with pragma and config but this is used later in UART as a variable

/////////////////////////////////////////////// Global variables ////////////////////////////////////////////////////////////////////
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2
unsigned char Button3History = 0x00; // Last 8 samples of BTN3

enum case_var{READ, LoopGame};// state machine uses enums
enum case_var cur_var = READ;// set main enum to read

///////////////////////////////////// BUTTON DEBOUNCING FUNCTION /////////////////////////////////////////////////////////////////////

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

      Button3History <<= 1; // Update button history
      if (PORTA & 0x01) // Read current position of BTN3
      Button3History |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}




//////////////////////////////// MAIN CODE /////////////////////////////////////////////////////////////
int main()
{
    DDPCONbits.JTAGEN = 0; // JTAG controller disable to use BTN3

    TRISGSET = 0x40;     // For BTN1: configure PORTG bit for input
    TRISGSET = 0x80;     // For BTN2: configure PORTG bit for input
    TRISASET = 0x1;      // For BTN3: configure PORTA bit for input

    TRISGCLR = 0xf000;   // For LEDs: configure PORTG pins for output
    ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)
    LATGCLR  = 0xf000;   // Initialize LEDs to 0000

   // Enumeration states below are of type enum named case_var. 
   // They have the states of being READ, BTN1_START, BTN1_STOP, and BTN1_RESET. 
   // the current case variable will be hard set to READ. 
   // ENUM Default will be used just in case but setting current state is better.
    // used for comparing values of second to make loop run
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

   // Set up CPU to respond to interrupts from Timer2
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);
   INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
   INTEnableInterrupts();

   // Initialize message to the OLED display
   // LED Disiplay will state StopWatch on it and be in the format of 00:00 going by minutes and seconds. 
   OledClearBuffer();

   OledUpdate();
   
   ///////////////////////////// Main processing loop ////////////////////////////////////////////////////
   while (1)
   {

      switch(cur_var)
      {
         case READ:
        
          if((Button1History == 0xFF) || (PORTG & 0x40) ) // BTN1 causes Main Single player mode
          {


          }

          if((Button2History == 0xFF) || (PORTG & 0x80) ) //BTN2 causes 2 player mode to start
          {

          }

          if((Button3History == 0xFF) || (PRTA & 0x01 ) )
          {

          }

          // Do Something

         default:
            cur_var = READ;
            break;

      }

   }
   return 0; // c code requires this as it is older.
}

