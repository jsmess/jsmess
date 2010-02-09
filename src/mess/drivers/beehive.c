/***************************************************************************

    BEEHIVE DM3270

    25/05/2009 Skeleton driver [Robbbert]

    This is a conventional computer terminal using a serial link.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(beehive_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x17ff ) AM_ROM
	AM_RANGE( 0x1800, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( beehive_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( beehive )
INPUT_PORTS_END


static MACHINE_RESET(beehive)
{
}

static VIDEO_START( beehive )
{
}

static VIDEO_UPDATE( beehive )
{
    return 0;
}

static MACHINE_DRIVER_START( beehive )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8085A, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(beehive_mem)
    MDRV_CPU_IO_MAP(beehive_io)

    MDRV_MACHINE_RESET(beehive)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(beehive)
    MDRV_VIDEO_UPDATE(beehive)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( beehive )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "dm3270-1.rom", 0x0000, 0x0800, CRC(781bde32) SHA1(a3fe25baadd2bfc2b1791f509bb0f4960281ee32) )
	ROM_LOAD( "dm3270-2.rom", 0x0800, 0x0800, CRC(4d3476b7) SHA1(627ad42029ca6c8574cda8134d047d20515baf53) )
	ROM_LOAD( "dm3270-3.rom", 0x1000, 0x0800, CRC(dbf15833) SHA1(ae93117260a259236c50885c5cecead2aad9b3c4) )

	/* character generator not dumped */
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, beehive,  0,       0, 	beehive, 	beehive, 	 0,  "BeeHive",   "DM3270",		GAME_NOT_WORKING | GAME_NO_SOUND)

