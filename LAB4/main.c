///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        LAB4
// STUDENT:         Bowei Zhao
// Description:     File program to play Zombie Hero GAME using ADXL345
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
// Last modified:   12/7/2015

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


////////////////////////////////////// ADC, ISR, and INTERRUPT  ////////////////////////////////////////////////////////////////////////
//
//
// ---------------------------------------EXTRA CREDIT FEATURE ------------------------------------------------------
// The #Define configurations below are used to set values for the ADC later on so they can be used and initialized.
//
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
// ---------------------------------------------------------------------end ADC DEFINES ------------------------------------------/// 
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CHECK_RET_VALUE(a) { \
  if (a == 0) { \
    LATGSET = 0xF << 12; \
    return(EXIT_FAILURE) ; \
  } \
}

////////////////////////////////////////// FUNCTIONS DECLARATION ////////////////////////////////////////////////////////////////////////
void START_initial();
void Device_initial();
void DEBUG_XYZ();
void Get_Random_Location(int sel);
int phys_bounce();
void zomb_move();
void human_move();
void high_score_calc(int cur_score);
void ENTER_SCROLL();
void joy_zomb_move();
/////////////////////////////////////////////// Global variables ////////////////////////////////////////////////////////////////////
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2
unsigned char Button3History = 0x00;
bool run_once = false; // boolean used to make OledUpdate run only once during while loops or cases
int retValue = 0; // used in conjunction with #define and main code to make sure Glyphs are valid
char buf1[50]; // global char output for sprintf usage. 

int singleloop = 0; // used throughout switch and while loop to run loops for only N iterations to limit CPU usage. 
int over_alarm = 0; // variable used to know if external functions triggered a case where game is now over.
int zombie_number = 0; // Variable that is used to specify # of zombies to make and create.
int zombie_randLR = 0; // used to continously calculated random value that will be compared to determine zombie direction Left Right
int zombie_randUD = 0;// used to continously calculated random value that will be compared to determine zombie direction Up  Down
                    
int high_score1 = 0; // All variables here are used to store 3 values for high scores and then later compare to each other
int high_score2 = 0; // used to find second highest
int high_score3 = 0;  // finds 3rd highest score. 

int velocity = 200; // Default Velocity number. Used to re-calculate speed of human moving across room later

      // ---------------------------- ADXL345 Variables ----------------------------------------------

short x = 0, y = 0, z = 0; // Stores GetXYZ variables of Accel force for movement
unsigned char tapped = 0; // used for detecting TAPs in ADXL

            // the following SHORT variables are used for continuous movement
            // in the code for human movement with ADXL 
short L = 0; // keeps Left location movement
short R = 0; // keeps right location movement
short U = 0; // keeps up location movement
short D = 0; // keeps down location movement.

int alarm = 0; // Supposed to be boolean. Used to know if ADXL345 is plugged in. If not, it will 
                // display error code to check ADXL connection


      // ---------------------------- ADC Joystick Variables ----------------------------------------------
      // Initialize direction of movement of Player2 Zombie based on joystick value for use later
int Joy_L = 0; // Joystick moving Left continuous 
int Joy_R = 0; // Joystick moving right continous
int Joy_U = 0; // joystick moving up continous
int Joy_D = 0; // joystick moving down continous. 

int cur_LR, cur_UD; // global variable to keep track of LR and UD for ADC values in ADC loop




      // ---------------------------- TIME Variables ----------------------------------------------
unsigned int sec1000 = 0; // This is updated 1000 times per second by the interrupt handler
unsigned int sec10 = 0; // used in interrupt handler to know to read ADXLGetXYZ only once every 10s
unsigned int sec_spawn = 0; // keeps track of spawning time of spiders
unsigned int sec_freeze = 0; // keeps track of seconds elapsed for Freeze Powerup
unsigned int sec_slow = 0; // keeps track of seconds elapsed for Slow-down Powerup


      // ---------------------------- DEBUG Variables ----------------------------------------------
// 
// Code from HW6 is also included and was used as Easy access DEBUG platform
// variables here are used only in dEBUg loop. This loop can not be activated in game. It requires changing a case variable below
short min_x = 0, min_y = 0, min_z = 0, max_x = 0, max_y = 0, max_z = 0;


// Creates glyphs of various sizes and shapes.  This is the head char
BYTE head[8] = {0x30, 0x46, 0x86, 0x80, 0x80, 0x86, 0x46, 0x30};  // Creates Smiley face for the HERO
char head_char = 0x00;

BYTE body[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}; // not used in this game
char body_char = 0x01;

BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Creates Blank character for when in between moving. 
char blank_char = 0x02;

BYTE zombie[8] = {0x81, 0xC1, 0xA1, 0xB1, 0x99, 0x8D, 0x87, 0x83};  // Creates Z for zombie character for Zombie
char zombie_char = 0x03;

// A struct is used to create human and zombie positions that store X and Y position for easy calling and usage later. 
typedef struct xy_position
{
    int Xpos; // stores Horizontal location
    int Ypos; // stores vertical location

}position;
// I initialize them as global variables so my void functions can have access to modify them as they wish. 
// While the below shows that I can have up to 6 (0 to 5) human or zombies. I will have at most 1 human and 3 zombies. 
position human_position[5]; // only 1 human
position zombie_position[5]; // only 3 zombies max as of currently


 ////////////////////////////////////// ISR LOOPS ////////////////////////////////////////////////////////////////////////
//
// The configurations below are used as continous running loops


      // ---------------------------- ISR VECTOR 2 LOOP ----------------------------------------------
// tHIS isr interrupt handler is soley used for Button debouncing
// inside it, it waits for an interrupt and then checks Button values

// NOTE: This loop is only used later with Single Player Mode
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
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
      // ---------------------------- ISR VECTOR ADC LOOP ----------------------------------------------
// This interrupt handler is used for the ADC only and reads values from the buffer to be used for where the ADC will go

// NOTE: Used with Two Player mode only

// *********************  EXTRA CREDIT FEATURE ************************
// Two player mode with Joystick in ADC
void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) 
{

        // Reads first byte of stuff for LR and UD data to then determine position from bits 0 and 1
    if (ReadActiveBufferADC10() == 0)
    {
        cur_LR = ReadADC10(0); // LR position read from bit 0
        cur_UD = ReadADC10(1); // UD position read from bit 1
    }
    // Reads second byte of stuff for LR and UD data to then determine position from bits 8 and 9
    if (ReadActiveBufferADC10() == 1)
    {
        cur_LR = ReadADC10(8); // LR position read from bit 8
        cur_UD = ReadADC10(9); // UD position read from bit 9
    }
    // Clears the flag so that we can use it like an interupt
    INTClearFlag(INT_AD1);
}

      // ---------------------------- ISR VECTOR 4 LOOP ----------------------------------------------
// this loop is used in Single player mode to keep track of time, get ADXL values for XYZ, and store time values.

// NOTE: Used only with single player mode
void __ISR(_TIMER_4_VECTOR, IPL4AUTO) _Timer4Handler(void) 
{
    // only polls every 10 ms
  if(sec10 > 10)
  { 
    ADXL345_GetXyz(&x,&y,&z); // store values for X Y Z into registers
    sec10 = 0;
  }
  // code below increments every millisecond
  sec10++; 
  sec_spawn++;
  sec1000++;
  sec_slow++;
  sec_freeze++;
  INTClearFlag(INT_T4);
}

