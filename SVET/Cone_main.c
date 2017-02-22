////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 3
//
// File name:       ADC_Example_main
//
// Description:     Uses ADC and OLED to play Snake!!!!  It works...that's all
//                  you need to know :).  THe only thing I have to say is when
//                  you try to move the joystick, hold it in the direction you
//                  want to go just a little bit longer.  Like if you want to
//                  go left, move the joystick to the left..for 1 second.
//
// Written by:      PEP, conniel1
// Last modified:   3 November 2015

#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include <stdlib.h>
#include <time.h>

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

// Global variable to count number of times in timer2 ISR
unsigned int timer2_ms_value = 0;

// Computed mean and variance from ADC values
int ADC_mean, ADC_variance;
int upAndDown = 0;
int leftAndRight = 0;
unsigned millisec;


// The interrupt handler for the ADC
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
// Note that, in general, one should not be doing math
// in the ISR. I am doing it in this case so that you can
// see the time that it takes on the logic analyzer.
void __ISR(_ADC_VECTOR, IPL7SRS)  _ADCHandler(void) {
    static unsigned int current_reading = 0;
    static unsigned int ADC_Readings[NUM_ADC_SAMPLES];
    unsigned int i;
    unsigned int total;

    //if the mux on the left is writing, then read in the values from the right
    //mux that is reading.  8 is the value from muxA and 9 is the value from
    //muxB
    if (ReadActiveBufferADC10() == 0)
    {
        upAndDown = ReadADC10(9);
        leftAndRight = ReadADC10(8);
    }
    else if ( ReadActiveBufferADC10() == 1 )
    {
        upAndDown = ReadADC10(1);
        leftAndRight = ReadADC10(0);
    }
    DBG_ON(MASK_DBG2);
    ADC_Readings[current_reading] = ReadADC10(0);
    current_reading++;
    if (current_reading == NUM_ADC_SAMPLES) {
        current_reading = 0;
        total = 0;
        for (i = 0; i < NUM_ADC_SAMPLES; i++) {
            total += ADC_Readings[i];
        }
        ADC_mean = total >> LOG2_NUM_ADC_SAMPLES; // divide by num of samples
        total = 0;
        for (i = 0; i < NUM_ADC_SAMPLES; i++) {
            total += abs(ADC_mean - ADC_Readings[i]);
        }
        ADC_variance = total; // BTW, this isn't actually the variance..
    }

    DBG_OFF(MASK_DBG2);
    INTClearFlag(INT_AD1);
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

//***********************************************
//* IMPORTANT: THE ADC CONFIGURATION SETTINGS!! *
//***********************************************

// ADC MUX Configuration
// Only using MUXA, MUXB, AN3, AN2 as positive input, VREFL as negative input
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEB_AN3 | ADC_CH0_NEG_SAMPLEB_NVREF

// ADC Config1 settings
// Data stored as 16 bit unsigned int
// Internal clock used to start conversion
// ADC auto sampling (sampling begins immediately following conversion)
#define AD_CONFIG1 ADC_FORMAT_INTG | ADC_CLK_TMR | ADC_AUTO_SAMPLING_ON

// ADC Config2 settings
// Using internal (VDD and VSS) as reference voltages
// Do not scan inputs
// Two samples per interrupt
// Buffer mode is two 8-word buffer
// Alternate sample mode on
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | \
                  ADC_SAMPLES_PER_INT_2 | \
                  ADC_BUF_8 | ADC_ALT_INPUT_ON

// ADC Config3 settings
// Autosample time in TAD = 8
// Prescaler for TAD:  the 20 here corresponds to a
// ADCS value of 0x27 or 39 decimal => (39 + 1) * 2 * TPB = 8.0us = TAD
// NB: Time for an AD conversion is thus, 8 TAD for aquisition +
//     12 TAD for conversion = (8+12)*TAD = 20*8.0us = 160us.
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy

// ADC Port Configuration (PCFG)
// Not scanning, so nothing need be set here..
// NB: AN2 was selected via the MUX setting in AD_MUX_CONFIG which
// sets the AD1CHS register (true, but not that obvious...)
#define AD_CONFIGPORT ENABLE_ALL_DIG

// ADC Input scan select (CSSL) -- skip scanning as not in scan mode
#define AD_CONFIGSCAN SKIP_SCAN_ALL

// Initialize the ADC using my definitions
// Set up ADC interrupts
void initADC(void) {

    // Configure and enable the ADC HW
    SetChanADC10(AD_MUX_CONFIG);
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN);
    EnableADC10();

        // Configure Timer 3 to request a real-time interrupt once
    // per millisecond based on constant from above.
//    OpenTimer3(T3_ON|T3_IDLE_CON|T3_SOURCE_INT|T3_PS_1_16|T3_GATE_OFF,
//            624);
    
    // Set up, clear, and enable ADC interrupts
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
}

