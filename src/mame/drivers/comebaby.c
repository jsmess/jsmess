/* Come On Baby
  (c) 2000 ExPotato Co. Ltd (Excellent Potato)

  There also appears to be a sequel which may be running on the same hardware
  Come On Baby - Ballympic Heroes!  (c) 2001

  This is a Korean PC based board running Windows.  The game runs fully from
  the hard disk making these things rather fragile and prone to damage.

  Very little is known about the actual PC at this time, and the driver is
  just a skeleton placeholder for the CHD dump of the hard disk.

*/

#include "driver.h"

VIDEO_START(comebaby)
{
}

VIDEO_UPDATE(comebaby)
{
	return 0;
}

static ADDRESS_MAP_START( comebaby_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0001ffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( comebaby )
INPUT_PORTS_END


static MACHINE_DRIVER_START( comebaby )
	/* basic machine hardware */
	MDRV_CPU_ADD(PENTIUM, 2000000000) /* Probably a Pentium .. ?? Mhz*/
	MDRV_CPU_PROGRAM_MAP(comebaby_map, 0)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(comebaby)
	MDRV_VIDEO_UPDATE(comebaby)
MACHINE_DRIVER_END


ROM_START(comebaby)
	ROM_REGION32_LE(0x20000, REGION_CPU1, 0)	/* motherboard bios */
	ROM_LOAD("comeonbaby.pcbios", 0x000000, 0x10000, NO_DUMP )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "comebaby", 0, SHA1(e187db3d99f7a2b0f1c33727ad6716e695ac250e) MD5(ed1c8e13a34bbf348493d2848d351849) )
ROM_END


GAME( 2000, comebaby,  0,   comebaby, comebaby, 0, ROT0, "ExPotato", "Come On Baby", GAME_NOT_WORKING|GAME_NO_SOUND )
