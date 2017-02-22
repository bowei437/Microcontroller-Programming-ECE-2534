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
/////////////////////////////////////////////// Global variables ////////////////////////////////////////////////////////////////////
bool run_once = false;
int retValue = 0;
int difficulty = 0;
short x = 0, y = 0, z = 0; // uses float to get values from GETXYZ
char buf1[50];
unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler
unsigned int sec10;
unsigned int sec500;
unsigned int sec_spawn;
unsigned char tapped = 0;
unsigned char tapped2 = 0;
int alarm = 0;
int singleloop = 0;

short L = 0;
short R = 0;
short U = 0;
short D = 0;

int over_alarm = 0;

int diffic_choice = 0;

int zombie_randLR = 0;
int zombie_randUD = 0;

int left_Z = 0;
int right_Z = 0;
int up_Z = 0;
int down_Z = 0;

int high_score1 = 0;
int high_score2 = 0;
int high_score3 = 0;

int velocity = 200;

short min_x = 0, min_y = 0, min_z = 0, max_x = 0, max_y = 0, max_z = 0;

// Creates glyphs of various sizes and shapes.  This is the head char
BYTE head[8] = {0x30, 0x46, 0x86, 0x80, 0x80, 0x86, 0x46, 0x30};
char head_char = 0x00;

