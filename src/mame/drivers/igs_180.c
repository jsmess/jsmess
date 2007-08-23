/* Some IGS games with a HD64180 */
/* also see iqblock.c and tarzan.c */

#include "driver.h"
#include "sound/2413intf.h"
#include "sound/okim6295.h"

VIDEO_START(igs_180)
{
}

VIDEO_UPDATE(igs_180)
{
	return 0;
}

INPUT_PORTS_START( igs_180 )
INPUT_PORTS_END

static ADDRESS_MAP_START( igs_180_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x07fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( igs_180_portmap, ADDRESS_SPACE_IO, 8 )
ADDRESS_MAP_END

static MACHINE_DRIVER_START( igs_180 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z180,16000000)	/* 16 MHz? */
	MDRV_CPU_PROGRAM_MAP(igs_180_map,0)
	MDRV_CPU_IO_MAP(igs_180_portmap,0)
//  MDRV_CPU_VBLANK_INT(interrupt,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(igs_180)
	MDRV_VIDEO_UPDATE(igs_180)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2413, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	MDRV_SOUND_ADD(OKIM6295, 16000000/16)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.47)
MACHINE_DRIVER_END


/*

IQ Block (alt hardware)
IGS, 1996

PCB Layout
----------

IGS PCB N0- 0131-4
|---------------------------------------|
|uPD1242H     VOL    U3567   3.579545MHz|
|                               AR17961 |
|   HD64180RP8                          |
|  16MHz                         BATTERY|
|                                       |
|                         SPEECH.U17    |
|                                       |
|J                        6264          |
|A                                      |
|M      8255              V.U18         |
|M                                      |
|A                                      |
|                                       |
|                                       |
|                      |-------|        |
|                      |       |        |
|       CG.U7          |IGS017 |        |
|                      |       |        |
|       TEXT.U8        |-------|   PAL  |
|            22MHz               61256  |
|                   DSW1  DSW2  DSW3    |
|---------------------------------------|
Notes:
      HD64180RP8 - Hitachi HD64180 CPU. Clocks 16MHz (pins 2 & 3), 8MHz (pin 64)
      61256   - 32k x8 SRAM (DIP28)
      6264    - 8k x8 SRAM (DIP28)
      IGS017  - Custom IGS IC (QFP208)
      AR17961 - == Oki M6295 (QFP44). Clock 1.000MHz [16/16]. pin 7 = high
      U3567   - == YM2413. Clock 3.579545MHz
      VSync   - 60Hz
      HSync   - 15.31kHz

  */

ROM_START( iqblocka )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DIP28 Code */
	ROM_LOAD( "v.u18", 0x00000, 0x40000, CRC(2e2b7d43) SHA1(cc73f4c8f9a6e2219ee04c9910725558a80b4eb2) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "speech.u17", 0x00000, 0x40000, CRC(d9e3d39f) SHA1(bec85d1ac2dfca77453cbca0e7dd53fee8fb438b) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROM_LOAD( "cg.u7",   0x000000, 0x080000, CRC(cb48a66e) SHA1(6d597193d1333a97957d5ceec8179a24bedfd928) )
	ROM_LOAD( "text.u8", 0x000000, 0x080000, CRC(48c4f4e6) SHA1(b1e1ca62cf6a99c11a5cc56705eef7e22a3b2740) )
ROM_END

ROM_START( iqblockf )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DIP28 Code */
	ROM_LOAD( "v113fr.u18", 0x00000, 0x40000, CRC(346c68af) SHA1(ceae4c0143c288dc9c1dd1e8a51f1e3371ffa439) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sp.u17", 0x00000, 0x40000, CRC(71357845) SHA1(25f4f7aebdcc0706018f041d3696322df569b0a3) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROM_LOAD( "cg.u7",   0x000000, 0x080000, CRC(cb48a66e) SHA1(6d597193d1333a97957d5ceec8179a24bedfd928) )
	ROM_LOAD( "text.u8", 0x000000, 0x080000, CRC(48c4f4e6) SHA1(b1e1ca62cf6a99c11a5cc56705eef7e22a3b2740) )
ROM_END

/*

Mahjong Tian Jiang Shen Bing
IGS, 1997

This PCB is almost the same as IQBlock (IGS, 1996)
but the 8255 has been replaced with the IGS025 IC

PCB Layout
----------

IGS PCB N0- 0157-2
|---------------------------------------|
|uPD1242H     VOL    U3567   3.579545MHz|
|                         AR17961       |
|   HD64180RP8                   SPDT_SW|
|  16MHz                         BATTERY|
|                                       |
|                         S0703.U15     |
|                                       |
|J     |-------|          6264          |
|A     |       |                        |
|M     |IGS025 |          P0700.U16     |
|M     |       |                        |
|A     |-------|                        |
|                                       |
|                                       |
|                      |-------|        |
|                      |       |   PAL  |
|       A0701.U3       |IGS017 |        |
|                      |       |   PAL  |
|       TEXT.U6        |-------|        |
|            22MHz               61256  |
|                   DSW1  DSW2  DSW3    |
|---------------------------------------|
Notes:
      HD64180RP8 - Hitachi HD64180 CPU. Clocks 16MHz (pins 2 & 3), 8MHz (pin 64)
      61256   - 32k x8 SRAM (DIP28)
      6264    - 8k x8 SRAM (DIP28)
      IGS017  - Custom IGS IC (QFP208)
      IGS025  - Custom IGS IC (PLCC68)
      AR17961 - == Oki M6295 (QFP44). Clock 1.000MHz [16/16]. pin 7 = high
      U3567   - == YM2413. Clock 3.579545MHz
      VSync   - 60Hz
      HSync   - 15.30kHz

*/

ROM_START( tjsb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DIP28 Code */
	ROM_LOAD( "p0700.u16", 0x00000, 0x40000,CRC(1b2a50df) SHA1(95a272e624f727df9523667864f933118d9e633c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "s0703.u15", 0x00000, 0x80000,  CRC(c6f94d29) SHA1(ec413580240711fc4977dd3c96c288501aa7ef6c) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "a0701.u3", 0x000000, 0x200000, CRC(aa182140) SHA1(37c2053386c183ff726ba417d13f2063cf9a22df) )
	ROM_LOAD( "text.u6", 0x000000, 0x080000,  CRC(3be886b8) SHA1(15b3624ed076640c1828d065b01306a8656f5a9b) )
ROM_END


 GAME( 1996, iqblocka, iqblock, igs_180, igs_180, 0, ROT0, "IGS", "IQ-Block (set 2)", GAME_NOT_WORKING )
 GAME( 1996, iqblockf, iqblock, igs_180, igs_180, 0, ROT0, "IGS", "IQ-Block (set 3, French?)", GAME_NOT_WORKING )
 GAME( 1997, tjsb,     0,       igs_180, igs_180, 0, ROT0, "IGS", "Mahjong Tian Jiang Shen Bing", GAME_NOT_WORKING )
