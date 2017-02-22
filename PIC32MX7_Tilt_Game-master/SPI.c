#include <plib.h>

#include "SPI.h"

#define SS_PORT IOPORT_D
#define SS_PIN1 BIT_14


void initSPI(){
  DDPCONbits.JTAGEN=0;
	AD1PCFG=0xFFFF;

	PORTSetPinsDigitalOut(SS_PORT, SS_PIN1);

	SpiChnOpen(SPI_CHN,SPI_OPEN_MSTEN|SPI_OPEN_MODE8|SPI_OPEN_ENHBUF|SPI_OPEN_CKP_HIGH,4);
	
}


void SPIio(SpiChannel chn, unsigned int sendLen, UINT8* sendBuf, unsigned int recvLen, UINT8* recvBuf, unsigned int recvBytesToDrop){//

	while(SpiChnIsBusy(chn)){}

	while(!SpiChnRxBuffEmpty(chn)){
		SpiChnReadC(chn);
	}

	PORTClearBits(SS_PORT, SS_PIN1);

	int i = 0, j = 0;
	while(i < sendLen || j < recvLen){
		if(!SpiChnTxBuffFull(chn)){

			if(i < sendLen){
				SpiChnWriteC(chn,*sendBuf); // stuck
				sendBuf++;
				i++;
			}else{
				SpiChnWriteC(chn,0);
			}


		}

		if(!SpiChnRxBuffEmpty(chn)){

			if(recvBytesToDrop == 0){
				*recvBuf=SpiChnReadC(chn);
				recvBuf++;
				j++;
			}else{
				recvBytesToDrop--;
				SpiChnReadC(chn);
			}
		}
	}
	PORTSetBits(SS_PORT, SS_PIN1);
}
