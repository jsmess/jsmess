/***************************************************************************

    Micronics 1000

    http://www.philpem.me.uk/elec/micronic/
    http://members.lycos.co.uk/leeedavison/z80/micronic/index.html

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(micronic_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( micronic )
INPUT_PORTS_END


static MACHINE_RESET(micronic)
{
}

static VIDEO_START( micronic )
{
}

static VIDEO_UPDATE( micronic )
{
    return 0;
}

static MACHINE_DRIVER_START( micronic )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, 4000000)
    MDRV_CPU_PROGRAM_MAP(micronic_mem)

    MDRV_MACHINE_RESET(micronic)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(micronic)
    MDRV_VIDEO_UPDATE(micronic)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( micronic )
    ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "micron1.bin", 0x0000, 0x8000, CRC(5632c8b7) SHA1(d1c9cf691848e9125f9ea352e4ffa41c288f3e29))
	ROM_LOAD( "micron2.bin", 0x8000, 0x8000, CRC(dc8e7341) SHA1(927dddb3914a50bb051256d126a047a29eff7c65))
	ROM_LOAD( "monitor2.bin", 0x10000, 0x8000, CRC(c6ae2bbf) SHA1(1f2e3a3d4720a8e1bb38b37f4ab9e0e32676d030))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 198?, micronic,  0,       0, 	micronic, 	micronic, 	 0,  0,  	 "Victor Micronic",   "Micronic 1000",		GAME_NOT_WORKING)