BYTE body[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
char body_char = 0x01;

BYTE blank[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char blank_char = 0x02;

BYTE zombie[8] = {0x81, 0xC1, 0xA1, 0xB1, 0x99, 0x8D, 0x87, 0x83};
char zombie_char = 0x03;

typedef struct xy_position
{
    int Xpos;
    int Ypos;

}position;
// I initialize them as global variables so my void functions can have access to modify them as they wish. 
position human_position[40]; // create an array of 20 for SNAKE position
position zombie_position[40];

//////////////////////////////////////////////// MAIN CODE /////////////////////////////////////////////////////////////

int main()
{
    START_initial(); // calls function for intiialization devices
    Device_initial(); // calls ADXL init function
    int game_over_sec = 0;



    enum case_var{SPLASH, SELECT, DIFFIC, RUN, OVER, RESET, DEBUG};
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

    OledClearBuffer(); // just clears OLED buffer before start

   while(1)
   {
      if(alarm != 1)
      {
        switch(cur_var)
        {
          case SPLASH:
            sec1000 = 0;
            run_once = false;
            while(sec1000 <= 2000)
            {
              OledSetCursor(0, 0);
              OledPutString("MAD MAX:THE GAME");
              OledSetCursor(0, 1);
              OledPutString("By Bowei Zhao"); // name
              OledSetCursor(0, 2);
              OledPutString("ECE 2534");
              if(run_once == 0)
              {
                OledUpdate();
                run_once = true;
              }
            }
            OledClearBuffer();
            sec1000 = 0;
            while (sec1000 <= 2000)
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
            cur_var = SELECT;

            break;

          case SELECT:
            L = 0, R = 0, U = 0, D = 0;
            LATGCLR = 0xF << 12; // fancy light effect
            run_once = false;
            OledSetCursor(0, 0);
            OledPutString("-Difficulty-");
            OledSetCursor(0, 1);
            OledPutString("Btn1: HARD");
            OledSetCursor(0, 2);
            OledPutString("Btn2: MED");
            OledSetCursor(0, 3);
            OledPutString("Btn3: EASY");
            OledUpdate();
            if (run_once == 0)
            {
              OledUpdate();
              run_once = true;
            }
            if(PORTG & 0x40)
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
              difficulty = 200;
              diffic_choice = 0; // CURRENT DEBUG 0
              Get_Random_Location(diffic_choice);
              cur_var = RUN;
              break;
            }
            else if (PORTG & 0x80)
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
              difficulty = 350; // artifficial ms delay for loop
              diffic_choice = 2;
              Get_Random_Location(diffic_choice);
              cur_var = RUN; // sets next state and breaks
              break;
            }
            else if (PORTA & 0x1)
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
              difficulty = 500;
              diffic_choice = 1;
              Get_Random_Location(diffic_choice);

              OledClearBuffer();
              cur_var = RUN;
              break;
            }
            sec1000 = 0;
            sec_spawn = 0;
            singleloop = 0;
            Get_Random_Location(0);
            cur_var = SELECT;
            break;

////////////////////////////// RUN CASE /////////////////////////////////////////////

          case RUN:
            OledSetCursor(13, 0);
            int i = 0;
            sprintf(buf1,"%3d",sec1000/1000);
            OledPutString(buf1);


            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(blank_char);

            

            if (!(sec_spawn < 5000))
            {
              zomb_move();
            }
            if(over_alarm == 1)
            {
              cur_var = OVER;
              break;
            }
            
            human_move();
            DelayMs(velocity);

            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(head_char);

            OledUpdate();
            
            singleloop++;
            if(singleloop >5)
            {
              if(ADXL345_SingleTapDetected() == true)
              {
                cur_var = OVER;
                break;
              }
            }
            
            cur_var = RUN;
            break;

////////////////////////////// OVER CASE /////////////////////////////////////////////

          case OVER:
            over_alarm = 0;
            singleloop = 0;
            game_over_sec = sec1000/1000; // store current game score
            DelayMs(500); // artificial delay for effect
            OledClearBuffer(); // clear the screen
            sec1000 = 0;
            while(sec1000 < 3000) // loops around for 3 seconds. 
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
            sec1000 = 0;
            OledClearBuffer();
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
            cur_var = SELECT;
            break;







          case DEBUG:
            DEBUG_XYZ();
            cur_var = DEBUG;
            break;

        }

      }


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

void human_move()
{
  if(!(phys_bounce()))
  {
    ///////////////////// MOVE FASTER! ////////////////////////
    if ((y < -150) && (x > 150)) // if moving right and upwards
    {
      R = 1;
      L = 0;
      U = 1;
      D = 0;
      velocity = 50;
    }
    else if ((y < -150) && (x < - 150)) // if moving right and down
    {
      R = 1;
      L = 0;
      U = 0;
      D = 1;
      velocity = 50;
    }
    else if ((y > 150) && (x > 150)) // if moving left and upwards
    {
      R = 0;
      L = 1;
      U = 1;
      D = 0;
      velocity = 50;
    }
    else if ((y > 150) && (x < -150)) // if moving left and down
    {
      R = 0;
      L = 1;
      U = 0;
      D = 1;
      velocity = 50;
    }
    else if(y < -150) // MOVE RIGHT
    {
      R = 1;
      L = 0;
      U = 0;
      D = 0;
      velocity = 50;
    }
    else if(y > 150) // MOVE LEFT
    {
      R = 0;
      L = 1;
      U = 0;
      D = 0;
      velocity = 50;
    }
    else if(x > 150) // MOVE UP
    {
      R = 0;
      L = 0;
      U = 1;
      D = 0;
      velocity = 50;
    }
    else if (x < -150) // MOVE DOWN
    {
      R = 0;
      L = 0;
      U = 0;
      D = 1;
      velocity = 50;
    }
    else
    {

      if ((y < -100) && (x > 100)) // if moving right and upwards
      {
        R = 1;
        L = 0;
        U = 1;
        D = 0;
        velocity = 200;
      }
      else if ((y < -100) && (x < - 100)) // if moving right and down
      {
        R = 1;
        L = 0;
        U = 0;
        D = 1;
        velocity = 200;
      }
      else if ((y > 100) && (x > 100)) // if moving left and upwards
      {
        R = 0;
        L = 1;
        U = 1;
        D = 0;
        velocity = 200;
      }
      else if ((y > 100) && (x < -100)) // if moving left and down
      {
        R = 0;
        L = 1;
        U = 0;
        D = 1;
        velocity = 200;
      }
      else if(y < -100) // MOVE RIGHT
      {
        R = 1;
        L = 0;
        U = 0;
        D = 0;
        velocity = 200;
      }
      else if(y > 100) // MOVE LEFT
      {
        R = 0;
        L = 1;
        U = 0;
        D = 0;
        velocity = 200;
      }
      else if(x > 100) // MOVE UP
      {
        R = 0;
        L = 0;
        U = 1;
        D = 0;
        velocity = 200;
      }
      else if (x < -100) // MOVE DOWN
      {
        R = 0;
        L = 0;
        U = 0;
        D = 1;
        velocity = 200;
      }
      else
      {
        if ((y < -50) && (x > 50)) // if moving right and upwards
        {
          R = 1;
          L = 0;
          U = 1;
          D = 0;
          velocity = 400;
        }
        else if ((y < -50) && (x < - 50)) // if moving right and down
        {
          R = 1;
          L = 0;
          U = 0;
          D = 1;
          velocity = 400;
        }
        else if ((y > 50) && (x > 50)) // if moving left and upwards
        {
          R = 0;
          L = 1;
          U = 1;
          D = 0;
          velocity = 400;
        }
        else if ((y > 50) && (x < -50)) // if moving left and down
        {
          R = 0;
          L = 1;
          U = 0;
          D = 1;
          velocity = 400;
        }
        else if(y < -50) // MOVE RIGHT
        {
          R = 1;
          L = 0;
          U = 0;
          D = 0;
          velocity = 400;
        }
        else if(y > 50) // MOVE LEFT
        {
          R = 0;
          L = 1;
          U = 0;
          D = 0;
          velocity = 400;
        }
        else if(x > 50) // MOVE UP
        {
          R = 0;
          L = 0;
          U = 1;
          D = 0;
          velocity = 400;
        }
        else if (x < -50) // MOVE DOWN
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
    R = 0;
    L = 0;
    U = 0;
    D = 0;

  } // end of phys bounce

}

void zomb_move()
{
  run_once = false;
  int back_2 = 0;
  over_alarm = 0;

  int j = 0;
  for(j=0;j<diffic_choice;j++)
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

  
  for(j=0;j<diffic_choice;j++)
  {
    srand(sec1000 + j);
    zombie_randUD = rand() % 100;

    if(zombie_randUD < 15)
    {
      zombie_position[j].Ypos = zombie_position[j].Ypos;
    }
    else if (zombie_randUD > 75)
    {
      zombie_position[j].Ypos--;
    }
    else if (zombie_randUD > 50)
    {
      zombie_position[j].Ypos++;
    }
    else
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

    srand(sec1000 - j);
    zombie_randLR = rand() % 100;

    if(zombie_randLR < 15)
    {
      zombie_position[j].Xpos = zombie_position[j].Xpos;
    }
    else if (zombie_randLR > 75)
    {
      zombie_position[j].Xpos--;
    }
    else if (zombie_randLR > 50)
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
    OledSetCursor(zombie_position[j].Xpos, zombie_position[j].Ypos);
    OledDrawGlyph(zombie_char);
  }


}


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
        while ((zombie_position[i].Ypos == 1) || (zombie_position[0].Ypos == 2))
        {
          zombie_position[i].Ypos = rand() % 3;
        }

      }

    }

    
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

    TRISBCLR = 0x0040; // initialize U/D pin B6
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

///////////////////////////////////////// DELAY & OLED ENABLING ///////////////////////////////////////////////////////
    DelayInit(); // initializes delays
    OledInit(); // initializes OLED display
    OledSetCursor(0, 0);
    OledClearBuffer();
    //OledPutString("Initial");
    OledUpdate();

/////////////////////////////////////////TIMERS ENABLING ///////////////////////////////////////////////////////
   
   OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624); // sets timer for 1ms
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);

   OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_16 | T3_GATE_OFF, 634); // sets timer for 1ms
  INTSetVectorPriority(INT_TIMER_3_VECTOR, INT_PRIORITY_LEVEL_4);
  INTClearFlag(INT_T3);
  INTEnable(INT_T3, INT_ENABLED);

   INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR); // multi vector just because
   INTEnableInterrupts();

}

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
    /*
      INT1 – JE-07, digital pin 36, RE08
      INT2 – JF-07, digital pin 44, RE09
    */
    ADXL345_SingleTapDetected();

