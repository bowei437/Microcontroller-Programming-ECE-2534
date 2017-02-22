////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        SPI Program Example
//
// File name:       SPI_Example_main - A demonstration of Master/Slave SPI
//
// Description:     Pressing BTN1 asks for the name of a fruit. Pressing
//                  BTN2 asks for the name of a veggie. In either case, the
//                  Slave devices receives the request and returns a C-string
//                  to the Master. The returned string is then displayed on
//                  the OLED.
//
// Resources:       timer2 is configured to interrupt every 1.0ms, and
//                  the Slave SPI device is configured to interrupt on the
//                  receipt of a byte from the Master. The Master SPI is
//                  not interrupt driven.
//
// How to use:      Pins to the two SPI peripherals are located on headers
//                  JE (SPI3) and JF (SPI4). SPI3 is configured as the Master
//                  and SPI4 is configured as the Slave. (These configurations
//                  can be changed via the program definitions.) To have the
//                  two SPI peripherals talk to each other, they must be
//                  connected. You can use your "medusa" cable for this.
//                  On both headers, the pin assignments are SPI Select (SS) on
//                  Pin 1, SPI Data Out (SDO) on Pin 2, SPI Data In (SDI) on
//                  Pin 3, and SPI Clock (SCK) on Pin 4. Connected these 4 pins
//                  between headers JPE and JPF will allow the two SPI peripherals
//                  to communicate. Important: Note that you have connect the SDO
//                  of one SPI device to the SDI of the other SPI device (and visa
//                  versa for the other SDO/SDI wire. Otherwise, they cannot
//                  talk to each other!
//                  A logic analyzer can be used to see the timer ISR and
//                  SPI Slave ISR occurring (header JB, pins 1, 2, and 3).
//                  The logic analyzer can also be connected to the SPI input
//                  and output lines to see the SPI traffic.
//
// Written by:      PEP
// Last modified:   1 November 2015

#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "stdbool.h"
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

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
#define DBG_ON(a)  LATESET = a
#define DBG_OFF(a) LATECLR = a
#define DBG_INIT() TRISECLR = 0x7



#define NUM_WORDS 10
#define MAX_WORD_LENGTH 17

const char FRUIT_NAMES[NUM_WORDS][MAX_WORD_LENGTH] = {
    "Kumquat",
    "Carambola",
    "Durian",
    "Jackfruit",
    "Guava",
    "Mango",
    "Tomato",
    "Tomatillo",
    "Bhut Jolokia",
    "Pomegranate",
};

const char VEGGIE_NAMES[NUM_WORDS][MAX_WORD_LENGTH] = {
    "Cabbage",
    "Collard greens",
    "Fiddlehead",
    "Lizard's tail",
    "Pak choy",
    "Sea beet",
    "Tatsoi",
    "Wheatgrass",
    "Melokhia",
    "Dandelion",
};

// Constants for defining the SPI Master and Slave configuration
// For these definitions, Master is on SPI3 (header JE),
// the Slave is on SPI4 (header JF)
// The SPI clock period should be SPI_SOURCE_CLOCK_DIVIDER*T_PB
// Thus, T_SPI = 2.0usec for SPI_SOURCE_CLOCK_DIVIDER = 20
const unsigned int SPI_SOURCE_CLOCK_DIVIDER = 20;


// Global variable to count number of times in timer2 ISR
unsigned int timer2_ms_value = 0;

// The interrupt handler for the SPI
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
// Note that, in general, one should not be spending
// much time in the ISR. However, in this case, I
// have to maintain a FSM so that I know where I
// am in the transmission of the requested C-String.
// I have the ISR instrumented so that you can see
// when you are in the SPI Slave ISR, and also when
// the ISR is in the initial "WAITING_FOR_REQUEST"
// state (where the ISR will read the request from
// the SPI master for either a fruit or veggie.

// Programming Note: check out the pointer
// "current_char_pointer". This is used to keep
// track of where we are in the current C-string
// that we are transmitting. We increment this pointer
// as we move through the C-string, and then dereference
// the pointer when want to get the current char to
// send back to the SPI Master. Also, note the "static"
// definitions are used so that the  these values are
// persistent between calls to the ISR.

typedef enum _isr_state {
    WAITING_FOR_REQUEST, SENDING_DATA, FINISHING_SEND
} isr_state;

