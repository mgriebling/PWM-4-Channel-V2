//************************************************************************************
//
// This source is Copyright (c) 2011 by Computer Inspirations.  All rights reserved.
// You are permitted to modify and use this code for personal use only.
//
//************************************************************************************
/**
* \file   	PWM.c
* \details  This module implements the Pulse-Width Modulation (PWM) timers that drive
*			the external FETs.  Four independent hardware timers are used that can
*			generate PWM pulses from 0 to 100% with a resolution of 10 bits.  Note:
*			currently only the upper 8 bits of the PWM register are used.  
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//************************************************************************************
#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "Types.h"
#include "PWM.h"

#define	PERIOD		200							/*!< Desired clock in Hz - 5mS */
#define	SCALE		64							/*!</ Timer 4 prescaler */
#define	PRCOUNT		(IPERIOD/SCALE/PERIOD)

// PWM state definitions
typedef enum _PWMState {	
	OFF, FADING, HOLDING
} PWMState;

static unsigned int prevPWM[4];		/*!< previous pwm values */
static unsigned int newPWM[4];		/*!< new pwm values */
static unsigned int fadeCount;		/*!< count down fade value in 5mS increments */
static unsigned int counter;		/*!< counter used for fade/hold count down */
static unsigned int holdCount;		/*!< count down hold value in 5mS increments */
static PWMState pwmState;			/*!< current PWM state */

//#define SETPWM1(pwm) 	{ CCP3CONbits.CCP3M = 0b1100; CCPR3L = pwm >> 2; CCP3CONbits.DC3B = pwm & 0x03;}
//#define SETPWM2(pwm) 	{ CCP1CONbits.CCP1M = 0b1100; CCPR1L = pwm >> 2; CCP1CONbits.DC1B = pwm & 0x03;}
//#define SETPWM3(pwm) 	{ CCP2CONbits.CCP2M = 0b1100; CCPR2L = pwm >> 2; CCP2CONbits.DC2B = pwm & 0x03;}
//#define SETPWM4(pwm) 	{ CCP4CONbits.CCP4M = 0b1100; CCPR4L = pwm >> 2; CCP4CONbits.DC4B = pwm & 0x03;}
#define SETPWM1(pwm) 	{ CCP3CONbits.CCP3M = 0b1100; CCPR3L = pwm; CCP3CONbits.DC3B = 0;}
#define SETPWM2(pwm) 	{ CCP1CONbits.CCP1M = 0b1100; CCPR1L = pwm; CCP1CONbits.DC1B = 0;}
#define SETPWM3(pwm) 	{ CCP2CONbits.CCP2M = 0b1100; CCPR2L = pwm; CCP2CONbits.DC2B = 0;}
#define SETPWM4(pwm) 	{ CCP4CONbits.CCP4M = 0b1100; CCPR4L = pwm; CCP4CONbits.DC4B = 0;}

