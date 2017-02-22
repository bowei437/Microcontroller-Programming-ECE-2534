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

char output[50];        // Following are all Temp string for OLED display
char output2[50];
char output3[50];
char output4[50];
char OLEDout[50];

int trials = 10; // sets trials to 10 so game will restart after
enum case_var{READ, LoopGame};// state machine uses enums
enum case_var cur_var = READ;// set main enum to read
unsigned int time1s =0;
unsigned int timekeep;
int saved = 0; // keeps track of time value to iterate seconds
int flag = 0; // default value for not breaking loops
int loop_stop = 0;
char outstr[30]; // what prints out for the string
char cmpstr[30]; // used to compare string values
int gamewin = 0; // if value is high enough, the game wins. 



////////////////////////////////////// FUNCTIONS DECLARATION ////////////////////////////////////////////////////////////////////////
void OLEDTIMER();
void UART_START(UART_MODULE myUART);
void MAIN_START();
int puts(const char *strp);
char* getWord();
char* asterisk(const char *str);
int getChar(void);
void GAME_LOOP(const char *inw, char outstr[], UINT8 bufet);

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
            if (( (Button1History == 0xFF && (PORTG & 0x40)) || flag == 1 )) // if the button is pressed or flag is set to 1
            {
              // resetting all values here
               trials = 10;
               loop_stop = 0;
               flag = 0;
               gamewin = 0;
               OledSetCursor(0, 0);
               OledPutString("TIME:"); // outputting time
               OledUpdate();
               const char welcome_msg[] =
               {
                  "ECE 2534 Hangman! \r\n"\
                  "Let's Play! \r\n"
               };

               puts(welcome_msg);
               cur_var = LoopGame;
               break;
            }
            else
            {
               cur_var = READ;
               break;
            }
         case LoopGame:
                OledSetCursor(0, 0);
                OledUpdate();
                char *word = getWord(); // get word gets a random word
                char *dot = asterisk(word); // used to print out the '******'
                if (loop_stop==0) // this so it runs this loop only once per game as given variables differ
                {
                  sprintf(output4, "Secret Word: %s \r\nRemaining trials: %d \r\n", dot, trials);
                  puts(output4);
                }
    
                UINT8 bufet; // unsigned integer of 8 bits for bufet to store input characters from terminal
   
               while(!trials == 0)
               {
                  if(loop_stop!=0) // run after the first loop
                  {
                    sprintf(output3, "Secret Word: %s \r\nRemaining trials: %d \r\n", outstr, trials);
                    puts(output3);

                  }

                  char loc_out[50];
                  sprintf(loc_out, "Enter your selection: "); // non blocking method of asking for a selection
                  puts(loc_out);
                  bufet = getChar();
                  while(bufet == 255) // if it keeps returning -1, then keep running until i type something.
                  {
                    bufet = getChar();
                    OLEDTIMER(); // poll led for time while this is happening
                    bufet = getChar();

                  }
                  //sprintf(output4, "***DEBUG LoopGame: Pressed = %c or %d \r\n", bufet, bufet);
                  //puts(output4);
                  if(flag == 1) // this is used with the getchar loop to break the loop if # or PB1 is pressed
                  {
                     cur_var = READ;
                     break;
                  }
                  else
                  {
                    flag = 0; // always reset the flag after each iteration
 
                      GAME_LOOP(word,outstr,bufet); // official string comparison loop here
                      if(gamewin==strlen(word)) // this is used to see if the values are the same, if they are, then it means the strings match
                      {
                        sprintf(output, "Secret word: %s\r\nCONGRATULATIONS - YOU WIN!\r\nTo start a new game, press PB1\r\n", word);
                        puts(output);
                        cur_var = READ;
                        break;
                      }
                      //sprintf(output3, "L1: word %s outstr %s cmpstr %s gamewin %d bufet %c", word, outstr,cmpstr,gamewin, bufet);
                      //puts(output3);
                      loop_stop++;

                  }
                }
                if(cur_var != READ) // this is here so that it won't run the same Game End output twice. 
                {
                   sprintf(output, "The secret word was: %s\r\nGAME OVER - THE COMPUTER WINS!\r\nTo start a new game, press PB1\r\n", word);
                   puts(output);

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
void GAME_LOOP(const char *inw, char outstr[], UINT8 bufet)
{
    // main game loop
    int check; // check is used to see if a wrong answer was given
    int i = 0; // used for iteration
    int sizegl;
    sizegl = strlen(inw); // finds size of the randomized word
    gamewin=0; // re-initializes gamewin to 0
    check = 0;

    // this loop only runs at the beginning of the game once. Used to reset all values and fill values. 
    // used for later game wining comparisons and to erase the characters
    if(loop_stop == 0)
    {
      
      for(i=0;i<30;i++)
      {
        outstr[i] = '\0';
        cmpstr[i] = '\0';
      }
      
      for(i=0;i<sizegl;i++)
      {
        outstr[i]= '*';
        cmpstr[i] = inw[i];
      }
    }
  
    // this is the loop used for most purposes/times finds size of the word and outputs the string
    for(i=0;i<sizegl;i++)
    {
      if(inw[i] == bufet)
      {
        outstr[i] = inw[i];
        check = 1;
      }// gamewin used to compare values of cmp and out. If gamewin values = string of word then word has no * so correct
      if(outstr[i] == cmpstr[i])
      {
        gamewin++;
      }
    }// used to see if a bad guess was put in.
    if(check == 0)
    {
      trials--;
    }
    //sprintf(output2,"DEBUG:outstr= %s | cmpstr= %s\r\n", outstr, cmpstr);
    //puts(output2);

}

// function to continuously monitor time when it is called in the while loop
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
   {  // saved time is to make sure timkeep reaches 39061 or else it may not work
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
// function to initialize UART
void UART_START(UART_MODULE myUART)
{
   UARTConfigure( myUART, UART_ENABLE_PINS_TX_RX_ONLY );
   UARTSetFifoMode(myUART, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
   UARTSetLineControl(myUART, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
   UARTSetDataRate(myUART, GetPGClock(), 9600); // defines baud to be 9600
   UARTEnable( myUART, UART_ENABLE|UART_PERIPHERAL|UART_RX|UART_TX );

}
// initializes all previous main functions 
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
   OledPutString(" ");
   OledSetCursor(0, 2);
   OledPutString("00:00"); // initializes timer display
   OledUpdate();
   // instructions for how to play the game
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
   // finds length of the string and then tags on \n and \r onto it
   UARTSendDataByte(UART1, '\n');
   UARTSendDataByte(UART1, '\r'); 

   while(sized)
   {
      while(!UARTTransmitterIsReady(UART1)); // if the transmitter is not ready loop. If it is ready, then send data. 
      
      UARTSendDataByte(UART1, *strp);
      strp++;
      sized--;
      // used to make sure everything compltely is sent through
   }
   while(!UARTTransmissionHasCompleted(UART1)); // if transmission is complete then we can prepare the next while loop for waiting.

   return sized; // returns a size but we don't really use it

}

// non-blocking method of char.
int getChar(void)
{
   UINT8  key_send;
   int neg1 = -1;

   if(UARTReceivedDataIsAvailable(UART1)) // if data is availble then run, otherwise break and re-iterate
   {
      key_send = UARTGetDataByte(UART1); // reads from the UART

      // used to convert uppercase ASCII letters to lowercase
      if(key_send>=65 && key_send<=90)
      {
          key_send = key_send +32; // difference between lower case and upper is 32 added to char. 
      }

      // checks to see if # is pressed. If so, it sends a flag which restarts the code
      if(key_send == '#')
      {
        flag = 1;
      } 
      // at the end, it will return the character of the key that we wanted it to send. 
      return (UINT32)key_send;
   }
   else // if nothing gets pressed when our first iteration goes through, then it will first see if PB1 is pressed and then return -1
   {
      if (Button1History == 0xFF)
      {
        flag = 1;
        return 0;
      }
      else
      {
        return -1; // used to iterate through this loop until a character is pressed

      }

   }

}

// randomized function to get various states of fruits and veggies
char* getWord()
{   // I used various five character words but my code will work for any size characters 
   const char *words[] =
   {
      "apple","peach","olive","grape",
      "prune", "onion", "pecan", "gourd",
      "melon", "mango"
      
   };
   srand(time1s); // using the different values of time to 'seed' the values of srand so rand is different every time
   int outnow = rand() % 10; // gets a random value from 1 to 10

   return words[outnow]; // returns fruit

}

// function to display ******* on the terminal
char* asterisk(const char *str)
{
   char storage[] = "*************";
   int size;
   size = strlen(str); // finds length of word/fruit sent in
   char stardot[size+1];
   int i = 0;
   for(i=0; i<size; i++) // iterates through and makes a * for every letter of fruit we gave it
   {
      stardot[i] = storage[i];
   }
   return stardot;// returns it to my while loop

}