// ///////////////////////////////////// Reasonable Comment Block at Beginning of Main.C
//
// MAD MAX : THE GAME is a game that will bring the user to the main game mode screen after running it in MPLAB X.
// Here, you can choose to enter two different modes. Note that you cannot switch between Two Player or Single Player modes while in game. 
//You must reset the game through MPLAB X for this. Single Player mode is what the project is based on so this is not a problem. 
//You can test my Two Player mode for extra credit.

// The game is split so enums and switch cases are used to differentiate initially which Game Mode the user wants and moves accordinly
// 2 player modes use their own initial, running and game over states than single player mode. 
// Depending on mode chosen is when the timers are initialized. This is because during past trials
// I was unable to get the code running 'fast' enough when all Timers and interrupts were used together
// To speed up game in both modes, they were seperated

//////////////////////////////////////////////// MAIN CODE /////////////////////////////////////////////////////////////

int main()
{

    START_initial(); // calls function for intiialization devices
    Device_initial(); // Calls function to initialize ADXL 345 and ADXL TAP

    // Below code is used for 2s initial splash screen.
    OledSetCursor(0, 0);
    OledPutString("MAD MAX:THE GAME");
    OledSetCursor(0, 1);
    OledPutString("By Bowei Zhao"); // name
    OledSetCursor(0, 2);
    OledPutString("ECE 2534");
    OledUpdate();
    DelayMs(2000);

    // Variables are re-initialized or initially initialzed here to make sure they are the same.
    int game_over_sec = 0;
    int slowup = 0;
    int power_freeze = 0;
    int power_slow = 0;
    int flicker = 0;
    int flicker2 = 0;
    char locbuf[20]; // used as a local output buffer

    // enums control my game states and allows me to switch between them at ease. 
    enum case_var{SPLASH, Mode_double, Mode_single, SELECT, DIFFIC, RUN, PlayTwo, OVER, OverDouble, RESET, DEBUG};
    enum case_var cur_var = SPLASH;

    // Code below checks if they are 'valid' glyphs. Nothing substantial. 
    retValue = OledDefUserChar(blank_char, blank);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(head_char, head);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(body_char, body);
    CHECK_RET_VALUE(retValue);
    retValue = OledDefUserChar(zombie_char, zombie);
    CHECK_RET_VALUE(retValue);
    int slow_zomb = 0;

    OledClearBuffer(); // just clears OLED buffer before start

   while(1)
   {
      if(alarm != 1) // just checks if my ADXL is plugged into either Channel 3 or 4. If it is, it outputs ERROR message. Else it runs the game.
      {
        switch(cur_var)
        {

// --------- GAME STATE: SPLASH  -----------////// ------------- ///// ------------ /////// -----------
          // This is the initial and default Game Mode selection Screen. 
          // Code here will decide if the PIC32 will enter Main Single Player mode or Two Player mode.
          // 
          // This screen is run only once in the entire code. Game needs reset before it can change game modes. 

            // *********************  EXTRA CREDIT FEATURE ************************
            // Features: 2 Player Mode with Joystick and ADC. Also flashing OLED display
          case SPLASH:
            sec1000 = 0;
            run_once = false;
            // outputs selection
            OledSetCursor(0, 0);
            OledPutString("-GAME MODE-");
            OledSetCursor(0, 2);
            OledPutString("Btn2: 2Player");
            OledSetCursor(0, 3);
            OledPutString("Btn1: 1Player");
            OledUpdate();

            // game waits for button press. Note that due to how my Timers are implemented. Buttons are not
            // currently debounced here. But since this is extra credit it doesn't count.
            // All main game buttons for Single Player are debounced once Timer is initialized below. 

            if((Button2History == 0xFF) || (PORTG & 0x80) ) //BTN2 causes 2 player mode to start
            {
              cur_var = Mode_double; // game mode selects 2 player mode
              break;
            }
            if((Button1History == 0xFF) || (PORTG & 0x40) ) // BTN1 causes Main Single player mode
            {
              cur_var = Mode_single; // single player mode begins
              break;
            }
            cur_var = SPLASH; // if button isn't pressed yet, continue the loop and wait. 
            break;

// --------- GAME STATE: Mode_Double  -----------////// ------------- ///// ------------ /////// -----------
          // This is the Two Player mode for my game.
          // 
          // The game will now initialize only the Timers and Interrupts it needs for completion. 
          // Note only ADC and Timer 3 are used as that is all we need. 

           // *********************  EXTRA CREDIT FEATURE ************************
           // Features: 2 Player Mode with Joystick and ADC and flashing OLED

          case Mode_double:
            OledClearBuffer();
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

            Get_Random_Location(0); // Get random location for the human
            Get_Random_Location(1); // get random location for the one zombie character.

            // No timer in this code for speed and efficency so hard coded loops for special effect are used.

            // Code here only flahses the Ready Player 1 and 2 message for retro game immersion.
            OledSetCursor(0, 0);
            OledPutString("READY PLAYER 1");
            OledSetCursor(0, 2);
            OledPutString("READY PLAYER 2");
            OledUpdate();
            DelayMs(550);
            OledSetCursor(0, 0);
            OledPutString("               ");
            OledSetCursor(0, 2);
            OledPutString("               ");
            OledUpdate();
            DelayMs(550);
            OledSetCursor(0, 0);
            OledPutString("READY PLAYER 1");
            OledSetCursor(0, 2);
            OledPutString("READY PLAYER 2");
            OledUpdate();
            DelayMs(550);
            OledClearBuffer();

            cur_var = PlayTwo; // moves to new case switch now. This one only dedicated for 2 player mode of course
            break;
// --------- GAME STATE: PlayTwo  -----------////// ------------- ///// ------------ /////// -----------
          // main running loop for game with two players in real life
            // allows for control through Accelerometer and ADC Joystick
            // one player controls human with ADXL345 and the other with the joystick for the zombie
            // no score board or time. It just constantly loops after game end back to the game

            // *********************  EXTRA CREDIT FEATURE ************************
            // Features: 2 Player Mode with Joystick and ADC

          case PlayTwo:
            // if the two players hit, then the game is over
            if ((human_position[0].Xpos == zombie_position[0].Xpos) && (human_position[0].Ypos == zombie_position[0].Ypos))
            {
              // moves to a special 2 player OVER screen and lounge
              cur_var = OverDouble;
              break;
            }

            ADXL345_GetXyz(&x,&y,&z); // manually get value of ADXL accel for human

            // creates blank of Human and Zombie at current spot
            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(blank_char);

            OledSetCursor(zombie_position[0].Xpos, zombie_position[0].Ypos);
            OledDrawGlyph(blank_char);

            // moves and allows for bouncing of human along walls
            human_move();
            // allows for zombie human movement thorugh joystick
            joy_zomb_move();
            // DelayMs is used for creating velocity around the playing map
            DelayMs(velocity);
            // code below just draws the glpyhs in the new zones they are in. 

            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(head_char);

            OledSetCursor(zombie_position[0].Xpos, zombie_position[0].Ypos);
            OledDrawGlyph(zombie_char);
            OledUpdate();


            cur_var = PlayTwo;
            break;

// --------- GAME STATE: OVERDOuble  -----------////// ------------- ///// ------------ /////// -----------
            // TWO PLAYER GAME MODE ONLY
            // loop that displays Game over and then restarts two player game back to above case loop to replay again
            // no score or time is recorded as it is uneeded in a player vs player game.

             // *********************  EXTRA CREDIT FEATURE ************************
            // Features: 2 Player Mode with Joystick and ADC and flashing OLED
          case OverDouble:
              OledClearBuffer();
              OledSetCursor(0, 1);
              OledPutString("GAME OVER");
              OledUpdate();
              DelayMs(400);
              OledSetCursor(0, 1);
              OledPutString("               ");
              OledUpdate();
              DelayMs(400);
              OledSetCursor(0, 1);
              OledPutString("GAME OVER");
              OledUpdate();
              DelayMs(400);
              OledClearBuffer();
              cur_var = Mode_double;
              break;

// --------- GAME STATE: Mode_single  -----------////// ------------- ///// ------------ /////// -----------
          // This is the Main Single Player mode select of the game.
          // 
          // We initialize the needed Timers and interrupts we will be using here and then continue with the game code to ready the player
          // We need one timer for debouncing the buttons and one for seconds and ADXL get XYZ function
          // Possibility of combining into one but right now two are used as we can and it doesn't reduce game speed

            // *********************  EXTRA CREDIT FEATURE ************************
            // Features: Flashing OLED and CINEMATIC VIDEO scroll Immersion 

          case Mode_single:
            OledClearBuffer();
            OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624); // sets timer for 1ms
            INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
            INTClearFlag(INT_T2);
            INTEnable(INT_T2, INT_ENABLED);
            
            OpenTimer4(T4_ON | T4_IDLE_CON | T4_SOURCE_INT | T4_PS_1_16 | T4_GATE_OFF, 624); // sets timer for 1ms
            INTSetVectorPriority(INT_TIMER_4_VECTOR, INT_PRIORITY_LEVEL_4);
            INTClearFlag(INT_T4);
            INTEnable(INT_T4, INT_ENABLED);

            INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR); // multi vector just because we have multiples
            INTEnableInterrupts();
            DelayMs(1500); // used a delay so Enter_Scroll next won't be auto-tapped by accident.

            ENTER_SCROLL(); // initiates CINEMATIC VIDEO MODE presentation.
            sec1000 = 0; // RESTARTS the millisecond counter so we can use it for smart loops below

            while (sec1000 <= 2500) // loops for 2 seconds. 
            {
              OledSetCursor(0, 1);
              OledPutString("READY PLAYER 1");
              OledUpdate();
              DelayMs(400);
              OledSetCursor(0, 1);
              OledPutString("               ");
              OledUpdate();
              DelayMs(400);
            }
            // elements are re-initialized below for when Single Player  mode continues
            sec_spawn = 0;
            sec1000 = 0;
            zombie_number = 1;
            OledClearBuffer();
            cur_var = SELECT; // moves to new state SELECT. Another state used only for Single Player
            break;

// --------- GAME STATE: SELECT  -----------////// ------------- ///// ------------ /////// -----------
          // This is the difficulty selection screen for Single Player mode
          //
          // Exclusive mode for single players again. Here, values are re-initialized and the game loops waiting for a button press
          // Game here will enter CINEMATIC mode every 9s of inactivity from user in making a decision.
          // This mode also sets Zombie speed, strength, number, and Ratio of Movement. 

           // *********************  EXTRA CREDIT FEATURE ************************
            // Features: Difficulty Selector, Zombie Ratio of Movement, Zombie Randomness, Random Generation
          case SELECT:
            L = 0, R = 0, U = 0, D = 0;
            LATGCLR = 0xF << 12; // fancy light effect
            run_once = false; // boolean for function so it only updates OLED once with what we have. 
            OledSetCursor(0, 0);
            OledPutString("-Difficulty-");
            OledSetCursor(0, 1);
            OledPutString("Btn3: EASY");
            OledSetCursor(0, 2);
            OledPutString("Btn2: MED");
            OledSetCursor(0, 3);
            OledPutString("Btn1: HARD");
            OledUpdate();
            if (run_once == 0) // used to update OLED only once. 
            {
              OledUpdate();
              run_once = true;
            }
            // if no button press has happened to move, a CINEMATIC video will replay again
            if (sec_spawn >= 9000)
            {
              OledClearBuffer();
              ENTER_SCROLL();
              run_once = 0; // resets updating oled amount so it can update OLED later
              sec_spawn = 0; // sets time to 0 again so it can wait another 9 seconds later
            }

            // if Button1 is pressed. HARD MODE is selected for use. 
            if(Button1History == 0xFF)
            { OledClearBuffer();
              OledSetCursor(2, 0);
              OledPutString("CHOICE: HARD");
              OledSetCursor(0, 2);
              OledPutString("Zombie spawn");
              OledSetCursor(0, 3);
              OledPutString("in 5 sec");
              OledUpdate();
              LATGSET = 1 << 12; // LED's display difficulty level
              DelayMs(150); // fancy effect
              LATGSET = 1 << 13;
              DelayMs(150); // fancy effect
              LATGSET = 1 << 14;
              DelayMs(1500);  // fancy effect
              OledClearBuffer(); // 

              // CODE below will spawn 3 zombies. It will then tell Random_location to give it 3 unique semi-random coordinates.
              // lastly it makes values 0 again for future use
              zombie_number = 3; 
              Get_Random_Location(zombie_number);
              sec_spawn = 0;
              cur_var = RUN; //next single player state
              break;
            }
            // if button 2 is pressed, MED difficulty is used
            else if (Button2History == 0xFF)
            {
              OledClearBuffer();
              OledSetCursor(2, 0);
              OledPutString("CHOICE: MED");
              OledSetCursor(0, 2);
              OledPutString("Zombie spawn");
              OledSetCursor(0, 3);
              OledPutString("in 5 sec");
              OledUpdate();
              LATGSET = 1 << 12; // LED's display difficulty level
              DelayMs(150); // fancy effect
              LATGSET = 1 << 13;
              DelayMs(1500);  // fancy effect
              OledClearBuffer();

              // Spawns 2 zombies and gives 2 unique locations
              zombie_number = 2;
              Get_Random_Location(zombie_number);
              sec_spawn = 0;
              cur_var = RUN; // sets next state and breaks
              break;
            }

            // button 3 pressed means easy mode.
            else if (Button3History == 0xFF)
            {
              OledClearBuffer();
              OledSetCursor(2, 0);
              OledPutString("CHOICE: EASY");
              OledSetCursor(0, 2);
              OledPutString("Zombie spawn");
              OledSetCursor(0, 3);
              OledPutString("in 5 sec");
              OledUpdate();
              LATGSET = 1 << 12; // only one basic fancy light and that is it. 
              DelayMs(1500); // fancy effect
              // spanws only 1 zombie in one unique location
              zombie_number = 1;
              sec_spawn = 0;
              Get_Random_Location(zombie_number);

              OledClearBuffer();
              cur_var = RUN;
              break;
            }
            // game is about to start so many global variables are re-initialized to 0 so it can be used later in the game when it runs
            sec1000 = 0;  // keeps track of score
            singleloop = 0; // runs tap loop only X times
            slow_zomb = 0; // used to slow down the zombie
            slowup = 0;  // enables powerup slow
            power_slow = 0; // enables power slow
            power_freeze = 0; // enables power freeze
            flicker = 0; // creates flicker in screen for cool elapsed effect
            flicker2 = 0;

            Get_Random_Location(0); // gives human character a random location. 
            cur_var = SELECT;
            break;


// --------- GAME STATE: RUN  -----------////// ------------- ///// ------------ /////// -----------
          // Game code for Single Player game only.
           // Code below moves both Human and Zombie characters while bouncing them
           // Code then also checks if Hero is infected or not
           // Code also enables use of powerups 
           // Spawning of zombie delayed is implemented
           // and finally the TAP is enabled after a few seconds of run time. 
           // *********************  EXTRA CREDIT FEATURE ************************
            // Features: Multiple Zombies, Ratio of Movement, Powerups, Difficulty Levels

          case RUN:
            // outputs the Score in the top right Window as seconds. 
            OledSetCursor(13, 0);
            sprintf(buf1,"%3d",sec1000/1000);
            OledPutString(buf1);
            // keeps track of for loop variables
            int i = 0;
            int r = 0;

          // The below code checks if a power up has been used.
          // if not, it will at start, display the F and S to show that they are available
          // once the power has been used by pressing the button
          // the Letter will dissappear and flicker from 0 to 5 will start as a countdown
          // to show how many seconds remain before power dissapears. 
            if(power_freeze == 0)
            {
              OledSetCursor(15, 2);
              OledPutString("F");
            }
            else if (power_freeze == 1)
            {
              if(flicker >=2)
              {
                OledSetCursor(15, 2);
                sprintf(locbuf,"%1d",sec_freeze/1000);
                OledPutString(locbuf);
                flicker = 0;
              }
              else
              {
                OledSetCursor(15, 2);
                OledPutString(" ");
              }

            }
            else
            {
                OledSetCursor(15, 2);
                OledPutString(" ");
            }


            if(power_slow == 0)
            {
              OledSetCursor(15, 1);
              OledPutString("S");
            }
            else if (power_slow == 1)
            {
              if(flicker2 >=2)
              {
                OledSetCursor(15, 1);
                sprintf(locbuf,"%1d",sec_slow/1000);
                OledPutString(locbuf);
                flicker2 = 0;
              }
              else
              {
                OledSetCursor(15, 1);
                OledPutString(" ");
              }

            }
            else
            {
                OledSetCursor(15, 1);
                OledPutString(" ");
            }


            // Below code activates 'F' for freeze powerup
            if(Button1History == 0xFF)
            {
              if (!(power_freeze == 2)) // prevents powerup from being used more than once. 
              {
                sec_freeze = 0;
                power_freeze = 1;
              }

            }
            // Below code activates 'S' for slow powerup
            if(Button2History == 0xFF)
            {
              if (!(power_slow == 2)) // prevents powerup from being used twice. 
              {
                power_slow = 1;
                sec_slow = 0;
                slowup = 5; // slows zombies considerably as this adds 5 to each ratio of movement. 
              }

            }
            // draaws the blank for the humans

            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(blank_char);

            // Code below is what trully 'activates' the power in the game besides showing it on OLED
            // a countdown is used with sec_slow for slowdown powerup and sec_spawn for freeze powerup. 
            // if the power is enabled. Then it will first detect which difficulty the game is running on by using zombie_number
            // it then adds 5=slowup to each loop so that the ratio is increased exponentially so zombies don't update as fast for 5 seconds.
            // once time is up, powerslow is set to 2 meaning it can't be used anymore

            if(power_slow == 1)
            {
              if(sec_slow < 5000)
              {
                if(zombie_number == 1)
                {
                  if(slow_zomb >= (3+slowup))
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
                else if (zombie_number == 2)
                {
                  if(slow_zomb >= (2+slowup) )
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
                else
                {
                  if(slow_zomb >= (1+slowup) )
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
              }
              else if (sec_slow > 5000)
              {
                power_slow = 2;
              }

            }
            // if activated, note that the do nothing loop is on purpose to stop zombie from updating. 
            // then it just sets powerfreeze to 2 so it can't be activated anymore
            else if (power_freeze == 1)
            {
              if ((sec_freeze < 5000))
              {
                // do nothing
              }
              else if (sec_freeze > 5000)
              {
                power_freeze = 2;
              }
            }
            // otherwise IF NO POWERUPS ARE USED
            // then the code will do normal operations of updating normally with its respective difficulty level.
// Code below is the MAIN ZOMBIE MOVEMENT CODE. THIS LOOP will run often and is what updates the zombie. 
            else
            {
               if (!(sec_spawn < 5000)) // spawns zombie after about 5-7 seconds from game start. 
              {

                if(zombie_number == 1) // if game is on Easy mode because zombie_number =1 is hard set to EASY
                {
                  if(slow_zomb >= (3))// makes ratio of zombie movement 1:4 so they move and update not too much
                  {
                    zomb_move(); // calls zombie movement code only then
                    slow_zomb = 0;
                  }
                }
                else if (zombie_number == 2) // if difficulty is medium
                {
                  if(slow_zomb >= (2) ) // makes ratio of movement 1:3 so medium speed
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
                else
                {
                  if(slow_zomb >= (1) ) // if difficulty is hard then it makes it ratio of 1:2 updating. 
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
              }
            }
// Code below is the MAIN HUMAN MOVEMENT CODE. This function will produce and make the human move around the playing field. 
            human_move();
            // used for velocity of human moving around the entire map.
            DelayMs(velocity);
            // draws the human afterwards and updates the OLED
            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(head_char);
            OledUpdate();

            // Code below checks over_alarm and uses a loop to check if N number of zombies have hit a human hero.
            // over_alarm is used in functions as global variable to signal game needs to end. Function can't end game directly so it will set a flag 
            // that this will then detect.
            if(over_alarm == 1)
            {
              cur_var = OVER;
              break;
            }

            // loop to check human player hit all zombies.
            for(r=0;r<zombie_number;r++)
            {
              if ((human_position[0].Xpos == zombie_position[r].Xpos) && (human_position[0].Ypos == zombie_position[r].Ypos))
              {
                cur_var = OVER;
                break;
              }
            }
            // code below updates iteration loops to be used later in powerups, TAPs, and ratio of movement
            slow_zomb++; // ratio of movement ++
            flicker++;
            flicker2++;
            singleloop++; // used for TAPS as seen below

            // only after game goes through this loop 12 times or more will TAPS become enabled. 
            if(singleloop >12)
            {
              if(ADXL345_SingleTapDetected() == true)
              {
                cur_var = OVER;
                break;
              }
            }
            
            cur_var = RUN; // continous to loop the game
            break;


// --------- GAME STATE: OVER  -----------////// ------------- ///// ------------ /////// -----------
          // Single player game over loop
          // loop will reset all previously used variables and store current game score to be used for high score calculation
            // also produces a nice GAME OVER screen effect. 

            // *********************  EXTRA CREDIT FEATURE ************************
            // Features: Flashing OLED effects and High Score Board
          case OVER:
            slowup = 0;
            slow_zomb = 0;
            over_alarm = 0;
            zombie_number = 1;
            singleloop = 0;
            game_over_sec = sec1000/1000; // store current game score
            DelayMs(500); // artificial delay for effect
            OledClearBuffer(); // clear the screen

            sec1000 = 0;
            while (sec1000 <= 3000)
            {
              OledSetCursor(0, 1);
              OledPutString("GAME OVER");
              OledUpdate();
              DelayMs(400);
              OledSetCursor(0, 1);
              OledPutString("               ");
              OledUpdate();
              DelayMs(400);
            }
            OledClearBuffer();
            sec1000 = 0;
            OledClearBuffer();

            // The current game score is sent over to my high score calc function where it will determine the global and max HighScore1,2, and 3 for the top
            // 3 scores during the entire current gaming session. 
            high_score_calc(game_over_sec);
            while (sec1000 < 3000)
            {
              OledSetCursor(4, 0);
              sprintf(buf1, "Score %d", game_over_sec);
              OledPutString(buf1);
              OledSetCursor(0,1);
              sprintf(buf1, "#1: %d", high_score1);
              OledPutString(buf1);
              OledSetCursor(0,2);
              sprintf(buf1, "#2: %d", high_score2);
              OledPutString(buf1);
              OledSetCursor(0,3);
              sprintf(buf1, "#3: %d", high_score3);
              OledPutString(buf1);
              if(singleloop == 0)
              {
                OledUpdate();
              }
              singleloop = 1;
            }
            sec_spawn = 0;
            // the game is returned back to difficulty selection screen.
            cur_var = SELECT;
            break;
// --------- GAME STATE: DEBUG  -----------////// ------------- ///// ------------ /////// -----------
            // This state can not be activated unless code is modified. Used only during initial debugging
            // basically just runs HW6 code to check/see ADXL return values 
          case DEBUG:
            DEBUG_XYZ();
            cur_var = DEBUG;
            break;

        }

      }
      // if ADXL_INIT fails to detect a ADXL345 connected to Channel 3 or 4, then my main loop will create a warning error.
      // this will not go away until the game is reset. 
      else
      {
        OledSetCursor(0, 0);
        OledPutString("ERROR SPI/ADXL");
        OledSetCursor(0, 1);
        OledPutString("NOT FOUND.");
        OledSetCursor(0, 3);
        OledPutString("Replug/Rerun");
        OledUpdate();
      }
 
   }

   return 0;
}

////////////////////////////// NOTICE: Comments from here written in Block style to shorten Time ///////////////////////////////




//////////////////////////////Function: void joy_zomb_move()      ////////////////////////////////////////////////////////////////
//
// Game Mode:   2 Player Mode Only
//
// Description: This function here is used with the ADC Joystick to move the human-controlled Zombie when 2player game mode 
//              is choosen. It takes values from ISR ADC which stores into cur_LR and cur_UD and then makes Zombie move in continous
//              Movement based on that value.
//
// Extra Credit: Implements 2 player ADC movement code and also has continuous movement inside it as well. Both controls work at once.
//

void joy_zomb_move()
{
              ///////////Initial ADC Movement Code //////////////////////////////////////////
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

          ///////// Auto Movement Code /////////////////////////////////////////
          // based off the previous hard coded values of Joy_L,Joy_R,Joy_U,Joy_D, if they are stil the same, then it continues

  if (Joy_R == 1)
  {
    zombie_position[0].Xpos++;
  }
  else if (Joy_L == 1)
  {
    zombie_position[0].Xpos--;
  }
  else if (Joy_U == 1)
  {
    zombie_position[0].Ypos--;
  }
  else if ( Joy_D == 1)
  {
    zombie_position[0].Ypos++;
  }
  
  // prevents zombie from moving out of bounds of screen and barrier
  if(zombie_position[0].Xpos >= 12)
  {
    zombie_position[0].Xpos = 12;
  }
  if(zombie_position[0].Xpos <= 0)
  {
    zombie_position[0].Xpos = 0;
  }
  if(zombie_position[0].Ypos >= 3)
  {
    zombie_position[0].Ypos = 3;
  }
  if (zombie_position[0].Ypos <= 0)
  {
    zombie_position[0].Ypos = 0;
  }
  //////////////////////////////////////////////////////////////////////////////////////////
}


//////////////////////////////    Function: void high_score_calc(int cur_score)       ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode Only
//
// Description: This function calculates high scores given the current score argument feed from the game.
//              It not only outputs the high score but compares and only outputs the correct Top 3 scores in order of magnitude
//              It does simple > or < calculation to determine high score. 
//
// Extra Credit: Implements High Score board for Single Player end game display. Recalculates values as well.
//
void high_score_calc(int cur_score)
{
  if(cur_score > high_score1)
  {
    if(high_score1 > high_score2)
    {
      high_score3 = high_score2;
      high_score2 = high_score1;
    }
    high_score1 = cur_score;
  }
  else if (cur_score > high_score2)
  {
    if(high_score2 > high_score3)
    {
      high_score3 = high_score2;
    }
    high_score2 = cur_score;
  }
  else if (cur_score > high_score3)
  {
    high_score3 = cur_score;
  }

}

//////////////////////////////    Function: int phys_bounce()       ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode & Two Player Mode
//
// Description: This function is called within the human_move() function which is used in all game modes. Here it does calculations
//              using the human hero's position to determine where and in what direction/style it should bounce if it hits a barrier
//              It can bounce back elastically but also in a realistic angle. 
//
// Extra Credit: NONE
//

int phys_bounce()
{
  int back = 0;
  if(human_position[0].Xpos >= 12)
  {
    back = 1;
    if ((R == 1) && (U == 1)) // bounce back left and down
    {
      human_position[0].Xpos--; 
      human_position[0].Ypos++;
      
    }
    else if ((R == 1) && (D == 1)) // bounce back left and up
    {
      human_position[0].Xpos--; 
      human_position[0].Ypos--; 

    }
    else 
    {
      human_position[0].Xpos--;
    }
  }
  else if(human_position[0].Xpos <= 0)
  {
    back = 1;
    if ((L == 1) && (U == 1)) // bounce back right and down
    {
      human_position[0].Xpos++; 
      human_position[0].Ypos++;
    }
    else if ((L == 1) && (D == 1)) // bounce back right and up
    {
      human_position[0].Xpos++; 
      human_position[0].Ypos--;
    }
    else
    {
      human_position[0].Xpos++; // bounce back right 
    }
  }
  else if (human_position[0].Ypos <= 0)
  {
    back = 1;
    if((L == 1) && (U == 1)) // bounce back left and down
    {
      human_position[0].Xpos--; 
      human_position[0].Ypos++;
    }
    else if ((R == 1) && (U == 1)) // bounce back right and down
    {
      human_position[0].Xpos++; 
      human_position[0].Ypos++;
    }
    else // else, just bounce straight down
    {
      human_position[0].Ypos++;
    }
  }
  else if (human_position[0].Ypos >= 3)
  {
    back = 1;
    if((L == 1) && (D == 1))
    {
      human_position[0].Xpos--; 
      human_position[0].Ypos--;
    }
    else if ((R == 1) && (D == 1))
    {
      human_position[0].Xpos++; 
      human_position[0].Ypos--;
    }
    else
    {
      human_position[0].Ypos--;
    }
  }
  else
  {
    back = 0;
  }

  return back;

}


//////////////////////////////    Function: void human_move()      ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode & Two Player Mode
//
// Description: This function is used in all modes to move the human around based on ADXL345 values. the values of X and Y are used from
//              one of the ISR's or directly in the game code to be the current value that ADXL reads and then the game produces an action
//              based on the tilt of it. The more tilt it has, the faster the ball goes down towards an angle 
//              Velocity was implemetned based on a suggestion by TA Jack and Nahush in using DelayMs with velocity where a greater tilt
//              should decrease the Delay so the human moves down faster and vice versa. 
//
// Extra Credit: NONE
//

void human_move()
{
  over_alarm = 0;
  int j = 0;

  if(!(phys_bounce()))
  {
    ///////////////////// MOVE FASTER! ////////////////////////
    if ((y < -125) && (x > 125)) // if moving right and upwards
    {
      R = 1;
      L = 0;
      U = 1;
      D = 0;
      velocity = 50;
    }
    else if ((y < -125) && (x < - 125)) // if moving right and down
    {
      R = 1;
      L = 0;
      U = 0;
      D = 1;
      velocity = 50;
    }
    else if ((y > 125) && (x > 125)) // if moving left and upwards
    {
      R = 0;
      L = 1;
      U = 1;
      D = 0;
      velocity = 50;
    }
    else if ((y > 125) && (x < -125)) // if moving left and down
    {
      R = 0;
      L = 1;
      U = 0;
      D = 1;
      velocity = 50;
    }
    else if(y < -125) // MOVE RIGHT
    {
      R = 1;
      L = 0;
      U = 0;
      D = 0;
      velocity = 50;
    }
    else if(y > 125) // MOVE LEFT
    {
      R = 0;
      L = 1;
      U = 0;
      D = 0;
      velocity = 50;
    }
    else if(x > 125) // MOVE UP
    {
      R = 0;
      L = 0;
      U = 1;
      D = 0;
      velocity = 50;
    }
    else if (x < -125) // MOVE DOWN
    {
      R = 0;
      L = 0;
      U = 0;
      D = 1;
      velocity = 50;
    }
    else
    {

      if ((y < -75) && (x > 75)) // if moving right and upwards
      {
        R = 1;
        L = 0;
        U = 1;
        D = 0;
        velocity = 200;
      }
      else if ((y < -75) && (x < - 75)) // if moving right and down
      {
        R = 1;
        L = 0;
        U = 0;
        D = 1;
        velocity = 200;
      }
      else if ((y > 75) && (x > 75)) // if moving left and upwards
      {
        R = 0;
        L = 1;
        U = 1;
        D = 0;
        velocity = 200;
      }
      else if ((y > 75) && (x < -75)) // if moving left and down
      {
        R = 0;
        L = 1;
        U = 0;
        D = 1;
        velocity = 200;
      }
      else if(y < -75) // MOVE RIGHT
      {
        R = 1;
        L = 0;
        U = 0;
        D = 0;
        velocity = 200;
      }
      else if(y > 75) // MOVE LEFT
      {
        R = 0;
        L = 1;
        U = 0;
        D = 0;
        velocity = 200;
      }
      else if(x > 75) // MOVE UP
      {
        R = 0;
        L = 0;
        U = 1;
        D = 0;
        velocity = 200;
      }
      else if (x < -75) // MOVE DOWN
      {
        R = 0;
        L = 0;
        U = 0;
        D = 1;
        velocity = 200;
      }
      else
      {
        if ((y < -25) && (x > 25)) // if moving right and upwards
        {
          R = 1;
          L = 0;
          U = 1;
          D = 0;
          velocity = 400;
        }
        else if ((y < -25) && (x < - 25)) // if moving right and down
        {
          R = 1;
          L = 0;
          U = 0;
          D = 1;
          velocity = 400;
        }
        else if ((y > 25) && (x > 25)) // if moving left and upwards
        {
          R = 0;
          L = 1;
          U = 1;
          D = 0;
          velocity = 400;
        }
        else if ((y > 25) && (x < -25)) // if moving left and down
        {
          R = 0;
          L = 1;
          U = 0;
          D = 1;
          velocity = 400;
        }
        else if(y < -25) // MOVE RIGHT
        {
          R = 1;
          L = 0;
          U = 0;
          D = 0;
          velocity = 400;
        }
        else if(y > 25) // MOVE LEFT
        {
          R = 0;
          L = 1;
          U = 0;
          D = 0;
          velocity = 400;
        }
        else if(x > 25) // MOVE UP
        {
          R = 0;
          L = 0;
          U = 1;
          D = 0;
          velocity = 400;
        }
        else if (x < -25) // MOVE DOWN
        {
          R = 0;
          L = 0;
          U = 0;
          D = 1;
          velocity = 400;
        }


      }

      
    }
      // Continual Movement Code //////////////////
    //      code here is used for continous movement. Where it was previously moving in one direction
    //   then it will continue to move in that direction
    if ((R == 1) && (U == 1)) // right and up
    {
      human_position[0].Xpos++;
      human_position[0].Ypos--;
    }
    else if ((R == 1) && (D == 1)) // right and down
    {
      human_position[0].Xpos++;
      human_position[0].Ypos++;
    }
    else if ((L == 1) && (U == 1)) // left and up
    {
      human_position[0].Xpos--;
      human_position[0].Ypos--;
    }
    else if ((L == 1) && (D == 1)) // left and down
    {
      human_position[0].Xpos--;
      human_position[0].Ypos++;
    }
    else if (R == 1)
    {
      human_position[0].Xpos++;
    }
    else if (L == 1)
    {
      human_position[0].Xpos--;
    }
    else if (U == 1)
    {
      human_position[0].Ypos--;
    }
    else if (D == 1)
    {
      human_position[0].Ypos++;
    }
    // variables are reset to stop continuous movement from occuring. Simple fix/and add if I want to re-enable this feature. 
    R = 0;
    L = 0;
    U = 0;
    D = 0;

  } // end of phys bounce
  // does a quick check here if it hit a zombie during incrementation and sets a global alarm
  // since the human moves at greater ratio of movement, it could be the human touches a zombie but you don't see or can tell as it wasn't directly shown on screen.
  // this is thus used to prevent that from happening
  for(j=0;j<zombie_number;j++)
  {
    if ((human_position[0].Xpos == zombie_position[j].Xpos) && (human_position[0].Ypos == zombie_position[j].Ypos))
    {
      over_alarm = 1;
    }

  }

}

//////////////////////////////    Function: void zomb_move()     ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode Only
//
// Description: This function produces zombie movement and is used in the single player running code. It first checks if all zombie entities are 
//              possibly infecting a hero and then goes on to blank out its current glyph and then increment its position before placing Z glyph back
//              This here Features the Random zombie movement that will still move towards the player using the rand functions for LR and UD.
//              It checks for zombie infection at start of code and at end of code as incrementing position can sometimes cause position to overlap without
//              main code noticing. 
//
// Extra Credit: Implements Random Zombie movement that still moves towards player. Creates realistic zombie limp and immersion for game experience.
//
void zomb_move()
{
  run_once = false;
  int back_2 = 0;
  over_alarm = 0;

  int j = 0;
  for(j=0;j<zombie_number;j++)
  {
    if ((human_position[0].Xpos == zombie_position[j].Xpos) && (human_position[0].Ypos == zombie_position[j].Ypos))
    {
      over_alarm = 1;
    }
    else
    {
      OledSetCursor(zombie_position[j].Xpos, zombie_position[j].Ypos);
      OledDrawGlyph(blank_char);
    }

  }

  // uses rand with SRAND given time/score for unique seeds so zombie moves in semi-random ways that still follow player
  for(j=0;j<zombie_number;j++)
  {
    srand(sec1000 + j); // gets new seed
    zombie_randUD = rand() % 100; // gets some value out of 100

    if(zombie_randUD < 10) // if less than 10
    {
      zombie_position[j].Ypos = zombie_position[j].Ypos; // stays same position
    }
    else if (zombie_randUD > 80) // if greater than 80, then y moves down
    {
      zombie_position[j].Ypos--;
    }
    else if (zombie_randUD > 60) // if greater than 60, then y moves up
    {
      zombie_position[j].Ypos++;
    }
    else // else, it will compare itself to human position and move towards human.
    {
      if (human_position[0].Ypos > zombie_position[j].Ypos)
      {
        zombie_position[j].Ypos++;
      }
      else
      {
        zombie_position[j].Ypos--;
      }
    }
    // logic here for LR is same as logic above. 
    srand(sec1000 - j);
    zombie_randLR = rand() % 100;

    if(zombie_randLR < 10)
    {
      zombie_position[j].Xpos = zombie_position[j].Xpos;
    }
    else if (zombie_randLR > 80)
    {
      zombie_position[j].Xpos--;
    }
    else if (zombie_randLR > 60)
    {
      zombie_position[j].Xpos++;
    }
    else
    {
      if(human_position[0].Xpos > zombie_position[j].Xpos)
      {
        zombie_position[j].Xpos++;
      }
      else
      {
        zombie_position[j].Xpos--;
      }
    }

    // code here is used to prevent zombie from moving past the same barrier that the human plays in
    // the playing field has to stay solid for the game to work so this code here is used
    if(zombie_position[j].Xpos >= 12)
    {
      zombie_position[j].Xpos = 12;
    }
    if(zombie_position[j].Xpos <= 0)
    {
      zombie_position[j].Xpos = 0;
    }
    if(zombie_position[j].Ypos >= 3)
    {
      zombie_position[j].Ypos = 3;
    }
    if (zombie_position[j].Ypos <= 0)
    {
      zombie_position[j].Ypos = 0;
    }
    // draws zombie glyph
    OledSetCursor(zombie_position[j].Xpos, zombie_position[j].Ypos);
    OledDrawGlyph(zombie_char);
  }
  // again tries to see if zombie infects hero
  for(j=0;j<zombie_number;j++)
  {
    if ((human_position[0].Xpos == zombie_position[j].Xpos) && (human_position[0].Ypos == zombie_position[j].Ypos))
    {
      over_alarm = 1;
    }

  }


}

//////////////////////////////    Function: void Get_Random_Location(int sel)      ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode & Two Player Mode
//
// Description: This function is used to give random locations for both human and zombie. It takes in an argument where if I give it 0, it will only
//               generate new location for human. But if I give it a value, it will generate that value of zombie locations. I pass in zombie_number
//               which is also equal to number of zombies produced to this function to get unique locations for them beforehand. 
//                Note that the zombie random location is more restricted than the Hero's is on purpose. 
//
// Extra Credit: Supports any number of zombies to be created or given random locations
//


void Get_Random_Location(int sel) // function to generate a random location on the board for the human and zombie to pop up in. 
{
    srand(sec1000); // using srand to generate a seed giving it my seconds timer
    int i = 0;
    if (sel == 0) // if the variable input is = 0 then we have a sanake that needs replacing. We need to make it ASAP
    {
        srand(sec1000+5); // srand of seed giving it a diferent seed to make
        human_position[0].Xpos = rand() % 10; // snake position on playing board
        human_position[0].Ypos = rand() % 3; // snake position Y on playing board
    }
    else
    {
      for(i=0;i<sel;i++)
      {
        zombie_position[i].Xpos = rand() % 10; // generating random location for zombie position always
        zombie_position[i].Ypos = rand() % 3;
        while ((zombie_position[i].Ypos == 1) || (zombie_position[0].Ypos == 2)) // restricts where zombie can show up. 
        {
          zombie_position[i].Ypos = rand() % 3;
        }

      }

    }
}

//////////////////////////////    Function: void START_initial()      ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode & Two Player Mode
//
// Description: This function is used at beginning of game to do initial PIC32 and button initializations. It allows us use of BTN1 through 3 and Light LEDs
//              Here, we get the OLED initialized and working while printing a message before it returns back to the main loop
//
// Extra Credit: NONE
//

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

    TRISBCLR = 0x0040; // initialize U/D pin B6
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

    ///////////////////////////////////////// DELAY & OLED ENABLING ///////////////////////////////////////////////////////
    DelayInit(); // initializes delays
    OledInit(); // initializes OLED display
    OledSetCursor(0,0);
    OledPutString("Initialized");

}


//////////////////////////////    Function: void Device_initial()     ////////////////////////////////////////////////////////////////
//
// Game Mode:   Single Player Mode & Two Player Mode
//
// Description: This function is used at beginning of game to do ADXL initialization. Used to set up ADXL345, it's tap, and the range I can get
//              from it so I can not have to use any offset to get values that I want. It also sets up initial External interrupts for the game.
//              Lastly, if ADXL init returns a bad value, then we invoke alarm so main code loop will print an error. 
//
// Extra Credit: Supports Plug and Play for both channels and will alarm otherwise. Builds off HW6 code. 
//

void Device_initial()
{

  if(ADXL345_Init() == 0) // calls aDXL function for if status == 0
  {
    ADXL345_SetRangeResolution(ADXL345_RANGE_PM_2G,0x0); // gives it a 4G range

    OledClearBuffer();
    OledSetCursor(0, 0);

    DelayMs(500);

    // reset alarm value to 0 so it won't run just in case
    alarm = 0;

    ADXL345_SingleTapDetected(); // call custom created Boolean function for initial setup and erasing current flag if there is any.

    ADXL345_SetPowerMode(0x1);
    
  }
  else // if status returns any value but 0 such as -1 or even 1, then we will get error statements. 
  {
    // set alarm so error will display above
    alarm = 1;
  }

}
//////////////////////////////    Function: void DEBUG_XYZ()     ////////////////////////////////////////////////////////////////
//
// Game Mode:   NONE
//
// Description: This function is used only for debugging. It is easier to keep it here than to continuously re open HW6 code and run that. 
//               Only used at beginning to find values of X, Y, Z I can use and if I need possible offsets to create velocity idea for speed. 
//
// Extra Credit: NONE
//
void DEBUG_XYZ()
{
     // local  output char protocals for max and min
   char bufout[4];
   char bufout2[4];
   char bufout3[4];
    while(1)
    {
      if(sec1000 > 500)
      { 
        ADXL345_SingleTapDetected();

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
        sprintf(bufout3,"t: %c ",tapped);
        OledPutString(bufout3);

        OledUpdate();

        sec1000 = 0; // reset sec1000 to 0 so it can repoll back up to 500ms

      } 

    }
  

}

//////////////////////////////    Function: void ENTER_SCROLL()     ////////////////////////////////////////////////////////////////
//
// Game Mode:   SINGLE PLAYER MODE ONLY
//
// Description: This function performs the CINEMATIC video display at the beginning of the Single Player mode and during 9s non-involvement 
//              periods within the difficulty screen. It prints out our Hero story and the TA name KABIR in cool stylized letters for credit.
//              Features a Star Wars like scrolling effect and the ability to use tapping to skip it at any time. Adds game immersion.
//
// Extra Credit: Implements CINEMATIC video scroll effect to tell story and set the mode. Also has cool ASCII stylized art. 
//

void ENTER_SCROLL()
{

  typedef struct story_time
  {
      char out[30];

  }story;
  story s[200];

  char str0[30];
  char str1[30];
  char str2[30];
  char str3[30];
  int i = 0;
  int breaknow = 0;

  strcpy(s[0].out,"               ");
  strcpy(s[1].out,"               ");
  strcpy(s[2].out,"               ");
  strcpy(s[3].out,"  -- STORY --  ");
  strcpy(s[4].out,"There once was");
  strcpy(s[5].out,"   a man       ");
  strcpy(s[6].out,"named MAX      ");
  strcpy(s[7].out,"Who was king   ");
  strcpy(s[8].out,"of the Zombies!");
  strcpy(s[9].out,"               ");
  strcpy(s[10].out,"He Had fame   ");
  strcpy(s[11].out,"  POWER       ");
  strcpy(s[12].out,"and Wealth    ");
  strcpy(s[13].out,"              ");
  strcpy(s[14].out,"Beyond your ");
  strcpy(s[15].out,"  WILDEST  ");
  strcpy(s[16].out,"Dreams. ");
  strcpy(s[17].out,"          ");
  strcpy(s[18].out,"Before He died");
  strcpy(s[19].out,"These were his ");
  strcpy(s[20].out,"final words: ");
  strcpy(s[21].out,"              ");
  strcpy(s[22].out,"-My fortune-");
  strcpy(s[23].out,"-is yours for-");
  strcpy(s[24].out,"-the taking,but-");
  strcpy(s[25].out,"-you have to");
  strcpy(s[26].out,"-find it first.-");
  strcpy(s[27].out,"-I left all-");
  strcpy(s[28].out,"-I own down in-");
  strcpy(s[29].out,"-the Underworld-");
  strcpy(s[30].out,"                ");
  strcpy(s[31].out,"Ever Since then.");
  strcpy(s[32].out,"All the HEROS");
  strcpy(s[33].out,"have begun their");
  strcpy(s[34].out,"journeys into");
  strcpy(s[35].out,"the Underworld  ");
  strcpy(s[36].out,"to find MAX's ");
  strcpy(s[37].out,"  Treaure   ");
  strcpy(s[38].out,"That would make");
  strcpy(s[39].out,"their dreams,");
  strcpy(s[40].out,"come TRUE! ");
  strcpy(s[41].out,"          ");
  strcpy(s[42].out,"          ");
  strcpy(s[43].out,"This Game is");
  strcpy(s[44].out," the story of");
  strcpy(s[45].out,"one such hero");
  strcpy(s[46].out,"who ventured");
  strcpy(s[47].out,"into the ");
  strcpy(s[48].out,"  Underworld  ");
  strcpy(s[49].out,"");
  strcpy(s[50].out,"To find the ");
  strcpy(s[51].out,"Treasure of ");
  strcpy(s[52].out,"MAD MAX ");
  strcpy(s[53].out,"");
  strcpy(s[54].out,"");
  strcpy(s[55].out,"");
  strcpy(s[56].out,"");

  strcpy(s[90].out,"");
  strcpy(s[91].out,"");
  strcpy(s[92].out,"");
  strcpy(s[93].out,"");
  strcpy(s[94].out,"Tap at any time");
  strcpy(s[95].out,"             ");
  strcpy(s[96].out,"To skip intro!");
  strcpy(s[97].out,"");
  strcpy(s[98].out,"");
  strcpy(s[99].out,"");
  strcpy(s[100].out," ____  __.");
  strcpy(s[101].out,"|    |/ _|");
  strcpy(s[102].out,"|      <  ");
  strcpy(s[103].out,"|    |   \\ ");
  strcpy(s[104].out,"|____|__ \\");
  strcpy(s[105].out,"        \\/");
  strcpy(s[106].out,"               ");
  strcpy(s[107].out,"   _____   ");
  strcpy(s[108].out,"  /  _  \\  ");
  strcpy(s[109].out," /  /_\\  \\ ");
  strcpy(s[110].out,"/    |    \\");
  strcpy(s[111].out,"\\____|__  /");
  strcpy(s[112].out,"        \\/ ");
  strcpy(s[113].out,"");
  strcpy(s[114].out,"__________ ");
  strcpy(s[115].out,"\\______   \\");
  strcpy(s[116].out," |    |  _/");
  strcpy(s[117].out," |    |   \\");
  strcpy(s[118].out," |______  /");
  strcpy(s[119].out,"        \\/ ");
  strcpy(s[120].out,"");
  strcpy(s[121].out,".___ ");
  strcpy(s[122].out,"|   |");
  strcpy(s[123].out,"|   |");
  strcpy(s[124].out,"|   |");
  strcpy(s[125].out,"|___|");
  strcpy(s[126].out,"     ");
  strcpy(s[126].out,"");
  strcpy(s[127].out,"__________ ");
  strcpy(s[128].out,"\\______   \\");
  strcpy(s[129].out," |       _/");
  strcpy(s[130].out," |    |   \\");
  strcpy(s[131].out," |____|_  /");
  strcpy(s[132].out,"        \\/ ");
  strcpy(s[133].out,"");
  strcpy(s[134].out,"");
  strcpy(s[135].out,"");
  strcpy(s[136].out,"Tap at any time");
  strcpy(s[137].out,"           ");
  strcpy(s[138].out,"To skip intro!");
  strcpy(s[139].out,"");
  strcpy(s[140].out,"");
  strcpy(s[141].out,"");
  strcpy(s[142].out,"");
  strcpy(s[143].out,"");

  OledSetCursor(0, 0);
  OledPutString("-Tap at any time-");
  OledSetCursor(0, 1);
  OledPutString("To skip Intro");
  OledUpdate();
  DelayMs(2000);
  OledClearBuffer();



// loop below is used for creating the scrolling effect. It constantly checks if a tap happened and breaks if so
  for (i=0;i<53;i++)
  {

    if(ADXL345_SingleTapDetected() == true)
    {
      breaknow = 1;
      break;
    }
    OledSetCursor(0, 0);
    sprintf(str0,"%s",s[i].out);
    OledPutString(str0);

    OledSetCursor(0, 1);
    sprintf(str1,"%s",s[i+1].out);
    OledPutString(str1);

    OledSetCursor(0, 2);
    sprintf(str2,"%s",s[i+2].out);
    OledPutString(str2);

    OledSetCursor(0, 3);
    sprintf(str3,"%s",s[i+3].out);
    OledPutString(str3);
    OledUpdate();
    DelayMs(500);
    OledClearBuffer();
  }

  if (breaknow == 0)
  {
    // display is broken up into two parts and will break intentionally at certain parts. 
    for (i=91;i<142;i++)
    {

      if(ADXL345_SingleTapDetected() == true)
      {
        break;
      }
      if(i == 141 )
      {
        break;
      }
      OledSetCursor(0, 0);
      sprintf(str0,"%s",s[i].out);
      OledPutString(str0);

      OledSetCursor(0, 1);
      sprintf(str1,"%s",s[i+1].out);
      OledPutString(str1);

      OledSetCursor(0, 2);
      sprintf(str2,"%s",s[i+2].out);
      OledPutString(str2);

      OledSetCursor(0, 3);
      sprintf(str3,"%s",s[i+3].out);
      OledPutString(str3);
      OledUpdate();

      DelayMs(150);
      OledClearBuffer();

    }
    OledClearBuffer();
    
  }
  return;

}
