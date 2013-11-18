/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include "system.h"        /* System funct/params, like osc/peripheral config */
// #include "user.h"          /* User funct/params, such as InitApp */

#include "Types.h"
#include "SBUS.h"
#include "PWM.h"
#include "Pushbuttons.h"
#include "NightSense.h"
#include "Macros.h"
#include "EEPROM.h"
#include "Sequences.h"
#include "MemoryMap.h"

/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/
//#define FLASHCOPY     /* define this variable to copy the sequences in Sequences.inc */
                        /* to external EEPROM */

//#ifdef FLASHCOPY
#include "Sequences.inc"
//#endif

#define FW_VERSION	(2)

// Initial internal EEPROM default contents
// Default data definitions for internal EEPROM:
#define NIGHTMODE       (0)     //      state = Night mode off,
#define OFFDELAYTIME    (5)     // 	offDelayTime = 5 mins,
#define ONDELAYTIME     (5)     //	onDelayTime = 5 mins,
#define DURATION        (360)   //	duration = 360 mins = 6 hrs
#define STARTSEQ        (0)     //      start Sequence = 0
#define LASTSEQ         (61)    //      last Sequence = 61
#define DEVICEADDVALUE  (255)   //      device Address = FF

#define HI(x)           ((x >> 8) & 0xFF)
#define LO(x)           (x & 0xFF)

