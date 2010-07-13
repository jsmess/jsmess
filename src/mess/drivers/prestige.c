/***************************************************************************

        VTech PreComputer Prestige

        

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(prestige_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( prestige_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( prestige )
INPUT_PORTS_END


static MACHINE_RESET(prestige)
{
}

static VIDEO_START( prestige )
{
}

static VIDEO_UPDATE( prestige )
{
    return 0;
}

static MACHINE_DRIVER_START( prestige )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(prestige_mem)
    MDRV_CPU_IO_MAP(prestige_io)

    MDRV_MACHINE_RESET(prestige)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", LCD)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE( 240, 100 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 240-1, 0, 100-1 )
	MDRV_DEFAULT_LAYOUT( layout_lcd )

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(prestige)
    MDRV_VIDEO_UPDATE(prestige)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( prestige )
    ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD( "27-6020-02", 0x00000, 0x100000, CRC(6bb6db14) SHA1(5d51fc3fd799e7f01ee99c453f9005fb07747b1e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1998, prestige,  0,       0,	prestige, 	prestige, 	 0,  "VTech",   "PreComputer Prestige",		GAME_NOT_WORKING | GAME_NO_SOUND)
