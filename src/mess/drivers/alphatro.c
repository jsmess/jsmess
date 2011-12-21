/***************************************************************************

	Triumph-Adler's Alphatronic PC

	skeleton driver

	z80 + m6847 as a CRTC

***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

#define MAIN_CLOCK XTAL_4MHz

class alphatro_state : public driver_device
{
public:
	alphatro_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }
};

static VIDEO_START( alphatro )
{

}

static SCREEN_UPDATE( alphatro )
{
	return 0;
}

static ADDRESS_MAP_START( alphatro_map, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( alphatro_io, AS_IO, 8 )
//  ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

static INPUT_PORTS_START( alphatro )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,16,
	RGN_FRAC(1,1),
	1,
	{ RGN_FRAC(0,1) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static GFXDECODE_START( alphatro )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout,     0, 1 )
GFXDECODE_END

static MACHINE_START( alphatro )
{
//  alphatro_state *state = machine.driver_data<alphatro_state>();

}

static MACHINE_RESET( alphatro )
{

}

static PALETTE_INIT( alphatro )
{
}

static MACHINE_CONFIG_START( alphatro, alphatro_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80,MAIN_CLOCK)
	MCFG_CPU_PROGRAM_MAP(alphatro_map)
	MCFG_CPU_IO_MAP(alphatro_io)

	MCFG_MACHINE_START(alphatro)
	MCFG_MACHINE_RESET(alphatro)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MCFG_SCREEN_UPDATE(alphatro)

	MCFG_GFXDECODE(alphatro)

	MCFG_PALETTE_INIT(alphatro)
	MCFG_PALETTE_LENGTH(8)

	MCFG_VIDEO_START(alphatro)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
//  MCFG_SOUND_ADD("aysnd", AY8910, MAIN_CLOCK/4)
//  MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
2258: DB BB         in   a,($BB)
225A: DB B5         in   a,($B5)
225C: DB C1         in   a,($C1)
225E: DB CA         in   a,($CA)
2260: DB F4         in   a,($F4)
2262: DB F7         in   a,($F7)
2264: DB DF         in   a,($DF)
2266: DB 06         in   a,($06)
2268: DC 09 DC      call c,$DC09

in various places of the ROM ... most likely a bad dump.
*/

ROM_START( alphatro )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "2764.ic-1038"	,     0x0000, 0x2000, BAD_DUMP CRC(e337db3b) SHA1(6010bade6a21975636383179903b58a4ca415e49) ) // ???
    ROM_LOAD( "rom1.bin",     0x2000, 0x2000, BAD_DUMP CRC(1509b15a) SHA1(225c36411de680eb8f4d6b58869460a58e60c0cf) )
	ROM_LOAD( "rom2.bin",     0x4000, 0x2000, BAD_DUMP CRC(998a865d) SHA1(294fe64e839ae6c4032d5db1f431c35e0d80d367) )
	ROM_LOAD( "rom3.bin",     0x6000, 0x2000, BAD_DUMP CRC(55cbafef) SHA1(e3376b92f80d5a698cdcb2afaa0f3ef4341dd624) )

	ROM_REGION( 0x10000, "user1", ROMREGION_ERASE00 )
	ROM_LOAD( "613256.ic-1058"	,     0x2000, 0x6000, BAD_DUMP CRC(ceea4cb3) SHA1(b332dea0a2d3bb2978b8422eb0723960388bb467) )
    ROM_LOAD( "ipl.bin",     0x8000, 0x2000, NO_DUMP  )

	ROM_REGION( 0x1000, "gfx1", ROMREGION_ERASE00 )
	ROM_LOAD( "2732.ic-1067",      0x0000, 0x1000, BAD_DUMP CRC(61f38814) SHA1(35ba31c58a10d5bd1bdb202717792ca021dbe1a8) ) // should be 1:1
ROM_END

COMP( 1983, alphatro,   0,       0,    alphatro,   alphatro,  0,  "Triumph-Adler", "Alphatronic PC", GAME_NOT_WORKING | GAME_NO_SOUND)
