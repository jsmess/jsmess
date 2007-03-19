/* Ganbare Chinsan Ooshoubu

 without the key from the 317-5012 (MC-8123A) internal RAM we can't emulate this, decryption of
 the program roms is impossible without this key.  See src\machine\mc8123.c for details on the
 encryption scheme used

 a WORKING PCB is NEEDED to extract the key data.

 does this run on already emulated hardware?
*/


/*

Ganbare Chinsan Ooshoubu
(c)1987 Sanritsu

C1-00114-B

CPU:    317-5012 (MC-8123A)
SOUND:  YM2203
        M5205
OSC:    10.000MHz
        384KHz
Chips:  8255AC-2


MM00.7D   prg.
MM01.8D

MM20.7K   chr.
MM21.8K
MM22.9K

MM40.13D  samples

MM60.2C   ?

MM61.9M   color
MM62.9N
MM63.10N

*/

#include "driver.h"

VIDEO_START(chinsan)
{
	return 0;
}

VIDEO_UPDATE(chinsan)
{
	return 0;
}

static ADDRESS_MAP_START( chinsan_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( chinsan )
INPUT_PORTS_END



static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};


static MACHINE_DRIVER_START( chinsan )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/2)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(chinsan_map,0)
//  MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(chinsan)
	MDRV_VIDEO_UPDATE(chinsan)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( chinsan )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* encrypted code / data */
	ROM_LOAD( "mm01.8d", 0x00000, 0x10000, CRC(c69ddbf5) SHA1(9533365c1761b113174d53a2e23ce6a7baca7dfe) )
	ROM_LOAD( "mm00.7d", 0x10000, 0x10000, CRC(f7a4414f) SHA1(f65223b2928f610ab97fda2f2c008806cf2420e5) )

	ROM_REGION( 0x30000, REGION_USER1, 0 ) /* keys are usually included in source for this type of cpu, but flag it as missing anyway */
	ROM_LOAD( "317-5012.key", 0x00000, 0x2000, NO_DUMP )

	ROM_REGION( 0x30000, REGION_GFX1, 0 )
	ROM_LOAD( "mm20.7k", 0x00000, 0x10000, CRC(54efb409) SHA1(333adadd7f3dc3393dbe334303bae544b3d26c00) )
	ROM_LOAD( "mm21.8k", 0x10000, 0x10000, CRC(25f6c827) SHA1(add72a3cfa2f24105e36d0464c2db6a6bedd4139) )
	ROM_LOAD( "mm22.9k", 0x20000, 0x10000, CRC(6092f6e1) SHA1(32f53027dc954e314d7c5d04ff53f17358bbcf77) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* M5205 samples */
	ROM_LOAD( "mm40.13d", 0x00000, 0x10000, CRC(a408b8f7) SHA1(60a2644922cb60c0a1a3409761c7e50924360313) )

	ROM_REGION( 0x20, REGION_USER2, 0 )
	ROM_LOAD( "mm60.2c", 0x000, 0x020, CRC(88477178) SHA1(03c1c9e3e88a5ae9970cb4b872ad4b6e4d77a6da) )

	ROM_REGION( 0x300, REGION_USER3, 0 )
	ROM_LOAD( "mm61.9m",  0x000, 0x100, CRC(57024262) SHA1(e084e6baa3c529217f6f8e37c9dd5f0687ba2fc4) )
	ROM_LOAD( "mm62.9n",  0x100, 0x100, CRC(b5a1dbe5) SHA1(770a791c061ce422f860bb8d32f82bbbf9b4d12a) )
	ROM_LOAD( "mm63.10n", 0x200, 0x100, CRC(b65e3567) SHA1(f146af51dfaa5b4bf44c4e27f1a0292f8fd07ce9) )
ROM_END

GAME( 1987, chinsan,  0,    chinsan, chinsan, 0, ROT0, "Sanritsu", "Ganbare Chinsan Ooshoubu (MC-8123A, 317-5012)", GAME_NOT_WORKING|GAME_NO_SOUND )
