/***************************************************************************

        instruct

        08/04/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

static ADDRESS_MAP_START(instruct_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x0800, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( instruct_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( instruct )
INPUT_PORTS_END


static MACHINE_RESET(instruct)
{
}

static VIDEO_START( instruct )
{
}

static VIDEO_UPDATE( instruct )
{
    return 0;
}

static MACHINE_CONFIG_START( instruct, driver_data_t )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(instruct_mem)
    MDRV_CPU_IO_MAP(instruct_io)

    MDRV_MACHINE_RESET(instruct)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(instruct)
    MDRV_VIDEO_UPDATE(instruct)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( instruct )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "instruct.rom", 0x0000, 0x0800, CRC(131715a6) SHA1(4930b87d09046113ab172ba3fb31f5e455068ec7))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1978, instruct,  0,       0,	instruct,	instruct,	 0,  "Signetics",   "Signetics Instructor 50",		GAME_NOT_WORKING | GAME_NO_SOUND )

