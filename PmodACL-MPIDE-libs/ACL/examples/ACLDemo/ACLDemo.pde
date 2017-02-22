/************************************************************************/
/*				                        		*/
/*	  ACLDemoProject 		                                */
/*						                  	*/
/*					                        	*/
/************************************************************************/
/*	Author: Cristian Fatu						*/
/*	Copyright 2011, Digilent Inc.					*/
/************************************************************************/
/*  File Description: 			             		        */
/*					                        	*/
/* This file implements a simple demo application that demonstrates     */
/* how to use the ACL library.				                */
/*									*/
/*	Functionality:							*/
/*									*/
/* In the setup() function, the application instantiates and initializes*/
/* the ACL library object. Then, it performs calibration.               */
/* In the loop() function, the application reads accelerations and      */
/* displays their values on the serial terminal.                        */
/*					                        	*/
/*	Required Hardware:						*/
/*	  1. Cerebot 32MX4cK    					*/
/*	  2. PmodACL - plugged into JB connector (SPI0 interface  )	*/
/************************************************************************/
/*  Revision History:			        			*/
/*					                        	*/
/*	11/20/2011 (CristianF): created		       			*/
/*    25/06/2012(CristianF): lite version of ACL                        */
/*						                     							*/
/************************************************************************/

/* -------------------------------------------------------------------- */
/*				Include File Definitions  	        */
/* -------------------------------------------------------------------- */
#include "ACL.h"
// Wire and DSPI libraries headers must be included in the sketch
#include <Wire.h>
#include <DSPI.h>
/* -------------------------------------------------------------------- */
/*				Global Variables		        */
/* -------------------------------------------------------------------- */
ACL myACL; // the library object
char strMes[150];

float dX, dY, dZ;

/* -------------------------------------------------------------------- */
/*	               Procedure Definitions	                        */
/* -------------------------------------------------------------------- */
/***	setup
**
**	Parameters:
**		none
**
**	Return Value:
**		none
**
**	Errors:
**		none
**
**	Description:
**		 Performs basic board initialization.		
/*** ---------------------------------------------------------- ***/
void setup()
{ 
  //Create a serial connection to display the data on the terminal.
  Serial.begin(9600);
  
  // initialize PmodACL on SPI
  
  myACL.begin(PAR_ACCESS_DSPI0); // corresponds to DSPI0

  //Put the device into standby mode to configure it.
  myACL.SetMeasure(false);

  
  // set data range to +/- 4g
  myACL.SetGRange(PAR_GRANGE_PM4G);

  // set Measure bit to true
  myACL.SetMeasure(true);

  // calibrate the accelerometer using Z axis as gravitational
  myACL.CalibrateOneAxisGravitational(PAR_AXIS_ZP);

}

/*** ---------------------------------------------------------- ***/
/***	loop
**
**	Parameters:
**		none
**
**	Return Value:
**		none
**
**	Errors:
**		none
**
**	Description:
**		Main program module.Enters the main program loop.
/*** ---------------------------------------------------------- ***/
void loop()
{  
  // read accelerometer values in g
  myACL.ReadAccelG(dX, dY, dZ);
  
  // format the display string
  sprintf(strMes, "X:%6.3f, Y:%6.3f, Z:%6.3f", dX, dY, dZ);  

  // send the string to display
  Serial.println(strMes);

  
  delay(100); 
}