// This is the timer interrupt handler. The interrupt handler is structured as a
// C function, but it has the distinction of not being called by main. In a sense,
// an interrupt handler is a hardware-called (or hardware-triggered) function and
// not a software-called function.

// Initialize timer2 and set up the interrupts
void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 ms.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}


//border stuff
// Return value check macro
#define CHECK_RET_VALUE(a) { \
  if (a == 0) { \
    LATGSET = 0xF << 12; \
    return(EXIT_FAILURE) ; \
  } \
}

 //all of my awesome hand crafted glyphs...:D
    //this is the head of the snake/a block
    BYTE topAndBot[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    char topAndBot_char = 0x00;
    //a blank 8x8 pixel
    BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    char blank_char = 0x01;
    //the apple that the snake will eat
    BYTE apple[8] = {0x78, 0xfc, 0xfc, 0xff, 0xff, 0xf4, 0xe4, 0x78};
    char apple_char = 0x02;
    //the body of the snake
    BYTE body[8] = {0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff};
    char body_char = 0x03;
    //a failed border
    BYTE border[8] = {0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00};
    char border_char = 0x04;

int main() {
   char oledstring[17];
    unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;
    int retValue = 0;

    DDPCONbits.JTAGEN = 0; // JTAG controller disable to use BTN3

    TRISGSET = 0x40;     // For BTN1: configure PORTG bit for input
    TRISGSET = 0x80;     // For BTN2: configure PORTG bit for input
    TRISASET = 0x1;      // For BTN3: configure PORTA bit for input

        // led configuring
    TRISGCLR = 0xf000;    
    ODCGCLR = 0xf000;

    //Initialize BPIO for Vcc
    TRISBCLR = 0x0040;
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;


    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();

      // Set up our user-defined characters for the OLED
    retValue = OledDefUserChar(blank_char, blank);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(topAndBot_char, topAndBot);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(apple_char, apple);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(body_char, body);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(border_char, border);
    CHECK_RET_VALUE(retValue);
    
    // Initialize GPIO for debugging
    DBG_INIT();

    // Initial Timer2 and ADC
    initTimer2();
    initADC();

     OpenTimer3(T3_ON|T3_IDLE_CON|T3_SOURCE_INT|T3_PS_1_16|T3_GATE_OFF,
            624);
    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();

    //setspeed is what the user will choose.  the three set speeds are 0, 1, and
    //2 where 0 is slug, 1 is worm, and 2 is python
    int setspeed = 0;
    //the speed of the snake. the three speeds are 1, 1.5, 2 and those numbers
    //are 8x8 pixel per......time
    int speed = 1;

    //headPos is the boolean of whether the head is at one of the borders.  0
    //means that the head is not at a wall while 1 means that it is at a wall
    int headPos = 0;

    //lostgX is is the x position of the glyps for the lost scenario
    int lostGX = 0;
    //wongGX is the x position of the glyps for the won scenario
    int wonGX = 0;
    //bor is the y value of the border that is to the left of the score/goal
    int bor = 0;

    //these are the coordinates of the apple
    int xrand = 0;
    int yrand = 0;

    //goal is the number that the user wants to get
    int goal = 5;

   int currB1pos = 0; //if this is 0, that means it has been pressed
   int currB2pos = 0; //if this is 0, that means it has been pressed
   int currB3pos = 0; //if this is 0, that means it has been pressed

   //START - updates the screen with the start stuff
   //CHECKSTART - Moves on to the next case after 5 seconds..aka shows the start
   //             screen for five seconds
   //CREATE - creates a snake and an apple.  Also adds the score/goal to the oled
   //         and randomply places an apple
   //SETGOAL - shows the screen that tells the user how to set the goal.  the
   //          goal number is updated
   //LEFT - moves the snake to the left and updates the snake array
   //RIGHT - moves the snake to the right "                         "
   //UP - moves the snake up and "                            "
   //DOWN - moves the snake head down and "                     "
   //CHECKMOVE - checks the joystick and goes to the appropraite state
   //CHECKATE - checks to see if the apple was eaten and then goes to the
   //           appropriate state
   //ATE - increase the snake size, update the score, and create another
   //       apple at a random location
   //CHECKLOST - checks to see if the snake hit the wall or hit itself
   //LOST - Shows the lost message...>:)
   //CHECKWIN - checks to see if the user met the goal
   //WON - shows the win message
    enum MOVE{START, CHECKSTART, CREATE, SETGOAL, LEFT, RIGHT, UP, DOWN,
    CHECKMOVE, CHECKATE, ATE, CHECKLOST, LOST, CHECKWIN, WON, UPDATEGOAL};
    //movement is the state of the game
    enum MOVE MOVEMENT = START;
    //curr is the current snake movement, it's either left, right, up, or down
    enum MOVE CURR = RIGHT;

    //BTN1P - button 1 is pressed, this is where the speed is incremented,
    //        goal is incremented, or the game is reset
    //BTN2P - button 2 is pressed , this is where we check to see if the speed
    //        is set or they want to decrease the desired goal
    //BTN3P - sets the goal...yeeee
    //CHECKBUTTON - cehcks to see which button is pressed, else stays in the state
    enum BUTTON{BTN1P, BTN2P, BTN3P, CHECKBUTTON};
    enum BUTTON BTNSTATE = CHECKBUTTON;

    //the snake struct
    typedef struct __position {
        int x;
        int y;
    } position;

    while (1) {
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= 100) {
            DBG_ON(MASK_DBG3);
            LATGSET = 1 << 12; // Turn LED1 on
            timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            int time = timer2_ms_value_save / 1000;
                    
            switch(MOVEMENT){
                case START:
                    // Send a welcome message to the OLED display
                    OledClearBuffer();
                    sprintf(oledstring, "SNAKE GAME");
                    OledSetCursor(0,0);
                    OledPutString(oledstring);
                    sprintf(oledstring, "Connie Lim");
                    OledSetCursor(0,1);
                    OledPutString(oledstring);
                    //counting down
                    sprintf(oledstring, "5");
                    OledSetCursor(7,3);
                    OledPutString(oledstring);
                    OledUpdate();
                    DelayMs(1000);
                    sprintf(oledstring, "4");
                    OledSetCursor(7,3);
                    OledPutString(oledstring);
                    OledUpdate();
                    DelayMs(1000);
                    sprintf(oledstring, "3");
                    OledSetCursor(7,3);
                    OledPutString(oledstring);
                    OledUpdate();
                    DelayMs(1000);
                    sprintf(oledstring, "2");
                    OledSetCursor(7,3);
                    OledPutString(oledstring);
                    OledUpdate();
                    DelayMs(1000);
                    sprintf(oledstring, "1");
                    OledSetCursor(7,3);
                    OledPutString(oledstring);
                    OledUpdate();
                    DelayMs(1000);
                    MOVEMENT = SETGOAL;
                    break;
                case SETGOAL:
                    goal = 5;
                    OledClearBuffer();
                    sprintf(oledstring, "SET GOAL:%6d", goal);
                    OledSetCursor(0,0);
                    OledPutString(oledstring);
                    sprintf(oledstring, "BTN1: Goal++");
                    OledSetCursor(0,1);
                    OledPutString(oledstring);
                    sprintf(oledstring, "BTN2: Goal--");
                    OledSetCursor(0,2);
                    OledPutString(oledstring);
                    sprintf(oledstring, "BTN3: Accept");
                    OledSetCursor(0, 3);
                    OledPutString(oledstring);
                    OledUpdate();
                    MOVEMENT = UPDATEGOAL;
                    break;
                case UPDATEGOAL:
                    sprintf(oledstring, "SET GOAL:%6d", goal);
                    OledSetCursor(0,0);
                    OledPutString(oledstring);
                    break;
                case CREATE:
                    bor = 0;
                    OledClearBuffer();
                    //creating a snake with the length of goal and initializing
                    //the snake's length to 1
                    position snake_position[20];
                    unsigned int snake_length = 1;
                    //it starts at the origin, so assigning the head value to
                    //(0,0).  snake_position[0] is the head
                    snake_position[0].x = 0;
                    snake_position[0].y = 0; // snake starts in the upper left-hand corner of the playing field
                    //generating random values for the apple to randomly appear
                    //on the board
                    srand (timer2_ms_value);
                 /* generate secret number between 0 and 9: */
                    xrand = rand() % 9 ;
                    yrand = rand() % 4;
                    //creates new random numbers if the random coordinates == (0, 0)
                    while((xrand == snake_position[0].x) &&
                            (yrand == snake_position[0].y)){
                         srand (timer2_ms_value);
                 /* generate secret number between 1 and 10: */
                        xrand = rand() % 9 ;
                        yrand = rand() % 4;
                    }

                    //adding the snake and apple
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledDrawGlyph(topAndBot_char);
                    OledSetCursor(xrand, yrand);
                    OledDrawGlyph(apple_char);
                    //setting the score/goal screen with the border
                    sprintf(oledstring, "SCORE:");
                    OledSetCursor(10,0);
                    OledPutString(oledstring);
                    sprintf(oledstring, "%6d", snake_length - 1);
                    OledSetCursor(10,1);
                    OledPutString(oledstring);
                    sprintf(oledstring, "GOAL:");
                    OledSetCursor(10,2);
                    OledPutString(oledstring);
                    sprintf(oledstring, "%6d", goal);
                    OledSetCursor(10,3);
                    OledPutString(oledstring);
                    //adding the border
                    while(bor < 4){
                        OledSetCursor(9, bor);
                        OledDrawGlyph(topAndBot_char);
                        bor++;
                    }
                    OledUpdate();
                    MOVEMENT = RIGHT;
                    break;
                    //this is what the user will see when picking the speed
                case LEFT:
                    CURR = LEFT;
                    //setting the head of the snake before it's moved to a body
                    //part
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(body_char);
                    //setting the tail before the snake moves to a blank space
                    OledSetCursor(snake_position[snake_length - 1].x, snake_position[snake_length - 1].y);
                    OledDrawGlyph(blank_char);
                    //updating the array of the snake's position
                    int i = snake_length - 1;
                    while(i > 0){
                        int j = i - 1;
                        snake_position[i] = snake_position[j];
                        i--;
                    }
                    //moving the head the left and setting the head there
                    snake_position[0].x--;
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(topAndBot_char);
                    OledUpdate();
                    MOVEMENT = CHECKATE;
                    break;
                case RIGHT:
                    CURR = RIGHT;
                    //setting the head of the snake before it's moved to a body
                    //part
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(body_char);
                    //setting the tail to a blank space
                    OledSetCursor(snake_position[snake_length - 1].x, snake_position[snake_length - 1].y);
                    OledDrawGlyph(blank_char);
                    //updating the snake array
                    int k = snake_length - 1;
                    while(k > 0){
                        int j = k - 1;
                        snake_position[k] = snake_position[j];
                        k--;
                    }
                    //moving the head the right and setting the head there
                    snake_position[0].x++;
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(topAndBot_char);
                    OledUpdate();
                    MOVEMENT = CHECKATE;
                    break;
                case UP:
                    CURR = UP;
                    //setting the head of the snake before it's moved to a body
                    //part
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(body_char);
                    //setting the tail to a blank space
                    OledSetCursor(snake_position[snake_length - 1].x, snake_position[snake_length - 1].y);
                    OledDrawGlyph(blank_char);
                    int l = snake_length - 1;
                    while(l > 0){
                        int j = l - 1;
                        snake_position[l] = snake_position[j];
                        l--;
                    }
                    //moving the head the down and setting the head there
                    snake_position[0].y--;
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(topAndBot_char);
                    OledUpdate();
                    MOVEMENT = CHECKATE;
                    break;
                case DOWN:
                    CURR = DOWN;
                    //setting the head of the snake before it's moved to a body
                    //part
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(body_char);
                    //setting the tail to a blank space
                    OledSetCursor(snake_position[snake_length - 1].x, snake_position[snake_length - 1].y);
                    OledDrawGlyph(blank_char);
                    //updating the snake array
                    int m = snake_length - 1;
                    while(m > 0){
                        int j = m - 1;
                        snake_position[m] = snake_position[j];
                        m--;
                    }
                    //moving the head the up and setting the head there
                    snake_position[0].y++;
                    OledSetCursor(snake_position[0].x, snake_position[0].y);
                    OledDrawGlyph(topAndBot_char);
                    OledUpdate();
                    MOVEMENT = CHECKATE;
                    break;
                case CHECKATE:
                    //checks to see if the apple and the head of the snake
                    //has the same coordinates.  if it does, then that means
                    ///that the apple has been eaten
                    if(snake_position[0].x == xrand && snake_position[0].y == yrand){
                        MOVEMENT = ATE;
                    }
                    else{
                        MOVEMENT = CHECKLOST;
                    }
                    break;
                case CHECKLOST:
                    headPos = 0;
                    //checks to see if the snake hits a wall
                    if(snake_position[0].x < 0 || snake_position[0].x > 8 ||
                            snake_position[0].y < 0 || snake_position[0].y >3){
                        MOVEMENT = LOST;
                    }
                    else{
                        //looking to see if the snake hit itself.  comparing
                        //all of the values of the array aka the body to the
                        //snake's head
                        int p = snake_length - 1;
                        while(p > 0){
                            if(snake_position[0].x == snake_position[p].x &&
                                    snake_position[0].y == snake_position[p].y){
                                headPos = 1;
                            }
                            p--;
                        }
                        //if it did hit itself, then the game is lost
                        if(headPos == 1){
                            MOVEMENT = LOST;
                        }
                        else{
                            MOVEMENT = CHECKWIN;
                        }
                    }
                    break;
                case ATE:
                    //increasing the snake size
                     snake_length++;
                     //setting the position to be equal to the tail
                    snake_position[snake_length] = snake_position[snake_length - 1];
                    MOVEMENT = CHECKMOVE;
                    //creating the apple
                    //match is a boolean to see if the coordinates of the
                    //random apple match any part of the snake. 1 is match
                    //and 0 is no match
                    int match = 1;
                    //outer while loop creating the random number, this will
                    //run until it randomly comes up with coordinates that
                    //aren't in the snake
                    while(match == 1){
                        int r = snake_length;
                        srand (timer2_ms_value);
                 /* generate secret number between 1 and 10: */
                        xrand = rand() % 9 ;
                        yrand = rand() % 4;
                        //inner loop checking if it matches the snake's coordinates
                        while(r > 0){
                            if((xrand == snake_position[r - 1].x) &&
                                (yrand == snake_position[r - 1].y)){
                                match = 1;
                                break;
                            }
                            else{
                                match = 0;
                            }
                            r--;
                        }
                    }
                    //placing the apple
                    OledSetCursor(xrand, yrand);
                    OledDrawGlyph(apple_char);
                    //updating the score
                    sprintf(oledstring, "%6d", snake_length - 1);
                    OledSetCursor(10,1);
                    OledPutString(oledstring);
                    OledUpdate();
                    break;
                case CHECKMOVE:
                    //checking the joystick's potentiometer
                    if(upAndDown < 200){
                        //checking to see if the user wants to go up while
                        //the snake is going down. that shouldn't happen,
                        //so if it does happen, then just keep on going down
                        if(CURR == UP){
                            MOVEMENT = CURR;
                        }
                        else{
                            MOVEMENT = DOWN;
                        }
                    }
                    //this checks to see if the user wants to go up
                    else if (upAndDown >850){
                        if(CURR == DOWN){
                            MOVEMENT = CURR;
                        }
                        else{
                            MOVEMENT = UP;
                        }
                    }
                    //checks left
                    else if(leftAndRight < 200){
                        if(CURR == RIGHT){
                            MOVEMENT = CURR;
                        }
                        else{
                            MOVEMENT = LEFT;
                        }
                    }
                    //check right
                    else if (leftAndRight >850){
                        if(CURR == LEFT){
                            MOVEMENT = CURR;
                        }
                        else{
                            MOVEMENT = RIGHT;
                        }
                    }
                    //this means that the joystick is in the middle,
                    //so sest the direction of the snake as what it was
                    //previously.  if it was going left, then it will keep
                    //on going to the left
                    else{
                        MOVEMENT = CURR;
                    }
                    break;
                case LOST:
                    //pause the game for 250ms when the user "loses"
                    DelayMs(250);
                    lostGX = 4;

                    //drawing the "-___-" face
                     OledClearBuffer();
                     while(lostGX > 1){
                        OledSetCursor(lostGX, 0);
                        OledDrawGlyph(topAndBot_char);
                        lostGX--;
                     }
                     lostGX = 13;
                     while(lostGX > 10){
                        OledSetCursor(lostGX, 0);
                        OledDrawGlyph(topAndBot_char);
                        lostGX--;
                     }
                     lostGX = 10;
                     while(lostGX > 4){
                        OledSetCursor(lostGX, 2);
                        OledDrawGlyph(topAndBot_char);
                        lostGX--;
                     }
                     //writing noob under it
                      sprintf(oledstring, "NOOOOB");
                        OledSetCursor(4, 3);
                        OledPutString(oledstring);
                     OledUpdate();
                     //the dot dot dot animation.  it adds a dot to the oled,
                     //waits, then adds another one.
                     int periodW = 10;
                     while(periodW <13){
                          sprintf(oledstring, ".");
                        OledSetCursor(periodW, 3);
                        OledPutString(oledstring);
                        OledUpdate();
                        periodW++;
                        DelayMs(400);
                     }
                     sprintf(oledstring, ".");
                        OledSetCursor(periodW, 3);
                        OledPutString(oledstring);
                        OledUpdate();
                     DelayMs(2000);
                     MOVEMENT = START;
                    break;
                case CHECKWIN:
                    //checks to see if the snake ate as much apple as the goal
                    if((snake_length - 1) == goal){
                        MOVEMENT = WON;
                    }
                    else{
                        MOVEMENT = CHECKMOVE;
                    }
                    break;
                case WON:
                    DelayMs(1000);
                     wonGX = 3;
                     //amount of times the weiner string will false
                     int flashW = 3;
                     //drawing the ":D"
                      OledClearBuffer();

                      OledSetCursor(2, 0);
                        OledDrawGlyph(topAndBot_char);
                         OledSetCursor(2, 2);
                        OledDrawGlyph(topAndBot_char);
                         OledSetCursor(5, 0);
                        OledDrawGlyph(topAndBot_char);
                         OledSetCursor(5, 3);
                        OledDrawGlyph(topAndBot_char);

                     while(wonGX >= 0){
                        OledSetCursor(4, wonGX);
                        OledDrawGlyph(topAndBot_char);
                        wonGX--;
                     }
                     wonGX = 2;
                     while(wonGX > 0){
                        OledSetCursor(6, wonGX);
                        OledDrawGlyph(topAndBot_char);
                        wonGX--;
                     }

                     //flashing weiner!
                     while(flashW > 0){
                          sprintf(oledstring, "WEINER!");
                            OledSetCursor(8, 1);
                            OledPutString(oledstring);
                         OledUpdate();
                         DelayMs(250);

                         sprintf(oledstring, "       ");
                            OledSetCursor(8, 1);
                            OledPutString(oledstring);
                         OledUpdate();
                         DelayMs(250);

                         flashW--;
                     }

                     sprintf(oledstring, "WEINER!");
                            OledSetCursor(8, 1);
                            OledPutString(oledstring);
                         OledUpdate();
                         DelayMs(2000);

                    MOVEMENT = START;
                    break;
                    //this should never happen
                default:
                    OledClearBuffer();
                    sprintf(oledstring, "NOOOOO");
                    OledSetCursor(0,3);
                    OledPutString(oledstring);
                    OledUpdate();
                    break;
            }

           //if button1 is pressed, then the current pos is set to 1
           if (PORTG & 0x40){
               DelayMs(50);
               currB1pos = 1;
           }
           else{
               currB1pos = 0; //else, it's set to 0
           }

           //setting the current pos of BTN2 to the status of the button being pressed
           if(PORTG & 0x80){
               DelayMs(50);
               currB2pos = 1;
           }
           else{
               currB2pos = 0;
           }

            //setting the current pos of BTN2 to the status of the button being pressed
           if(PORTA & 0x1){
               DelayMs(50);
               currB3pos = 1;
           }
           else{
               currB3pos = 0;
           }

            //this is the button state
            switch(BTNSTATE){
                //btn has been pressed
                case BTN1P:
                    //if the game state is updategoal, then increment goal
                    //the max it can go up to is 21
                    if(MOVEMENT == UPDATEGOAL){
                        if(goal < 22){
                            goal++;
                        }
                    }
                    //this means that btn1 was not pressed during the goal
                    //setting, so reset the game
                    else{
                        MOVEMENT = START;
                    }
                    BTNSTATE = CHECKBUTTON;
                    break;
                    //btn2 is pressed
                case BTN2P:
                    //decrement the goal if we're at the update goal state
                    if(MOVEMENT == UPDATEGOAL){
                        if(goal > 5){
                            goal--;
                        }
                    }
                     BTNSTATE = CHECKBUTTON;
                     break;
                case BTN3P:
                    //btn3 is only used when setting the goal, so we're checking
                    //to make sure that it will change the game state only
                    //when the game state is at updategoal
                    if(MOVEMENT == UPDATEGOAL){
                       MOVEMENT = CREATE;
                    }
                     BTNSTATE = CHECKBUTTON;
                     break;
                     //checking the button push
                case CHECKBUTTON:
                   if(currB1pos == 1){
                        BTNSTATE = BTN1P;
                        LATGSET = 0xf000;
                    }
                   else if(currB2pos == 1){
                        BTNSTATE = BTN2P;
                    }
                   else  if(currB3pos){
                        BTNSTATE = BTN3P;
                    }
                    else{
                        BTNSTATE = CHECKBUTTON;
                    }
                   break;
                default:
                    break;
            }
            DBG_OFF(MASK_DBG3);
        }
    }
    return (EXIT_SUCCESS);
}
