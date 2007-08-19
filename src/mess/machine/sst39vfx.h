/*

	SST Multi-Purpose Flash (MPF)

	(c) 2001-2007 Tim Schuerewegen

	SST39VF020  - 256 KByte
	SST39VF400A - 512 Kbyte

*/

#ifndef _SST39VFX_H_
#define _SST39VFX_H_

#include "driver.h"

enum
{
	SST39VF020,
	SST39VF400A
};

// init/exit/reset
void sst39vfx_init( int type, int cpu_datawidth, int cpu_endianess);
void sst39vfx_exit( void);
void sst39vfx_reset( void);

// get base/size
UINT8* sst39vfx_get_base( void);
UINT32 sst39vfx_get_size( void);

// read/write handler
/*
READ8_HANDLER( sst39vfx_r );
WRITE8_HANDLER( sst39vfx_w );
*/

// load/save
void sst39vfx_load( mame_file *file);
void sst39vfx_save( mame_file *file);

// non-volatile ram handler
/*
NVRAM_HANDLER( sst39vfx );
*/

#endif
