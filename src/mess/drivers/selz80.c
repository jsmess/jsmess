/***************************************************************************

        SEL Z80 Trainer

        31/08/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(selz80_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0x1fff) AM_RAM
	AM_RANGE(0xa000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( selz80_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( selz80 )
INPUT_PORTS_END


static MACHINE_RESET(selz80)
{
}

static VIDEO_START( selz80 )
{
}

static VIDEO_UPDATE( selz80 )
{
    return 0;
}

static MACHINE_CONFIG_START( selz80, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(selz80_mem)
    MDRV_CPU_IO_MAP(selz80_io)

    MDRV_MACHINE_RESET(selz80)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(selz80)
    MDRV_VIDEO_UPDATE(selz80)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( selz80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "z80-trainer.rom", 0x0000, 0x1000, CRC(eed1755f) SHA1(72e6ebfccb0e50034660bc36db1a741932311ce1))
	ROM_LOAD( "term80-a000.bin", 0xa000, 0x2000, CRC(0a58c0a7) SHA1(d1b4b3b2ad0d084175b1ff6966653d8b20025252))
	ROM_LOAD( "term80-e000.bin", 0xe000, 0x2000, CRC(158e08e6) SHA1(f1add43bcf8744a01238fb893ee284872d434db5))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1985, selz80,  0,       0,	selz80, 	selz80, 	0,	 "SEL",   "Z80 Trainer",		GAME_NOT_WORKING | GAME_NO_SOUND)

