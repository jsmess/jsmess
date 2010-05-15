/***************************************************************************

        chaos

        08/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

static ADDRESS_MAP_START(chaos_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x2fff) AM_ROM
	AM_RANGE( 0x3000, 0x6fff) AM_RAM
	AM_RANGE( 0x7000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( chaos_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( chaos )
INPUT_PORTS_END


static MACHINE_RESET(chaos)
{
}

static VIDEO_START( chaos )
{
}

static VIDEO_UPDATE( chaos )
{
    return 0;
}

static MACHINE_DRIVER_START( chaos )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(chaos_mem)
    MDRV_CPU_IO_MAP(chaos_io)

    MDRV_MACHINE_RESET(chaos)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(chaos)
    MDRV_VIDEO_UPDATE(chaos)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( chaos )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "chaos.001", 0x0000, 0x1000, CRC(3b433e72) SHA1(5b487337d71253d0e64e123f405da9eaf20e87ac))
	ROM_LOAD( "chaos.002", 0x1000, 0x1000, CRC(8b0b487f) SHA1(0d167cf3004a81c87446f2f1464e3debfa7284fe))
	ROM_LOAD( "chaos.003", 0x2000, 0x1000, CRC(5880db81) SHA1(29b8f1b03c83953f66464ad1fbbfe2e019637ce1))
	ROM_LOAD( "chaos.004", 0x7000, 0x1000, CRC(5d6839d6) SHA1(237f52f0780ac2e29d57bf06d0f7a982eb523084))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, chaos,  0,       0, 	chaos,		chaos,	 0, 	  "<unknown>",   "Chaos 2",		GAME_NOT_WORKING | GAME_NO_SOUND )

