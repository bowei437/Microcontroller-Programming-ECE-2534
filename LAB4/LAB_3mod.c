///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        LAB3
// STUDENT:         Bowei Zhao
// Description:     File program to run snake game with Digilent board
//                  Using an OLED, joystick, and ADC configurations
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

////////////////////////////////////// ADC, ISR, and INTERRUPT  ////////////////////////////////////////////////////////////////////////
//
// The #Define configurations below are used to set values for the ADC later on so they can be used and initialized.

// Sets AN2, AN3 as positive input with reference to ground in their respective MUX A AND MUX B
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEB_AN3 | ADC_CH0_NEG_SAMPLEB_NVREF

// ADC configuration1 settings. Data stored as unsigned int, and it will use Timer3 clock with auto sampling options
#define AD_CONFIG1 ADC_FORMAT_INTG | ADC_CLK_TMR | ADC_AUTO_SAMPLING_ON

// ADC configuration2 settings. Uses internal reference voltages of Vcc and ground for knowing joystick potentiometer location. 
// Does not scan for inputs and only samples twice per interrupt storing in an 8bit buffer that then alternates for ReadActiveBuffer
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | \
                  ADC_SAMPLES_PER_INT_2 | \
                  ADC_BUF_8 | ADC_ALT_INPUT_ON

// Configuration file 3 for ADC that sets a sample time and speed
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy

// Enables all AN2, AN3 etc etc
#define AD_CONFIGPORT ENABLE_ALL_DIG

// we don't want any scanniing so we take this out
#define AD_CONFIGSCAN SKIP_SCAN_ALL

// used from the glyph code to check if the glyphs we input are legit glyphs. Since our glyphs are hard coded, they will ALWAYS be legit.
#define CHECK_RET_VALUE(a) { \
  if (a == 0) { \
    LATGSET = 0xF << 12; \
    return(EXIT_FAILURE) ; \
  } \
}
/////////////////////////////////////////////// Global variables ////////////////////////////////////////////////////////////////////

                                    
unsigned int Seconds_Timer = 0; // Global variable to count number of times in timer2 ISR
int cur_LR, cur_UD; // global variable to keep track of LR and UD
char buf1[50]; // used to store OLED output
char buf2[50]; // an extra one used to store OLED output
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2
unsigned char Button3History = 0x00; 
int goal_set = 12; // default goal amount set to 12
int goal_cur = 0; // default current number of gaols achieved.
int size = 0; // size of current snake body
int retValue = 0; // used for define return value to check if glyph is legit
int difficulty = 0; // sets difficulty level of game
int i =0; // int i used for a recursive for loop
int j = 0; // another random integer used for a loop
int win = 0; // a variable used to declare if the game was won. Used like a boolean

int stop = 0; // used to create static display of images without needing to constantly update. Used like a boolean

