/***************************************************************************

        VTech PreComputer 2000

        04/12/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(pc2000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc2000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pc2000 )
INPUT_PORTS_END


static MACHINE_RESET( pc2000 )
{
}

static VIDEO_START( pc2000 )
{
}

static VIDEO_UPDATE( pc2000 )
{
    return 0;
}

static MACHINE_DRIVER_START( pc2000 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz) /* probably not accurate */
    MDRV_CPU_PROGRAM_MAP(pc2000_mem)
    MDRV_CPU_IO_MAP(pc2000_io)

    MDRV_MACHINE_RESET(pc2000)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480) /* not accurate either */
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(pc2000)
    MDRV_VIDEO_UPDATE(pc2000)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pc2000 )
    ROM_REGION( 0x1000000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "lh532hee_9344_d.u4", 0x000000, 0x040000, CRC(0b03bf33) SHA1(cb344b94b14975c685041d3e669f386e8a21909f))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1993, pc2000,  0,       0, 	pc2000, 	pc2000, 	 0,  "VTech",   "PreComputer 2000",		GAME_NOT_WORKING)

