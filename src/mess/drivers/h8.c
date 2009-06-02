/***************************************************************************

        Heathkit H8

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(h8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_ROM
	AM_RANGE(0x0400, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( h8_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( h8 )
INPUT_PORTS_END


static MACHINE_RESET(h8)
{
}

static VIDEO_START( h8 )
{
}

static VIDEO_UPDATE( h8 )
{
    return 0;
}

static MACHINE_DRIVER_START( h8 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_12_288MHz / 6)
    MDRV_CPU_PROGRAM_MAP(h8_mem)
    MDRV_CPU_IO_MAP(h8_io)

    MDRV_MACHINE_RESET(h8)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(h8)
    MDRV_VIDEO_UPDATE(h8)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(h8)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( h8 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
    ROM_LOAD( "2708_444-13_pam8.rom", 0x0000, 0x0400, CRC(e0745513) SHA1(0e170077b6086be4e5cd10c17e012c0647688c39))
    ROM_REGION( 0x10000, "otherroms", ROMREGION_ERASEFF )
  	ROM_LOAD( "2708_444-13_pam8go.rom", 0x0000, 0x0400, CRC(9dbad129) SHA1(72421102b881706877f50537625fc2ab0b507752))
  	ROM_LOAD( "2716_444-13_pam8at.rom", 0x0000, 0x0800, CRC(fd95ddc1) SHA1(eb1f272439877239f745521139402f654e5403af))

  	ROM_LOAD( "2716_444-19_h17.rom", 0x0000, 0x0800, CRC(26e80ae3) SHA1(0c0ee95d7cb1a760f924769e10c0db1678f2435c))

  	ROM_LOAD( "2732_444-70_xcon8.rom", 0x0000, 0x1000, CRC(b04368f4) SHA1(965244277a3a8039a987e4c3593b52196e39b7e7))
  	ROM_LOAD( "2732_444-140_pam37.rom", 0x0000, 0x1000, CRC(53a540db) SHA1(90082d02ffb1d27e8172b11fff465bd24343486e))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, h8,  0,       0, 	h8, 	h8, 	 0,  	  h8,  	 "Heath, Inc.",   "Heathkit H8",		GAME_NOT_WORKING)

