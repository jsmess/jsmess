/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#ifndef THOM_CAS
#define THOM_CAS

#include "device.h"


/******************** TO7 ************************/

extern WRITE8_HANDLER ( to7_set_cassette_motor );

/* 1-bit cassette input (demodulated) */
extern int to7_get_cassette ( void );

/* 1-bit cassette output (modulated) */
extern void to7_set_cassette ( int i );

CASSETTE_FORMATLIST_EXTERN( to7_cassette_formats );

extern void to7_cassette_getinfo( const device_class *devclass, 
				  UINT32 state, union devinfo *info );


/******************** MO5 ************************/

extern WRITE8_HANDLER ( mo5_set_cassette_motor );

/* 1-bit cassette input  */
extern int mo5_get_cassette ( void );

/* 1-bit cassette output */
extern void mo5_set_cassette ( int i );

CASSETTE_FORMATLIST_EXTERN( mo5_cassette_formats );

extern void mo5_cassette_getinfo( const device_class *devclass, 
				  UINT32 state, union devinfo *info );

#endif
