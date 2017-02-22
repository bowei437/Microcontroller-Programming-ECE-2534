

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
#pragma config FNOSC        = PRIPLL    // Oscillator selection
#pragma config POSCMOD      = XT    // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2 // PLL input divider
#pragma config FPLLMUL      = MUL_20    // PLL multiplier
#pragma config FPLLODIV     = DIV_1 // PLL output divider
#pragma config FPBDIV       = DIV_8 // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF   // Secondary oscillator enable
// All this stuff does here is to get pulled from the code later to define stuff. You don't actually need it. 
// It's like a system info program and these defines are used to give system status updates. Not code required. 
// GetPGClock is kinda required though..so keep that one. 
#define GetSystemClock()            (80000000ul)
#define GetPeripheralClock()        (GetSystemClock()/(1 << OSCCONbits.PBDIV))
#define GetInstructionClock()       (GetSystemClock())
#define GetPGClock() 10000000

typedef char state;
// Define a lot of function declarations
void openMyUart(UART_MODULE theUART);
void beginMyOled();
// UINT 32 is just an unsigned integer of 32 bits. 
// UINT 8 is just an unsigned integer of 8 bits. 
void SendDataBuffer(const char *buffer, UINT32 size);
int getChar(void)
// This just STORES the lines below in char mainMenu. It doesn't do anything yet. Doesn't send to terminal. Just STORES!
// watch the syntax very carefully noting the \ and commas. 
const char mainMenu[] =
{
    "Welcome to PIC32 UART Demo!\r\n"\
    "Here are the main menu choices\r\n"\
    "1. View Actual BAUD rate\r\n"\
    "2. Get a funny sentence\r\n"\
    "\r\n\r\nPlease Choose a number\r\n"
};

int main() {
    // start OLED and UART. Look at functions below. All they do is initialize stuff and print more stuff out
    beginMyOled();
    openMyUart(UART1);
    // Write just is a function that will just print out something on start. 

    // these just define unsigned integers of variable menu_choice etc. Think of it as an int menu_choice for ease. 
    UINT32  menu_choice;
    UINT8   buf[1024];
    
   SendDataBuffer(mainMenu, sizeof(mainMenu));
    while(1)
    {
        // remember that 
        menu_choice = getChar();

        switch(menu_choice)
        {
        case 1:
            OledSetCursor(0, 0);
            OledClearBuffer();
            OledPutString("Choice: 1");
            OledUpdate();
            sprintf(buf, "\r\nActual Baud Rate: %ld\r\n\r\n", UARTGetDataRate(UART1, GetPeripheralClock()));
            SendDataBuffer(buf, strlen(buf));
            sprintf(buf, "\r\n%lc\r\n\r\n", menu_choice);
            SendDataBuffer(buf, strlen(buf));
            break;
            
        case 2:
            OledSetCursor(0, 0);
            OledClearBuffer();
            OledPutString("Choice: 2");
            OledUpdate();
            sprintf(buf, "\r\nI said HEEEYYYYYAYAYA yeeeaaahhh\r\n");
            SendDataBuffer(buf, strlen(buf));
            break;

        default:
            SendDataBuffer(mainMenu, sizeof(mainMenu));

        }

    }
    return (EXIT_SUCCESS);
}


// This 'sends' stuff from the code onto the terminal screen.
void SendDataBuffer(const char *buffer, UINT32 size)
{
    while(size)
    {
        while(!UARTTransmitterIsReady(UART1))
            ;

        UARTSendDataByte(UART1, *buffer);

        buffer++;
        size--;
    }

    while(!UARTTransmissionHasCompleted(UART1))
        ;
}
//This gets stuf you type into the terminal on PC and sends it to code/board
// non-blocking method of char.
int getChar(void)
{
   UINT8  key_send;
   int neg1 = -1;

   if(UARTReceivedDataIsAvailable(UART1)) // if data is availble then run, otherwise break and re-iterate
   {
      key_send = UARTGetDataByte(UART1); // reads from the UART

      // at the end, it will return the character of the key that we wanted it to send. 
      return (UINT32)key_send;
   }
   else // if nothing gets pressed when our first iteration goes through, then it will first see if PB1 is pressed and then return -1
   {

    return -1; // used to iterate through this loop until a character is pressed

   }

}
//  Opens a uart with the general settings
 
void openMyUart(UART_MODULE theUART){
    UARTConfigure( theUART, UART_ENABLE_PINS_TX_RX_ONLY );
    UARTSetFifoMode(theUART, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    UARTSetDataRate(theUART, GetPGClock(), 9600);
    UARTEnable( theUART, UART_ENABLE|UART_PERIPHERAL|UART_RX|UART_TX );

}
/*
    Initializes the OLED for debugging purposes
*/
void beginMyOled(){
   DelayInit();
   OledInit();
   OledSetCursor(0, 0);
   OledClearBuffer();
   OledPutString("YOLO");
   OledUpdate();
}

