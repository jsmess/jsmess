/***************************************************************************

    Tandy 2000

    Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(tandy2k_mem, ADDRESS_SPACE_PROGRAM, 16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tandy2k_io , ADDRESS_SPACE_IO, 16)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( tandy2k )
INPUT_PORTS_END


static MACHINE_RESET(tandy2k)
{
}

static VIDEO_START( tandy2k )
{
}

static VIDEO_UPDATE( tandy2k )
{
    return 0;
}

static MACHINE_DRIVER_START( tandy2k )
    /* basic machine hardware */
	MDRV_CPU_ADD("maincpu", I80186, 8000000)
    MDRV_CPU_PROGRAM_MAP(tandy2k_mem)
    MDRV_CPU_IO_MAP(tandy2k_io)

    MDRV_MACHINE_RESET(tandy2k)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(tandy2k)
    MDRV_VIDEO_UPDATE(tandy2k)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( tandy2k )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "u48_even.bin", 0x0000, 0x0000, CRC(a5ee3e90) SHA1(4b1f404a4337c67065dd272d62ff88dcdee5e34b))
	ROM_LOAD( "u47_odd.bin", 0x0000, 0x0000, CRC(345701c5) SHA1(a775cbfa110b7a88f32834aaa2a9b868cbeed25b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, tandy2k,  0,       0, 	tandy2k, 	tandy2k, 	 0,  "Tandy Radio Shack",   "Tandy 2000",		GAME_NOT_WORKING )