/*
    ADXL345_SetTapDetection(ADXL345_SINGLE_TAP |
                            ADXL345_DOUBLE_TAP,  
                            ADXL345_TAP_X_EN,   
                            0x10,  
                            0x20,   
                            0x40,   
                            0x60,   
                            0x00);    
                            */

    ADXL345_SetFreeFallDetection(0x01,  /*!< Free-fall detection enabled. */
                                 0x05,  /*!< Free-fall threshold. */
                                 0x14,  /*!< Time value for free-fall detection. */
                                 0x00); /*!< Interrupt Pin. */
    ADXL345_SetPowerMode(0x1);
    
  }
  else // if status returns any value but 0 such as -1 or even 1, then we will get error statements. 
  {
    // set alarm so error will display above
    alarm = 1;
  }

}

void DEBUG_XYZ()
{
     // local  output protocals for max and min
   
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
        // it then just displays the values out in deciaml format

        /*
              if((tapped & ADXL345_SINGLE_TAP) != 0)
              {
                  OledSetCursor(8, 1);
                  OledPutString("Single");
                  singleloop = 0;
              }
              singleloop++;
              if (singleloop >=5)
              {
                  OledSetCursor(8, 1);
                  OledPutString("       ");
                  singleloop = 0;
              }

              OledSetCursor(8, 2);
              sprintf(buf1,"%x",tapped);
              OledPutString(buf1);

              OledUpdate();


        */

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

 ////////////////////////////////////// ISR LOOPS ////////////////////////////////////////////////////////////////////////
//
// The #Define configurations below are used to set values for the ADC later on so they can be used and initialized
void __ISR(_TIMER_3_VECTOR, IPL4AUTO) _Timer3Handler(void) 
{
  sec1000++;
  INTClearFlag(INT_T3);
}


// isr that polls the ADXL for x, y, z values every 10 ms
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{
  // only polls every 10 ms
  if(sec10 > 10)
  { 
    ADXL345_GetXyz(&x,&y,&z);
    sec10 = 0;
  }
  if(sec500 >500)
  {
    //tapped = ADXL345_GetRegisterValue(ADXL345_INT_SOURCE);
    sec500 = 0;
  }
  
  //sec1000++; // Increment the millisecond counter.
  sec10++;
  sec500++;
  sec_spawn++;
  INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}