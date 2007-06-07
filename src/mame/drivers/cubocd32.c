/* Cubo CD32 (Original hardware by Commodore, additional hardware and games by CD Express, Milan, Italy)

   This is basically a CD32 (Amiga 68020, AGA based games system) hacked up to run arcade games.
   see http://ninjaw.ifrance.com/cd32/cubo/ for a brief overview.

   Several of the games have Audio tracks, therefore the CRC / SHA1 information you get when
   reading your own CDs may not match those in the driver.  There is currently no 100% accurate
   way to rip the audio data with full sub-track and offset information.

   CD32 Hardware Specs (from Wikipedia, http://en.wikipedia.org/wiki/Amiga_CD32)
    * Main Processor: Motorola 68EC020 at 14.3 MHz
    * System Memory: 2 MB Chip RAM
    * 1 MB ROM with Kickstart ROM 3.1 and integrated cdfs.filesystem
    * 1KB of FlashROM for game saves
    * Graphics/Chipset: AGA Chipset
    * Akiko chip, which handles CD-ROM and can do Chunky to Planar conversion
    * Proprietary (MKE) CD-ROM drive at 2x speed
    * Expansion socket for MPEG cartridge, as well as 3rd party devices such as the SX-1 and SX32 expansion packs.
    * 4 8-bit audio channels (2 for left, 2 for right)
    * Gamepad, Serial port, 2 Gameports, Interfaces for keyboard


   ToDo:

   Everything - This is a skeleton driver.
      Add full AGA (68020 based systems, Amiga 1200 / CD32) support to Amiga driver
      Add CD Controller emulation for CD32.
      ... work from there


   Known Games:
   Title                | rev. | year
   ----------------------------------------------
   Candy Puzzle         |  1.0 | 1995
   Harem Challenge      |      | 1995
   Laser Quiz           |      | 1995
   Laser Quiz 2 "Italy" |  1.0 | 1995
   Laser Strixx         |      | 1995
   Magic Premium        |  1.1 | 1996
   Laser Quiz France    |  1.0 | 1995
   Odeon Twister        |      | 199x
   Odeon Twister 2      |202.19| 1999


*/

#include "driver.h"

VIDEO_START(cd32)
{
	return 0;
}

VIDEO_UPDATE(cd32)
{
	return 0;
}

static ADDRESS_MAP_START( cd32_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
ADDRESS_MAP_END



INPUT_PORTS_START( cd32 )
INPUT_PORTS_END


static MACHINE_DRIVER_START( cd32 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 1431818) /* 14.3 Mhz */
	MDRV_CPU_PROGRAM_MAP(cd32_map,0)
	//MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 262)

	MDRV_SCREEN_REFRESH_RATE(59.997)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))

	//MDRV_MACHINE_RESET(amiga)
	//MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512*2, 262)
	MDRV_SCREEN_VISIBLE_AREA((129-8)*2, (449+8-1)*2, 44-8, 244+8-1)
	MDRV_PALETTE_LENGTH(4096)
	//MDRV_PALETTE_INIT(amiga)

	MDRV_VIDEO_START(cd32)
	MDRV_VIDEO_UPDATE(cd32)

	/* sound hardware */
	/*
    MDRV_SPEAKER_STANDARD_STEREO("left", "right")

    MDRV_SOUND_ADD(CUSTOM, 3579545)
    MDRV_SOUND_CONFIG(amiga_custom_interface)
    MDRV_SOUND_ROUTE(0, "left", 0.50)
    MDRV_SOUND_ROUTE(1, "right", 0.50)
    MDRV_SOUND_ROUTE(2, "right", 0.50)
    MDRV_SOUND_ROUTE(3, "left", 0.50)
    */
MACHINE_DRIVER_END



#define CD32_BIOS \
	ROM_REGION16_BE(0x100000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD( "391640-03.u6a", 0x000000, 0x100000, CRC(d3837ae4) SHA1(06807db3181637455f4d46582d9972afec8956d9) ) \


ROM_START( cd32 )
	CD32_BIOS
ROM_END

SYSTEM_BIOS_START( cd32 )
	SYSTEM_BIOS_ADD(0, "cd32", "Kickstart v3.1 rev 40.60 with CD32 Extended-ROM" )
SYSTEM_BIOS_END

ROM_START( cndypuzl )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cndypuzl", 0, SHA1(21093753a1875dc4fb97f23232ed3d8776b48c06) MD5(dcb6cdd7d81d5468c1290a3baf4265cb) )
ROM_END

ROM_START( haremchl )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "haremchl", 0, SHA1(4d5df2b64b376e8d0574100110f3471d3190765c) MD5(00adbd944c05747e9445446306f904be) )
ROM_END

ROM_START( lsrquiz )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "lsrquiz", 0, SHA1(4250c94ab77504104005229b28f24cfabe7c9e48) MD5(12a94f573fe5d218db510166b86fdda5) )
ROM_END

ROM_START( lsrquiz2 )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "lsrquiz2", 0, SHA1(ea92df0e53bf36bb86d99ad19fca21c6129e61d7) MD5(df63c32aca815f6c97889e08c10b77bc) )
ROM_END

ROM_START( mgprem11 )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "mgprem11", 0, SHA1(a8a32d10148ba968b57b8186fdf4d4cd378fb0d5) MD5(e0e4d00c6f981c19a1d20d5e7090b0db) )
ROM_END

ROM_START( lasstixx )
	CD32_BIOS

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "lasstixx", 0, SHA1(29c2525d43a696da54648caffac9952cec85fd37) MD5(6242dd8a3c0b15ef9eafb930b7a7e87f) )
ROM_END

/* BIOS */
GAMEB( 1993, cd32, 0, cd32, cd32, cd32, 0,	   ROT0, "Commodore", "Amiga CD32 Bios", NOT_A_DRIVER )

GAMEB( 1995, cndypuzl, cd32, cd32, cd32, cd32, 0,	   ROT0, "CD Express", "Candy Puzzle (v1.0)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, haremchl, cd32, cd32, cd32, cd32, 0,	   ROT0, "CD Express", "Harem Challenge", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, lsrquiz,  cd32, cd32, cd32, cd32, 0,	   ROT0, "CD Express", "Laser Quiz", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, lsrquiz2, cd32, cd32, cd32, cd32, 0,	   ROT0, "CD Express", "Laser Quiz '2' Italy (v1.0)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1996, mgprem11, cd32, cd32, cd32, cd32, 0,	   ROT0, "CD Express", "Magic Premium (v1.1)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEB( 1995, lasstixx, cd32, cd32, cd32, cd32, 0,	   ROT0, "CD Express", "Laser Strixx", GAME_NOT_WORKING|GAME_NO_SOUND )
