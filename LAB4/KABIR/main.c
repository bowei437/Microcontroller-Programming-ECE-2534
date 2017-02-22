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
void ENTER_SCROLL();
/////////////////////////////////////////////// Global variables ////////////////////////////////////////////////////////////////////
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2
unsigned char Button3History = 0x00;
bool run_once = false;
int retValue = 0;
int difficulty = 0;
short x = 0, y = 0, z = 0; // uses float to get values from GETXYZ
char buf1[50];

unsigned int sec1000; // This is updated 1000 times per second by the interrupt handler
unsigned int sec10;
unsigned int sec500;
unsigned int sec_spawn;
unsigned int sec_freeze = 0;
unsigned int sec_slow = 0;
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
    int slowup = 0;

    int power_freeze = 0;
    int power_slow = 0;
    int flicker = 0;
    int flicker2 = 0;
    char locbuf[20];



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
    int slow_zomb = 0;

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
            ENTER_SCROLL();
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
            sec_spawn = 0;
            sec1000 = 0;
            OledClearBuffer();
            cur_var = SELECT;

            break;

          case SELECT:
            L = 0, R = 0, U = 0, D = 0;
            LATGCLR = 0xF << 12; // fancy light effect
            run_once = false;
            OledSetCursor(0, 0);
            OledPutString("-Difficulty-");
            OledSetCursor(0, 1);
            OledPutString("Btn3: EASY");
            OledSetCursor(0, 2);
            OledPutString("Btn2: MED");
            OledSetCursor(0, 3);
            OledPutString("Btn1: HARD");
            OledUpdate();
            if (run_once == 0)
            {
              OledUpdate();
              run_once = true;
            }

            if (sec_spawn >= 9000)
            {
              OledClearBuffer();
              ENTER_SCROLL();
              run_once = 0;
              sec_spawn = 0;
            }

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
              difficulty = 200;
              diffic_choice = 3; // CURRENT DEBUG 0
              Get_Random_Location(diffic_choice);
              sec_spawn = 0;
              cur_var = RUN;
              break;
            }
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
              difficulty = 350; // artifficial ms delay for loop
              diffic_choice = 2;
              Get_Random_Location(diffic_choice);
              sec_spawn = 0;
              cur_var = RUN; // sets next state and breaks
              break;
            }
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
              difficulty = 500;
              diffic_choice = 1;
              sec_spawn = 0;
              Get_Random_Location(diffic_choice);

              OledClearBuffer();
              cur_var = RUN;
              break;
            }
            sec1000 = 0;
            
            singleloop = 0;
            slow_zomb = 0;
            slowup = 0;
            power_slow = 0;
            power_freeze = 0;
            flicker = 0;
            flicker2 = 0;
            Get_Random_Location(0);
            cur_var = SELECT;
            break;

////////////////////////////// RUN CASE /////////////////////////////////////////////

          case RUN:
            OledSetCursor(13, 0);
            sprintf(buf1,"%3d",sec1000/1000);
            OledPutString(buf1);
            int i = 0;
            int r = 0;


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
              if (!(power_freeze == 2))
              {
                sec_freeze = 0;
                power_freeze = 1;
              }

            }
            // Below code activates 'S' for slow powerup
            if(Button2History == 0xFF)
            {
              if (!(power_slow == 2))
              {
                power_slow = 1;
                sec_slow = 0;
                slowup = 5;
              }

            }

            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(blank_char);

            if(power_slow == 1)
            {
              if(sec_slow < 5000)
              {
                if(diffic_choice == 1)
                {
                  if(slow_zomb >= (3+slowup))
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
                else if (diffic_choice == 2)
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
            else
            {
               if (!(sec_spawn < 5000))
              {

                if(diffic_choice == 1)
                {
                  if(slow_zomb >= (3))
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
                else if (diffic_choice == 2)
                {
                  if(slow_zomb >= (2) )
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
                else
                {
                  if(slow_zomb >= (1) )
                  {
                    zomb_move();
                    slow_zomb = 0;
                  }
                }
              }
            }

            


            human_move();
            DelayMs(velocity);

            OledSetCursor(human_position[0].Xpos, human_position[0].Ypos);
            OledDrawGlyph(head_char);
            OledUpdate();

            
            if(over_alarm == 1)
            {
              cur_var = OVER;
              break;
            }
            
            for(r=0;r<diffic_choice;r++)
            {
              if ((human_position[0].Xpos == zombie_position[r].Xpos) && (human_position[0].Ypos == zombie_position[r].Ypos))
              {
                cur_var = OVER;
                break;
              }
            }
            slow_zomb++;
            flicker++;
            flicker2++;
            singleloop++;
            if(singleloop >12)
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
            slowup = 0;
            slow_zomb = 0;
            over_alarm = 0;
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
  for(j=0;j<diffic_choice;j++)
  {
    if ((human_position[0].Xpos == zombie_position[j].Xpos) && (human_position[0].Ypos == zombie_position[j].Ypos))
    {
      over_alarm = 1;
    }

  }

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

    if(zombie_randUD < 10)
    {
      zombie_position[j].Ypos = zombie_position[j].Ypos;
    }
    else if (zombie_randUD > 80)
    {
      zombie_position[j].Ypos--;
    }
    else if (zombie_randUD > 60)
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

  for(j=0;j<diffic_choice;j++)
  {
    if ((human_position[0].Xpos == zombie_position[j].Xpos) && (human_position[0].Ypos == zombie_position[j].Ypos))
    {
      over_alarm = 1;
    }

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

   OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_16 | T3_GATE_OFF, 624); // sets timer for 1ms
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
    // only polls every 10 ms
  if(sec10 > 10)
  { 
    ADXL345_GetXyz(&x,&y,&z);
    sec10 = 0;
  }
  if(sec500 >500)
  {
    sec500 = 0;
  }

  sec10++;
  sec500++;
  sec_spawn++;
  sec1000++;
  sec_slow++;
  sec_freeze++;
  INTClearFlag(INT_T3);
}

// Interrupt handler - respond to timer-generated interrupt
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

void ENTER_SCROLL()
{

  typedef struct story_time
  {
      char out[20];

  }story;
  story s[200];

  char str0[20];
  char str1[20];
  char str2[20];
  char str3[20];
  int i = 0;
  int breaknow = 0;

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
  strcpy(s[44].out,"is the story of");
  strcpy(s[45].out,"one such hero");
  strcpy(s[46].out,"who ventured");
  strcpy(s[47].out,"into the ");
  strcpy(s[48].out,"  Underworld  ");
  strcpy(s[49].out,"");
  strcpy(s[50].out,"To find the ");
  strcpy(s[51].out,"Treasure of ");
  strcpy(s[52].out,"~MAD MAX~");
  strcpy(s[53].out,"");
  strcpy(s[54].out,"");

  for (i=92;i<140;i++)
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
    DelayMs(150);
    OledClearBuffer();
  }

  if (breaknow == 0)
  {
    for (i=0;i<54;i++)
    {

      if(ADXL345_SingleTapDetected() == true)
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
      DelayMs(700);
      OledClearBuffer();
    }
  }




}

