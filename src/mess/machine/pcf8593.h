/*

	Philips PCF8593 CMOS clock/calendar circuit

	(c) 2001-2007 Tim Schuerewegen

*/

#ifndef _PCF8593_H_
#define _PCF8593_H_

#include "driver.h"

// init/exit/reset
void pcf8593_init( void);
void pcf8593_exit( void);
void pcf8593_reset( void);

// pins
void pcf8593_pin_scl( int data);
void pcf8593_pin_sda_w( int data);
int  pcf8593_pin_sda_r( void);

// load/save
void pcf8593_load( mame_file *file);
void pcf8593_save( mame_file *file);

// non-volatile ram handler
NVRAM_HANDLER( pcf8593 );

#endif