//********************************************************************************
/**
* \details  Shared interrupt service routine for the PWM timers, night sense
*			state machine timing, and the receive UART. 
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//********************************************************************************
void PWM_interrupt (void) {
    unsigned char i;
    unsigned char done;

    switch (pwmState) {
        case FADING:
            if (counter == 0) {
                // Fade the PWM values
                done = 0;
                for (i=CH1; i<=CH4; i++) {
                    if (prevPWM[i] < newPWM[i]) prevPWM[i]++;
                    else if (prevPWM[i] > newPWM[i]) prevPWM[i]--;
                    else done++;
                }

                // Update the PWM outputs with new values
                SETPWM1(prevPWM[CH1]);
                SETPWM2(prevPWM[CH2]);
                SETPWM3(prevPWM[CH3]); 
                SETPWM4(prevPWM[CH4]);

                if (done == 4) {
                    // change to holding state
                    counter = holdCount;
                    pwmState = HOLDING;
                } else {
                    counter = fadeCount;
                }
            }
            break;
        case HOLDING:
            if (counter == 0) {
                    pwmState = OFF;		// finished holding
            }
            break;
        default:
            // OFF state
            break;
    }
    if (counter > 0) counter--;
}

//********************************************************************************
/**
* \details  PWM timer and I/O port initialization.
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//********************************************************************************
void PWM_Init (void) {
	prevPWM[0] = 0; prevPWM[1] = 0; prevPWM[2] = 0; prevPWM[3] = 0;
	
//	APFCON1 = 0x00;				// PWM2 output on pin RC3
	ANSELA = 0;				// All analog inputs are digital
	ANSELB = 0;
	ANSELC = 0;
	
	// Set up PWM registers
	TRISCbits.TRISC3 = 1;			// disable PWM output until set up
	TRISAbits.TRISA2 = 1;			// disable PWM output until set up
	TRISCbits.TRISC5 = 1;			// disable PWM output until set up
	TRISCbits.TRISC6 = 1;			// disable PWM output until set up
	
	// PWM1 initialization -- initially off
	CCP1CONbits.CCP1M = 0b1100;		// PWM mode
	CCP1CONbits.DC1B = 0b00;		// Lowest 2 bits of PWM duty cycle
	CCPR1L = 0x00;				// Upper 8 bits of PWM duty cycle (50% duty cycle)
	CCPTMRS0bits.C1TSEL = 0b00;		// Use Timer2 for this PWM
	
	// PWM2 initialization -- initially off
	CCP2CONbits.CCP2M = 0b1100;		// PWM mode
	CCP2CONbits.DC2B = 0b00;		// Lowest 2 bits of PWM duty cycle
	CCPR2L = 0x00;				// Upper 8 bits of PWM duty cycle (50% duty cycle)
	CCPTMRS0bits.C2TSEL = 0b00;		// Use Timer2 for this PWM
	
	// PWM3 initialization -- initially off
	CCP3CONbits.CCP3M = 0b1100;		// PWM mode
	CCP3CONbits.DC3B = 0b00;		// Lowest 2 bits of PWM duty cycle
	CCPR3L = 0x00;				// Upper 8 bits of PWM duty cycle (50% duty cycle)
	CCPTMRS0bits.C3TSEL = 0b00;		// Use Timer2 for this PWM
	
	// PWM4 initialization -- initially off
	CCP4CONbits.CCP4M = 0b1100;		// PWM mode
	CCP4CONbits.DC4B = 0b00;		// Lowest 2 bits of PWM duty cycle
	CCPR4L = 0x00;				// Upper 8 bits of PWM duty cycle (50% duty cycle)
	CCPTMRS1bits.C4TSEL = 0b00;		// Use Timer2 for this PWM
	
	// Set up pwm Timer 2 registers
	PR2 = 0xFF;				// PWM period value
	PIR1bits.TMR2IF = 0;			// Clear Timer2 interrupt flag bit
	T2CONbits.T2CKPS = 0b10;		// Set up Timer2 prescale to /16
	T2CONbits.TMR2ON = 1;			// Enable Timer2
	
	// Turn on the PWM outputs
	TRISCbits.TRISC3 = 0;			// enable PWM output
	TRISAbits.TRISA2 = 0;			// enable PWM output
	TRISCbits.TRISC5 = 0;			// enable PWM output
	TRISCbits.TRISC6 = 0;			// enable PWM output
	
	// Initialize Timer 4 for pwm updates
	PR4 = PRCOUNT-1;			// PWM update period
	PIR5bits.TMR4IF = 0;			// Clear Timer4 interrupt flag bit
	T4CONbits.T4CKPS = 0b11;		// Set up Timer4 prescale to /64
	TMR4IE = 1;				// Enable Timer4 interrupts
	PEIE = 1;				// Also enable peripheral interrupts for Timer4 use
	T4CONbits.TMR4ON = 1;			// Enable Timer4
	ei();					// Global interrupts enabled
	
	counter = 0;
	pwmState = OFF;				// prevent PWM action
}

//********************************************************************************
/**
* \details  Returns \em TRUE iff the PWM state machine is currently performing a
*			PWM fade or hold function.  Although the PWM pulses are hardware-based,
*			the fading and hold features require software timers.
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//********************************************************************************
BOOL PWM_Busy (void) {
	return (pwmState != OFF);
}		

//********************************************************************************
/**
* \details  Override the active PWM fade/hold functions by setting fixed PWM 
*			outputs for the four channels.  The pwm value is applied during the 
*			next PWM period.  Function returns immmediately.  PWM values range 
*			from 0 to PWM_MAX where PWM_MAX represents 100% duty cycle.
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//********************************************************************************
void PWM_Set (unsigned char pwm1, unsigned char pwm2, unsigned char pwm3, unsigned char pwm4) {
	// .	
	pwmState = OFF;	// Stop ramping now
	__delay_ms(10);	// Wait for next interrupt
	
	prevPWM[CH1] = pwm1; SETPWM1(pwm1);
	prevPWM[CH2] = pwm2; SETPWM2(pwm2);
	prevPWM[CH3] = pwm3; SETPWM3(pwm3);
	prevPWM[CH4] = pwm4; SETPWM4(pwm4);
}	

//********************************************************************************
/**
* \details 	Ramps from the previous pwm values for all channels to the passed pwm
*			value over the fade time which has units of 10 milliseconds.  The hold
*			time has units of 50 milliseconds.  The total fade/hold function takes 
*			fade*5 + hold*50 milliseconds.  This function returns immediately and
*			will prevent another PWM fade/hold event until the current event has
*			completed.  See also the \em PWM_Busy function.
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//********************************************************************************
void PWM_Ramp (unsigned char pwm1, unsigned char pwm2, unsigned char pwm3, unsigned char pwm4, 
			   unsigned char fade, unsigned char hold) {
	if (pwmState != OFF) return;			// don't add a new ramp until the current one is finished

	// Initialize the next ramping stage
	newPWM[CH1] = pwm1;
	newPWM[CH2] = pwm2;
	newPWM[CH3] = pwm3;
	newPWM[CH4] = pwm4;
	holdCount = 10*(unsigned int)hold;
	fadeCount = fade;
	
	// Start next ramping now
	counter = fadeCount;
	pwmState = FADING;
}	
