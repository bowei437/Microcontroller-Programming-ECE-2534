////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 1
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
#include "ADXL345.h"
#include "Communication.h"


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
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler
unsigned int sec500;
unsigned int sec10;

// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      sec1000++;      
      sec500++;// Update global variable
      sec10++;
      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}

int main()
{
   char buf[17];        // Temp string for OLED display

   // Initialize GPIO for BTN1 and LED1
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGCLR = 0x1000;   // For LED1: configure PortG pin for output
   ODCGCLR  = 0x1000;   // For LED1: configure as normal output (not open drain)

   // Initialize Timer1 and OLED
   DelayInit();
   OledInit();
   int chnl = ADXL345_Init();
   if(chnl != -1)
   {
       ADXL345_SetRegisterValue(ADXL345_POWER_CTL, ADXL345_PCTL_MEASURE);
   }

   SpiMasterInit(chnl);
   signed short x = 0;
   signed short y = 0;
   signed short z = 0;

  // if(ADX345_Init() == 0)
     // ADXL345_SetRegisterValue(POWER_CTL, ADXL345_PCTL_MEASURE);
  // ADXL345_Init();
   

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

   // Main processing loop
   int first = 1;
   int minx = 0,
       miny = 0,
       minz = 0;

   int curx = 0,
       cury = 0,
       curz = 0;

   int maxx = 0,
       maxy = 0,
       maxz = 0;

   char signmin = '+';
   char signmax = '+';
   char signcur = '+';
   OledSetCursor(0,0);
   OledPutString("   MIN  CUR  MAX");
   sprintf(buf, "X %c%03d %c%03d %c%03d", signmin,minx,signcur,curx,signmax,maxx);
   OledSetCursor(0,1);
   OledPutString(buf);

   sprintf(buf, "Y %c%03d %c%03d %c%03d", signmin,miny,signcur,cury,signmax,maxy);
   OledSetCursor(0,2);
   OledPutString(buf);

   sprintf(buf, "Z %c%03d %c%03d %c%03d", signmin,minz,signcur,curz,signmax,maxz);
   OledSetCursor(0,3);
   OledPutString(buf);



   while (1)
   {
        ADXL345_GetXyz(&x,&y,&z);
       curx = x;
       cury = y;
       curz = z;
       if(sec10 >= 10)
       {
           if(first)
           {
               minx = x;
               miny = y;
               minz = z;
               maxx = x;
               maxy = y;
               maxz = z;
               curx = x;
               cury = y;
               curz = z;
               first = 0;
           }

           else
           {
               if(x < minx)
                   minx = x;

               if(y < miny)
                   miny = y;

               if(z < minz)
                   minz = z;

               if(x > maxx)
                   maxx = x;
               if(y > maxy)
                   maxy = y;
               if(z > maxz)
                   maxz = z;
           }

            sec10 = 0;
       }

       if(sec500 >= 500)
       {
           signmin = '+';
           signmax = '+';
           signcur = '+';

           if(minx < 0){
               signmin = '-';
               minx = minx * -1;
           }
           if(maxx < 0){
               signmax = '-';
               maxx = maxx * -1;
           }
           if(curx < 0){
               signcur = '-';
               curx = curx * -1;
           }

           sprintf(buf, "X %c%03d %c%03d %c%03d", signmin,minx,signcur,curx,signmax,maxx);
           OledSetCursor(0,1);
           OledPutString(buf);

           signmin = '+';
           signmax = '+';
           signcur = '+';

           if(miny < 0){
               signmin = '-';
               miny = miny * -1;
           }

           if(maxy < 0){
               signmax = '-';
               maxy = maxy * -1;
           }

           if(cury < 0)
           {
               signcur = '-';
               cury = cury * -1;
           }

           sprintf(buf, "Y %c%03d %c%03d %c%03d", signmin,miny,signcur,cury,signmax,maxy);
           OledSetCursor(0,2);
           OledPutString(buf);

            signmin = '+';
           signmax = '+';
           signcur = '+';

           if(minz < 0){
               signmin = '-';
               minz = minz * -1;
           }
           if(maxz < 0){
               signmax = '-';
               maxz = maxz * -1;
           }
           if(curz < 0){
               signcur = '-';
               curz = curz * -1;
           }

           sprintf(buf, "Z %c%03d %c%03d %c%03d", signmin,minz,signcur,curz,signmax,maxz);
           OledSetCursor(0,3);
           OledPutString(buf);
           sec500 = 0;

       }

       OledUpdate();

   }

      return 0;
}

