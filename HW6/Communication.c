/***************************************************************************//**
 *   @file   Communication.c
 *   @brief  Implementation of Communication Driver for PIC32MX320F128H 
             Processor.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "Communication.h"    /*!< Communication definitions */
#include <plib.h>
#include "ADXL345.h"

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/
const unsigned int SPI_SOURCE_CLOCK_DIVIDER = 20;
const SpiChannel CHANNEL_MASTER_SPI = SPI_CHANNEL3;
const SpiChannel CHANNEL_SLAVE_SPI = SPI_CHANNEL4;
const INT_SOURCE MASTER_SPI_INT = INT_SPI3;
const INT_SOURCE SLAVE_SPI_INT = INT_SPI4RX;
#define SLAVE_SPI_VECTOR _SPI_4_VECTOR

/***************************************************************************//**
 * @brief Initializes the SPI communication peripheral.
 *
 * @param lsbFirst  - Transfer format (0 or 1).
 *                    Example: 0x0 - MSB first.
 *                             0x1 - LSB first.
 * @param clockFreq - SPI clock frequency (Hz).
 *                    Example: 1000 - SPI clock frequency is 1 kHz.
 * @param clockPol  - SPI clock polarity (0 or 1).
 *                    Example: 0x0 - Idle state for clock is a low level; active
 *                                   state is a high level;
 *                             0x1 - Idle state for clock is a high level; active
 *                                   state is a low level.
 * @param clockEdg  - SPI clock edge (0 or 1).
 *                    Example: 0x0 - Serial output data changes on transition
 *                                   from idle clock state to active clock state;
 *                             0x1 - Serial output data changes on transition
 *                                   from active clock state to idle clock state.
 *
 * @return status   - Result of the initialization procedure.
 *                    Example:  0 - if initialization was successful;
 *                             -1 - if initialization was unsuccessful.
*******************************************************************************/

int SpiMasterInit(int channel)
{ // JE is Channel 3 and JF is Channel 4
    OledClearBuffer();

    if (channel == 3) // JE is selected as MASTER!
    {

        mPORTDSetPinsDigitalOut(BIT_14); // set ADX as Slave

        SpiChnOpen(SPI_CHANNEL3,
            // master mode, idle high, transmit from idle to active
            SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH |  
            // 8 bits, turn spi on
            SPI_OPEN_MODE8  | SPI_OPEN_ENHBUF ,
            // set bitrate to 10MHz/20 = .5 MHz
            8
            );


        return 0;

    }
    
    else if (channel == 4) // JF is selected as MASTER!
    {

        mPORTFSetPinsDigitalOut(BIT_12); // set ADX as Slave

        SpiChnOpen(SPI_CHANNEL4,
            // master mode, idle high, transmit from idle to active
            SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH |  
            // 8 bits, turn spi on
            SPI_OPEN_MODE8  | SPI_OPEN_ENHBUF ,
            // set bitrate to 10MHz/20 = .5 MHz
            8
            );

        return 0;

    }
    
    else if ((channel != 3) && (channel != 4) )
    {
        return -1; //incorrect input....and will never run 
    }
    else
    {
        return 1; // main will then print out error

    }
    
}

int SpiMasterIO(char bytes[], int numWriteBytes, int numReadBytes)
{
     int i = 0;

    if (final_comm == 3)
    {
        mPORTDClearBits(BIT_14); // set ADX as Slave

        for (i=0; i<numWriteBytes; i++)
        {
            SpiChnPutC(SPI_CHANNEL3, bytes[i]);
            SpiChnGetC(SPI_CHANNEL3);
            
        }
        for(i=numWriteBytes;i<(numReadBytes+numWriteBytes); i++)
        {
            SpiChnPutC(SPI_CHANNEL3, bytes[i]);
            bytes[i] = SpiChnGetC(SPI_CHANNEL3);
        }
        mPORTDSetBits(BIT_14); 
        return 0;
    }
    else if (final_comm == 4)
    {
        mPORTFClearBits(BIT_12);

        for (i=0; i<numWriteBytes; i++)
        {
            SpiChnPutC(SPI_CHANNEL4, bytes[i]);
            SpiChnGetC(SPI_CHANNEL4);
            
        }
        for(i=numWriteBytes;i<(numReadBytes+numWriteBytes); i++)
        {
            SpiChnPutC(SPI_CHANNEL4, bytes[i]);
            bytes[i] = SpiChnGetC(SPI_CHANNEL4);
        }

        mPORTFSetBits(BIT_12);
        return 0;

    }
    else
    {
        return -1;

    }
    

    

}

