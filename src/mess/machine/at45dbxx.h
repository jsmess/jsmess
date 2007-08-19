/*

	Atmel Serial DataFlash

	(c) 2001-2007 Tim Schuerewegen

	AT45DB041 -  528 KByte
	AT45DB081 - 1056 KByte
	AT45DB161 - 2112 KByte

*/

#ifndef _AT45DBXX_H_
#define _AT45DBXX_H_

#include "driver.h"

enum
{
	AT45DB041,
	AT45DB081,
	AT45DB161
};

// init/exit/reset
void at45dbxx_init( int type);
void at45dbxx_exit( void);
void at45dbxx_reset( void);

// pins
void at45dbxx_pin_cs( int data);
void at45dbxx_pin_sck( int data);
void at45dbxx_pin_si( int data);
int  at45dbxx_pin_so( void);

// load/save
void at45dbxx_load( mame_file *file);
void at45dbxx_save( mame_file *file);

// non-volatile ram handler
/*
NVRAM_HANDLER( at45dbxx );
*/

#endif