// Internal state variables - 16 bytes
__EEPROM_DATA(NIGHTMODE, OFFDELAYTIME, ONDELAYTIME, HI(DURATION), LO(DURATION), HI(STARTSEQ), LO(STARTSEQ), HI(LASTSEQ));
__EEPROM_DATA(LO(LASTSEQ), DEVICEADDVALUE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

// EEPROM macro sequences - 200 bytes
//      macro area blank (no macros to play)
__EEPROM_DATA(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);	

unsigned int activeSequence;		// active sequence to play
unsigned int maxAddress, minAddress;	// sequence start and end address
BOOL override;				// override outputs via SBUS
BOOL playMacros;			// play EEPROM macros if TRUE

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

// Wait for Sequence to finish playing while also scanning the pushbuttons and
// handling any external commands
BOOL Scan (void) {
	unsigned int i;

	do {
		PushButtons_Scan();			// update pushbutton status
		for (i=0; i<10; i++) {
			SBUS_Process_Command();	// handle protocol commands
			__delay_ms(1);
		}
		if (PushButtons_Active(BUTTON1|BUTTON2)) return TRUE;
//		if (PushButtons_Active(BUTTON2)) return TRUE;
	} while (PWM_Busy());
	return FALSE;
}

void ShowNumber (unsigned int version) {
	while (version > 0) {
		PWM_Ramp (0, 0, 255, 0, 0, 5);		// Flash blue
		Scan();
		PWM_Ramp (0, 0, 0, 0, 0, 20);		// Off
		Scan();
		version--;
	}
}

void Delay (CARDINAL ms) {
    // __delay_ms has limited size arguments
    while (ms > 0) {
        __delay_ms(1);
        ms--;
    }
}

void ConfirmCommand (void) {
	PWM_Set (0, 0, 255, 0);				// Blue flash
	Delay(500);
	PWM_Set (0, 0, 0, 0);
	Delay(250);
}

void Error (void) {
	PWM_Set (255, 0, 0, 0);				// Red flash
	Delay(2000);
	PWM_Set (0, 0, 0, 0);
	Delay(250);
}

// Play the 'sequence' numbered FLASH or EEPROM sequence.
void PlaySequence (unsigned int sequence) {
	BOOL ok;

	if (Seq_Find(sequence) != FIND_OK) {
		Error(); Scan(); return;
	}
	do {
		PWM_Ramp (Seq_GetPWM(0), Seq_GetPWM(1), Seq_GetPWM(2), Seq_GetPWM(3), Seq_GetFade(), Seq_GetHold());
		if (Scan()) {
			PWM_Set(0, 0, 0, 0);
			return;         		// handle push buttons
		}
		ok = Seq_Next(NOREPEAT);
	} while ((Seq_GetActive() == sequence) && ok);
}

//#ifndef FLASHCOPY
static void InitMode (void) {
    unsigned int total;

    // Validate the start/total sequence values
    activeSequence = ReadWord(STARTSEQADD);
    playMacros = FALSE;
    if (activeSequence == PLAYMACROS) {
        // play back internal EEPROM macros
        playMacros = TRUE;
        activeSequence = 0;
        total = Macros_Count();
    } else {
        // determine EEPROM maximum
        total = Seq_Count();
        minAddress = ReadWord(TOTALSEQADD);
        if (minAddress < total) total = minAddress;
        if (Seq_Find(activeSequence) != FIND_OK || (minAddress == 0)) {
            activeSequence = 0;
        }
    }
    minAddress = activeSequence;
    maxAddress = activeSequence+total-1;
}


void DefineEEMacros (void) {
    unsigned int sequence = 0;
    unsigned int prevStart;

    PWM_Set (0, 0, 0, 0);
    Seq_Find(sequence);
    prevStart = ReadWord(STARTSEQADD);
    if (prevStart == PLAYMACROS) prevStart = 0;
    do {
            PlaySequence(sequence);
            if (PushButtons_Pressed(BUTTON1)) {
                    // advance to next sequence
                    PWM_Set (0, 0, 0, 0);
                    PushButtons_Clear(BUTTON1);
                    sequence++;
                    if (sequence >= Seq_Count()) sequence = 0;
                    Seq_Find(sequence);
            }
            if (PushButtons_Pressed(BUTTON2)) {
                    // add sequence to EEPROM
                    PWM_Set (0, 0, 0, 0);
                    PushButtons_Clear(BUTTON2);
                    if (Macros_Add(sequence)) {
                            ConfirmCommand();
                    } else Error();
            }
    } while (!PushButtons_Held(BUTTON2));
    PushButtons_Clear(BUTTON2);

    if (Macros_Count() != 0) {
            WriteWord(STARTSEQADD, PLAYMACROS);	 	// enable macro playback
    } else {
            // restore previous playback mode
            WriteWord(STARTSEQADD, prevStart);		// enable normal playback
    }
    ConfirmCommand();
    ConfirmCommand();
    InitMode();
}

void DoSleep (void) {
    while (PWM_Busy());			// wait for PWM to complete
    PWM_Set(0, 0, 0, 0);
    __delay_ms(50);

    TRISBbits.TRISB4  = 1;		// change SDA to input temporarily
    TRISBbits.TRISB6  = 1;		// change SCL to input temporarily

    TRISAbits.TRISA0  = 1;		// temporarily make JTAG pins inputs
    TRISAbits.TRISA1  = 1;
//    WPUAbits.WPUA0 = 1;			// enable pull-ups
//    WPUAbits.WPUA1 = 1;
//    IOCANbits.IOCAN0 = 1;		// enable interrupt on negative edge on pin A0

    TRISBbits.TRISB5 = 1;		// Set as pulled up digital inputs
    WPUBbits.WPUB5 = 1;

    TMR4IE = 0;				// Disable Timer4 interrupts
    TMR6IE = 0;				// Disable Timer6 interrupts

    FVRCON = 0;				// Disable voltage reference
//    IOCIE = 1;				// Enable I/O interrupts

//*************************************************************************************
    SLEEP();						// processor goes into sleep mode and clears watchdog
//*************************************************************************************
    NOP();
//    IOCIE = 0;
    SBUS_Init();
    PWM_Init();
    PushButtons_Init();
    NightSense_Init();

    ConfirmCommand();
}

//#else

extern unsigned char MAGIC[];

void CopyFlashToEEPROM (void) {
	unsigned char compare;
	unsigned int i;

	// Copies the contents of FLASH in Sequences[] to EEPROM
	if (EEPROM_Present()) {
		// Write Sequences data to external EEPROM
		EEPROM_Write(0x0000, Sequences, sizeof(Sequences));

		// Verify the external EEPROM contents
		for (i=0; i<sizeof(Sequences); i++) {
			compare = EEPROM_ReadChar(i);
			if (compare != Sequences[i]) {
				 Error();
				 return;	// abort
			}
		}
		EEPROM_Write(EEPROM_GetSize()-2, MAGIC, 2);	// initialize EEPROM magic number

		// Set up internal EEPROM start address and sequence length
		WriteWord(STARTSEQADD, 0x0000);			// enable normal playback
		WriteWord(TOTALSEQADD, Seq_Count());	// identify how many sequences are defined
		ConfirmCommand();
	} else {
		Error();
	}
}
//#endif

void InitApp(void)
{
    SBUS_Init();
    EEPROM_Init();
    Macros_Init();
    PWM_Init();
    PushButtons_Init();
    NightSense_Init();
    Seq_Init();

    override = FALSE;

    // Play FLASH/EEPROM sequences or macros from internal EEPROM, changes operating modes, or define EEPROM macros
#ifndef FLASHCOPY
    InitMode();
#else
    CopyFlashToEEPROM();
#endif
}

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

TCHAR main(void)
{
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize I/O and Peripherals for application */
    InitApp();

    // Play FLASH/EEPROM sequences or macros from internal EEPROM, changes operating modes, or define EEPROM macros
    while (1) {
	// Display the firmware version number
	ShowNumber(FW_VERSION);

	for (;;) {
//#ifndef FLASHCOPY
            if (NightSense_IsNight()) {
                if (!override) {
                    if (playMacros) PlaySequence(Macros_Read(activeSequence));
                    else PlaySequence(activeSequence);
                    if (activeSequence < maxAddress) activeSequence++;
                    else activeSequence = minAddress;
                } else {
                    Scan();
                }
            } else {
                PWM_Ramp (0, 0, 0, 0, 1, 0);
                Scan();
            }

            // handle pushbuttons
            if (PushButtons_Pressed(BUTTON2)) {
                // Change operating modes
                PushButtons_Clear(BUTTON2);
                ConfirmCommand();
                DefineEEMacros();
            }
            if (PushButtons_Held(BUTTON2)) {
                PushButtons_Clear(BUTTON2);
                DoSleep();
            }
//#endif
	}
    }
    return 0;
}

