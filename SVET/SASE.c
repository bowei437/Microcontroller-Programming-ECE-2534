////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Svetlana Marhefka - Homework #4
// File name:       main.c
// Description:     This file contains the main program needed needed to create
//                  a stopwatch which will be displayed in your OLED chip.
// Resources:       main.c uses Timer2 to generate interrupts at intervals of 1 ms.
//                  delay.c uses Timer1 to provide delays with increments of 1 ms.
//                  PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      CDP (and modified by ALA)
// Last modified:   9/1/2014
// IMPORTANT NOTE:	When I was originally writing the program for this homework I
//					had gone to the CEL Lab for help twice. When I recieved help
//					help from the two TA's they both told me to use
//					INTGetFlag(INT_T2) and INTClearFlag(INT_T2) in my program.
//					However, I was just told today that we were not supposed to do
//					this.  I have tried (without success) to change my program but
//					because of work I need to hand this in now (8:00pm). It appears
//					that the TA's who were helping me did not know this either.
//					This is a note to let the TA grading this know that I was unaware
//					until recently that we were not allowed to use GET and Clear
//					flags and therefore have programmed my code implementing
//					those funtions.
//					Thank you,
//					Svetlana Marhefka
///////////////////////////////////////////////////////////////////////////////////

// Call other resource files that are needed for this homework
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include <stdlib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

// This is added for the UART
#define PIC32_CLOCK 10000000
#define DESIGNATED_RATE 9600
#define UART_MODULE UART1

// Digilent board configuration
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


/******************************* Global variables ***************************/
unsigned int interruptSeconds = 0;
// initialize time which will display in the count
unsigned int seconds = 0;
// initialize seconds which will display in the count
unsigned int minutes = 0;
// number of tries the user is given in the game
const int tries = 10;
// initialize the button delay to 0
//unsigned int Button1Delay = 0;
// Last eight samples of BTN1
unsigned char Button1History = 0x00;

/************************Functions Used in this Project*****************/
int UserInput();
void DisplayMessage (UART_MODULE UART1, const char *Dmessage);
void UARTSendByte(UART_MODULE UART1, char byte_sent);
unsigned char UARTReceiveByte(UART_MODULE UART1);

