///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        HW5 PArt 2
// STUDENT:         Bowei Zhao
// Description:     File Program to control joystick in finding values
//                  And changes value on screen
//     
//
// File name:       ADC_main.c
// the main file change log is set here
// 
// Resources:       main.c uses Timer2, and Timer 3 to generate interrupts at intervals of 1 ms and 1s.
//                  delay.c uses Timer1 to provide delays with increments of 1 ms.
//                  PmodOLED.c uses SPI1 for communication with the OLED.
// Written by:      Bowei Zhao 
// Last modified:   10/27/2015
#define _PLIB_DISABLE_LEGACY
#include <plib.h>
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
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


// Global variable to count number of times in timer2 ISR
unsigned int timer2_ms_value = 0;
// global variable to keep track of LR and UD
 int cur_LR, cur_UD;
// The interrupt handler for the ADC
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
// Note that, in general, one should not be doing math
// in the ISR. I am doing it in this case so that you can
// see the time that it takes on the logic analyzer.
void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) 
{

    if (ReadActiveBufferADC10() == 0)
    {
        cur_LR = ReadADC10(0);
        cur_UD = ReadADC10(1);
    }

    if (ReadActiveBufferADC10() == 1)
    {
        cur_LR = ReadADC10(8);
        cur_UD = ReadADC10(9);
    }
    INTClearFlag(INT_AD1);
    //          note use this                  --------bowei -------use READACTIVE BUFFER COMMAND to switch from 0-8 and 9-16
}
// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) 
{
    timer2_ms_value++; // Increment the millisecond counter.
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}
//***********************************************
//* IMPORTANT: THE ADC CONFIGURATION SETTINGS!! *
//***********************************************
// ADC MUX Configuration
// Only using MUXA, AN2 as positive input, VREFL as negative input
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEB_AN3 | ADC_CH0_NEG_SAMPLEB_NVREF
// configured bit AN2 as analog input. So can configure as analog in or digital high low. Also is first pin
// connected to MUX A, connecting negative reference to Mux A. 
// Vref is usually 0 V, Vref+ is 3.3V = 1023 for digital output
// ADC Config1 settings
// Data stored as 16 bit unsigned int
// Internal clock used to start conversion
// ADC auto sampling (sampling begins immediately following conversion)
#define AD_CONFIG1 ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON
// ADC Config2 settings
// Using internal (VDD and VSS) as reference voltages
// Do not scan inputs
// One sample per interrupt
// Buffer mode is one 16-word buffer
// Alternate sample mode off (use just MUXA)
// BOWEI CHANGES HERE ---------------------------------------------Change so it takes from both instead of just 1
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | \
                  ADC_SAMPLES_PER_INT_2 | \
                  ADC_BUF_8 | ADC_ALT_INPUT_ON
// adc alt input tells if you want to alternate between MUXA and MUXB
// ADC Config3 settings
// Autosample time in TAD = 8
// Prescaler for TAD:  the 20 here corresponds to a
// ADCS value of 0x27 or 39 decimal => (39 + 1) * 2 * TPB = 8.0us = TAD
// NB: Time for an AD conversion is thus, 8 TAD for aquisition +
//     12 TAD for conversion = (8+12)*TAD = 20*8.0us = 160us.
                  // Bowei Changes: Decides sample time. How long it 'sees' stuff
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy
// ADC Port Configuration (PCFG)
// Not scanning, so nothing need be set here..
// NB: AN2 was selected via the MUX setting in AD_MUX_CONFIG which
// sets the AD1CHS register (true, but not that obvious...)

//#define AD_CONFIGPORT ENABLE_ALL_ANA
#define AD_CONFIGPORT ENABLE_AN2_ANA | ENABLE_AN3_ANA

// ADC Input scan select (CSSL) -- skip scanning as not in scan mode
#define AD_CONFIGSCAN SKIP_SCAN_ALL
// Initialize the ADC using my definitions
// Set up ADC interrupts
void initADC(void) 
{
    // Configure and enable the ADC HW
    SetChanADC10(AD_MUX_CONFIG);
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN);
    EnableADC10();
    // Set up, clear, and enable ADC interrupts
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
}

// Initialize timer2 and set up the interrupts
void initTimer2() {
    // Configure Timer 2 to request a real-time interrupt once per millisecond.
    // The period of Timer 2 is (16 * 625)/(10 MHz) = 1 s.
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_256 | T2_GATE_OFF, 39061);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}

int main() {
    char oledstring[17];

    // Initialize GPIO for LEDs
    TRISGCLR = 0xf000; // For LEDs: configure PortG pins for output
    ODCGCLR = 0xf000; // For LEDs: configure as normal output (not open drain)
    LATGCLR = 0xf000; // Initialize LEDs to 0000

    TRISBCLR = 0x0040; // initialize U/D pin B6
    ODCBCLR = 0x0040;
    LATBSET = 0x0040;

    LATGSET = 1 << 12; // Turn LED1 on
    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();
    // Initial Timer2 and ADC
    initTimer2();
    initADC();
    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    // Enable the interrupt controller
    INTEnableInterrupts();
    OledClearBuffer();
    OledUpdate();

    while (1) 
    {
        sprintf(oledstring, "L/R=%4d %4d",cur_LR,timer2_ms_value);
        OledSetCursor(0, 0);
        OledPutString(oledstring);
        sprintf(oledstring, "U/D=%4d", cur_UD);
        OledSetCursor(0, 1);
        OledPutString(oledstring);
        if (cur_LR >= 600)
        {
            OledSetCursor(9, 2);
            sprintf(oledstring, "RIGHT");
            OledPutString(oledstring);
        }
        else
        {
            OledSetCursor(9, 2);
            sprintf(oledstring, "      ");
            OledPutString(oledstring);
        }

        if (cur_LR <= 460)
        {
            OledSetCursor(0, 2);
            sprintf(oledstring, "LEFT");
            OledPutString(oledstring);
        }
        else
        {
            OledSetCursor(0, 2);
            sprintf(oledstring, "      ");
            OledPutString(oledstring);
        }

        if (cur_UD >= 600)
        {
            OledSetCursor(0, 3);
            sprintf(oledstring, "UP");
            OledPutString(oledstring);
        }
        else
        {
            OledSetCursor(0, 3);
            sprintf(oledstring, "      ");
            OledPutString(oledstring);
        }

        if (cur_UD <= 460)
        {
            OledSetCursor(9, 3);
            sprintf(oledstring, "DOWN");
            OledPutString(oledstring);
        }
        else
        {
            OledSetCursor(9, 3);
            sprintf(oledstring, "      ");
            OledPutString(oledstring);
        }
        OledUpdate();
        INTClearFlag(INT_AD1);

        
    }
    return (EXIT_SUCCESS);
}

