/*

Mahjong Dunhuang?
199?, unknown manufacturer

PCB Layout
----------

|---------------------------------------|
|uPD1242H  VOL     UM3567  AR17961  ROM6|
|          3.579545MHz                  |
|                  DSW1(8)          ROM5|
|   VOL            DSW2(8)              |
|                  DSW3(8)          ROM4|
|                  DSW4(8)              |
|                  DSW5(8)          ROM3|
|1   WF19054                            |
|8                                  ROM2|
|W                      |-------|       |
|A                      |       |       |
|Y                      |  *    |       |
|                       |       |       |
|                       |-------|       |
|                    12MHz         6264 |
|                                       |
|                                       |
|1         PAL                     6264 |
|0                                      |
|W                     Z80              |
|A  HM86171-80   BATTERY           6264 |
|Y                     PAL              |
|          PAL         PAL          ROM1|
|---------------------------------------|
Notes:
      Uses common 10-way/18-way Mahjong pinout
      Z80     - clock 6.000MHz [12/2]
      WF19054 - == AY3-8910. Clock = 1.500MHz [12/8]
      UM3567  - == YM2413. Clock 3.579545MHz
      HM86171 - Hualon Microelectronics HMC HM86171 VGA 256 colour RAMDAC (DIP28)
      6264    - UT6264PC-70LL 8k x8 SRAM (DIP28)
      *       - QFP120 IC marked 55102602-12. Logo on the chip is of two 5 1/4 inch floppy
                discs standing upright and next to each other, with the corner facing up,
                like 2 pyramids. The middle hole and head access slot are visible too.
                Chip was manufactured 50th week of 1993
      AR17961 - == Oki M6295 (QFP44). Clock = 1.500MHz [12/8]. pin 7 = high
      VSync   - 60Hz
      HSync   - 15.28kHz

*/

#include "driver.h"
#include "sound/okim6295.h"

VIDEO_START(dunhuang)
{
	return 0;
}

VIDEO_UPDATE(dunhuang)
{
	return 0;
}

static ADDRESS_MAP_START( dunhuang_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( dunhuang )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 28, 24, 4, 0, 12, 8, 20, 16 }, // scrambled?...
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tiles8x32_layout =
{
	8,32,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },

	{ 28, 24, 4, 0, 12, 8, 20, 16 }, // scrambled?...

	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32,
	 16*32,17*32,18*32,19*32,20*32,21*32,22*32,23*32,
	 24*32,25*32,26*32,27*32,28*32,29*32,30*32,31*32
	},
	32*32
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX3, 0, &tiles8x32_layout, 0, 16 },
	{ REGION_GFX4, 0, &tiles8x32_layout, 0, 16 },
	{ -1 }
};



static MACHINE_DRIVER_START( dunhuang )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,12000000/2)		 /* 6 MHz */
	MDRV_CPU_PROGRAM_MAP(dunhuang_map,0)
	//MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-0-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(dunhuang)
	MDRV_VIDEO_UPDATE(dunhuang)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 12000000/8)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	/* +  AY3-8910  12000000/8 */
	/* +  YM2413  3.579545MHz */
MACHINE_DRIVER_END


ROM_START( dunhuang )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1.u9", 0x00000, 0x40000, CRC(843a0117) SHA1(26a838cb3552ea6a9ec55940fcbf83b06c068743) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROM_LOAD( "rom2.u5", 0x00000, 0x40000, CRC(384fa1d3) SHA1(f329db17aacacf1768ebd6ca2cc612503db93fac) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "rom3.u4", 0x00000, 0x80000, CRC(1ff5d35e) SHA1(b808eb4f81be8fc77a58dadd661a9cc2b376a509) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 )
	ROM_LOAD( "rom4.u3", 0x00000, 0x40000, CRC(7db45227) SHA1(2a12a2b8a1e58946ce3e7c770b3ca4803c3c3ccd) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 )
	ROM_LOAD( "rom5.u2", 0x00000, 0x80000, CRC(d609880e) SHA1(3d69800e959e8f24ef950fea4312610c4407f6ba) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom6.u1", 0x00000, 0x20000, CRC(31cfdc29) SHA1(725249eae9227eadf05418b799e0da0254bb2f51) )
ROM_END

GAME( 199?, dunhuang,  0,    dunhuang, dunhuang,  0, ROT0, "Unknown", "Mahjong Dunhuang", GAME_NOT_WORKING )
