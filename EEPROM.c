
//************************************************************************************
//
// This source is Copyright (c) 2011 by Computer Inspirations.  All rights reserved.
// You are permitted to modify and use this code for personal use only.
//
//************************************************************************************
/**
* \file   	EEPROM.c
* \details  This module implements the interface to the Microchip 24LC256 EEPROM.
*			Code in comments was the original (unsuccessful) attempt to use the I2C
*			hardware port.  I eventually resorted to using a bit-banged I2C software
*			implementation (see \em I2C.c) due to the problems I was having.  
*
*			If anyone can get the hardware I2C working, please send me the code.  To
*			simplify maintenance, I suggest putting the hardware I2C code in the
*			I2C.c module in place of the existing software I2C.
* \author   Michael Griebling
* \date   	10 Nov 2011
*/ 
//************************************************************************************

#include "Types.h"					// Required to interface with delay routines
#include "EEPROM.h"
#include "I2C.h"

//#define EEPROM_DEVICE	(0xA0)		// Base device address for EEPROM
#define PAGE_SIZE		(64)		// Write page size for Microchip's 24xx256 EEPROM
#define EEPROM_BYTES	(1024*32)	// 32KB EEPROM

void EEPROM_Init (void) {
 	// Set up the I2C registers
	I2C_BEGIN();
}		

void EEPROM_WriteChar(unsigned int add, unsigned char ch) {
	/////////////////////////////////////////////////////////////////////////	
	// Send a data byte
	I2C_Send(add, ch);
   __delay_ms(6);					/* write time delay */	
	
}	
	
void EEPROM_Write(unsigned int add, unsigned char buffer[], unsigned int size) {
	/////////////////////////////////////////////////////////////////////////	
	// Write a block of data to EEPROM -- we check to ensure that writes across
	// page boundaries are handled properly.
	unsigned int i;
	unsigned int lsize;
	
	lsize = add & (PAGE_SIZE-1);
	if (lsize != 0) {
		// starting in the middle of an EEPROM page -- write partial page first
		lsize = PAGE_SIZE - lsize;
		if (lsize > size) lsize = size;
		I2C_SendBuf(add, buffer, lsize);
   		__delay_ms(6);					/* write time delay */	
		size -= lsize;
		add += lsize; 
	}
	
	// Write all the PAGE_SIZEd segments to EEPROM
	while (size >= PAGE_SIZE) {
		I2C_SendBuf(add, &buffer[lsize], PAGE_SIZE);
   		__delay_ms(6);					/* write time delay */	
		size -= PAGE_SIZE;
		add += PAGE_SIZE;
		lsize += PAGE_SIZE; 		
	}
	
	// Write any remnant bytes
	if (size > 0) {
		I2C_SendBuf(add, &buffer[lsize], size);
   		__delay_ms(6);					/* write time delay */	
	}	
}

BOOL EEPROM_Present (void) {
	return I2C_Device_Present();	
}

unsigned int EEPROM_GetSize (void) {
	return EEPROM_BYTES;	
}	

unsigned char EEPROM_ReadChar(unsigned int add) {
	return I2C_Get(add);
}
	
void EEPROM_Read(unsigned int add, unsigned char buffer[], unsigned int size) {
    // Close down the read session
	I2C_GetBuf(add, buffer, size);
}	


