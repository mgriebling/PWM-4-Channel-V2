/******************************************************************************/
/* System Level #define Macros                                                */
/******************************************************************************/

#include "GenericTypeDefs.h"

/* Microcontroller MIPs (FCY) */
#define SYS_FREQ        3686400L
#define FCY             SYS_FREQ/4

#ifndef _XTAL_FREQ
 // Unless already defined define system frequency
 // This definition is required to calibrate __delay_us() and __delay_ms()
 	#define _XTAL_FREQ 	SYS_FREQ
	#define IPERIOD		(_XTAL_FREQ/4)		// Instruction clock in Hz
#endif

//#ifndef BOOL
//#define BOOL unsigned char
//#endif

//#ifndef FALSE
//#define FALSE  0
//#endif
//
//#ifndef TRUE
//#define TRUE  1
//#endif

typedef unsigned char BOOLEAN;
typedef unsigned char TCHAR;
typedef signed char SHORTINT;
typedef int INTEGER;
typedef long LONGINT;
typedef unsigned int CARDINAL;
typedef unsigned long LONGCARD;
typedef float REAL;

/******************************************************************************/
/* System Function Prototypes                                                 */
/******************************************************************************/

/* Custom oscillator configuration funtions, reset source evaluation
functions, and other non-peripheral microcontroller initialization functions
go here. */

void ConfigureOscillator(void); /* Handles clock switching/osc initialization */
