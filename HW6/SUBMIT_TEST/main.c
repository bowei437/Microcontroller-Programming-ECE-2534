///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        HW6
// STUDENT:         Bowei Zhao
// Description:     File program to output Accelerometer onto OLEd
//                  Using code from Analog Devices
//     
//
// File name:       main.c
// the main file change log is set here
// 
// Resources:       main.c uses Timer2, and Timer 3 to generate interrupts at intervals of 1 ms and 1s.
//                  delay.c uses Timer1 to provide delays with increments of 1 ms.
//                  PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      Bowei Zhao 
// Last modified:   11/4/2015

#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include <stdlib.h>
#include "stdbool.h"
#include "Communication.h"
#include "ADXL345.h"
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include <GenericTypeDefs.h>
// Implementation-dependent features
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

// initializes general functions that it call
void START_initial();
void Device_initial();

// uses float to get values from GETXYZ
short x = 0, y = 0, z = 0;

char buf1[50];
  
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler
unsigned int sec10;
int alarm = 0;

// isr that polls the ADXL for x, y, z values every 10 ms
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{
  // only polls every 10 ms
  if(sec10 > 10)
  {
    ADXL345_GetXyz(&x,&y,&z);
    sec10 = 0;
  }
  sec1000++; // Increment the millisecond counter.
  sec10++;
  INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

int main()
{
   START_initial(); // calls function for intiialization devices
   Device_initial(); // calls ADXL init function
   OledClearBuffer(); // just clears OLED buffer before start

   // local  output protocals for max and min
   short min_x = 0, min_y = 0, min_z = 0, max_x = 0, max_y = 0, max_z = 0;
   char bufout[4];
   char bufout2[4];
   char bufout3[4];


   while(1)
   {
      // used during a return for if ADXL was not initialized correctly
      if (alarm == 1)
      {
        OledSetCursor(0, 0);
        OledPutString("ERROR SPI/ADXL");

        OledSetCursor(0, 1);
        OledPutString("NOT FOUND.");

        OledSetCursor(0, 3);
        OledPutString("Replug/Rerun");

        OledUpdate();
      }
      else
      {
          // runs only once every 500ms
        if(sec1000 > 500)
        { 

          // code that finds minimum and maximum values for X , Y and Z
          if (x < min_x)
          {
            min_x = x;
          }
          else if (x > max_x)
          {
            max_x = x;
          }

          if (y < min_y)
          {
            min_y = y;
          }
          else if (y > max_y)
          {
            max_y = y;
          }

          if (z < min_z)
          {
            min_z = z;
          }
          else if (z > max_z)
          {
            max_z = z;
          }
          // it then just displays the values out in deciaml format

          OledSetCursor(0, 0);
          OledPutString("   MIN  CUR  MAX");

          OledSetCursor(0, 1);
          OledPutString("x");

          OledSetCursor(2, 1);
          sprintf(bufout,"%0+4d",min_x);
          OledPutString(bufout);

          OledSetCursor(7, 1);
          sprintf(bufout,"%0+4d",x);
          OledPutString(bufout);

          OledSetCursor(12, 1);
          sprintf(bufout,"%0+4d",max_x);
          OledPutString(bufout);

          OledSetCursor(0, 2);
          OledPutString("y");

          OledSetCursor(2, 2);
          sprintf(bufout2,"%0+4d",min_y);
          OledPutString(bufout2);

          OledSetCursor(7, 2);
          sprintf(bufout2,"%0+4d",y);
          OledPutString(bufout2);

          OledSetCursor(12,2);
          sprintf(bufout2,"%0+4d",max_y);
          OledPutString(bufout2);

          OledSetCursor(0, 3);
          OledPutString("z");

          OledSetCursor(2, 3);
          sprintf(bufout3,"%0+4d",min_z);
          OledPutString(bufout3);

          OledSetCursor(7, 3);
          sprintf(bufout3,"%0+4d",z);
          OledPutString(bufout3);

          OledSetCursor(12,3);
          sprintf(bufout3,"%0+4d",max_z);
          OledPutString(bufout3);


          OledUpdate();

          sec1000 = 0; // reset sec1000 to 0 so it can repoll back up to 500ms

        } 

      }

   }

 
   return 0;
}

void START_initial()
{
///////////////////////////////////////// Button and LED ENABLING ///////////////////////////////////////////////////////

    //DDPCONbits.JTAGEN = 0; // JTAG controller disable to use BTN3

    TRISGSET = 0x40;     // For BTN1: configure PORTG bit for input
    TRISGSET = 0x80;     // For BTN2: configure PORTG bit for input
    TRISASET = 0x1;

    TRISGCLR = 0xf000;   // For LEDs: configure PORTG pins for output
    ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)
    LATGCLR  = 0xf000;   // Initialize LEDs to 0000

    TRISBCLR = 0x0040; // initialize U/D pin B6
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

///////////////////////////////////////// DELAY & OLED ENABLING ///////////////////////////////////////////////////////
    DelayInit(); // initializes delays
    OledInit(); // initializes OLED display
    OledSetCursor(0, 0);
    OledClearBuffer();
    OledPutString("Initial");
    OledUpdate();

/////////////////////////////////////////TIMERS ENABLING ///////////////////////////////////////////////////////
   OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624); // sets timer for 1ms
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);

   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR); // multi vector just because
   INTEnableInterrupts();

}

void Device_initial()
{

  if(ADXL345_Init() == 0) // calls aDXL function for if status == 0
  {
    ADXL345_SetRangeResolution(ADXL345_RANGE_PM_2G,0x0); // gives it a 4G range
    unsigned char off;
    //off = 7000;

    // uses an offset to make the numbers bigger
    //ADXL345_SetOffset(off,off,off);
    OledClearBuffer();
    OledSetCursor(0, 0);
    //OledPutString("SP Init");
    //OledUpdate();
    DelayMs(500);

    // reset alarm value to 0 so it won't run just in case
    alarm = 0;
    
  }
  else // if status returns any value but 0 such as -1 or even 1, then we will get error statements. 
  {
    // set alarm so error will display above
    alarm = 1;
  }

}
