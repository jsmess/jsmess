/*****************************************************************************

	NEC PC-8201 skeleton driver

******************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"


static VIDEO_START( pc8201 )
{

}

static VIDEO_UPDATE( pc8201 )
{
	return 0;
}

/*
banking:
---- --00 0x0000 - 0x7fff IPL
---- --01 0x0000 - 0x7fff EXT ROM
---- --10 0x0000 - 0x7fff RAM 0x08000 - 0x0ffff
---- --11 0x0000 - 0x7fff RAM 0x10000 - 0x17fff
---- 00-- 0x8000 - 0xffff RAM 0x00000 - 0x07fff
---- 01-- 0x8000 - 0xffff NOP
---- 10-- 0x8000 - 0xffff RAM 0x08000 - 0x0ffff
---- 11-- 0x8000 - 0xffff RAM 0x10000 - 0x17fff
*/

static ADDRESS_MAP_START( pc8201_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM


ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8201_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(0x90, 0x9f) //system control
//	AM_RANGE(0xa0, 0xaf) //bank control
//	AM_RANGE(0xb0, 0xbf) //i8155 PIO
//	AM_RANGE(0xf0, 0xff) //LCD control
ADDRESS_MAP_END

static INPUT_PORTS_START( pc8201 )
INPUT_PORTS_END

static MACHINE_RESET( pc8201 )
{

}

static MACHINE_CONFIG_START( pc8201, driver_data_t )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",I8080,8000000)
	MDRV_CPU_PROGRAM_MAP(pc8201_map)
	MDRV_CPU_IO_MAP(pc8201_io)

	MDRV_MACHINE_RESET(pc8201)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(pc8201)
	MDRV_VIDEO_UPDATE(pc8201)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pc8201 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x8000, CRC(3725d32a) SHA1(5b63b520e667b202b27c630cda821beae819e914) )
ROM_END

static DRIVER_INIT( pc8201 )
{

}

GAME( 198?, pc8201,  0,   pc8201,  pc8201,  pc8201, ROT0, "NEC", "PC-8201 (Japan)", GAME_NOT_WORKING | GAME_NO_SOUND )
