#include <plib.h>
#include "Clock.h"
#include "Button.h"

extern int sprintf(char *, const char *, ...);


// A structure capturing time expressed in seconds, minutes and hours,
// and a flag indicating that the seconds field has been incremented.

typedef struct __timeStruct {
	unsigned int seconds;
	unsigned int minutes;
	unsigned int hours;
	BOOL         updateSecond;
} timeStruct;


// An instance of the time structure.

static volatile timeStruct currentTime;



static void __ISR(_TIMER_2_VECTOR, ipl7) _Timer2Handler(void)
{
	// Clear the interrupt flag.
	mT2ClearIntFlag();


	// Increment seconds, and possibly roll over to minutes and hours.
//    if(currentTime.seconds < 59){
//		currentTime.seconds = currentTime.seconds + 1;
//	}else if(currentTime.seconds == 59 && currentTime.minutes < 59){
//		currentTime.seconds = 0;
//		currentTime.minutes = currentTime.minutes + 1;
//	}else if(currentTime.seconds == 59 && currentTime.minutes == 59){
//		currentTime.seconds = 0;
//		currentTime.minutes = 0;
//		currentTime.hours = currentTime.hours + 1;	
//	}
//
//	// Set the updateSecond flag.
	currentTime.updateSecond = TRUE;
}



void initClock()
{
	// Reset the curent time fields.
	currentTime.seconds = 0;
	currentTime.minutes = 0;
	currentTime.hours = 0;

	// Setup timer 2 to interrupt once per second at interrupt priority 7.
	// Assuming a processor frequency of 80 MHz, peripheral bus frequency set
	// to 1/8 of the processor frequency, and a timer prescale of 256, we get 
	// 80,000,000 / (8 * 256) = 39062 ticks.
	OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_256, 3906);//39062
	ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_7);
}



BOOL secondTick()
{
	if (currentTime.updateSecond == TRUE) {
		currentTime.updateSecond = FALSE;
		return TRUE;
	}

	return FALSE;
}


void getCurrentClockString(char string[])
{
//	string = sprintf(string,"Timer: %.2d:%.2d:%.2d",currentTime.hours,currentTime.minutes,currentTime.seconds);
}
