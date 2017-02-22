////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        ADC Program Example
//
// File name:       ADC_Example_main - A simple demonstration of the ADC
// Description:     ADC looks at one channel, prints mean & "variance" to OLED
// Resources:       timer2 is configured to interrupt every 1.0ms, and
//                  ADC is configured to automatically restart and interrupt
//                  after each conversion (about 160us).
// How to use:      AN2 is on header JA, pin 1. Need to connect this to the
//                  analog line you are measuring (e.g., the U/D output on the
//                  2-axis parallax joystick.) Be sure and give power and ground
//                  to the side you are trying to measure (e.g., the GND and VSS
//                  pins on the JA header connected to GND and U/D+ on the
//                  joystick). Once running, the program should report a ADC
//                  average value between 0 and 1023, and a "variance" that is
//                  the sum of the absolute values of the difference of the
//                  readings and the mean value.
//                  A logic analyzer can be used to see
//                  exactly what is happening in the ISRs and the OLED
//                  update (header JB, pins 1, 2, and 3).
// Written by:      PEP
// Last modified:   17 October 2015

#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include <stdlib.h>
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

// Definitions for the ADC averaging. How many samples (should be a power
// of 2, and the log2 of this number to be able to shift right instead of
// divide to get the average.
#define NUM_ADC_SAMPLES 32
#define LOG2_NUM_ADC_SAMPLES 5
int ADC_mean;
int ADC_variance;

// Return value check macro
// TAKEN FROM THE GLYPH MAIN EXAMPLE CODE
#define CHECK_RET_VALUE(a) { \
  if (a == 0) { \
    LATGSET = 0xF << 12; \
    return(EXIT_FAILURE) ; \
  } \
}
//CREATE BLANK SPACE GLYPH
BYTE blankSpace[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char blankSpace_char = 0x00;
//CREATES THE HEAD OF THE SNAKE
BYTE snakeHead [8] = {0xFF, 0xFF, 0xCB, 0xDF, 0xDF, 0xCB, 0xFF, 0xFF};
char snakeHead_char = 0x01;
//CREATES THE BODY OF THE SNAKE
BYTE snakeBody [8] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
char snakeBody_char = 0x02;
//CREATES AN IMAGE OF THE APPLE
BYTE applePicture[8] = {0x00, 0x7C, 0x44, 0x4E, 0x4F, 0x45, 0x7C, 0x00};
char applePicture_char = 0x03;
//CREATE BORDER GLYPHS
BYTE borderLeft[8] = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char borderLeft_char = 0x06;

/************************** Button States *************************/
//SET UP BUTTON STATES
unsigned char Button1History = 0x00; // Last eight samples of BTN1
unsigned char Button2History = 0x00; // Last eight samples of BTN1
unsigned char Button3History = 0x00;

enum States {
    ST_READY, ST_BTN1_TRANSITION, ST_BTN1_DOWN, ST_BTN2_TRANSITION, ST_BTN2_DOWN, ST_BTN3_TRANSITION, ST_BTN3_DOWN
} sysState = ST_READY;

enum GameState {
    WELCOME, BUTTONSELECTION, BEGINGAME, CHECKMOVE,
};
enum GameState CurrGameState = WELCOME;
//enum GameState CurrentDirection = RIGHT;

typedef struct _position {
    int x;
    int y;
} position;
position snake_position[20];
/******************************************************************/

/******************** Global Variables ********************/
unsigned int Sec1000; // Millisecond counter
unsigned int SecPrev;
// SVETLANA EDIT: CREATED AN INT LEFTRIGHT AND UPDOWN
// LEFTRIGHT: MOVEMENT OF THE JOYSTICK LEFT TO RIGHT
// UPDOWN: MOVEMENT OF THE JOYSTICK UP AND DOWN
int LeftRight = 0;
int UpDown = 0;
int CurrentDirection = 0;
/**************************************************************/
char trackScore[20];
char goal[20];

int border = 0;
int retValue = 0;

unsigned int Goalcount = 12;
unsigned int snake_length = 1;
unsigned int snakesize = 0;

//RESET POSITION - TO USE LATER
//snake_position[0].x = 0;
//snake_position[0].y = 0;
/********************* GAME STATES*****************************/

/***************************************************************/
// The interrupt handler for the ADC
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
// Note that, in general, one should not be doing math
// in the ISR. I am doing it in this case so that you can
// see the time that it takes on the logic analyzer.
// THIS IS ALL PART OF THE GIVEN EXAMPLE CODE

void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) {
    //This macro returns the staus of the buffer fill bit
    //0 -> buffer locations 0-7 are being written by the ADC module
    //1 -> buffer locations 8-F are being written by the ADC module.
    if (ReadActiveBufferADC10() == 0) {
        //ADC will have data available in addresses 0 and 1
        //the input channel sampled by MUXA will have its digital output in address 0
        //input channel sampled by MUXB will have its digital output
        //in address 1)
        //or it will have data available in addresses 8 and 9
        LeftRight = ReadADC10(0);
        UpDown = ReadADC10(1);
    } else if (ReadActiveBufferADC10() == 1) {
        //ADC will have data available in addresses 0 and 1
        //the input channel sampled by MUXA will have its digital output in address 0
        //input channel sampled by MUXB will have its digital output
        //in address 1)
        //or it will have data available in addresses 8 and 9
        LeftRight = ReadADC10(8);
        UpDown = ReadADC10(9);
    }
    INTClearFlag(INT_AD1);
}


// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
// NONE OF THIS HAS BEEN MODIFIED.

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {


    Sec1000++; // Update millisecond counter

    Button1History <<= 1; // Update button history
    if (PORTG & 0x40) // Read current position of BTN1
        Button1History |= 0x01;

    Button2History <<= 1; // Update button history
    if (PORTG & 0x80) // Read current position of BTN2
        Button2History |= 0x01;

    Button3History <<= 1; // Update button history
    if (PORTA & 0x01) // Read current position of BTN2
        Button3History |= 0x01;
    INTClearFlag(INT_T2); // Acknowledge TMR2 interrupt

}

/****************************** HAVE NOT BEEN CHANGED FROM HOMEWORK 4*********/
//***********************************************
//* IMPORTANT: THE ADC CONFIGURATION SETTINGS!! *
//***********************************************

// ADC MUX Configuration
// Only using MUXA,MUXB, AN2 as positive input, VREFL as negative input
//ADC_CH0_POS_SAMPLEA_AN2: REFERS TO MUXA
//ADC_CH0_POS_SAMPLEB_AN3: REFERS TO MUXB
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEB_AN3 | ADC_CH0_NEG_SAMPLEB_NVREF

// ADC Config1 settings
// Data stored as 16 bit unsigned int
// Internal clock used to start conversion
// ADC auto sampling (sampling begins immediately following conversion)
// AUTO SAMPLING SHOULD NOT BE ON IF THERE IS ONLY 1 INPUT/MUX
#define AD_CONFIG1 ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON

// AD_CONFIG2: ADC Config2 settings
// ADC_VREF_AVDD_AVSS: USES (VDD and VSS) as reference voltages
// ADC_SCAN_OFF: Do not scan inputs
// ADC_SAMPLES_PER_INT_2: TAKES 2 SAMPLES PER INPUT
// ADC_BUF_8: Buffer mode is one 8-word buffer
// ADC_ALT_INPUT_OFF: Alternate sample mode ON
// WE WANT TO SAMPLE MUX A AND B
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | \
                  ADC_SAMPLES_PER_INT_2 | \
                  ADC_BUF_8 | ADC_ALT_INPUT_ON

// ADC Config3 settings
// Autosample time in TAD = 8
// Prescaler for TAD:  the 20 here corresponds to a
// ADCS value of 0x27 or 39 decimal => (39 + 1) * 2 * TPB = 8.0us = TAD
// NB: Time for an AD conversion is thus, 8 TAD for aquisition +
//     12 TAD for conversion = (8+12)*TAD = 20*8.0us = 160us.
// THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy

// ADC Port Configuration (PCFG)
// Not scanning, so nothing need be set here..
// NB: AN2 was selected via the MUX setting in AD_MUX_CONFIG which
// sets the AD1CHS register (true, but not that obvious...)
// THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
#define AD_CONFIGPORT ENABLE_ALL_DIG

// ADC Input scan select (CSSL) -- skip scanning as not in scan mode
// THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
#define AD_CONFIGSCAN SKIP_SCAN_ALL
/******************************************************************************/

// Initialize the ADC using my definitions
// Set up ADC interrupts

void initADC(void) {

    // Configure and enable the ADC HW
    // THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
    SetChanADC10(AD_MUX_CONFIG);
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN);
    EnableADC10();

    // Set up, clear, and enable ADC interrupts
    // THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
}

