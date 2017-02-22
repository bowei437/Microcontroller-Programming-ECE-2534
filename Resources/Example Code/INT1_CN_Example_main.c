////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        INT1 and CN Program Example
//
// File name:       INT1_CN_Example_main - A demonstration CN and INT1
//
// Description:     After wiring up the connections between headers correctly,
//                  pressing BTN1 will generate a digital signal (a square wave)
//                  that will trigger the INT1 ISR. Likewise, pressing
//                  BTN2 will generate a digital signal (again a square wave)
//                  that will twice trigger the CN12 interrupt. The number of
//                  interrupts trigger for both peripheral is displayed on
//                  the OLED.
//
// How to use:      The square wave pulses are on the JB header. Pin 4 on the JB
//                  header is triggered (via the program) by pressing BTN1. Pin 7
//                  is triggered by pressing BTN2. You can use the medusa cable
//                  and a male-to-male header to connect these pins to INT1 (located
//                  on header JE pin 7) and CN12 (located on header JC pin 7).
//                  A summary of the connections is given below.
//--------------------------------------------------------------------------------
//                  Wire 1:  connect
//                  Header  Pin#  Peripheral  to  Header Pin#   Peripheral
//                  JB      4     PortB/Bit4      JE     7      INT1
//--------------------------------------------------------------------------------
//                  Wire 2:  connect
//                  Header  Pin#  Peripheral  to  Header Pin#   Peripheral
//                  JB      7     PortB/Bit5      JC     7      CN12
//--------------------------------------------------------------------------------
//                  A logic analyzer or Oscope can be used to see the timer ISR,
//                  the square waves, and when the INT1 and CN ISR are triggered
//                  on (header JB, pins 1, 2, 3, 4, and 5).
//
// Written by:      PEP
// Last modified:   2 December 2015
////////////////////////////////////////////////////////////////////////////////

//#define _PLIB_DISABLE_LEGACY // Err... we use legacy macros
#include <plib.h>
#include "stdbool.h"
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include <peripheral/ports.h>
#include <peripheral/spi.h>

// Configure the microcontroller
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

// Intrumentation for the logic analyzer (or oscilliscope)
#define MASK_DBG1  0x1;
#define MASK_DBG2  0x2;
#define MASK_DBG3  0x4;
#define MASK_DBG4  0x8;
#define MASK_DBG5  0x10;
#define DBG_ON(a)  LATESET = a
#define DBG_OFF(a) LATECLR = a
#define DBG_INIT() TRISECLR = 0x1F

//-------------------------- Start INT1 Code -----------------------------------
// Global variables for keeping track of the INT1 interrupts
bool interrupt_INT1_occurred = false;
unsigned int interrupt_INT1_count = 0;

// The interrupt handler for the INT1
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use

void __ISR(_EXTERNAL_1_VECTOR, IPL7AUTO) _External1Handler(void) {
    DBG_ON(MASK_DBG2);
    interrupt_INT1_occurred = true;
    interrupt_INT1_count++;
    INTClearFlag(INT_INT1);
    DBG_OFF(MASK_DBG2);
}

//Initialize the INT1 peripheral

void initINT1() {
    mPORTESetPinsDigitalIn(BIT_8);
    ConfigINT1(EXT_INT_ENABLE | FALLING_EDGE_INT | EXT_INT_PRI_7);
    INTClearFlag(INT_INT1);
}
//-------------------------- End INT1 Code -------------------------------------

//-------------------------- Start CN Code -------------------------------------
// Global variables for keeping track of the CN interrupts
bool interrupt_CN_occurred = false;
unsigned int interrupt_CN_count = 0;
unsigned int value_Read = 0;

// The interrupt handler for the CN
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use

void __ISR(_CHANGE_NOTICE_VECTOR, IPL5AUTO) _CNHandler(void) {
    DBG_ON(MASK_DBG3);
    interrupt_CN_occurred = true;
    interrupt_CN_count++;
    value_Read = mPORTBRead();
    INTClearFlag(INT_CN);
    DBG_OFF(MASK_DBG3);
}

//Initialize the CN peripheral

void initCN() {
    mPORTBSetPinsDigitalIn(BIT_15);
    mCNOpen(CN_ON, CN12_ENABLE, 0x0000);
    value_Read = mPORTBRead();
    ConfigIntCN(CHANGE_INT_PRI_5 | CHANGE_INT_ON);
    INTClearFlag(INT_CN);
}
//-------------------------- End CN Code ---------------------------------------

//-------------------------- Start Timer2 Code ---------------------------------
// Global variable to count number of times in timer2 ISR
unsigned int timer2_ms_value = 0;

// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    DBG_ON(MASK_DBG1);
    timer2_ms_value++; // Increment the millisecond counter.
    DBG_OFF(MASK_DBG1);
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

// Initialize timer2 and set up the interrupts

void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}
//-------------------------- End Timer2 Code -----------------------------------

//-------------------------- Start Button Code ---------------------------------
// Code for reading the buttons, note that this
// based off of the FSM developed from class.
// Debouncing is not used as these routines are only
// called one every 100 milliseconds.
//
// Programming Note: The data structure "ButtonStruct"
// is used to keep track of the state of the FSM for
// both buttons. Because of this, we only have to write
// down the button FSM once, and can keep it in a function.
// The function returns "true" when the button has
// been depressed.

