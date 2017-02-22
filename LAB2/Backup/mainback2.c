///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        LAB2
// STUDENT:         Bowei Zhao
// Description:     File Program to test code on the Digilent MX board for LAB2.
//                  Program will write a timer for the OLED and play HANGMAN on Terminal
//                  PB1 will start the game and 10 tries will be given. 
//
// File name:       main.c
// the main file change log is set here
// 
// Resources:       main.c uses Timer2, and Timer 3 to generate interrupts at intervals of 1 ms and 1s.
//          delay.c uses Timer1 to provide delays with increments of 1 ms. It also uses a wide array of functions
//          to give it the ability to be modular in approach
//          PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/1/2014
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
//#include <time.h>
#include <stdlib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include <GenericTypeDefs.h>


// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL  // Oscillator selection
#pragma config POSCMOD      = XT  // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2 // PLL input divider
#pragma config FPLLMUL      = MUL_20  // PLL multiplier
#pragma config FPLLODIV     = DIV_1 // PLL output divider
#pragma config FPBDIV       = DIV_8 // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF // Secondary oscillator enable
#define GetPGClock() 10000000 // define FPGclock. It is already defined above with pragma and config but this is used later in UART as a variable

/////////////////////////////////////////////// Global variables ////////////////////////////////////////////////////////////////////
 // initialize time1s to be seconds. Will run later in code
unsigned int minutes = 0; // initialize minutes to be minutes. Will run later in code.
unsigned int buttondelay = 0; // Buttondelay is used so that the next 'press' won't be registered after switch state.
unsigned char Button1History = 0x00; // Last 8 samples of BTN1
unsigned char Button2History = 0x00; // Last 8 samples of BTN2
char output[50];        // Temp string for OLED display
char output2[50];
char output3[50];
char output4[50];
char OLEDout[50];
int trials = 10;
enum case_var{READ, Game_Start, LoopGame};
enum case_var cur_var = READ;
unsigned int time1s =0;
unsigned int timekeep;
int BTN_PRESS = 0;// BTN_PRESS is set to 0. BTN press is used as the tertiary check for a button press on BTN1
int saved = 0; // keeps position of time1s to compare it in while loop
int flag = 0; // default value for not breaking loops
int loop_stop = 0;



////////////////////////////////////// FUNCTIONS DECLARATION ////////////////////////////////////////////////////////////////////////
void OLEDTIMER();
void UART_START(UART_MODULE myUART);
void MAIN_START();
int puts(const char *strp);
char* getWord();
char* asterisk(const char *str);
int getChar(void);
char* GAME_LOOP(const char *word,UINT8 bufet);

///////////////////////////////////// BUTTON DEBOUNCING FUNCTION /////////////////////////////////////////////////////////////////////

// Interrupt handler - respond to timer-generated interrupt
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534( void )
{
   if( INTGetFlag(INT_T2) )             // Verify source of interrupt
   {
      Button1History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x40) // Record the latest BTN1 sample
      Button1History |= 0x01;
      Button2History <<= 1; // Discard oldest sample to make room for new
      if(PORTG & 0x80) // Record the latest BTN2 sample
      Button2History |= 0x01;

      INTClearFlag(INT_T2);             // Acknowledge interrupt
   }
   return;
}
//////////////////////////////// MAIN CODE /////////////////////////////////////////////////////////////
int main()
{
   UART_START(UART1); // initializes all code for UART by calling function of module 
   MAIN_START();

   
   ///////////////////////////// Main processing loop ////////////////////////////////////////////////////
   while (1)
   {
      OLEDTIMER(); // call function to update time in seconds. 

      switch(cur_var)
      {
         case READ:
            if (( (Button1History == 0xFF && (PORTG & 0x40)) || flag == 1 ))
            {
               OledSetCursor(0, 0);
               OledPutString("HANGMAN");
               OledUpdate();
               flag = 0;
               const char welcome_msg[] =
               {
                  "ECE 2534 Hangman! \r\n"\
                  "Let's Play! \r\n"
               };

               puts(welcome_msg);
               cur_var = Game_Start;
               break;
            }
            else
            {
               cur_var = READ;
               break;
            }
         case Game_Start:
            //OledSetCursor(0, 0);
            //OledPutString("HANGMAN START");
            //OledUpdate();


            //sprintf(output2, "Secret Word: %s \r\nRemaining trials: %d \r\n", dot, trials);
            //puts(output2);


            //GAME_LOOP();


            cur_var = LoopGame;
            break;
         case LoopGame:
                OledSetCursor(0, 0);
                //OledPutString("HANGMAN LOOP");
                OledUpdate();
                char *word = getWord();
                char *dot = asterisk(word);
                if (loop_stop==0)
                {
                  sprintf(output4, "Secret Word: %s \r\nRemaining trials: %d \r\n", dot, trials);
                  puts(output4);
                  loop_stop++;
                }
                


                sprintf(output3, "%s", word);
                OledSetCursor(0, 0);
                OledPutString(output3);
                OledUpdate();
    

                UINT8 bufet;
   
               while(!trials == 0)
               {
                  if(loop_stop!=0)
                  {
                    //sprintf(output3, "Secret Word: %s \r\nRemaining trials: %d \r\n", dot, trials);
                    puts(output3);

                  }

                  char loc_out[50];
                  sprintf(loc_out, "Enter your selection: ");
                  puts(loc_out);
                  bufet = getChar();
                  while(bufet == 255)
                  {
                    bufet = getChar();
                    OLEDTIMER();
                    bufet = getChar();

                  }
                  //sprintf(output4, "***DEBUG LoopGame: Pressed = %c or %d \r\n", bufet, bufet);
                  //puts(output4);
                  if(flag == 1) // 255 is return -1 in ASCII
                  {
                     cur_var = READ;
                     break;
                  }
                  else
                  {
                    flag = 0;
                    char *result[20];
                    *result = GAME_LOOP(word,bufet);
                    sprintf(output3, "\r\nresult char loop is: %c \r\n,", result);
                    puts(output3);

                    sprintf(output4, "\r\nresult str loop is: %s \r\n,", result);
                    puts(output4);
                  }

                  
                  
                     //sprintf(output3, "**DEBUG: Let's see if word changed: %s \r\n", word);
                     //puts(output3);

               }
               cur_var = READ;
               break;

         default:
            cur_var = READ;
            break;

      }


     
   }
   return 0; // c code requires this as it is older.
}