/***************************** Functions **********************************/
// Sets up the interrupt function
// This is the interrupt function that was created during homework 4
#pragma interrupt InterruptHandler_2534 ipl1 vector 0
void InterruptHandler_2534(void) 
{
    if (INTGetFlag(INT_T2))             // Verify source of interrupt
    {
    	interruptSeconds++;
        Button1History <<= 1;           // Discard oldest sample to
                                        // make room for new
        if (PORTG & 0x40)               // Record the latest BTN1 sample
        Button1History |= 0x01;

        INTClearFlag(INT_T2);           // Acknowledge interrupt
    }
    return;
}
/*************** Main Menu for Game Initialization *****************/
//const char GameMenu[] =
//{
//    "Welcome to PIC32 UART Demo!\r\n"\
//    "Here are the main menu choices\r\n"\
//    "1. View Actual BAUD rate\r\n"\
//    "2. Get a funny sentence\r\n"\
//    "\r\n\r\nPlease Choose a number\r\n"
//};
/**************************************************************************/
int main()
{
    /************************** Initializations ****************************/
    // Temp string for OLED display
    // Initialize GPIO for BTN1 and LED1
    // For BTN1: configure PortG bit for input
    // char buf[17];
    TRISGSET = 0x40;
    enum States { INITIALIZE, SGAME, COUNT, STOP, CLEAR };
    enum States CURR_STATE = INITIALIZE;
    /******************************* Start OLED *****************************/
    DelayInit();
    OledInit();
    /**************************************************************************/
    /******************************* Set up UART ****************************/
    UARTConfigure( UART1, UART_ENABLE_PINS_TX_RX_ONLY );
    //UARTSetFifoMode(UART1, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(UART1, PIC32_CLOCK, 9600);
    UARTEnable( UART1, UART_ENABLE|UART_PERIPHERAL|UART_RX|UART_TX );
    /*************************************************************************/
    /********************** Set Up Timers *********************************/
    int BUTTON1 = 0;                    // Initialize the count of how many times
                                        // Button1 is pressed
        // Set up Timer2 to roll over every ms
    OpenTimer2(T2_ON        |
            T2_IDLE_CON     |
            T2_SOURCE_INT   |
            T2_PS_1_16      |
            T2_GATE_OFF,
            624); // freq = 10MHz/16/625 = 1 kHz
    // Set up Timer3 to roll over every second
    OpenTimer3(T3_ON        |
            T3_IDLE_CON     |
            T3_SOURCE_INT   |
            T3_PS_1_256     |
            T3_GATE_OFF,
            39061); // 39061 freq = 10MHz/256/39061 = 1 kHz

    // Set up CPU to respond to interrupts from Timer2
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_1);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
    INTConfigureSystem(INT_SYSTEM_CONFIG_SINGLE_VECTOR);
    INTEnableInterrupts();

    // Send a welcome message to the OLED display
    char OledOutput[17];
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("ECE 2534 Hangman");       // Display Stopwatch - HW4 on OLED
    OledSetCursor(0, 2);
    OledPutString("00:00");      // Displays 00:00 right underneath
    OledUpdate();
    /******************************************************************************/

    //SendDataBuffer(GameMenu, sizeof(GameMenu));
    while(1)
    {
        if (INTGetFlag(INT_T3))
        {
            seconds++;
            INTClearFlag(INT_T3);
        }

        minutes = seconds%60;
        seconds++;

    	sprintf(OledOutput,"%02d:%02d", minutes, seconds);
    	OledSetCursor(0, 2);
    	OledPutString(OledOutput);
    	OledUpdate();

    	DisplayMessage("\n\r Welcome to ECE 2534 Hangman!\n\r");
        
//        switch(CURR_STATE)
//        {
//        case INITIALIZE:
//            char init_out[50];
//            sprintf(init_out, "\r\nActual Baud Rate: %ld\r\n\r\n", UARTGetDataRate(UART1, GetPeripheralClock()));
//            SendDataBuffer(init_out, strlen(init_out));
//            break;
//
//        case SGAME:
//            char sgame_out[50];
//            sprintf(sgame_out, "\r\nI said HEEEYYYYYAYAYA yeeeaaahhh\r\n");
//            SendDataBuffer(sgame_out, strlen(sgame_out));
//            break;
//
//        default:
//            SendDataBuffer(GameMenu, sizeof(GameMenu));
//
//        }

    }
    return 0;
}

/**********************************************************************************/
/*************************** Random Word ******************************************/
//char* GetRandomWord()
//{
//    const char *guess[] =
//    {
//       "engineer","electrical","grapefruit",
//       "timer","projects","sleep",
//       "virginia","hokies","junior",
//       "understand"
//    };
//    return guess[ ( rand() % 10 ) ];
//}
/*********************************** GET CHAR *************************************/
int UserInput()
{
    char Input;

    while(!UARTReceivedDataIsAvailable(UART1))
    {
    	// Do nothing
    }
    Input = UARTGetDataByte(UART1); // reads from the UART
    while(!UARTTransmitterIsReady(UART1))
    {
    	// Do nothing
    }
    UARTSendDataByte(UART1, Input);
    return (int)Input;
}

// This gets the stuff that is typed into the terminal and then sends it
// back to the code
void DisplayMessage (UART_MODULE UART1, const char* Dmessage)
{
	int messize;
	for (messize = 0; Dmessage[messize] != NULL; messize++)
	{
		while(!UARTTransmitterIsReady(UART1))
		{
			//Left empty on Pupose
		}
		UARTSendDataByte(UART1, Dmessage[messize]);
	}
}

//This gets stuf you type into the terminal on PC and sends it to code/board
void UARTSendByte(UART_MODULE UART1, char byte_sent)
{
    while(!UARTTransmitterIsReady(UART1))
    {
        // DO NOTHING
    }
    UARTSendDataByte(UART1, byte_sent);
}

unsigned char UARTReceiveByte(UART_MODULE UART1)
{
    unsigned char byte_received;
    while(!UARTReceivedDataIsAvailable(UART1))
    {
        // DO NOTHING
    }
    byte_received = UARTGetDataByte(UART1);
    return byte_received;
}