void __ISR(_SPI_3_VECTOR, IPL7SRS) _SPISlaveHandler(void) {
    static isr_state current_isr_state = WAITING_FOR_REQUEST;
    static char *current_char_pointer;
    static int current_veggie = 0;
    static int current_fruit = 0;
    int receivedData;
    char dataToSend = 0;
    DBG_ON(MASK_DBG2);
    receivedData = SpiChnReadC(SPI_CHANNEL3);
    switch (current_isr_state) {
        case WAITING_FOR_REQUEST:
            DBG_ON(MASK_DBG3);
            if (receivedData == 1) {
                dataToSend = FRUIT_NAMES[current_fruit][0];
                current_char_pointer = (char *) &FRUIT_NAMES[current_fruit][1];
                current_fruit = (current_fruit + 1) % NUM_WORDS;
            } else {
                dataToSend = VEGGIE_NAMES[current_veggie][0];
                current_char_pointer = (char *) &VEGGIE_NAMES[current_veggie][1];
                current_veggie = (current_veggie + 1) % NUM_WORDS;
            }
            current_isr_state = SENDING_DATA;
            DBG_OFF(MASK_DBG3);
            break;
        case SENDING_DATA:
            dataToSend = (*current_char_pointer);
            if (dataToSend == 0) {
                current_isr_state = FINISHING_SEND;
            } else {
                current_char_pointer++;
            }
            break;
        case FINISHING_SEND:
            dataToSend = 0xFF;
            current_isr_state = WAITING_FOR_REQUEST;
            break;
        default:
            LATG = 0xF << 12;
            while (1) {
                /* do nothing */
            } // Trap for debugging
            break;
    }
    SpiChnWriteC(SPI_CHANNEL3, dataToSend);
    DBG_OFF(MASK_DBG2);
    INTClearFlag(INT_SPI3RX);
}

// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    DBG_ON(MASK_DBG1);
    timer2_ms_value++; // Increment the millisecond counter.
    DBG_OFF(MASK_DBG1);
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

//Initialize the SPI channel that is being used

void initMasterSPI() {
    SpiChnOpen(SPI_CHANNEL4,
            SPI_OPEN_MSTEN
            | SPI_OPEN_MSSEN
            | SPI_OPEN_CKP_HIGH
            | SPI_OPEN_CKE_REV
            | SPI_OPEN_MODE8
            | SPI_OPEN_ENHBUF
            | SPI_OPEN_ON,
            SPI_SOURCE_CLOCK_DIVIDER);
}

//Initialize the SPI Slave channel that is being used

void initSlaveSPI() {
    SpiChnOpen(SPI_CHANNEL3,
            SPI_OPEN_SLVEN
            | SPI_OPEN_CKP_HIGH
            | SPI_OPEN_CKE_REV
            | SPI_OPEN_MODE8
            | SPI_OPEN_ENHBUF
            | SPI_OPEN_RBF_NOT_EMPTY
            | SPI_OPEN_ON,
            SPI_SOURCE_CLOCK_DIVIDER);
    INTSetVectorPriority(INT_SPI_3_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_SPI3RX);
    INTEnable(INT_SPI3RX, INT_ENABLED);
}

//Send request over SPI, read a C-string back

void sendMasterSPIData(int request, char inData[]) {
    int i = 0;
    // Make request first
    SpiChnPutC(SPI_CHANNEL4, request);
    char dummyValue = SpiChnGetC(SPI_CHANNEL4);
    while (1) {
        SpiChnPutC(SPI_CHANNEL4, dummyValue);
        inData[i] = SpiChnGetC(SPI_CHANNEL4);
        if (inData[i] == 0) {
            return;
        }
        i++;
    }
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
    char returnArray[17];
    unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;
    ButtonStruct button1_data;
    ButtonStruct button2_data;
    bool button1_pressed;
    bool button2_pressed;

    // Initialize buttons stuctures
    button1_data.buttonState = UP;
    button1_data.buttonNumber = 1;
    button2_data.buttonState = UP;
    button2_data.buttonNumber = 2;

    // Initialize GPIO for BTNs and LEDs
    // For BTN1 and BTN2: configure PortG bits for input
    TRISGSET = BUTTON1_MASK | BUTTON2_MASK;
    TRISGCLR = 0xf000; // For LEDs: configure PortG pins for output
    ODCGCLR = 0xf000; // For LEDs: configure as normal output (not open drain)
    LATGCLR = 0xf000; // Initialize LEDs to 0000

    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();

    // Initialize GPIO for debugging
    DBG_INIT();

    // Initialize Timer2, Master SPI, and Slave SPI
    initTimer2();
    initMasterSPI();
    initSlaveSPI();

    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();

    // Send a welcome message to the OLED display
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("SPI Example");

    while (1) {
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= 100) { // update every 100 milliseconds
            LATGSET = 1 << 12; // Turn LED1 on
            button1_pressed = checkButtonPressed(&button1_data);
            if (button1_pressed) {
                sendMasterSPIData(1, returnArray);
                writeLine(2, "Ask for fruit");
                writeLine(3, returnArray);
                LATGSET = 1 << 13; // Show BTN1 pressed on LED2
            } else {
                LATGCLR = 1 << 13;
            }
            button2_pressed = checkButtonPressed(&button2_data);
            if (button2_pressed) {
                sendMasterSPIData(2, returnArray);
                writeLine(2, "Ask for veggie");
                writeLine(3, returnArray);
                LATGSET = 1 << 14; // Show BTN2 pressed on LED3
            } else {
                LATGCLR = 1 << 14;
            }
            timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            sprintf(oledstring, "T2 Val: %6d", timer2_ms_value_save / 100);
            OledSetCursor(0, 1);
            OledPutString(oledstring);
            LATGCLR = 1 << 12; // Turn LED1 off
        }
    }
    return (EXIT_SUCCESS);
}