////////////////////////////////// FUNCTION CODE BLOCK ///////////////////////////////////////////////////////////
char* GAME_LOOP(const char *word,UINT8 bufet)
{
    
    //sprintf(output3, "\r\n\r\n***DEBUG GAME_LOOP: WORD change: %s \r\n***DEBUG GAME_LOOP: Character change: %c \r\n", word, bufet);
    //puts(output3);
    int check;
    int i = 0;
    int sizegl;
    sizegl = strlen(word);
    char outstr[sizegl+1];

    check = 0;
    //char cmp2[sizegl];
    //char storage2[] = "*************";

    for(i=0;i<sizegl;i++)
    {

      if(word[i] == bufet)
      {
        outstr[i] = word[i];
        check = 1;
      }
      else if (word[i] != bufet)
      {
        outstr[i] = '*';
      }

    }
    if(check == 0)
    {
      trials--;
    }

    sprintf(output, "outstring is: %s\r\ntrials is: %d\r\n", outstr, trials);
    puts(output);


    return outstr;
}

void OLEDTIMER()
{
   char OLEDtime[50];
   if (minutes == 59 && time1s == 60)
   {
      time1s = 0;
      minutes = 0;
   }
   else if (time1s == 60)
   {
      time1s = 0;
      minutes++;
   }
   else
   {
      while(saved == time1s)
      {
         timekeep = ReadTimer3();
         if(timekeep == 39061)
         {
            time1s++;
         }

      }
      sprintf(OLEDtime, "%2.2d:%2.2d",minutes, time1s);
      OledSetCursor(0, 2);
      // set cursor is used to puts it on the second row/line
      OledPutString(OLEDtime); //buf is loaded into the OLED
      OledUpdate(); // OLED display is updated
      saved = time1s;

   }

}

