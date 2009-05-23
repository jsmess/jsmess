/***************************************************************************

        NEC PC-6001 series
        NEC PC-6600 series

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(pc6001_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc6001_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( pc6001 )
INPUT_PORTS_END


static MACHINE_RESET(pc6001)
{
}

static VIDEO_START( pc6001 )
{
}

static VIDEO_UPDATE( pc6001 )
{
	return 0;
}

static MACHINE_DRIVER_START( pc6001 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(pc6001_mem)
	MDRV_CPU_IO_MAP(pc6001_io)

	MDRV_MACHINE_RESET(pc6001)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(pc6001)
	MDRV_VIDEO_UPDATE(pc6001)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(pc6001)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( pc6001 )	/* screen = 8000-83FF */
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.60", 0x0000, 0x4000, CRC(54c03109) SHA1(c622fefda3cdc2b87a270138f24c05828b5c41d2) )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.60", 0x0000, 0x1000, CRC(b0142d32) SHA1(9570495b10af5b1785802681be94b0ea216a1e26) )
ROM_END

ROM_START( pc6001a )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.60a", 0x0000, 0x4000, CRC(fa8e88d9) SHA1(c82e30050a837e5c8ffec3e0c8e3702447ffd69c) )

	ROM_REGION( 0x1000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.60a", 0x0000, 0x1000, CRC(49c21d08) SHA1(9454d6e2066abcbd051bad9a29a5ca27b12ec897) )
ROM_END

ROM_START( pc6001m2 )
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.62", 0x0000, 0x8000, CRC(950ac401) SHA1(fbf195ba74a3b0f80b5a756befc96c61c2094182) )
	/* This rom loads at 4000 with 0-3FFF being RAM (a bankswitch, obviously) */
	ROM_LOAD( "voicerom.62", 0x10000, 0x4000, CRC(49b4f917) SHA1(1a2d18f52ef19dc93da3d65f19d3abbd585628af) )

	ROM_REGION( 0xc000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.62",  0x0000, 0x2000, CRC(81eb5d95) SHA1(53d8ae9599306ff23bf95208d2f6cc8fed3fc39f) )
	ROM_LOAD( "cgrom60m.62", 0x2000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
	ROM_LOAD( "kanjirom.62", 0x4000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )
ROM_END

ROM_START( pc6001sr )
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	/* If you split this into 4x 8000, the first 3 need to load to 0-7FFF (via bankswitch). The 4th part looks like gfx data */
	ROM_LOAD( "systemrom1.64", 0x0000, 0x10000, CRC(b6fc2db2) SHA1(dd48b1eee60aa34780f153359f5da7f590f8dff4) )
	ROM_LOAD( "systemrom2.64", 0x10000, 0x10000, CRC(55a62a1d) SHA1(3a19855d290fd4ac04e6066fe4a80ecd81dc8dd7) )

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "cgrom68.64", 0x0000, 0x4000, CRC(73bc3256) SHA1(5f80d62a95331dc39b2fb448a380fd10083947eb) )
ROM_END

ROM_START( pc6600 )	/* Variant of pc6001m2 */
	ROM_REGION( 0x14000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "basicrom.66", 0x0000, 0x8000, CRC(c0b01772) SHA1(9240bb6b97fe06f5f07b5d65541c4d2f8758cc2a) )
	ROM_LOAD( "voicerom.66", 0x10000, 0x4000, CRC(91d078c1) SHA1(6a93bd7723ef67f461394530a9feee57c8caf7b7) )

	ROM_REGION( 0xc000, "gfx1", 0 )
	ROM_LOAD( "cgrom60.66",  0x0000, 0x2000, CRC(d2434f29) SHA1(a56d76f5cbdbcdb8759abe601eab68f01b0a8fe8) )
	ROM_LOAD( "cgrom66.66",  0x2000, 0x2000, CRC(3ce48c33) SHA1(f3b6c63e83a17d80dde63c6e4d86adbc26f84f79) )
	ROM_LOAD( "kanjirom.66", 0x4000, 0x8000, CRC(20c8f3eb) SHA1(4c9f30f0a2ebbe70aa8e697f94eac74d8241cadd) )
ROM_END

/* There exists an alternative (incomplete?) dump, consisting of more .68 pieces, but it's been probably created for emulators:
systemrom1.68 = 0x0-0x8000 BASICROM.68 + ??
systemrom2.68 = 0x0-0x2000 ?? + 0x2000-0x4000 SYSROM2.68 + 0x4000-0x8000 VOICEROM.68 + 0x8000-0x10000 KANJIROM.68
cgrom68.68 = CGROM60.68 + CGROM66.68
 */
ROM_START( pc6600sr )	/* Variant of pc6001sr */
	ROM_REGION( 0x20000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "systemrom1.68", 0x0000, 0x10000, CRC(b6fc2db2) SHA1(dd48b1eee60aa34780f153359f5da7f590f8dff4) )
	ROM_LOAD( "systemrom2.68", 0x10000, 0x10000, CRC(55a62a1d) SHA1(3a19855d290fd4ac04e6066fe4a80ecd81dc8dd7) )

	ROM_REGION( 0x4000, "gfx1", 0 )
	ROM_LOAD( "cgrom68.68", 0x0000, 0x4000, CRC(73bc3256) SHA1(5f80d62a95331dc39b2fb448a380fd10083947eb) )
ROM_END

/*    YEAR  NAME      PARENT   COMPAT MACHINE   INPUT     INIT    CONFIG     COMPANY  FULLNAME          FLAGS */
COMP( 1981, pc6001,   0,       0,     pc6001,   pc6001,   0,  	  pc6001,  	 "NEC",   "PC-6001",		GAME_NOT_WORKING)
COMP( 1981, pc6001a,  pc6001,  0,     pc6001,   pc6001,   0,  	  pc6001,  	 "NEC",   "PC-6001A",		GAME_NOT_WORKING)	// US version of PC-6001
COMP( 1983, pc6001m2, pc6001,  0,     pc6001,   pc6001,   0,  	  pc6001,  	 "NEC",   "PC-6001mkII",	GAME_NOT_WORKING)
COMP( 1983, pc6600,   pc6001,  0,     pc6001,   pc6001,   0,  	  pc6001,  	 "NEC",   "PC-6600",		GAME_NOT_WORKING)	// high-end version of PC-6001mkII
COMP( 1984, pc6001sr, pc6001,  0,     pc6001,   pc6001,   0,  	  pc6001,  	 "NEC",   "PC-6001mkIISR",	GAME_NOT_WORKING)
COMP( 1984, pc6600sr, pc6001,  0,     pc6001,   pc6001,   0,  	  pc6001,  	 "NEC",   "PC-6600SR",		GAME_NOT_WORKING)	// high-end version of PC-6001mkIISR
