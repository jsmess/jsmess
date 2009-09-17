/***************************************************************************

	Univac Terminals

	The terminals are models UTS20, UTS30, UTS40, UTS50 and SVT1120,
	however only the UTS20 is dumped (program roms only).

	25/05/2009 Skeleton driver [Robbbert].

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(uts20_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x4fff ) AM_ROM
	AM_RANGE( 0x5000, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( uts20_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( uts20 )
INPUT_PORTS_END


static MACHINE_RESET(uts20)
{
}

static VIDEO_START( uts20 )
{
}

static VIDEO_UPDATE( uts20 )
{
    return 0;
}

static MACHINE_DRIVER_START( uts20 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(uts20_mem)
    MDRV_CPU_IO_MAP(uts20_io)

    MDRV_MACHINE_RESET(uts20)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(uts20)
    MDRV_VIDEO_UPDATE(uts20)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( uts20 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "uts20a.rom", 0x0000, 0x1000, CRC(1a7b4b4e) SHA1(c3732e25b4b7c7a80172e3fe55c77b923cf511eb) )
	ROM_LOAD( "uts20b.rom", 0x1000, 0x1000, CRC(7f8de87b) SHA1(a85f404ad9d560df831cc3e651a4b45e4ed30130) )
	ROM_LOAD( "uts20c.rom", 0x2000, 0x1000, CRC(4e334705) SHA1(ff1a730551b42f29d20af8ecc4495fd30567d35b) )
	ROM_LOAD( "uts20d.rom", 0x3000, 0x1000, CRC(76757cf7) SHA1(b0509d9a35366b21955f83ec3685163844c4dbf1) )
	ROM_LOAD( "uts20e.rom", 0x4000, 0x1000, CRC(0dfc8062) SHA1(cd681020bfb4829d4cebaf1b5bf618e67b55bda3) )

	/* Character generator rom not dumped */
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, uts20,  0,       0, 	uts20, 	uts20, 	 0,  	  0,  	 "Sperry Univac",   "UTS-20", GAME_NOT_WORKING)

