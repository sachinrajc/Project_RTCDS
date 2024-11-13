/*-----------------------------------------------------------------------------
	File Name:		RTCDS_System.c
	Author:			Sachin Raj Charupadikkal
	Date:			17/10/2024
	Modified:		None
	© Fanshawe College, 2024

	Description:    Integrated Railway Track Crack Detection System (RTCDS)
                    with ultrasonic, tilt, and crack detection sensors, 
                    and motor control functionality.
-----------------------------------------------------------------------------*/

/* Preprocessor ---------------------------------------------------------------
   Hardware Configuration Bits ----------------------------------------------*/
#pragma config FOSC		= INTIO67      
#pragma config PLLCFG	= OFF         
#pragma config PRICLKEN = ON         
#pragma config FCMEN	= OFF         
#pragma config IESO		= OFF           
#pragma config PWRTEN	= OFF         
#pragma config BOREN	= ON          
#pragma config BORV		= 285          
#pragma config WDTEN	= OFF        
#pragma config PBADEN	= OFF         
#pragma config LVP		= OFF          
#pragma config MCLRE	= EXTMCLR      

// Libraries ------------------------------------------------------------------
#include<stdio.h>
#include<delays.h>

// Global Variables -----------------------------------------------------------
unsigned int distance;     // Variable to store the measured distance
unsigned int pulseDuration;

// Constants ------------------------------------------------------------------
#define TRUE			1	
#define FALSE			0
#define TILT_INPUT		PORTBbits.RB2   // Tilt sensor input pin (RB2)
#define IR_SENSOR		PORTBbits.RB3   // Crack detection sensor input pin (RB3)
#define TRIGGERPIN		LATBbits.LATB0  // Ultrasonic trigger pin (RB0)
#define ECHOPIN 		PORTBbits.RB1   // Ultrasonic echo pin (RB1)
#define TILT_LED		LATDbits.LATD2  // LED output pin for tilt sensor (RD2)
#define IR_LED			LATDbits.LATD0  // LED output pin for crack sensor (RD0)
#define UR_LED			LATDbits.LATD1  // LED output pin for ultrasonic sensor (RD1)
#define MOTOR			LATDbits.LATD3  // Motor control pin (RD3)

// Functions ------------------------------------------------------------------

/*>>> setFrequency: -----------------------------------------------------------
Author:		Sachin Raj Charupadikkal
Date:		29/09/2024
Modified:	None
Desc:		Setting the oscillator frequency to 4MHz until it stabilizes.
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void setFrequency(void)
{
	OSCCON = 0x52;  // Set internal oscillator to 4 MHz
	while(!OSCCONbits.HFIOFS);  // Wait for oscillator to stabilize
}

/*>>> setPorts: -------------------------------------------------------------
Author:		Sachin Raj Charupadikkal
Date:		29/09/2024
Modified:	None
Desc:		Ports setting for pin.
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void setPorts(void)
{
	ANSELB = 0x00;	// Set PORTB as digital I/O
	TRISB = 0x0E;	// Set RB0 (Ultrasonic trigger) as output, RB1-RB3 as inputs

	ANSELD = 0x00;  // Set PORTD as digital I/O
	TRISD = 0x00;	// Set all PORTD pins as outputs
	LATD = 0x00;    // Initialize PORTD (all LEDs and motor off initially)
}

/*>>> configTMR0: -------------------------------------------------------------
Author:		
Date:		
Modified:	None
Desc:		
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void configTMR0(void) 
{
	T0CON = 0x88;  // Enable Timer0, 16-bit mode, 1:1 prescaler
	TMR0H = 0;     // Reset Timer0 high byte
	TMR0L = 0;     // Reset Timer0 low byte
}// eo configTMR0::

/*>>> readTMR0: -------------------------------------------------------------
Author:		
Date:		
Modified:	None
Desc:		
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
unsigned int readTMR0(void) 
{
	unsigned char lowByte = TMR0L;  // Read low byte
	unsigned char highByte = TMR0H;  // Read high byte
	return ((unsigned int)highByte << 8) | lowByte;  // Combine to 16-bit value
}//eo readTMR0::

/*>>> measureDistance: -------------------------------------------------------------
Author:		
Date:		
Modified:	None
Desc:		
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void measureDistance(void) 
{
	TRIGGERPIN = 1;  // Set trigger pin high
	Delay10TCYx(10);  // Delay for 10us
	TRIGGERPIN = 0;  // Set trigger pin low

	// Wait for the echo pin to go high
	while (!ECHOPIN);

	// Start Timer0
	TMR0H = 0;
	TMR0L = 0;
	T0CONbits.TMR0ON = 1;  // Turn on Timer0

	// Wait for the echo pin to go low
	while (ECHOPIN);
	T0CONbits.TMR0ON = 0;  // Stop Timer0

	// Calculate distance in cm
	pulseDuration = readTMR0();
	distance = (pulseDuration * 0.034) / 2;

	// Control LED based on distance
	UR_LED = (distance < 20) ? 1 : 0;
}//eo measureDistance::

/*>>> checkTiltSensor: -------------------------------------------------------------
Author:		
Date:		
Modified:	None
Desc:		
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void checkTiltSensor(void)
{
	TILT_LED = (TILT_INPUT == 1) ? 1 : 0;  // Turn on LED if tilt detected
}//e0 checkTiltSensor::

/*>>> checkCrackSensor: -------------------------------------------------------------
Author:		
Date:		29/09/2024
Modified:	None
Desc:		
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void checkCrackSensor(void)
{
	IR_LED = (IR_SENSOR == 1) ? 1 : 0;  // Turn on LED if crack detected
}

/*>>> systemInitialisation: -------------------------------------------------------------
Author:		
Date:		
Modified:	None
Desc:		
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void systemInitialisation()
{
	setFrequency();       
	setPorts();
	configTMR0();
}//eo systemInitialisation::

/*>>> motorAction: -------------------------------------------------------------
Author:		Sachin Raj Charupadikkal
Date:		29/09/2024
Modified:	None
Desc:		Ports setting for pin.
Input: 		None 
Returns:	None 
----------------------------------------------------------------------------*/
void motorAction(void)
{
	// Stop motor if any LED is on (indicating a sensor is triggered)
	if (UR_LED == 1 || TILT_LED == 1 || IR_LED == 1) 
	{
		MOTOR = 0;  // Stop motor
	}
	else
	{
		MOTOR = 1;  // Keep motor running if no sensor is triggered
	}
}//eo motorAction::

/*--- MAIN FUNCTION -----------------------------------------------------------
----------------------------------------------------------------------------*/
void main(void)
{
	systemInitialisation();  // Initialize system
	MOTOR = 1;  // Start motor initially
	
	while(TRUE)
	{
		measureDistance();       // Measure distance using ultrasonic sensor
		checkTiltSensor();       // Check tilt sensor
		checkCrackSensor();      // Check crack detection sensor
		motorAction();  // Stop motor if any sensor is triggered

		Delay10KTCYx(50);  // Small delay for stability
	}
} // eo main::
