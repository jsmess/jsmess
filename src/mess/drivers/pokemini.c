/********************************************************************

Driver file to handle emulation of the Nintendo Pokemon Mini handheld
by Wilbert Pol.

The LCD is likely to be a SSD1828 LCD.

********************************************************************/

#include "driver.h"
#include "includes/pokemini.h"
#include "cpu/minx/minx.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( pokemini_mem_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x000000, 0x000FFF )  AM_ROM							/* bios */
	AM_RANGE( 0x001000, 0x001FFF )	AM_RAM AM_BASE( &pokemini_ram)				/* VRAM/RAM */
	AM_RANGE( 0x002000, 0x0020FF )  AM_READWRITE( pokemini_hwreg_r, pokemini_hwreg_w )	/* hardware registers */
	AM_RANGE( 0x002100, 0x1FFFFF )  AM_ROM							/* cartridge area */
ADDRESS_MAP_END

static INPUT_PORTS_START( pokemini )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Button A")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("Button B")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("Button C")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("Up")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("Down")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("Left")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("Right")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1) PORT_NAME("Power")
INPUT_PORTS_END

static PALETTE_INIT( pokemini ) {
	static const unsigned char pokemini_pal[4][3] = {
		{ 0xFF, 0xFB, 0x87 },
		{ 0xB1, 0xAE, 0x4E },
		{ 0x84, 0x80, 0x4E },
		{ 0x4E, 0x4E, 0x4E }
	};
	int i;
	for( i = 0; i < 4; i++ ) {
		palette_set_color_rgb( machine, i, pokemini_pal[i][0], pokemini_pal[i][1], pokemini_pal[i][2] );
	}
}

static MACHINE_DRIVER_START( pokemini )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", MINX, 4000000 )
	MDRV_CPU_PROGRAM_MAP( pokemini_mem_map, 0 )

	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( pokemini )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	/* This still needs to be improved to actually match the hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 96, 64 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 95, 0, 63 )
	MDRV_PALETTE_LENGTH( 4 )
	MDRV_PALETTE_INIT( pokemini )
	MDRV_SCREEN_REFRESH_RATE( 30 )
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_CPU_VBLANK_INT( pokemini_int, 1 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO( "left", "right" )
	MDRV_SOUND_ROUTE( 0, "left", 0.50 )
	MDRV_SOUND_ROUTE( 0, "right", 0.50 )
MACHINE_DRIVER_END

static void pokemini_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info ) {
	switch( state ) {
	case MESS_DEVINFO_INT_COUNT:			info->i = 1; break;
	case MESS_DEVINFO_INT_MUST_BE_LOADED:	info->i = 0; break;
	case MESS_DEVINFO_PTR_INIT:			info->init = device_init_pokemini_cart; break;
	case MESS_DEVINFO_PTR_LOAD:			info->load = device_load_pokemini_cart; break;
	case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy( info->s = device_temp_str(), "min"); break;
	default:				cartslot_device_getinfo( devclass, state, info ); break;
	}
}

SYSTEM_CONFIG_START( pokemini )
	CONFIG_DEVICE( pokemini_cartslot_getinfo )
SYSTEM_CONFIG_END

ROM_START( pokemini )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD( "bios.min", 0x0000, 0x1000, CRC(aed3c14d) SHA1(daad4113713ed776fbd47727762bca81ba74915f) )
ROM_END

CONS( 1999, pokemini, 0, 0, pokemini, pokemini, 0, pokemini, "Nintendo", "Pokemon Mini", GAME_NOT_WORKING )