void UART_START(UART_MODULE myUART)
{
   UARTConfigure( myUART, UART_ENABLE_PINS_TX_RX_ONLY );
   UARTSetFifoMode(myUART, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
   UARTSetLineControl(myUART, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
   UARTSetDataRate(myUART, GetPGClock(), 9600);
   UARTEnable( myUART, UART_ENABLE|UART_PERIPHERAL|UART_RX|UART_TX );

}

void MAIN_START()
{
   TRISGSET = 0x40;     // For BTN1: configure PortG bit for input
   TRISGSET = 0x80;     // For BTN2: configure PortG bit for input
   TRISGCLR = 0xf000;   // For LEDs: configure PortG pins for output
   ODCGCLR  = 0xf000;   // For LEDs: configure as normal output (not open drain)
   LATGCLR  = 0xf000;   // Initialize LEDs to 0000
   // Enumeration states below are of type enum named case_var. 
   // They have the states of being READ, BTN1_START, BTN1_STOP, and BTN1_RESET. 
   // the current case variable will be hard set to READ. 
   // ENUM Default will be used just in case but setting current state is better.
    // used for comparing values of second to make loop run
   // Initialize Timer1, Timer 2, Time3 and OLED
   DelayInit();
   OledInit();
   // Set up Timer2 to roll over every ms
   OpenTimer2(T2_ON         |
             T2_IDLE_CON    |
             T2_SOURCE_INT  |
             T2_PS_1_16     |
             T2_GATE_OFF,
             624);  // freq = 10MHz/16/625 = 1 kHz
   // New Timer 3 is initialized to roll over every second. 
   // it is given a FPBCLK of 10MHz, Prescale of 256, and Period Register of 39061 to give T
   // MRx=Frequency Rollover of 1Hz = 1 second.
   OpenTimer3(T3_ON         |
             T3_IDLE_CON    |
             T3_SOURCE_INT  |
             T3_PS_1_256     |
             T3_GATE_OFF,
             39061); // freq = 10MHz/265/39061 = 1 Hz == 1 second 
   // Set up CPU to respond to interrupts from Timer2 and Timer 3
   INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);
   INTClearFlag(INT_T2);
   INTEnable(INT_T2, INT_ENABLED);
   INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
   INTEnableInterrupts();
   // Initialize message to the OLED display
   // LED Disiplay will state StopWatch on it and be in the format of 00:00 going by minutes and seconds. 
   OledClearBuffer();
   OledSetCursor(0, 0);
   OledPutString("HANGMAN");
   OledSetCursor(0, 2);
   OledPutString("00:00");
   OledUpdate();
   const char Instructions[] =
   {
       "Welcome to Hangman: The Game!\n"\
       "This is a text based version of the game run through a Serial Terminal Emulator\n"\
       "Created by: Bowei Zhao ECE2534 Fall 2015\r\n\n"\
       "INSTRUCTIONS: \n"\
       "** Ready Player One **  \n"\
       "1. Press Button1 on PIC32 to Start Game.\r\n"\
       "2. A secret word will be chosen at random from a database.\r\n"\
       "   Guess this word by viewing the '*' to determine size of word\r\n"\
       "3. Type a character on your keyboard to 'guess' a letter of the secret word\r\n"\
       "4. You will have 10 tries to guess the word not including correct tries.\r\n"\
       "   Once you out of tries, the game will end.\r\n\n"\
       "NOTE: \n"\
       " - The 'secret word' is only represented by alphabetical characters. \r\n"\
       "   You may give it non-alphabetical characters, but you will be out of a turn\r\n"\
       " - Uppercase alphabetical characters are allowed but will be converted to lowercase. \r\n"\
       " - To restart the game, type '#' during the selection entry or press Button 1 again.\r\n"\
       " - Your time elapsed playing the game and the 'Game State' are displayed on the PIC32's OLED\r\n"
   };
   puts(Instructions);
}

// this function sends a string to UART 1 and appends newline character to it
// takes an input of str - pointer to the string to be transmitted
// Returns length of the string, not including newline char. Else return -1 for error. 
int puts(const char *strp)
{
   int sized;
   sized = strlen(strp);
   /*
   OledClearBuffer();
   sprintf(OLEDout, "size is: %d",size);
   OledSetCursor(0, 0); // set cursor is used to puts it on the second row/line
   OledPutString(OLEDout); //buf is loaded into the OLED
   OledUpdate();
   */
   UARTSendDataByte(UART1, '\n');
   UARTSendDataByte(UART1, '\r'); 

   while(sized)
   {
      while(!UARTTransmitterIsReady(UART1)); // if the transmitter is not ready loop. If it is ready, then send data. 
      
      UARTSendDataByte(UART1, *strp);
      strp++;
      sized--;
      
   }
   while(!UARTTransmissionHasCompleted(UART1)); // if transmission is complete then we can prepare the next while loop for waiting.

   return sized;

}

int getChar(void)
{
   UINT8  key_send;
   int neg1 = -1;

   if(UARTReceivedDataIsAvailable(UART1))
   {
      key_send = UARTGetDataByte(UART1); // reads from the UART
      //sprintf(output4, "***DEBUG getChar: PRE Convert received character: %d", key_send);
      //puts(output4);

      if(key_send>=65 && key_send<=90)
      {
          key_send = key_send +32;
      }
      //sprintf(output2, "***DEBUG getChar: Post Convert received character: %d", key_send);
      //puts(output2);

      if(key_send == '#')
      {
        flag = 1;
      } 
      return (UINT32)key_send;
   }
   else
   {
      if (Button1History == 0xFF)
      {
        flag = 1;
        return 0;
      }
      else
      {
        return -1;

      }

   }



}



char* getWord()
{
   const char *words[] =
   {
      "fig","cherry","peach","squash","grapefruit",
      "kiwi","brussels","tomato","pineapple","blueberry"
   };
   srand(time1s); // using the different values of time to 'seed' the values of srand so rand is different every time
   int outnow = rand() % 10;

   //sprintf(output4, "Random value is: %d", outnow);
   //puts(output4);
   return words[outnow];

}



char* asterisk(const char *str)
{
   char storage[] = "*************";
   int size;
   size = strlen(str);
   char stardot[size+1];
   int i = 0;
  // sprintf(output2, "Size of strlen of input is %d", size);
   //puts(output2);
   for(i=0; i<size; i++)
   {
      stardot[i] = storage[i];
   }
   //sprintf(output3, "the string stardot is: %s",stardot);
   //puts(output3);
   return stardot;

}