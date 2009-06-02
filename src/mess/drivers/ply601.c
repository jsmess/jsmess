/***************************************************************************

        Plydin-601

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"

static ADDRESS_MAP_START(ply601_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ply601 )
INPUT_PORTS_END


static MACHINE_RESET(ply601)
{
}

static VIDEO_START( ply601 )
{
}

static VIDEO_UPDATE( ply601 )
{
    return 0;
}

static MACHINE_DRIVER_START( ply601 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6800, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(ply601_mem)

    MDRV_MACHINE_RESET(ply601)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ply601)
    MDRV_VIDEO_UPDATE(ply601)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(ply601)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( ply601 )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "bios.rom",   0x0000, 0x1000, CRC(41fe4c4b) SHA1(d8ca92aea0eb283e8d7779cb976bcdfa03e81aea))
  ROM_LOAD( "video.rom",  0x0000, 0x0800, CRC(1c23ba43) SHA1(eb1cfc139858abd0aedbbf3d523f8ba55d27a11d))

  ROM_LOAD( "str$08.rom", 0x0000, 0x2000, CRC(60a00fff) SHA1(0ddb01aad65f0a1c784e1830912250750b57e30e))
  ROM_LOAD( "str$09.rom", 0x0000, 0x2000, CRC(cd496e11) SHA1(ee9de325d218dbc21128b605cb557c8b15533804))
  ROM_LOAD( "str$0b.rom", 0x0000, 0x2000, CRC(4cae092e) SHA1(d18154595e15f5adc9f8d1bc7e0ceb41ee38c47f))
  ROM_LOAD( "str$0c.rom", 0x0000, 0x2000, CRC(912dc572) SHA1(9ffa59a2a9478c7026c5713ab4ff90fdb1210f3e))
  ROM_LOAD( "str$0d.rom", 0x0000, 0x2000, CRC(e52f9386) SHA1(3e596de9bbdebf0c5e23740a1104b76b5185422d))
  ROM_LOAD( "str$0f.rom", 0x0000, 0x2000, CRC(42aafbec) SHA1(b3bde7ebc05bf54942db75e47b5817f61b9a7b41))
ROM_END

ROM_START( ply601a )
  ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  ROM_LOAD( "bios_a.rom", 0x0000, 0x1000, CRC(e018b11e) SHA1(884d59abd5fa5af1295d1b5a53693facc7945b63))
  ROM_LOAD( "video.rom",  0x0000, 0x0800, CRC(1c23ba43) SHA1(eb1cfc139858abd0aedbbf3d523f8ba55d27a11d))

  ROM_LOAD( "str$08.rom", 0x0000, 0x2000, CRC(60a00fff) SHA1(0ddb01aad65f0a1c784e1830912250750b57e30e))
  ROM_LOAD( "str$09.rom", 0x0000, 0x2000, CRC(cd496e11) SHA1(ee9de325d218dbc21128b605cb557c8b15533804))
  ROM_LOAD( "str$0b.rom", 0x0000, 0x2000, CRC(4cae092e) SHA1(d18154595e15f5adc9f8d1bc7e0ceb41ee38c47f))
  ROM_LOAD( "str$0c.rom", 0x0000, 0x2000, CRC(912dc572) SHA1(9ffa59a2a9478c7026c5713ab4ff90fdb1210f3e))
  ROM_LOAD( "str$0d.rom", 0x0000, 0x2000, CRC(e52f9386) SHA1(3e596de9bbdebf0c5e23740a1104b76b5185422d))
  ROM_LOAD( "str$0f.rom", 0x0000, 0x2000, CRC(42aafbec) SHA1(b3bde7ebc05bf54942db75e47b5817f61b9a7b41))
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, ply601,  0,       0, 	ply601, 	ply601, 	 0,  	  ply601,  	 "Mikroelektronika",   "Plydin-601",		GAME_NOT_WORKING)
COMP( ????, ply601a, ply601,  0, 	ply601, 	ply601, 	 0,  	  ply601,  	 "Mikroelektronika",   "Plydin-601A",		GAME_NOT_WORKING)

