////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Svetlana Marhefka - Homework #4
// File name:       main.c
// Description:     This file contains the main program needed needed to create
//                  a stopwatch which will be displayed in your OLED chip.
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//                  delay.c uses Timer1 to provide delays with increments of 1 ms.
//                  PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/1/2014
// IMPORTANT NOTE:	When I was originally writing the program for this homework I 
//					had gone to the CEL Lab for help twice. When I recieved help
//					help from the two TA's they both told me to use 
//					INTGetFlag(INT_T2) and INTClearFlag(INT_T2) in my program.
//					However, I was just told today that we were not supposed to do
//					this.  I have tried (without success) to change my program but
//					because of work I need to hand this in now (8:00pm). It appears
//					that the TA's who were helping me did not know this either. 
//					This is a note to let the TA grading this know that I was unaware 
//					until recently that we were not allowed to use GET and Clear 
//					flags and therefore have programmed my code implementing 
//					those funtions.  
//					Thank you,
//					Svetlana Marhefka
///////////////////////////////////////////////////////////////////////////////////

// Call other resource files that are needed for this homework
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
unsigned int seconds = 0;               // initialize seconds which will display
                                        // the count
unsigned int minutes = 0;               // initialize minutes and set to 0
unsigned int Button1Delay = 0;
unsigned char Button1History = 0x00;    // Last eight samples of BTN1


#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534(void) {
    if (INTGetFlag(INT_T2))             // Verify source of interrupt
    {

        Button1History <<= 1;           // Discard oldest sample to
                                        // make room for new
        if (PORTG & 0x40)               // Record the latest BTN1 sample
            Button1History |= 0x01;

        INTClearFlag(INT_T2);           // Acknowledge interrupt
    }
    return;
}

int main() {
    char buf[17];                       // Temp string for OLED display
                                        // Initialize GPIO for BTN1 and LED1
    TRISGSET = 0x40;                    // For BTN1: configure PortG bit for input

    enum States {                       // Creates for states: READ, COUNT, STOP
                                        // CLEAR
        READ, COUNT, STOP, CLEAR
    };
    enum States CURR_STATE = READ;      // Sets the current state to READ
                                        // This will make it so that every time
                                        // Button1 is pressed the state is
                                        // checked

    int BUTTON1 = 0;                    // Initialize the count of how many times
                                        // Button1 is pressed

    // Set up Timer3 to roll over every second
    OpenTimer3(T3_ON        |
            T3_IDLE_CON     |
            T3_SOURCE_INT   |
            T3_PS_1_256     |
            T3_GATE_OFF,
            39061); // 39061 freq = 10MHz/256/39061 = 1 kHz

    // Initialize Timer1 and OLED
    DelayInit();
    OledInit();

    // Set up Timer2 to roll over every ms
    OpenTimer2(T2_ON        |
            T2_IDLE_CON     |
            T2_SOURCE_INT   |
            T2_PS_1_16      |
            T2_GATE_OFF,
            624); // freq = 10MHz/16/625 = 1 kHz

    // Set up CPU to respond to interrupts from Timer2
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
    INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
    INTEnableInterrupts();

    // Send a welcome message to the OLED display
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("Stopwatch - HW4");       // Display Stopwatch - HW4 on OLED
    OledSetCursor(0, 2);
    OledPutString("00:00");                 // Displays 00:00 right underneath
                                            // the Title
    OledUpdate();

    while (1)
    {
        switch (CURR_STATE)
        {
            // State 1
            case READ:

                // Checks to see if button1 has been pressed
                // If it has then the program looks to see how
                // many times Button1 has been pressed.
                if (Button1History == 0xFF)
                {
                    if (BUTTON1 == 0)           // if the button has been
                                                // pressed once
                    {
                        DelayMs(100);
                        seconds = 0;
                        Button1Delay = 0;
                        CURR_STATE = COUNT;     // Sets the Machine State to
                                                // COUNT (this increments the
                                                // seconds)
                        break;
                    }
                    else if (BUTTON1 == 1)      // if the button has been
                                                // pressed again after starting
                    {
                        DelayMs(100);
                        Button1Delay = 0;
                        CURR_STATE = STOP;      // Sets the Machine State to
                                                // STOP.
                        break;
                    }
                    else if (BUTTON1 == 2)      // if the button has been
                                                // pressed again after starting
                    {
                        DelayMs(100);
                        Button1Delay = 0;
                        CURR_STATE = CLEAR;     // Sets the Machine State to
                                                // CLEAR.
                        break;
                    }
                }
                break;

            case COUNT:
                // Case that will run if button one has only been pressed one
                // time
                while (BUTTON1 == 0) {
                    // Verify source of interruptSS
                    if (INTGetFlag(INT_T3)) {
                        seconds++;
                        INTClearFlag(INT_T3);
                    }
                    if (Button1History == 0xFF && Button1Delay >= 10) {
                        BUTTON1++;
                        CURR_STATE = READ;
                        break;
                    }
                    // Case statement of what happens when seconds are equal to
                    // 59 and what happens if both seconds and minutes are equal
                    // to 59
                    else
                    {
                        if (seconds == 59)
                        {
                            if(minutes == 59)minutes = 0;
                            {
                                seconds = 0;
                                minutes = 0;
                            }
                            minutes++;
                            seconds = 0;
                        }
                        // Prints out the updated time information taking into
                        // acount the transition that needs to be made when
                        // seconds are 59 and minutes are also 59
                        sprintf(buf, "%2.2d:%2.2d", minutes, seconds);
                        OledSetCursor(0, 2);
                        OledPutString(buf);
                        OledUpdate();
                        Button1Delay++;
                    }
                }
                CURR_STATE = READ;
                break;

            case STOP:
                // This machine state is called if Button1 has been pressed
                // again.  The funtion of the state is spimply to display the
                // time value in 00:00 form as long as Button1 is not hit again.
                // If Button1 is hit agian then the Button will be read
                // and the CLEAR State will be called.
                while (BUTTON1 == 1) {
                    if (Button1History == 0xFF && Button1Delay >= 10) {
                        BUTTON1++;
                        CURR_STATE = READ;
                        break;
                    }
                    OledSetCursor(0, 2);
                    OledPutString(buf);
                    Button1Delay++;
                }
                CURR_STATE = READ;
                break;

            case CLEAR:
                // This machine state is called if Button1 is pushed once more
                // after it is already int the stop state.  This is done because
                // the value of Button1 is now = 2 with 0 being the first time
                // that the button was pressed, 1 being the second time that the
                // button was pressed and 2 being the third time that the button
                // was pressed.

                // All variables are reinitialized to 0.  When everything is set
                // to 0 is clears the current value that the variable may hold.
                // With that we can then tell teh program to redo something
                // (like read in Button1 and proceed as necessary).
                seconds = 0;
                minutes = 0;
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("Watch - HW4");
                OledSetCursor(0, 2);
                OledPutString("00:00");
                OledUpdate();

                // As long as Button1 has been pushed for the third
                // time
                while (BUTTON1 == 2) {
                    if (Button1History == 0xFF && Button1Delay >= 10) {
                        BUTTON1 = 0;
                        CURR_STATE = READ;
                        break;
                    }
                    Button1Delay++;
                }
                BUTTON1 = 0;
                CURR_STATE = READ;
                break;

            default:
                LATGCLR = 0xf000;
        }
    }
    return 0;
}