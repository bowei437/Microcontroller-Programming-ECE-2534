

#include <plib.h>
#include <string.h>
#include "HardwareProfile.h"
#include "i2c.h"

BOOL StartTransfer( BOOL restart )
{
    I2C_STATUS  status;

    // Send the Start (or Restart) signal
    if(restart)
    {
        I2CRepeatStart(I2C_MODULE);
    }
    else
    {
        // Wait for the bus to be idle, then start the transfer
        while( !I2CBusIsIdle(I2C_MODULE) );

        if(I2CStart(I2C_MODULE) != I2C_SUCCESS)
        {
            DBPRINTF("Error: Bus collision during transfer Start\n");
            return FALSE;
        }
    }

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(I2C_MODULE);

    } while ( !(status & I2C_START) );

    return TRUE;
}


BOOL TransmitOneByte( UINT8 data )
{
    // Wait for the transmitter to be ready
    while(!I2CTransmitterIsReady(I2C_MODULE));

    // Transmit the byte
    if(I2CSendByte(I2C_MODULE, data) == I2C_MASTER_BUS_COLLISION)
    {
        DBPRINTF("Error: I2C Master Bus Collision\n");
        return FALSE;
    }

    // Wait for the transmission to finish
    while(!I2CTransmissionHasCompleted(I2C_MODULE));

    return TRUE;
}

BOOL RecieveOneByte( UINT8* data, BOOL ack )
{


  if(I2CReceiverEnable(I2C_MODULE, TRUE) == I2C_SUCCESS){

		while(!I2CReceivedDataIsAvailable(I2C_MODULE)){}

        	I2CAcknowledgeByte(I2C_MODULE, ack);
        	*data = I2CGetByte(I2C_MODULE);

		while(!I2CAcknowledgeHasCompleted(I2C_MODULE)){}


		return TRUE;
	}
	
	return FALSE;
//    // Wait for the transmitter to be ready
//    while(!I2CTransmitterIsReady(I2C_MODULE));
//
//    // Transmit the byte
//    if(I2CSendByte(I2C_MODULE, data) == I2C_MASTER_BUS_COLLISION)
//    {
//        DBPRINTF("Error: I2C Master Bus Collision\n");
//        return FALSE;
//    }
//
//    // Wait for the transmission to finish
//    while(!I2CTransmissionHasCompleted(I2C_MODULE));
//
//    return TRUE;
}

void StopTransfer( void )
{
    I2C_STATUS  status;

    // Send the Stop signal
    I2CStop(I2C_MODULE);

    // Wait for the signal to complete
    do
    {
        status = I2CGetStatus(I2C_MODULE);

    } while ( !(status & I2C_STOP) );
}

void initI2C(){

    // Set the I2C baudrate.
    I2CSetFrequency(I2C_MODULE, GetPeripheralClock(), I2C_CLOCK_FREQ);

	// Enable the I2C bus.
    I2CEnable(I2C_MODULE, TRUE);

}

BOOL I2CSend(UINT8 address,int length, const UINT8* buffer){
	BOOL success = TRUE;
	if(StartTransfer(FALSE)){
		I2C_7_BIT_ADDRESS SlaveAddress;
    	I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, address, I2C_WRITE);
		if(TransmitOneByte(SlaveAddress.byte)){
			const char* cp;
			int i = 0;
			while(i < length){
				if(!TransmitOneByte(*buffer)){
					success = FALSE;
					break;
				}
				buffer++;
				i++;
			}
			if(success){
				StopTransfer();
			}
		}
		else{
			success = FALSE;
		}
	}
	else {
		success = FALSE;
	}
	return success;
}

BOOL I2CRecv(UINT8 address, int length, UINT8* buffer){
	BOOL success = TRUE;
	if(StartTransfer(FALSE)){
		I2C_7_BIT_ADDRESS SlaveAddress;
    	I2C_FORMAT_7_BIT_ADDRESS(SlaveAddress, address, I2C_READ);
	
		if(TransmitOneByte(SlaveAddress.byte)){
			const char* cp;
			int i = 0;
			//BOOL sender=TRUE;
			while(i < length){
				if(i < length - 1){
					RecieveOneByte(buffer++,TRUE);
					
				}else{
					RecieveOneByte(buffer++,FALSE);
				}
				i++;

//				if(i == length-1){
//					sender = FALSE;
//				}
//
//				if(!RecieveOneByte(buffer++,sender)){
//					success = FALSE;
//					break;
//				}
//				i++;

			}
			if(success){
				StopTransfer();
			}
		}
		else{
			success = FALSE;
		}
	}
	else {
		success = FALSE;
	}
	return success;
}

