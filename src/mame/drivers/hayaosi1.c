/*

Hayaoshi Quiz Ouza Ketteisen
(c)1991 Jaleco

JALECO ED9075
EB90004-20027

CPU: HD68000PS8 x2
MCU: M50747? (labeled "MO-91044")
Sound: YM2151 YM3012 M6295x2
Custom: GS-9000401 (44pin QFP)
        GS-9000403 (44pin QFP, x2)
        GS-9000404 (44pin QFP)
        GS-9000405 (80pin QFP, x3)
        GS-9000406 (80pin QFP, x3)
        GS-9000407 (80pin QFP)

ROMs:
1 - near 68000 (actual label is ???????N?C?Y[1])
2 /            (actual label is ???????N?C?Y[2])

3 - near 6295 (actual label is ???????N?C?Y[3])
4 /           (actual label is ???????N?C?Y[4])

5 - near 68000 (actual label is ???????N?C?Y[5] Ver1.1)
6 /            (actual label is ???????N?C?Y[6] Ver1.1)

7  - near customs (actual label is ???????N?C?Y[7])
8  |              (actual label is ???????N?C?Y[8])
9  |              (actual label is ???????N?C?Y[9])
10 /              (actual label is ???????N?C?Y[10])

PR-91044 (82S131N, not dumped)

*/

#include "driver.h"
#include "sound/okim6295.h"


VIDEO_START( hayaosi1 )
{

}

VIDEO_UPDATE( hayaosi1 )
{
	return 0;
}

static ADDRESS_MAP_START( hayaosi1_mainmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( hayaosi1_submap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( hayaosi1 )
INPUT_PORTS_END

static const gfx_layout hayaosi1_16x16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,
	 512+0*4,512+1*4,512+2*4,512+3*4,512+4*4,512+5*4,512+6*4,512+7*4},
	{ 0*32,1*32, 2*32, 3*32,4*32,5*32,6*32,7*32,8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32 },
	32*32
};

static const gfx_layout hayaosi1_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4 },
	{ 0*32,1*32, 2*32, 3*32,4*32,5*32,6*32,7*32 },
	8*32
};

static GFXDECODE_START( hayaosi1 )
	GFXDECODE_ENTRY( REGION_GFX1, 0, hayaosi1_16x16_layout,   0, 0  )
	GFXDECODE_ENTRY( REGION_GFX2, 0, hayaosi1_16x16_layout,   0, 0  )
	GFXDECODE_ENTRY( REGION_GFX3, 0, hayaosi1_layout,   0, 0  )
	GFXDECODE_ENTRY( REGION_GFX4, 0, hayaosi1_layout,   0, 0  )
GFXDECODE_END


static MACHINE_DRIVER_START( hayaosi1 )
	MDRV_CPU_ADD_TAG("main", M68000, 8000000 ) // CPU rated 8mhz, clocks unknown
	MDRV_CPU_PROGRAM_MAP(hayaosi1_mainmap,0)

	MDRV_CPU_ADD_TAG("main", M68000, 8000000 ) // CPU rated 8mhz, clocks unknown
	MDRV_CPU_PROGRAM_MAP(hayaosi1_submap,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(hayaosi1)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(hayaosi1)
	MDRV_VIDEO_UPDATE(hayaosi1)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 8000000/8)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)

	MDRV_SOUND_ADD(OKIM6295, 8000000/8)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)

	/* YM2151 */

MACHINE_DRIVER_END



ROM_START( hayaosi1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "5", 0x00001, 0x40000, CRC(eaf38fab) SHA1(0f9cd6e674668a86d2bb54228b50217c934e96af) )
	ROM_LOAD16_BYTE( "6", 0x00000, 0x40000, CRC(341f8057) SHA1(958d9fc870bc13a9c1720d21776b5239db771ce2) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "1", 0x00001, 0x20000, CRC(b088b27e) SHA1(198e2520ce4f9b19ea108e09ff00f7e27768f290) )
	ROM_LOAD16_BYTE( "2", 0x00000, 0x20000, CRC(cebc7b16) SHA1(18b166560ffff7c43cec3d52e4b2da79256dfb2e) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* MCU Internal Code (no dump) */
	ROM_LOAD( "mo-91044.mcu", 0x000000, 0x40000, NO_DUMP )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* M6295 Samples #1 */
	ROM_LOAD( "3", 0x000000, 0x40000, CRC(f3f5787a) SHA1(5e0416726de7b78583c9e1eb7944a41d307a9308) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 ) /* M6295 Samples #2 */
	ROM_LOAD( "4", 0x000000, 0x40000, CRC(ac3f9bd2) SHA1(7856f40daa30de9077e68a5ea977ec39c044c2f8) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROM_LOAD( "7", 0x000000, 0x80000, CRC(3629c455) SHA1(c216b600750861b073062c165f36e6949db10d78) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "8", 0x000000, 0x80000, CRC(15f0b2a3) SHA1(48080de7818bd1c4ac6a7cd81aa86b69bdda2668) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 )
	ROM_LOAD( "9", 0x000000, 0x20000, CRC(64d5b95e) SHA1(793714b2b049afd1cb66c888545cb8379c702010) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 )
	ROM_LOAD( "10", 0x000000, 0x80000, CRC(593e93d6) SHA1(db449b45301e3f7c26e0dfe1f4cf8293ae7dfdaa) )

	ROM_REGION( 0x80000, REGION_PROMS, 0 )
	ROM_LOAD( "pr-91044", 0x000, 0x100, NO_DUMP )
ROM_END

GAME( 1991, hayaosi1,    0,        hayaosi1,    hayaosi1,    0, ROT0,  "Jaleco", "Hayaoshi Quiz Ouza Ketteisen", GAME_NOT_WORKING )

