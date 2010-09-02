/***************************************************************************

        Fidelity Chess Challenger 10

        More data :
            http://home.germany.net/nils.eilers/cc10.htm

        30/08/2010 Skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(cc10_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cc10_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( cc10 )
INPUT_PORTS_END

static MACHINE_RESET(cc10)
{
}

static VIDEO_START( cc10 )
{
}

static VIDEO_UPDATE( cc10 )
{
	return 0;
}

static MACHINE_CONFIG_START( cc10, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
    MDRV_CPU_PROGRAM_MAP(cc10_mem)
	MDRV_CPU_IO_MAP(cc10_io)

    MDRV_MACHINE_RESET(cc10)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)

    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(cc10)
    MDRV_VIDEO_UPDATE(cc10)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( cc10 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "cc10.bin",   0x0000, 0x1000, CRC(bb9e6055) SHA1(18276e57cf56465a6352239781a828c5f3d5ba63))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 1978, cc10,  0,       0,	cc10,	cc10,	 0, 	  "Fidelity",   "Chess Challenger 10",		GAME_NOT_WORKING | GAME_NO_SOUND)
