
#include <plib.h>
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
void myDisplayOnOLED(char *str);
void openMyUart(UART_MODULE theUART);
void beginMyOled();
void write();

// UINT 32 is just an unsigned integer of 32 bits. 
// UINT 8 is just an unsigned integer of 8 bits. 

void SendDataBuffer(const char *buffer, UINT32 size);
UINT32 GetMenuChoice(void);
UINT32 GetDataBuffer(char *buffer, UINT32 max_size);

const char mainMenu[] =
{
    "Welcome to Bowei PIC32 UART Demo!\r\n"\
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
    write();

    // these just define unsigned integers of variable menu_choice etc. Think of it as an int menu_choice for ease. 
    UINT32  menu_choice;
    UINT8   buf[1024];
    
    // SendBuffer is pretty much how the code 'sends' stuff to the terminal emulator through serial. 
    // think of it as you need to FIRST load text or something into something like 'mainMenu'.
    // so we first 'load' the stuff into a char array and then send it to SendDataBuffer. 
   SendDataBuffer(mainMenu, sizeof(mainMenu));
    while(1)
    {
        // remember that 
        menu_choice = GetMenuChoice();

        switch(menu_choice)
        {
        case 1:
            OledSetCursor(0, 0);
            OledClearBuffer();
            OledPutString("Choice: 1");
            OledUpdate();
            sprintf(buf, "\r\nActual Baud Rate: %ld\r\n\r\n", UARTGetDataRate(UART1, GetPeripheralClock()));
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


/*
    Displays a message on the OLED
 */
void myDisplayOnOLED(char *str){
    if (!*str) return;
    OledSetCursor(0, 2);
    OledPutString(str);
}

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

UINT32 GetDataBuffer(char *buffer, UINT32 max_size)
{
    UINT32 num_char;

    num_char = 0;

    while(num_char < max_size)
    {
        UINT8 character;

        while(!UARTReceivedDataIsAvailable(UART1))
            ;

        character = UARTGetDataByte(UART1);

        if(character == '\r')
            break;

        *buffer = character;

        buffer++;
        num_char++;
    }

    return num_char;
}

//
UINT32 GetMenuChoice(void)
{
    UINT8  menu_item;

    while(!UARTReceivedDataIsAvailable(UART1))
        ;

    menu_item = UARTGetDataByte(UART1); // reads from the UART

    menu_item -= '0'; // makes sure it isn't 0

    return (UINT32)menu_item;
}


void write()
{
    char *str = "\n\r2534 is the best course in the curriculum\n\r";
    while(*str != '\0')
    {
        while(!UARTTransmissionHasCompleted(UART1));
        UARTSendDataByte(UART1, *str);
        str++;
    }

}

/*
    Opens a uart with the general settings
 */
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
   OledPutString("Bowei Zhao");
   OledUpdate();
}