// Button Definitions
#define BUTTON1_MASK 0x40
#define BUTTON1_PORT PORTG
#define BUTTON2_MASK 0x80
#define BUTTON2_PORT PORTG

// Button FSM definitions

typedef enum _ButtonStates {
    UP, DOWN_TRANSITION, DOWN
} ButtonStates;

// Data structure to keep track of a button's state in the FSM
// and how to read the button.

typedef struct _ButtonStruct {
    ButtonStates buttonState;
    int buttonNumber;
} ButtonStruct;

// We want to call this from the FSM function, so we have to
// know which button we are reading from.

bool readButton(int buttonNumber) {
    bool buttonPressed = false;
    if (buttonNumber == 1) {
        buttonPressed = (bool) (BUTTON1_PORT & BUTTON1_MASK);
    } else if (buttonNumber == 2) {
        buttonPressed = (bool) (BUTTON2_PORT & BUTTON2_MASK);
    }
    return (buttonPressed);
}

// An implementation of the button FSM machine from class. It is
// not debounced because it is called on a multi-millisecond
// time-scale. The state and button number is contained in the
// ButtonStruct data structure passed to the routine by
// reference.

bool checkButtonPressed(ButtonStruct *buttonStruct) {
    ButtonStates newButtonState = buttonStruct->buttonState;
    bool returnButtonPressedEvent;
    bool buttonPressed = readButton(buttonStruct->buttonNumber);
    returnButtonPressedEvent = false;
    switch (buttonStruct->buttonState) {
        case UP:
            if (buttonPressed) {
                newButtonState = DOWN_TRANSITION; // 1-to-0 transition detected
            }
            break;
        case DOWN_TRANSITION:
            returnButtonPressedEvent = true; // We record the button event
            newButtonState = DOWN; // This was the intermediate state
            break;
        case DOWN:
            if (!buttonPressed) {
                newButtonState = UP; // 1-to-0 transition detected
            }
            break;
        default: // Should never occur
            LATG = 0xF << 12;
            while (1) {
                /* do nothing */
            } // Trap for debugging
    }
    buttonStruct->buttonState = newButtonState;
    return (returnButtonPressedEvent);
}
//-------------------------- End Button Code -----------------------------------

// Helper funtion to write a line to the OLED.
// Be sure to clear line before writing so
// that non-overwritten letters dissappear...

void writeLine(int lineNumber, char line[]) {
    char oledstring[17];
    OledSetCursor(0, lineNumber);
    sprintf(oledstring, "                ");
    OledPutString(oledstring);
    OledSetCursor(0, lineNumber);
    OledPutString(line);
}

int main() {
    char oledstring[17];
    unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;

    ButtonStruct button1_data;
    ButtonStruct button2_data;
    bool button1_pressed;
    bool button2_pressed;

    // Initialize buttons' stuctures
    button1_data.buttonState = UP;
    button1_data.buttonNumber = 1;
    button2_data.buttonState = UP;
    button2_data.buttonNumber = 2;

    // Initialize GPIO for BTNs and LEDs
    // For BTN1 and BTN2: configure PortG bits for input
    mPORTGSetPinsDigitalIn(BUTTON1_MASK | BUTTON2_MASK);
    // For LEDs: configure PortG pins for output
    mPORTGSetPinsDigitalOut(BIT_12 | BIT_13 | BIT_14 | BIT_15);
    // Initialize LEDs to be off
    mPORTGClearBits(BIT_12 | BIT_13 | BIT_14 | BIT_15);

    // Initialize GPIO for debugging
    DBG_INIT();

    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();

    // Initialize Timer2, CN, and INT1
    initTimer2();
    initCN();
    initINT1();

    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();

    // Send a welcome message to the OLED display
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("INT1/CN Ex");

    while (1) {
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= 100) { // update every 100 milliseconds
            mPORTGSetBits(BIT_12); // Turn LED1 on
            button1_pressed = checkButtonPressed(&button1_data);
            if (button1_pressed) {
                DBG_ON(MASK_DBG4); // Start square wave on JB pin 4
                mPORTGSetBits(BIT_13); // Show BTN1 pressed on LED2
                writeLine(1, "Button1 Press");
            } else {
                DBG_OFF(MASK_DBG4); // End square wave on JB pin 4
                writeLine(1, "             "); // Clear line on OLED
                mPORTGClearBits(BIT_13); // End BTN1 on LED2
            }
            button2_pressed = checkButtonPressed(&button2_data);
            if (button2_pressed) {
                DBG_ON(MASK_DBG5); // Start square wave on JB pin 7
                mPORTGSetBits(BIT_14); // Show BTN2 pressed on LED3
                writeLine(1, "Button2 Press");
            } else {
                DBG_OFF(MASK_DBG5); // End square wave on JB pin 7
                writeLine(1, "             "); // Clear line on OLED
                mPORTGClearBits(BIT_14); // End BTN2 on LED3
            }
            if (interrupt_INT1_occurred) {
                sprintf(oledstring, "INT1 cnt: %3d", interrupt_INT1_count);
                writeLine(2, oledstring);
                interrupt_INT1_occurred = false;
            }
            if (interrupt_CN_occurred) {
                sprintf(oledstring, "CN cnt:   %3d", interrupt_CN_count);
                writeLine(3, oledstring);
                interrupt_CN_occurred = false;
            }
            timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            mPORTGClearBits(BIT_12); // Turn LED1 off
        }
    }
    return (EXIT_SUCCESS);
}

