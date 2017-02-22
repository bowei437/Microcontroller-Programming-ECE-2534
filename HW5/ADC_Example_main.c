////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        ADC Program Example
//
// File name:       ADC_Example_main - A simple demonstration of the ADC
// Description:     ADC looks at one channel, prints mean & "variance" to OLED
// Resources:       timer2 is configured to interrupt every 1.0ms, and
//                  ADC is configured to automatically restart and interrupt
//                  after each conversion (about 160us).
// How to use:      AN2 is on header JA, pin 1. Need to connect this to the
//                  analog line you are measuring (e.g., the U/D output on the
//                  2-axis parallax joystick.) Be sure and give power and ground
//                  to the side you are trying to measure (e.g., the GND and VSS
//                  pins on the JA header connected to GND and U/D+ on the
//                  joystick). Once running, the program should report a ADC
//                  average value between 0 and 1023, and a "variance" that is
//                  the sum of the absolute values of the difference of the
//                  readings and the mean value.
//                  A logic analyzer can be used to see
//                  exactly what is happening in the ISRs and the OLED
//                  update (header JB, pins 1, 2, and 3).
// Written by:      PEP
// Last modified:   17 October 2015

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

// Intrumentation for the logic analyzer (or oscilliscope)
#define MASK_DBG1  0x1;
#define MASK_DBG2  0x2;
#define MASK_DBG3  0x4;
#define DBG_ON(a)  LATESET = a
#define DBG_OFF(a) LATECLR = a
#define DBG_INIT() TRISECLR = 0x7

// Definitions for the ADC averaging. How many samples (should be a power
// of 2, and the log2 of this number to be able to shift right instead of
// divide to get the average.
#define NUM_ADC_SAMPLES 32
#define LOG2_NUM_ADC_SAMPLES 5

// Global variable to count number of times in timer2 ISR
unsigned int timer2_ms_value = 0;

// Computed mean and variance from ADC values
int ADC_mean, ADC_variance;

// The interrupt handler for the ADC
// IPL7 highest interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
// Note that, in general, one should not be doing math
// in the ISR. I am doing it in this case so that you can
// see the time that it takes on the logic analyzer.
void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) {
    static unsigned int current_reading = 0;
    static unsigned int ADC_Readings[NUM_ADC_SAMPLES];
    unsigned int i;
    unsigned int total;

    DBG_ON(MASK_DBG2);
    ADC_Readings[current_reading] = ReadADC10(0);
    current_reading++;
    if (current_reading == NUM_ADC_SAMPLES) {
        current_reading = 0;
        total = 0;
        for (i = 0; i < NUM_ADC_SAMPLES; i++) {
            total += ADC_Readings[i];
        }
        ADC_mean = total >> LOG2_NUM_ADC_SAMPLES; // divide by num of samples
        total = 0;
        for (i = 0; i < NUM_ADC_SAMPLES; i++) {
            total += abs(ADC_mean - ADC_Readings[i]);
        }
        ADC_variance = total; // BTW, this isn't actually the variance..
    }
    DBG_OFF(MASK_DBG2);
    INTClearFlag(INT_AD1);
}

// The interrupt handler for timer2
// IPL4 medium interrupt priority
// SOFT|AUTO|SRS refers to the shadow register use
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    DBG_ON(MASK_DBG1);
    timer2_ms_value++; // Increment the millisecond counter.
    DBG_OFF(MASK_DBG1);
    INTClearFlag(INT_T2); // Acknowledge the interrupt source by clearing its flag.
}

//***********************************************
//* IMPORTANT: THE ADC CONFIGURATION SETTINGS!! *
//***********************************************

// ADC MUX Configuration
// Only using MUXA, AN2 as positive input, VREFL as negative input
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_NEG_SAMPLEA_NVREF

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
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | \
                  ADC_SAMPLES_PER_INT_1 | \
                  ADC_BUF_16 | ADC_ALT_INPUT_OFF

// ADC Config3 settings
// Autosample time in TAD = 8
// Prescaler for TAD:  the 20 here corresponds to a
// ADCS value of 0x27 or 39 decimal => (39 + 1) * 2 * TPB = 8.0us = TAD
// NB: Time for an AD conversion is thus, 8 TAD for aquisition +
//     12 TAD for conversion = (8+12)*TAD = 20*8.0us = 160us.
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy

// ADC Port Configuration (PCFG)
// Not scanning, so nothing need be set here..
// NB: AN2 was selected via the MUX setting in AD_MUX_CONFIG which
// sets the AD1CHS register (true, but not that obvious...)
#define AD_CONFIGPORT ENABLE_ALL_DIG

// ADC Input scan select (CSSL) -- skip scanning as not in scan mode
#define AD_CONFIGSCAN SKIP_SCAN_ALL

// Initialize the ADC using my definitions
// Set up ADC interrupts
void initADC(void) {

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
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 624);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
}

int main() {
    char oledstring[17];
    unsigned int timer2_ms_value_save;
    unsigned int last_oled_update = 0;
    unsigned int ms_since_last_oled_update;

    // Initialize GPIO for LEDs
    TRISGCLR = 0xf000; // For LEDs: configure PortG pins for output
    ODCGCLR = 0xf000; // For LEDs: configure as normal output (not open drain)
    LATGCLR = 0xf000; // Initialize LEDs to 0000

    // Initialize Timer1 and OLED for display
    DelayInit();
    OledInit();

    // Initialize GPIO for debugging
    DBG_INIT();

    // Initial Timer2 and ADC
    initTimer2();
    initADC();

    // Configure the system for vectored interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    // Enable the interrupt controller
    INTEnableInterrupts();

    // Send a welcome message to the OLED display
    OledClearBuffer();
    OledSetCursor(0, 0);
    OledPutString("ADC Example");
    OledUpdate();

    while (1) {
        ms_since_last_oled_update = timer2_ms_value - last_oled_update;
        if (ms_since_last_oled_update >= 100) {
            DBG_ON(MASK_DBG3);
            LATGSET = 1 << 12; // Turn LED1 on
            timer2_ms_value_save = timer2_ms_value;
            last_oled_update = timer2_ms_value;
            sprintf(oledstring, "T2 Val: %6d", timer2_ms_value_save / 100);
            OledSetCursor(0, 1);
            OledPutString(oledstring);
            sprintf(oledstring, "ADC Avg: %4d", ADC_mean);
            OledSetCursor(0, 2);
            OledPutString(oledstring);
            sprintf(oledstring, "ADC Var: %4d", ADC_variance);
            OledSetCursor(0, 3);
            OledPutString(oledstring);
            OledUpdate();
            LATGCLR = 1 << 12; // Turn LED1 off
            DBG_OFF(MASK_DBG3);
        }
    }
    return (EXIT_SUCCESS);
}