// Creates glyphs of various sizes and shapes.  This is the head char
BYTE head[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
char head_char = 0x00;

BYTE body[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
char body_char = 0x01;

BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char blank_char = 0x02;

BYTE apple[8] = {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF};
char apple_char = 0x03;

// Created a typedef of struct giving each one an X and Y position. 
typedef struct xy_position
{
    int Xpos;
    int Ypos;

}position;
// I initialize them as global variables so my void functions can have access to modify them as they wish. 
position s_position[40]; // create an array of 20 for SNAKE position
position a_position[3]; // initialize apple position. these will change

 ////////////////////////////////////// FUNCTIONS DECLARATION ////////////////////////////////////////////////////////////////////////

void START_initial(); // mass initialization of code for timer, ADC, and interrupts
void Welcome_Msg(); // displays the initial class 5 second message on our information
void Get_Random_Location(int sel); // used to output random positions for both snake and apple so they are different
int Game_EndLoop(); // funciton used to constantly check if the snake hit itself. Ends the game if done so. 

 ////////////////////////////////////// ISR LOOPS ////////////////////////////////////////////////////////////////////////

void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) 
{
    // Reads first byte of stuff for LR and UD data to then determine position from bits 0 and 1
    if (ReadActiveBufferADC10() == 0)
    {
        cur_LR = ReadADC10(0);
        cur_UD = ReadADC10(1);
    }
    // Reads second byte of stuff for LR and UD data to then determine position from bits 8 and 9
    if (ReadActiveBufferADC10() == 1)
    {
        cur_LR = ReadADC10(8);
        cur_UD = ReadADC10(9);
    }

    // Clears the flag so that we can use it like an interupt
    INTClearFlag(INT_AD1);

}

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{   
    Seconds_Timer++; // Increment the millisecond counter.
    
    INTClearFlag(INT_T2); // Acknowledg-e the interrupt source by clearing its flag.
}

// ///////////////////////////////////// Reasonable Comment Block at Beginning of Main.C
//
//  The following code below uses a finite state machine to produce the actions needed to run through the game 'Snake' on an OLED Display
// After initializing all the I/O stuff for the ADC, Timer, Interrupts, and Snake Game, we can then decide that we want to first 'enter the game'.
// State machine state START, begins the game and loops through waiting for goal setting before continuing.
// State machine state DIFFIC, enters into a loop that receives value later to become a 'difficulty' threshold for the game. 
// State machine state Run, enters into the legit main loop of the code. Here it does the general moving of the snake, its head, incrementing, eating apples, getting scores, and finding
//          random locations for itself within the score borders. Many stop cases are present in this state to break the loop and go into game end mode. 
// State machine state OVER is run whenever the snake hits itself, hits the wall, or wins the game by hitting the goals needed. Depending on what happens, a boolean is sent which
//          allows the state to determine if the Game is OVER for a loss or if the user Won the round. 
// State machine code REset is just as it says. Many of the elements, global variables, loops, array locations in the snake, and also the board need to be cleared and reset for next round
//          This state thus just re-initializes all the variables so the game is fresh the next run through. 
// The code restarts again and this thus forms the basis of the State machine for LAB3 for Snake. 

//////////////////////////////// MAIN CODE /////////////////////////////////////////////////////////////

int main() 
{
    START_initial(); // initializes all code in a function for clarity
    Welcome_Msg(); // Welcome default screen output. 

    // Initialize direction of movement of snake based on joystick value for use later
    int Joy_L = 0;
    int Joy_R = 0;
    int Joy_U = 0;
    int Joy_D = 0;
    // Finite State Machine Cases
    enum case_var{START, DIFFIC, RUN, OVER, RESET};
    enum case_var cur_var = START; // setting default/starting cur_var to START so we can start code.

    // Code below checks if they are 'valid' glyphs. Nothing substantial. 
    retValue = OledDefUserChar(blank_char, blank);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(head_char, head);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(body_char, body);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(apple_char, apple);
    CHECK_RET_VALUE(retValue);


///////////////////////////// Main processing loop ////////////////////////////////////////////////////
    while (1) 
    {
        switch(cur_var)
        {
// ---------          -----------------------------------------------------------------------------------------------------------
            case START: // initial game State Machine for START
                // Creating a Display for changing goal amount. 
                if (stop == 0) // Stop is used as initial boolean to create static display that only displays when an 'action' is performed
                {
                    LATGCLR = 0xF << 12;
                    OledSetCursor(0, 0);
                    sprintf(buf2,"Set Goal: %2d", goal_set); // goal_set is default to 12 always and will dynamically increment. 
                    OledPutString(buf2);
                    OledSetCursor(0, 1);
                    OledPutString("Btn1: Goal++");
                    OledSetCursor(0, 2);
                    OledPutString("Btn2: Goal--");
                    OledSetCursor(0, 3);
                    OledPutString("Btn3: Accept");
                    OledUpdate();
                    stop++; // increments Stop so that the code won't run the second loop through if no user input. Efficent.
                }
                if(PORTG & 0x40) // If Button 1 is pressed then do this action
                {
                    goal_set++; // the final goal of the apples to be eaten increases
                    while((PORTG & 0x40)); // debouncing of button where we wait until the button is released
                    stop = 0; // set stop = 0 so that the score board increments/
                }
                else if (PORTG & 0x80) // if Button 2 is pressed, then do this action
                {
                    goal_set--; // decrease final goal amount for the lesser achieveers
                    while((PORTG & 0x80)); // debounces the button until it is released
                    stop = 0; // sets stop = 0 so board increaes. 
                }
                else if (PORTA & 0x1) // otherwise, if Button 3 is pressed, it means the game is now going to start!
                {
                    LATGSET = 0xF << 12; // Fancy lights to show selection
                    while((PORTA & 0x1)); // debouncing of button 3 for release
                    OledClearBuffer(); // wipes the entire screen for melodramatic effect
                    OledSetCursor(0, 0);
                    OledPutString("Ready Player 1");
                    OledSetCursor(0, 1);
                    OledPutString("You have Chosen:");
                    OledSetCursor(0, 2);
                    sprintf(buf2,"%2d Goals", goal_set);
                    OledPutString(buf2);
                    OledUpdate();
                    DelayMs(2000); // delay for fancy effect
                    OledClearBuffer(); // clearing buffor in same loop with delay use to create illusion of a different state
                    stop = 0; // reset boolean stop = 0 so it can be used in later codes as a run once variable
                    int select = 0; // hard setting integer select to 0 so function GetRandomLocation can get one
                    Get_Random_Location(select); // giving 0 means it will get both a new Apple and snake head position

                    cur_var = DIFFIC; // set the next state to Dific and break the loop!
                    break;
                }
                cur_var = START; // otherwise if nothing happened or BTN3 wasn't pressed, reloop through the code. 
                break;
// ---------          -----------------------------------------------------------------------------------------------------------
            case DIFFIC: // next state in state machine code
                LATGCLR = 0xF << 12; // fancy light effect
                OledSetCursor(0, 0);
                OledPutString("-Difficulty-");
                OledSetCursor(0, 1);
                OledPutString("Btn1: HARD");
                OledSetCursor(0, 2);
                OledPutString("Btn2: MED");
                OledSetCursor(0, 3);
                OledPutString("Btn3: EASY");
                OledUpdate();
                if(PORTG & 0x40) // if button 1 is pressed. 
                {
                    LATGSET = 1 << 12; // LED's display difficulty level
                    DelayMs(150); // fancy effect
                    LATGSET = 1 << 13;
                    DelayMs(150); // fancy effect
                    LATGSET = 1 << 14;
                    DelayMs(350); // fancy effect
                    OledClearBuffer(); // 

                    difficulty = 0; // hard code difficulty amount into global integers
                    cur_var = RUN; // breaks into next coding state. 
                    break;
                }
                else if (PORTG & 0x80) // if button 2 is pressed
                {
                    LATGSET = 1 << 12; // LED's display difficulty level
                    DelayMs(150); // fancy effect
                    LATGSET = 1 << 13;
                    DelayMs(350); // fancy effect
                    OledClearBuffer();
                    difficulty = 200; // artifficial ms delay for loop
                    cur_var = RUN; // sets next state and breaks
                    break;
                }
                else if (PORTA & 0x1) // if button 3 is pressed
                {
                    LATGSET = 1 << 12; // only one basic fancy light and that is it. 
                    DelayMs(150); // fancy effect
                    difficulty = 500;
                    OledClearBuffer();
                    cur_var = RUN;
                    break;
                }
                cur_var = DIFFIC; // if nothing happens , then we reloop through the state Diffic
                break;
// ---------          -----------------------------------------------------------------------------------------------------------
            case RUN:
                ////////////////////////// Run once for Static Display ////////////////////////////////////////////
                if(stop == 0) // stop used as boolean. 
                {
                    OledSetCursor(10, 0);
                    OledPutString("Score");
                    OledSetCursor(13, 1);
                    sprintf(buf2,"%2d", goal_cur);
                    OledPutString(buf2);
                    OledSetCursor(13, 3);
                    sprintf(buf2,"%2d", goal_set);
                    OledPutString(buf2);
                    OledSetCursor(11, 2);
                    OledPutString("Goal");
                    stop++; // increment stop so the code is only run once. 
                }
                ////////////////////////// 'Incident' Code for when Stuff happens ////////////////////////////////////////////
                DelayMs(difficulty);
                if (goal_cur == goal_set) // if the amount of goals wanted is the current score, then you win the game
                {
                    cur_var = OVER; // go into OVER state
                    win = 1; // set global variable win to 1 so a different case of OVER will be run and then break
                    break;

                }
                if ((s_position[0].Xpos < 0) || (s_position[0].Ypos < 0)) // if the snake moves to the left border or Y goes upwards too much
                {
                    cur_var = OVER; // game over
                    break;
                }
                else if ((s_position[0].Xpos > 9) || (s_position[0].Ypos > 3) ) // if the snake moves to the right border or Y moves down too much
                {
                    cur_var = OVER; // game over
                    break;
                }
                if(PORTG & 0x40) // if button 1 is pressed. Game OVER and Restart as universal idea. 
                {
                    cur_var = OVER;
                    break;
                }
                if ((s_position[0].Xpos == a_position[0].Xpos) && (s_position[0].Ypos == a_position[0].Ypos)) // if the snake eats an apple. 
                {
                    goal_cur++; // adds a score to the score board
                    size++; // increments size of the snake
                    int select2 = 1;
                    Get_Random_Location(select2); // gets new Apple location by sending 1 which allows function to ONLY update apple location. 
                    OledSetCursor(13, 1); // this code below ONLY updates goal score so score screen doesn't constantly update
                    sprintf(buf2,"%2d", goal_cur); // more efficent than constanly running through code above
                    OledPutString(buf2); // the rest of the score board is just a static image. 

                }

                ////////////////////////// Tail Increment Code ////////////////////////////////////////////

                int local_OVER = 0; // initalize and constant reset state of local_Over
                if(size == 0) // if the snake is still initial size
                {
                    OledSetCursor(s_position[0].Xpos, s_position[0].Ypos); // hard code blanking for position 0
                    OledDrawGlyph(blank_char); // where blanking out previous space
                }
                else // code for moving snake changes the moment it is no longer alone
                {
                    OledSetCursor(s_position[size].Xpos, s_position[size].Ypos); // delete end tail glyph
                    OledDrawGlyph(blank_char);
 
                    for (i=size; i>=1; i--) // goes from back of tail to top so no overwrites of former coordinate data
                    {   
                        local_OVER=Game_EndLoop(); // we use Game_Endloop function that returns an int we store into local_Over. If local_Over is = 1 

                        s_position[i].Xpos = s_position[i-1].Xpos; // decrements the tail coordinates from last one to the one before and vice versa
                        s_position[i].Ypos = s_position[i-1].Ypos;
                        if (local_OVER == 1) // then the game is over as the tail ran into itself
                        {
                            cur_var = OVER;
                            break;
                        }

                        OledSetCursor(s_position[i].Xpos, s_position[i].Ypos); // sets the cursor and then draws the glyphs of the new positions of i into that position
                        OledDrawGlyph(body_char);
                    }

                }
                if (local_OVER == 1) // then the game is over as the tail ran into itself
                {
                    cur_var = OVER;
                    break;
                }

                OledSetCursor(a_position[0].Xpos, a_position[0].Ypos); // draws apple position on each update so snake tail passing through it can't overwrite it's visibility. 
                OledDrawGlyph(apple_char);

                ///////////////////////////////Initial ADC Movement Code //////////////////////////////////////////
                if (cur_LR >= 650) // if the potentiometer reads this value
                {
                    Joy_R = 1; // we hard code a value for it moving right
                    
                    Joy_L=0, Joy_U=0, Joy_D=0; // hard code all other movement values to 0
                }
                else if (cur_LR <= 400)
                {
                    Joy_L = 1;

                    Joy_R=0, Joy_U=0, Joy_D=0;
                }
                else if (cur_UD >=650)
                {
                    Joy_U = 1;

                    Joy_L=0, Joy_R=0, Joy_D=0;
                }
                else if (cur_UD <= 400)
                {
                    Joy_D = 1;

                    Joy_L=0, Joy_U=0, Joy_R=0;
                }

                /////////////////////////// Auto Movement Code /////////////////////////////////////////
                // based off the previous hard coded values of Joy_L,Joy_R,Joy_U,Joy_D, if they are stil the same, then it continues

                if (Joy_R == 1)
                {
                    s_position[0].Xpos++;
                }
                else if (Joy_L == 1)
                {
                    s_position[0].Xpos--;
                }
                else if (Joy_U == 1)
                {
                    s_position[0].Ypos--;
                }
                else if ( Joy_D == 1)
                {
                    s_position[0].Ypos++;
                }
                //////////////////////////////////////////////////////////////////////////////////////////
                OledSetCursor(s_position[0].Xpos, s_position[0].Ypos); // sets position for new head once the position has changed
                OledDrawGlyph(head_char);

                OledUpdate(); // finally update th eOLED in the main score boared and clear the flag 
                INTClearFlag(INT_T2);
                cur_var = RUN; // go to next state
                break;

// ---------          -----------------------------------------------------------------------------------------------------------
            case OVER:
                DelayMs(100); // artificial delay for effect
                LATGCLR = 0xF << 12; // clear all lights in case there were any
                OledClearBuffer(); // clear the screen
                DelayMs(100);
                Seconds_Timer = 0;

                if (win == 0) // IF over WAS PASSED win = 0, then the game was lost as win is default to 0
                {
                    while(Seconds_Timer < 6) // used to loop for 6 seconds. 
                    { 

                        LATGCLR = 0xF << 12; // turns off lights
                        OledClearBuffer(); // clears screen
                        DelayMs(350); // waits a bit
                        LATGSET = 0xF << 12; // turns on lights
                        OledSetCursor(5, 1);
                        OledPutString("GAME"); // writes message
                        OledSetCursor(5, 2);
                        OledPutString("OVER!");
                        OledUpdate();
                        DelayMs(350); // stops
                    }

                }
                else if (win == 1) // used for when passed a game where the user won
                {
                    while(Seconds_Timer < 6)
                    {
                        LATGCLR = 0xF << 12; // turns off lights
                        OledClearBuffer();
                        DelayMs(350);
                        LATGSET = 0xF << 12;
                        OledSetCursor(5, 1);
                        OledPutString("YOU");
                        OledSetCursor(5, 2);
                        OledPutString("WIN!");
                        OledUpdate();
                        DelayMs(350);

                    }
                }
                stop = 0;
                LATGCLR = 0xF << 12; // resets lights to off
                OledClearBuffer(); // clears screen 
                cur_var = RESET; // moves onto next state
                break;
// ---------          -----------------------------------------------------------------------------------------------------------
            case RESET:
                if (PORTG & 0x40) // waits for BTN 1 to be pressed to formally restart the game
                {
                    int k = 0; // local variable of k used to clear entire s_position array 
                    for(k=0; k<40; k++)
                    {
                        s_position[k].Xpos = 0;
                        s_position[k].Ypos = 0;
                    }
                    a_position[0].Xpos = 0; // clears apple default position
                    a_position[0].Ypos = 0; // clears apple y default positions
                    goal_cur = 0; // changes the score board back to 0
                    Joy_L=0; // reinitializes Joy_L, Joy_R, Joy_U,Joy_D to 0 so it won't auto move the joystick
                    Joy_R=0;
                    Joy_U=0;
                    Joy_D=0;
                    size = 0; // makes the snake super small again
                    stop = 0; // sets boolean stop back to 0
                    i = 0; // integer is also reset
                    win = 0; // wining boolean reset
                    cur_var = START; // next State machine state is START to go through process again
                    goal_set = 12; // default value of goal_set = 12 is reset
                    OledClearBuffer(); // the screen is cleared 
                    break;
                }
                // otherwise if BTN1 isn't pressed, run this. 
                if (stop == 0) // stop was reset to 0 just before cur_var OVER ended so it can be used as a boolean to create static final score board
                {
                    OledSetCursor(0, 0);
                    OledPutString("Final Score:");
                    OledSetCursor(0, 1);
                    sprintf(buf2,"%2d out of %2d", goal_cur, goal_set);
                    OledPutString(buf2);
                    OledSetCursor(2, 3);
                    OledPutString("BTN1 to RePlay");
                    OledUpdate(); // therefore it only updates once
                    stop++; // updates stop = 0 to stop = 1 so it only runs once
                }
                cur_var = RESET; // reloops the code if no input
                break;

// ---------          -----------------------------------------------------------------------------------------------------------
            default: // default state is a syntax that people like. It is currently kept for looks and general feel. 
                cur_var = START;
                LATGSET = 0xF << 12;
                OledClearBuffer();
                OledSetCursor(0, 1);
                OledPutString("ERROR Default");
                OledUpdate();
                break;
        }   
    }
    return (EXIT_SUCCESS);
}

void START_initial()
{
///////////////////////////////////////// Button and LED ENABLING ///////////////////////////////////////////////////////

    DDPCONbits.JTAGEN = 0; // JTAG controller disable to use BTN3

    TRISGSET = 0x40;     // For BTN1: configure PORTG bit for input
    TRISGSET = 0x80;     // For BTN2: configure PORTG bit for input
    TRISASET = 0x1;

    TRISGCLR = 0xf000;   // For LEDs: configure PORTG pins for output
    ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)
    LATGCLR  = 0xf000;   // Initialize LEDs to 0000

    TRISBCLR = 0x0040; // initialize Joy_U/Joy_D pin B6
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

///////////////////////////////////////// DELAY & OLED ENABLING ///////////////////////////////////////////////////////
    DelayInit(); // initializes delays
    OledInit(); // initializes OLED display
/////////////////////////////////////////TIMERS ENABLING ///////////////////////////////////////////////////////
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_256 | T2_GATE_OFF, 39061); // creates a Timer 2 that runs at 1 s 
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4); // priority level 4
    INTClearFlag(INT_T2); // has flags you can clear and is a vector
    INTEnable(INT_T2, INT_ENABLED);
    
 ///////////////////////////////////////// ADC ENABLING ///////////////////////////////////////////////////////
    SetChanADC10(AD_MUX_CONFIG); // enable me some good ADC
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN); // configure all the setttings for it
    EnableADC10();  // turn it on after it is configured
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7); // Set up, clear, and enable ADC interrupts 
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
///////////////////////////////// MULTIPLE TIMER INTERRUPT DECLARATION//////////////////////////////

    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_16 | T3_GATE_OFF, 3); // make Timer3 for use with the ADC TMR CLK

    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR); // configure code to use all the vectored interrupts we give it
    INTEnableInterrupts(); // finally actually enable the use of everything

}

