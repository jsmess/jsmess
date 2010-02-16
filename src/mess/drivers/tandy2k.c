/***************************************************************************

    Tandy 2000

    Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"

static ADDRESS_MAP_START(tandy2k_mem, ADDRESS_SPACE_PROGRAM, 16)
//  AM_RANGE(0x00000, 0x00001) speaker/clock control
//  AM_RANGE(0x00002, 0x00003) dma mux control
//  AM_RANGE(0x00004, 0x00005) fdc terminal count
//  AM_RANGE(0x00010, 0x0001f) 8251
//  AM_RANGE(0x00030, 0x0003f) 8727
//  AM_RANGE(0x00040, 0x0004f) 8253
//  AM_RANGE(0x00050, 0x0005f) 8255
//  AM_RANGE(0x00060, 0x0006f) 8259 0
//  AM_RANGE(0x00070, 0x0007f) 8259 1
//  AM_RANGE(0x00080, 0x0009f) fdc dma ack
//  AM_RANGE(0x000e0, 0x000ff) hdc dma ack
	AM_RANGE(0x00000, 0x7ffff) AM_RAM
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
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
	ROM_REGION( 0x100000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "u48_even.bin", 0xfe000, 0x1000, CRC(a5ee3e90) SHA1(4b1f404a4337c67065dd272d62ff88dcdee5e34b))
	ROM_LOAD16_BYTE( "u47_odd.bin",  0xfe001, 0x1000, CRC(345701c5) SHA1(a775cbfa110b7a88f32834aaa2a9b868cbeed25b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, tandy2k,  0,       0,	tandy2k,	tandy2k,	 0,  "Tandy Radio Shack",   "Tandy 2000",		GAME_NOT_WORKING | GAME_NO_SOUND)