// Initialize timer2 and set up the interrupts

void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    // THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);

}

void initTimer3() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    // THIS HAS NOT BEEN MODIFIED FROM THE EXAMPLE
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_16 | T3_GATE_OFF, 3);
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();

}

void WelcomeScreen();
void buttonPress();
void CreateScoreBoard();
void CreateSnake();
void CreateApple();
void CreateBorder();
void CheckDirections();
void MoveLeft();
void MoveRight();
void MoveUp();
void MoveDown();
void HeadBeforeTail();
void ClearScreen();

int main() {

    DDPCONbits .JTAGEN = 0; // disable JTAG programmer so we can use BTN3
    TRISGSET = 0x40; // For BTN1: configure PortG bit for input
    TRISGSET = 0x80; // For BTN2: configure PortG bit for input
    TRISASET = 0x01;

    TRISBCLR = 0x0040;
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();

    retValue = OledDefUserChar(blankSpace_char, blankSpace);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(snakeHead_char, snakeHead);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(snakeBody_char, snakeBody);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(applePicture_char, applePicture);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(borderLeft_char, borderLeft);
    CHECK_RET_VALUE(retValue);

    // Initialize GPIO for debugging
    DBG_INIT();

    // Initial Timer2 and ADC
    initTimer2();
    initADC();
    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    // Enable the interrupt controller
    INTEnableInterrupts();

    snake_position[0].x = 0;
    snake_position[0].y = 0;

    while (1) {

        switch (CurrGameState) {
            case WELCOME:
                WelcomeScreen();
                SecPrev = Sec1000;
                while (Sec1000 < SecPrev + 5000);
                CurrGameState = BUTTONSELECTION;
                OledClear();
                break;
            case BUTTONSELECTION:
                buttonPress();
                break;
            case BEGINGAME:
                CreateScoreBoard();
                CreateSnake();
                CreateBorder();
                CurrGameState = CHECKMOVE;
                break;
            case CHECKMOVE:
                if (LeftRight <= 350) {
                    CurrentDirection = 1;
                }//RIGHT
                if (LeftRight >= 700) {
                    CurrentDirection = 2;
                }//UP
                if (UpDown <= 300) {
                    CurrentDirection = 3;
                } 
                if (UpDown >= 600) {
                    CurrentDirection = 4;
                }

                if (CurrentDirection == 1) {
                    MoveLeft();
                }
                if (CurrentDirection == 2) {
                    MoveRight();
                }
                if (CurrentDirection == 3) {
                    MoveDown();
                }
                if (CurrentDirection == 4) {
                    MoveUp();
                }
                break;
        }
    }
    return (EXIT_SUCCESS);
}

void WelcomeScreen() {
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("Snake (Kaa) Game");
    OledSetCursor(0, 1);
    OledPutString("Svetlana M.");
    OledSetCursor(0, 2);
    OledPutString("Made: 11/3/2015");
    OledClearBuffer();
}

void buttonPress() {
    char OLEDString[20];
    OledSetCursor(0, 0);
    sprintf(OLEDString, "Set Goal: %2d", Goalcount);
    OledPutString(OLEDString);
    OledSetCursor(0, 1);
    OledPutString("Btn1: Goal++");
    OledSetCursor(0, 2);
    OledPutString("Btn2: Goal--");
    OledSetCursor(0, 3);
    OledPutString("Btn3: Accept");

    switch (sysState) {
        case ST_READY:
            if (Button1History == 0xFF)
                sysState = ST_BTN1_TRANSITION; // BTN1 transition detected
            else if (Button2History == 0xFF)
                sysState = ST_BTN2_TRANSITION; // BTN2 transition detected
            else if (Button3History == 0xFF)
                sysState = ST_BTN3_TRANSITION; // BTN2 transition detected
            break;

        case ST_BTN1_TRANSITION:
            if (CurrGameState = BUTTONSELECTION) {
                Goalcount++; // Update display
                OledSetCursor(0, 0);
                sprintf(OLEDString, "Set Goal: %2d", Goalcount);
                OledPutString(OLEDString);
                OledUpdate();
                sysState = ST_BTN1_DOWN;
            }
            break;
        case ST_BTN1_DOWN:
            if (Button1History == 0x00)
                sysState = ST_READY; // Release transition detected
            break;

        case ST_BTN2_TRANSITION:
            if (Goalcount > 0) {
                Goalcount = Goalcount - 1; // Update display
            }
            OledSetCursor(0, 0);
            sprintf(OLEDString, "Set Goal: %2d", Goalcount);
            OledPutString(OLEDString);
            OledUpdate();
            sysState = ST_BTN2_DOWN;
            break;
        case ST_BTN2_DOWN:
            if (Button2History == 0x00)
                sysState = ST_READY; // Release transition detected
            break;

        case ST_BTN3_TRANSITION:
            sysState = ST_BTN3_DOWN;
            break;
        case ST_BTN3_DOWN:
            if (Button3History == 0x00) {
                CurrGameState = BEGINGAME;
                sysState = ST_READY;
                // elease transition detected
            }
            break;
    }
}

