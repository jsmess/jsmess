/***************************************************************************

    Driver for Casio PV-1000

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/cartslot.h"


static UINT8 *pv1000_ram;


static ADDRESS_MAP_START( pv1000, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x3fff ) AM_MIRROR( 0x4000 ) AM_ROM AM_REGION( "cart", 0 )
	AM_RANGE( 0xb800, 0xbfff ) AM_RAM AM_BASE( &pv1000_ram )
ADDRESS_MAP_END


static ADDRESS_MAP_START( pv1000_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK( 0xff )
	AM_RANGE( 0xf8, 0xf8 ) AM_RAM	/* R?/W */
	AM_RANGE( 0xf9, 0xf9 ) AM_RAM	/* R?/W */
	AM_RANGE( 0xfa, 0xfa ) AM_RAM	/* R?/W */
	AM_RANGE( 0xfb, 0xfb ) AM_RAM	/* R?/W */
	AM_RANGE( 0xfc, 0xfc ) AM_RAM	/* R?/W */
	AM_RANGE( 0xfd, 0xfd ) AM_RAM	/* R/W */
	AM_RANGE( 0xfe, 0xfe ) AM_RAM	/* R?/W */
	AM_RANGE( 0xff, 0xff ) AM_RAM	/* R?/W */
ADDRESS_MAP_END


static INPUT_PORTS_START( pv1000 )
INPUT_PORTS_END


static DEVICE_IMAGE_LOAD( pv1000_cart )
{
	UINT8 *cart = memory_region( image->machine, "cart" );
	int size = image_length( image );

	if ( size != 0x2000 && size != 0x4000 )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unsupported cartridge size" );
		return INIT_FAIL;
	}

	if ( image_fread( image, cart, size ) != size )
	{
		image_seterror( image, IMAGE_ERROR_UNSPECIFIED, "Unable to fully read from file" );
		return INIT_FAIL;
	}

	/* Mirror 8KB rom */
	if ( size == 0x2000 )
	{
		memcpy( cart+0x2000, cart, 0x2000 );
	}

	return INIT_PASS;
}


static MACHINE_DRIVER_START( pv1000 )
	MDRV_CPU_ADD( "maincpu", Z80, 17897725/5 )
	MDRV_CPU_PROGRAM_MAP( pv1000 )
	MDRV_CPU_IO_MAP( pv1000_io )

	MDRV_SCREEN_ADD( "screen", RASTER )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_REFRESH_RATE( 60 )
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_SCREEN_VISIBLE_AREA( 0, 255, 0, 191 )

	MDRV_PALETTE_LENGTH( 16 )

	/* D65010G031 - Video & sound chip */

	/* Cartridge slot */
	MDRV_CARTSLOT_ADD( "cart" )
	MDRV_CARTSLOT_EXTENSION_LIST( "bin" )
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_LOAD( pv1000_cart )
MACHINE_DRIVER_END


ROM_START( pv1000 )
	ROM_REGION( 0x4000, "cart", ROMREGION_ERASE00 ) 
ROM_END


/*    YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT    COMPANY   FULLNAME    FLAGS */
CONS( 1983, pv1000,  0,      0,      pv1000,  pv1000,   0,   "Casio",  "PV-1000",  GAME_NOT_WORKING )

