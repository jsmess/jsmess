/*

	SST Multi-Purpose Flash (MPF)

	(c) 2001-2007 Tim Schuerewegen

	SST39VF020  - 256 KByte
	SST39VF400A - 512 Kbyte

*/

#include "sst39vfx.h"

#define LOG_LEVEL  1
#define _logerror(level,...)  if (LOG_LEVEL > level) logerror(__VA_ARGS__)

typedef struct
{
	UINT8 *data;
	UINT32 size;
	UINT8 swap;
} SST39VFX;

static SST39VFX flash;

void sst39vfx_state_save( void);

void sst39vfx_init( int type, int cpu_datawidth, int cpu_endianess)
{
	_logerror( 0, "sst39vfx_init (%d)\n", type);
	memset( &flash, 0, sizeof( flash));
	switch (type)
	{
		case SST39VF020  : flash.size = 256 * 1024; break;
		case SST39VF400A : flash.size = 512 * 1024; break;
	}
	flash.data = (UINT8*)malloc( flash.size);
#ifdef LSB_FIRST
	if (cpu_endianess != CPU_IS_LE) flash.swap = cpu_datawidth / 8; else flash.swap = 0;
#else
	if (cpu_endianess != CPU_IS_BE) flash.swap = cpu_datawidth / 8; else flash.swap = 0;
#endif
	sst39vfx_state_save();
}

void sst39vfx_exit( void)
{
	_logerror( 0, "sst39vfx_exit\n");
	free( flash.data);
}

void sst39vfx_reset( void)
{
	_logerror( 0, "sst39vfx_reset\n");
}

void sst39vfx_state_save( void)
{
	const char *name = "sst39vfx";
	state_save_register_item_pointer( name, 0, flash.data, flash.size);
	state_save_register_item( name, 0, flash.swap);
}

UINT8* sst39vfx_get_base( void)
{
	return flash.data;
}

UINT32 sst39vfx_get_size( void)
{
	return flash.size;
}

/*
#define OFFSET_SWAP(offset,width) (offset & (~(width - 1))) | (width - 1 - (offset & (width - 1)))
*/

/*
READ8_HANDLER( sst39vfx_r )
{
	_logerror( 1, "sst39vfx_r (%08X)\n", offset);
	if (flash.swap) offset = OFFSET_SWAP( offset, flash.swap);
	return flash.data[offset];
}
*/

/*
WRITE8_HANDLER( sst39vfx_w )
{
	_logerror( 1, "sst39vfx_w (%08X/%02X)\n", offset, data);
	if (flash.swap) offset = OFFSET_SWAP( offset, flash.swap);
	flash.data[offset] = data;
}
*/

void sst39vfx_swap( void)
{
	int i, j;
	UINT8 *base, temp[8];
	base = flash.data;
	for (i=0;i<flash.size;i+=flash.swap)
	{
		memcpy( temp, base, flash.swap);
		for (j=flash.swap-1;j>=0;j--) *base++ = temp[j];
	}
}

void sst39vfx_load( mame_file *file)
{
	_logerror( 0, "sst39vfx_load (%p)\n", file);
	mame_fread( file, flash.data, flash.size);
	if (flash.swap) sst39vfx_swap();
}

void sst39vfx_save( mame_file *file)
{
	_logerror( 0, "sst39vfx_save (%p)\n", file);
	if (flash.swap) sst39vfx_swap();
	mame_fwrite( file, flash.data, flash.size);
	if (flash.swap) sst39vfx_swap();
}

/*
NVRAM_HANDLER( sst39vfx )
{
	_logerror( 0, "nvram_handler_sst39vfx (%p/%d)\n", file, read_or_write);
	if (read_or_write)
	{
		sst39vfx_save( file);
	}
	else
	{
		if (file)
		{
			sst39vfx_load( file);
		}
		else
		{
			memset( flash.data, 0xFF, flash.size);
		}
	}
}
*/
