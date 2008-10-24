/***************************************************************************

  pcfx.c

  Driver file to handle emulation of the NEC PC-FX.

***************************************************************************/

#include "driver.h"
#include "video/vdc.h"
#include "deprecat.h"


static ADDRESS_MAP_START( pcfx_mem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x001FFFFF ) AM_RAM	/* RAM */
	AM_RANGE( 0x80700000, 0x807FFFFF ) AM_NOP	/* EXTIO */
	AM_RANGE( 0xE0000000, 0xE7FFFFFF ) AM_NOP
	AM_RANGE( 0xE8000000, 0xE9FFFFFF ) AM_NOP
	AM_RANGE( 0xF8000000, 0xF8000007 ) AM_NOP	/* PIO */
	AM_RANGE( 0xFFF00000, 0xFFFFFFFF ) AM_ROMBANK(1)	/* ROM */
ADDRESS_MAP_END


static READ32_HANDLER( pcfx_vdc_0_r )
{
	UINT32 data;

	logerror("pcfx_vdc_0_r: read from offset %08x\n", offset );

	if ( offset & 1 )
	{
		data = vdc_0_r( machine, 2 );
		data |= ( vdc_0_r( machine, 3 ) << 8 );
	}
	else
		data = vdc_0_r( machine, 0 );

	return data;
}


static WRITE32_HANDLER( pcfx_vdc_0_w )
{
	logerror("pcfx_vdc_0_w: write %08x to offset %08x\n", data, offset );

	if ( offset & 1 )
	{
		vdc_0_w( machine, 2, data );
		vdc_0_w( machine, 3, data >> 8 );
	}
	else
		vdc_0_w( machine, 0, data );
}


/*
	100h - channel select
	102h - Main volume
	104h - frequency (low)
	106h - frequency (high)
	108h - channel volume
	10Ah - pan
	10Ch - wave data
	10Eh - noise
	110h - LFO freq.
	112h - LFO control
	120h - ADPCM control
	122h - ADPCM vol0 left (0-3Fh)
	124h - ADPCM vol0 right (0-3Fh)
	126h - ADPCM vol1 left (0-3Fh)
	128h - ADPCM vol1 right (0-3Fh)
	12Ah -
	12Ch -
*/
static READ32_HANDLER( huc6230_r )
{
	UINT32 data = 0xFFFFFFFF;

	logerror("huc6230_r(): offset = %08x\n", offset );

	return data;
}


static WRITE32_HANDLER( huc6230_w )
{
	logerror("huc630_w(): offset = %08x, data = %08x\n", offset, data );
}


static READ32_HANDLER( huc6271_r )
{
	UINT32 data = 0xFFFFFFFF;

	logerror("huc6271_r(): offset = %08x\n", offset );

	return data;
}


static WRITE32_HANDLER( huc6271_w )
{
	logerror("huc6271_w(): offset = %08x, data = %08x\n", offset, data );
}


static READ32_HANDLER( huc6261_r )
{
	UINT32 data = 0xFFFFFFFF;

	logerror("huc6261_r(): offset = %08x\n", offset );

	return data;
}


static WRITE32_HANDLER( huc6261_w )
{
	logerror("huc6261_w(): offset = %08x, data = %08x\n", offset, data );
}


static READ32_HANDLER( pcfx_vdc_1_r )
{
	UINT32	data;

	logerror("pcfx_vdc_1_r: read from offset %08x\n", offset );

	if ( offset & 1 )
	{
		data = vdc_1_r( machine, 2 );
		data |= ( vdc_1_r( machine, 3 ) << 8 );
	}
	else
		data = vdc_1_r( machine, 0 );

	return data;
}


static WRITE32_HANDLER( pcfx_vdc_1_w )
{
	logerror("pcfx_vdc_1_w: write %08x to offset %08x\n", data, offset );

	if ( offset & 1 )
	{
		vdc_1_w( machine, 2, data );
		vdc_1_w( machine, 3, data >> 8 );
	}
	else
		vdc_1_w( machine, 0, data );
}


static READ32_HANDLER( huc6272_r )
{
	UINT32 data = 0xFFFFFFFF;

	logerror("huc6272_r(): offset = %08x\n", offset );

	return data;
}


static WRITE32_HANDLER( huc6272_w )
{
	logerror("huc6272_w(): offset = %08x, data = %08x\n", offset, data );
}


static ADDRESS_MAP_START( pcfx_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE( 0x00000000, 0x000000FF ) AM_NOP	/* PAD */
	AM_RANGE( 0x00000100, 0x000001FF ) AM_READWRITE( huc6230_r, huc6230_w)				/* HuC6230 */
	AM_RANGE( 0x00000200, 0x000002FF ) AM_READWRITE( huc6271_r, huc6271_w)				/* HuC6271 */
	AM_RANGE( 0x00000300, 0x000003FF ) AM_READWRITE( huc6261_r, huc6261_w)				/* HuC6261 */
	AM_RANGE( 0x00000400, 0x000004FF ) AM_READWRITE( pcfx_vdc_0_r, pcfx_vdc_0_w )		/* HuC6270-A */
	AM_RANGE( 0x00000500, 0x000005FF ) AM_READWRITE( pcfx_vdc_1_r, pcfx_vdc_1_w )		/* HuC6270-B */
	AM_RANGE( 0x00000600, 0x000006FF ) AM_READWRITE( huc6272_r, huc6272_w)				/* HuC6272 */
	AM_RANGE( 0x00000C80, 0x00000C83 ) AM_NOP
	AM_RANGE( 0x00000E00, 0x00000EFF ) AM_NOP
	AM_RANGE( 0x00000F00, 0x00000FFF ) AM_NOP
	AM_RANGE( 0x80500000, 0x805000FF ) AM_NOP	/* HuC6273 */
ADDRESS_MAP_END


static MACHINE_RESET( pcfx )
{
	memory_set_bankptr( 1, memory_region(machine, "user1") );
}


static MACHINE_DRIVER_START( pcfx )
	MDRV_CPU_ADD( "main", V810, 21477270 )
	MDRV_CPU_PROGRAM_MAP( pcfx_mem, 0 )
	MDRV_CPU_IO_MAP( pcfx_io, 0 )
	MDRV_CPU_VBLANK_INT_HACK(sgx_interrupt, VDC_LPF)	/* This needs to be updated */

	MDRV_MACHINE_RESET( pcfx )

	MDRV_SCREEN_ADD( "main", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_RGB32 )
	MDRV_SCREEN_RAW_PARAMS(21477270/2, VDC_WPF, 70, 70 + 512 + 32, VDC_LPF, 14, 14+242)

	MDRV_VIDEO_START( pce )	/* This needs to be updated */
	MDRV_VIDEO_UPDATE( pce )	/* This needs to be updated */

MACHINE_DRIVER_END


ROM_START( pcfx )
	ROM_REGION( 0x100000, "user1", 0 )
	ROM_LOAD( "pcfxbios.bin", 0x000000, 0x100000, CRC(76ffb97a) SHA1(1a77fd83e337f906aecab27a1604db064cf10074) )
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY FULLNAME        FLAGS */
CONS( 1994, pcfx,       0,      0,      pcfx,       0,      0,      0,      "NEC",  "PC-FX",        GAME_NOT_WORKING )

