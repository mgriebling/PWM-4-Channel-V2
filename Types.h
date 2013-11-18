#ifndef _TYPES_H_
#define _TYPES_H_

#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>        /* HiTech General Include File */
#elif defined(__18CXX)
    #include <p18cxxx.h>    /* C18 General Include File */
#endif

//#ifndef BOOL
//#define BOOL unsigned char
//#endif
//
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
//#ifndef SHORTSET
//typedef unsigned char SHORTSET;
//#endif
//#ifndef SET
//typedef unsigned short SET;
//#endif
//#ifndef LONGSET
//typedef unsigned long LONGSET;
//#endif

#endif
