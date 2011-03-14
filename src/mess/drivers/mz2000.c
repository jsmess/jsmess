/***************************************************************************

	Sharp MZ-2000

	skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class mz2000_state : public driver_device
{
public:
	mz2000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

static VIDEO_START( mz2000 )
{
}

static SCREEN_UPDATE( mz2000 )
{
    return 0;
}

static ADDRESS_MAP_START(mz2000_map, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(mz2000_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)

ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( mz2000 )
INPUT_PORTS_END


static MACHINE_RESET(mz2000)
{
}


static const gfx_layout mz2000_charlayout =
{
	8, 8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( mz2000 )
	GFXDECODE_ENTRY( "chargen", 0x0000, mz2000_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( mz2000, mz2000_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(mz2000_map)
	MCFG_CPU_IO_MAP(mz2000_io)

	MCFG_MACHINE_RESET(mz2000)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MCFG_GFXDECODE(mz2000)
	MCFG_PALETTE_LENGTH(8)
//	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(mz2000)
	MCFG_SCREEN_UPDATE(mz2000)

MACHINE_CONFIG_END




ROM_START( mz2000 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "mz20ipl.bin",0x0000, 0x0800, CRC(d7ccf37f) SHA1(692814ffc2cf50fa8bf9e30c96ebe4a9ee536a86))

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "mzfont.rom", 0x0000, 0x0800, CRC(0631efc3) SHA1(99b206af5c9845995733d877e9e93e9681b982a8) )
ROM_END



/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE     INPUT    INIT    COMPANY           FULLNAME       FLAGS */
COMP( 1982, mz2000,   mz80b,    0,   mz2000,   mz2000,  0, "Sharp",   "MZ-2000", GAME_NOT_WORKING | GAME_NO_SOUND )
