/***************************************************************************

Driver file to handle emulation of the Tiger Game.com by
  Wilbert Pol

Todo:
  everything
  - Finish memory map, fill in details
  - Finish input ports
  - Finish palette code
  - Finish machine driver struct
  - Finish cartslot code
  - Etc, etc, etc.


***************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "includes/gamecom.h"
#include "cpu/sm8500/sm8500.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START(gamecom_mem_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x03FF )  AM_READWRITE( gamecom_internal_r, gamecom_internal_w ) /* CPU internal register file and RAM */
	AM_RANGE( 0x0400, 0x0FFF )  AM_NOP                                                 /* Nothing */
	AM_RANGE( 0x1000, 0x1FFF )  AM_ROM                                                 /* Internal ROM (initially), or External ROM/Flash. Controlled by MMU0 (never swapped out in game.com) */
	AM_RANGE( 0x2000, 0x3FFF )  AM_ROMBANK(1)                                          /* External ROM/Flash. Controlled by MMU1 */
	AM_RANGE( 0x4000, 0x5FFF )  AM_ROMBANK(2)                                          /* External ROM/Flash. Controlled by MMU2 */
	AM_RANGE( 0x6000, 0x7FFF )  AM_ROMBANK(3)                                          /* External ROM/Flash. Controlled by MMU3 */
	AM_RANGE( 0x8000, 0x9FFF )  AM_ROMBANK(4)                                          /* External ROM/Flash. Controlled by MMU4 */
	AM_RANGE( 0xA000, 0xDFFF )  AM_READWRITE( gamecom_vram_r, gamecom_vram_w )         /* VRAM */
	AM_RANGE( 0xE000, 0xFFFF )  AM_RAM                                                 /* Extended I/O, Extended RAM */
ADDRESS_MAP_END

static gfx_decode gamecom_gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

SM8500_CONFIG gamecom_cpu_config = {
	gamecom_handle_dma,
	gamecom_update_timers
};

INPUT_PORTS_START( gamecom )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME( "Up" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME( "Down" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME( "Left" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME( "Right" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME( "Menu" ) PORT_CODE( KEYCODE_M )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME( DEF_STR(Pause) ) PORT_CODE( KEYCODE_V )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME( "Sound" ) PORT_CODE( KEYCODE_B )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME( "Button A" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME( "Button B" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME( "Button C" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME( "Power(?)" ) PORT_CODE( KEYCODE_N )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME( "Button D" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME( "Stylus press" ) PORT_CODE( KEYCODE_Z )

	PORT_START
	PORT_BIT( 0xff, 100, IPT_LIGHTGUN_X ) PORT_MINMAX(0,199) PORT_SENSITIVITY(50) PORT_KEYDELTA(8)

	PORT_START
	PORT_BIT( 0xff, 80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,159) PORT_SENSITIVITY(50) PORT_KEYDELTA(8)
INPUT_PORTS_END

#define GAMECOM_PALETTE_LENGTH	5	

static unsigned char palette[] =
{
	0xDF, 0xFF, 0x8F,	/* White */
	0x8F, 0xCF, 0x8F,	/* Gray 3 */
	0x6F, 0x8F, 0x4F,	/* Gray 2 */
	0x0F, 0x4F, 0x2F,	/* Gray 1 */
	0x00, 0x00, 0x00,	/* Black */
};

static PALETTE_INIT( gamecom )
{
	int index;
	for ( index = 0; index < GAMECOM_PALETTE_LENGTH; index++ )
	{
		palette_set_color(machine,  4-index, palette[index*3+0], palette[index*3+1], palette[index*3+2] );
		colortable[index] = index;
	}
}

static MACHINE_DRIVER_START( gamecom )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG( "main", SM8500, 11059200/2 )   /* actually it's an sm8521 microcontroller containing an sm8500 cpu */
        MDRV_CPU_PROGRAM_MAP( gamecom_mem_map, 0 )
	MDRV_CPU_CONFIG( gamecom_cpu_config )
	MDRV_CPU_VBLANK_INT( gamecom_scanline, 200 )

	MDRV_SCREEN_REFRESH_RATE( 59.732155 )
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( gamecom )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 200, 200 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 199, 0, 159 )
	MDRV_GFXDECODE( gamecom_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH( GAMECOM_PALETTE_LENGTH )
	MDRV_COLORTABLE_LENGTH( GAMECOM_PALETTE_LENGTH )
	MDRV_PALETTE_INIT( gamecom )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO( "left", "right" )
	/* MDRV_SOUND_ADD( CUSTOM, 0 ) */
	/* MDRV_SOUND_CONFIG */
	MDRV_SOUND_ROUTE( 0, "left", 0.50 )
	MDRV_SOUND_ROUTE( 1, "right", 0.50 )
MACHINE_DRIVER_END

static void gamecom_cartslot_getinfo( const device_class *devclass, UINT32 state, union devinfo *info ) {
	switch( state ) {
	case DEVINFO_INT_COUNT:
		info->i = 1;
		break;
	case DEVINFO_PTR_INIT:
		info->init = device_init_gamecom_cart;
		break;
	case DEVINFO_PTR_LOAD:
		info->load = device_load_gamecom_cart;
		break;
	case DEVINFO_STR_FILE_EXTENSIONS:
		strcpy(info->s = device_temp_str(), "bin");
		break;
	default:
		cartslot_device_getinfo( devclass, state, info );
		break;
	}
}

SYSTEM_CONFIG_START( gamecom )
	CONFIG_DEVICE( gamecom_cartslot_getinfo )
SYSTEM_CONFIG_END

ROM_START( gamecom )
	ROM_REGION( 0x2000, REGION_CPU1, 0 )
	ROM_LOAD( "internal.bin", 0x1000,  0x1000, CRC(a0cec361) SHA1(03368237e8fed4a8724f3b4a1596cf4b17c96d33) )
	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "external.bin", 0x00000, 0x40000, CRC(e235a589) SHA1(97f782e72d738f4d7b861363266bf46b438d9b50) )
ROM_END

/*    YEAR  NAME     PARENT COMPAT MACHINE  INPUT    INIT CONFIG   COMPANY  FULLNAME */
CONS( 1997, gamecom, 0,     0,     gamecom, gamecom, 0,   gamecom, "Tiger", "Game.com", GAME_NOT_WORKING )


