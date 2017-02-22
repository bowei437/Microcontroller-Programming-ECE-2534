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
void START_initial();
void Device_initial();

float          x                 = 0; /*!< X-axis's output data. */
float          y                 = 0; /*!< Y-axis's output data. */
float          z                 = 0; /*!< Z-axis's output data. */
char buf1[50];
char buf2[50]; 
char buf3[50];  
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{
    ADXL345_GetGxyz(&x,&y,&z);
    sec1000++; // Increment the millisecond counter.
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

int main()
{
   START_initial();
   Device_initial();
   OledClearBuffer();

   while(1)
   {
        
        OledSetCursor(0, 0);
        sprintf(buf1,"X: %f",x);
        OledPutString(buf1);

        OledSetCursor(0, 1);
        sprintf(buf2,"Y: %f",y);
        OledPutString(buf2);

        OledSetCursor(0, 2);
        sprintf(buf3,"Z: %f",z);
        OledPutString(buf3);

        //OledSetCursor(0, 2);
        //sprintf(buf3,"sec: %g",sec1000 );
        //OledPutString(buf3);

        OledUpdate();



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
    //sprintf(buf,"SPI Initialized"); // goal_set is default to 12 always and will dynamically increment. 
    
    //OledUpdate();
/////////////////////////////////////////TIMERS ENABLING ///////////////////////////////////////////////////////
   OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_256 | T2_GATE_OFF, 39061);
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);

   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
   INTEnableInterrupts();

}

void Device_initial()
{

  if(ADXL345_Init() == 0)
  {
    OledClearBuffer();
    OledSetCursor(0, 0);
    //sprintf(buf,"SPI Initialized"); // goal_set is default to 12 always and will dynamically increment. 
    OledPutString("SP Init");
    OledUpdate();
    DelayMs(500);
    //printf("YOLO\n");

  }

   /*
      ADXL345_SetTapDetection(ADXL345_SINGLE_TAP |
                                    ADXL345_DOUBLE_TAP,  !< Tap type. 
                                    ADXL345_TAP_X_EN,    /*!< Axis control. 
                                    0x64,   /*!< Tap duration.--10 
                                    0x20,   /*!< Tap latency.--10 
                                    0x40,   /*!< Tap window. 
                                    0x10,   /*!< Tap threshold. 
                                    0x00);    /*!< Interrupta Pin. 
            ADXL345_SetFreeFallDetection(0x01,  /*!< Free-fall detection enabled. 
                                         0x05,  /*!< Free-fall threshold. 
                                         0x14,  /*!< Time value for free-fall detection. 
                                         0x00); /*!< Interrupt Pin. 
            ADXL345_SetPowerMode(0x1);    /*!< Measure mode. 
  */
}