void CreateScoreBoard() {
    OledClearBuffer();
    OledSetCursor(10, 0);
    OledPutString("Score:");
    OledSetCursor(10, 1);
    //SOMETHING WRONG WITH THIS LINE
    sprintf(trackScore, "%6d", snake_length - 1);
    OledPutString(trackScore);
    OledSetCursor(10, 2);
    OledPutString("Goal:");
    OledSetCursor(10, 3);
    sprintf(goal, "%6d", Goalcount);
    OledPutString(goal);
}

void CreateSnake() {
    snake_position[0].x = 0;
    snake_position[0].y = 0;
    OledSetCursor(0, 0);
    OledDrawGlyph(snakeHead_char);
}

void CreateApple() {
    //CREATES A RANDOM X COORDINATE FROM 0 TO 9
    int randomapplex = rand() % 9;
    //CREATES A RANDOM Y COORDINATE TAHT CAN RANGE
    //FROM 0 - 4
    int randomappley = rand() % 4;
    srand(Sec1000);

    if ((randomapplex == snake_position[0].x) &&
            (randomappley == snake_position[0].y)) {
        randomapplex;
        randomappley;
        srand(Sec1000);
    }
    OledSetCursor(randomapplex, randomappley);
    OledDrawGlyph(applePicture_char);
}

void CreateBorder() {
    while (border < 4) {
        OledSetCursor(9, border);
        OledPutChar(borderLeft_char);
        border++;
    }
    OledUpdate();
}

void CheckDirections() {
}

void MoveLeft() {
    HeadBeforeTail();

    snake_position[0].x--;

    OledSetCursor(snake_position[0].x, snake_position[0].y);
    OledDrawGlyph(snakeHead_char);
    OledUpdate();
    CurrGameState = CHECKMOVE;
}

void MoveRight() {
   HeadBeforeTail();

    snake_position[0].x++;

    OledSetCursor(snake_position[0].x, snake_position[0].y);
    OledDrawGlyph(snakeHead_char);
    OledUpdate();
    CurrGameState = CHECKMOVE;
}

void MoveUp() {
    HeadBeforeTail();

    snake_position[0].y--;

    OledSetCursor(snake_position[0].x, snake_position[0].y);
    OledDrawGlyph(snakeHead_char);
    OledUpdate();
    CurrGameState = CHECKMOVE;
}

void MoveDown() {
   HeadBeforeTail();

    snake_position[0].y++;

    OledSetCursor(snake_position[0].x, snake_position[0].y);
    OledDrawGlyph(snakeHead_char);
    OledUpdate();
    CurrGameState = CHECKMOVE;
}

void HeadBeforeTail() {
    OledSetCursor(snake_position[0].x, snake_position[0].y);
    OledDrawGlyph(snakeBody_char);

    OledSetCursor(snake_position[snake_length - 1].x, snake_position[snake_length - 1].y);
    OledDrawGlyph(blankSpace_char);

    int snakesize = snake_length - 1;
    while (snakesize > 0) {
        int snake = snakesize - 1;
        snake_position[snakesize] = snake_position[snake];
        snakesize--;
    }
}

void ClearScreen() {
    if (PORTG & 0x40) {
        OledClear();
        //SET UP BUTTON STATES
        Button1History = 0x00; // Last eight samples of BTN1
        Button2History = 0x00; // Last eight samples of BTN1
        Button3History = 0x00;

        position snake_position[20];
        Sec1000 = 0; // Millisecond counter
        SecPrev = 0;

        LeftRight = 0;
        UpDown = 0;
        CurrentDirection = 0;

        border = 0;
        retValue = 0;

        Goalcount = 12;
        snake_length = 1;
        snakesize = 0;

        CurrGameState = WELCOME;
    }

}