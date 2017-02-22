#include <plib.h>
#include <string.h>
#include "HardwareProfile.h"
#include "LCD.h"
#include "i2c.h"



//BOOL StartTransfer( BOOL restart )
//{
//    I2C_STATUS  status;
//
//    // Send the Start (or Restart) signal
//    if(restart)
//    {
//        I2CRepeatStart(LCD_I2C_BUS);
//    }
//    else
//    {
//        // Wait for the bus to be idle, then start the transfer
//        while( !I2CBusIsIdle(LCD_I2C_BUS) );
//
//        if(I2CStart(LCD_I2C_BUS) != I2C_SUCCESS)
//        {
//            DBPRINTF("Error: Bus collision during transfer Start\n");
//            return FALSE;
//        }
//    }
//
//    // Wait for the signal to complete
//    do
//    {
//        status = I2CGetStatus(LCD_I2C_BUS);
//
//    } while ( !(status & I2C_START) );
//
//    return TRUE;
//}


//BOOL TransmitOneByte( UINT8 data )
//{
//    // Wait for the transmitter to be ready
//    while(!I2CTransmitterIsReady(LCD_I2C_BUS));
//
//    // Transmit the byte
//    if(I2CSendByte(LCD_I2C_BUS, data) == I2C_MASTER_BUS_COLLISION)
//    {
//        DBPRINTF("Error: I2C Master Bus Collision\n");
//        return FALSE;
//    }
//
//    // Wait for the transmission to finish
//    while(!I2CTransmissionHasCompleted(LCD_I2C_BUS));
//
//    return TRUE;
//}



//void StopTransfer( void )
//{
//    I2C_STATUS  status;
//
//    // Send the Stop signal
//    I2CStop(LCD_I2C_BUS);
//
//    // Wait for the signal to complete
//    do
//    {
//        status = I2CGetStatus(LCD_I2C_BUS);
//
//    } while ( !(status & I2C_STOP) );
//}



void initLCD()
{
	const char resetDisplay[]  = "*";   // Equivalent to a cycling the power.
	const char enableDisplay[] = "1e";  // Enables display with the backlight off.

//    // Set the I2C baudrate.
//    I2CSetFrequency(LCD_I2C_BUS, GetPeripheralClock(), I2C_CLOCK_FREQ);
//
//	// Enable the I2C bus.
//    I2CEnable(LCD_I2C_BUS, TRUE);

	// Reset and clear the display.
    lcdString(resetDisplay);
	
}


BOOL lcdString(const char string[])
{
	return I2CSend(LCD_ADDRESS,strlen(string),string);
	


//    BOOL success = TRUE;
//
//    // Start the transfer for writing data to the LCD.
//    if (StartTransfer(FALSE)) {
//    	I2C_7_BIT_ADDRESS SlaveAddress;
//    	I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, LCD_ADDRESS, I2C_WRITE);
//		if (TransmitOneByte(SlaveAddress.byte)) {
//			const char* cp;
//            // Call TransmitOneByte() for each non-null character in the string.
//            // Stop with success set to false if TransmitOneByte() returns failure.
//			
//			int i = 0;
//
//			while(string[i] != '\0'){
//				if(!TransmitOneByte(string[i])){
//					success = FALSE;
//					break;
//				}				
//				i++;
//			}
//
//			if (success) {
//				StopTransfer();
//			}
//		}
//		else {
//			success = FALSE;
//    	}
//	}
//	else {
//		success = FALSE;
//	}
//
//	return success;
}



BOOL lcdInstruction(const char string[])
{

	

	char instruction[32] = "\x1B[";  // Instruction escape preamble.

	return lcdString(strncat(instruction,string,28));

	
}