void Welcome_Msg()
{
    int run_once = 0; // boolena of one runce used
    OledClearBuffer(); // screen is cleared in case of past artificats
    while( Seconds_Timer <=5) ////// Currently set to only 1 Second for DEBUG
    {
       
        OledSetCursor(0, 0);
        OledPutString("SNAKE GAME"); // title
        OledSetCursor(0, 1);
        OledPutString("By Bowei Zhao"); // name
        OledSetCursor(0, 2);
        OledPutString("ECE 2534");
        OledSetCursor(0, 3);
        sprintf(buf2,"s: %6d", Seconds_Timer);
        OledPutString(buf2);

        if(run_once == 0) // always will run at least once as true
        {
            OledUpdate(); // only updates OLED once to save on cycles. 
            run_once = 1; // doesn't let OLED keep updating
        }
        
    }
    OledClearBuffer();
}

void Get_Random_Location(int sel) // function to generate a random location on the board for the snake and apple to pop up in. 
{
    srand(Seconds_Timer); // using srand to generate a seed giving it my seconds timer
    a_position[0].Xpos = rand() % 9; // generating random location for apple position always
    a_position[0].Ypos = rand() % 3;

    if (sel == 0) // if the variable input is = 0 then we have a sanake that needs replacing. We need to make it ASAP
    {
        srand(Seconds_Timer+5); // srand of seed giving it a diferent seed to make
        s_position[0].Xpos = rand() % 9; // snake position on playing board
        s_position[0].Ypos = rand() % 3; // snake position Y on playing board
    }
    
}
int Game_EndLoop() // used to see if i end up hitting/eating myself
{
    int OVERloop = 0; // overloop used as return variable
    int j = size; // j variable used for real life size modeling. 
    while(j > 0)
    {
        if ((s_position[0].Xpos == s_position[j].Xpos) && (s_position[0].Ypos == s_position[j].Ypos))
        {
            OVERloop = 1;
            return OVERloop;
            break;

        }
        j--; // decrements j every time the original loop does no run. 
    }

}
