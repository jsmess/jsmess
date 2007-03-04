/* Merit MSX Video based games

  The actual name of the hardware is unknown.
  Merit have several boards with the same basic specification.

  Main CPU: 1xZ80B
  Sound: 1xYM2149F
  Video: 2xYamaha V9938 (MSX2 video chip!)
  Other: 2xZ80APIO

  Other components vary, however, the MegaTouch games appear to use a Dallas chip for protection
  The later PCBs are of a different design use less roms of a higher capacity, however they still
  have the same basic chips.  Some of the readmes with the sets give inaccurate information.

  Known Games using this basic hardware:

  Type 1:
  Pitboss II (c)1988
  Super Pitboss (c)19??

  Type 2:
  Pitboss Megastar (c)1994

  Type 3:
  Megatouch 3 (c)1995
  Megatouch 5 (c)1997

  This is currently just a placeholder for games using this hardware, everything still to do

 */

#include "driver.h"

VIDEO_START(meritm)
{
	return 0;
}

VIDEO_UPDATE(meritm)
{
	return 0;
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



INPUT_PORTS_START( meritm )
INPUT_PORTS_END


static MACHINE_DRIVER_START( meritm )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,6000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
//  MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_PALETTE_LENGTH(0x100)

	/* no gfx decode, MSX video chip doesn't use tiles, roms are memory mapped */

	MDRV_VIDEO_START(meritm)
	MDRV_VIDEO_UPDATE(meritm)
MACHINE_DRIVER_END

/* Type 1 */

/*
    Pit Boss II - Merit Industries Inc. 1988
    ----------------------------------------

    All eproms are 27C512

    One 8 bank dip switch.

    Two YAMAHA V9938 Video Processors.

    21.47727 MHz Crystal

    CPU Z80

    Audio AY8930

    Two Z80A-PIO

    One bq4010YMA-150 NVRAM
    Eight V53C464AP80 (41464) RAMS

    One PAL16L8AN
    One PAL20L10NC
*/

ROM_START( pitboss2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "u9",  0x00000, 0x10000, CRC(a1b6ac15) SHA1(b7b395f3e7e14dbb84003e03bf7d054e795a7211) )
	ROM_LOAD( "u10", 0x10000, 0x10000, CRC(207aa83c) SHA1(1955d75b9e561312e98831571c9853579ded3734) )
	ROM_LOAD( "u11", 0x20000, 0x10000, CRC(2052e043) SHA1(36b6cbc5712fc736c748a68bd12675291eae669d) )
	ROM_LOAD( "u12", 0x30000, 0x10000, CRC(33653f16) SHA1(57b9822499324502d66dc5a40e662596e5336943) )
	ROM_LOAD( "u13", 0x40000, 0x10000, CRC(4f139e88) SHA1(425dd34804cc614aa93a468d2ba3e16de62f099c) )
	ROM_LOAD( "u14", 0x50000, 0x10000, CRC(a58078cd) SHA1(a028be67fa05670a689144dfb9c9da51c5732389) )
	ROM_LOAD( "u15", 0x60000, 0x10000, CRC(239b5d03) SHA1(fffb69cd7af215445da2b1281bcbc5f4fb6cfcc3) )
	ROM_LOAD( "u16", 0x70000, 0x10000, CRC(574fb3c7) SHA1(213741df3055b97ddd9889c2aa3d3e863e2c86d3) )
ROM_END

ROM_START( spitboss )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "u9-0a.rom",  0x00000, 0x10000, CRC(e0c45c9c) SHA1(534bff67c8fee08f1c348275de8977659efa9f69) )
	ROM_LOAD( "u10.rom",    0x10000, 0x10000, CRC(ed010c58) SHA1(02750944a28c1c27ce2a9904d11b7e46272a940e) )
	ROM_LOAD( "u11-0a.rom", 0x20000, 0x10000, CRC(0c65fa86) SHA1(7906a8d615116ca67bf370dfb2da8cb2389a313d) )
	ROM_LOAD( "u12.rom",    0x30000, 0x10000, CRC(0cf95b0e) SHA1(c6ffc13703892b9ae0da39a02db37c4ec890f79e) )
	ROM_LOAD( "u13",        0x40000, 0x10000, CRC(4f139e88) SHA1(425dd34804cc614aa93a468d2ba3e16de62f099c) ) // matches pitboss2
	ROM_LOAD( "u14",        0x50000, 0x10000, CRC(a58078cd) SHA1(a028be67fa05670a689144dfb9c9da51c5732389) ) // matches pitboss2
	ROM_LOAD( "u15",        0x60000, 0x10000, CRC(239b5d03) SHA1(fffb69cd7af215445da2b1281bcbc5f4fb6cfcc3) ) // matches pitboss2
	ROM_LOAD( "u16",        0x70000, 0x10000, CRC(574fb3c7) SHA1(213741df3055b97ddd9889c2aa3d3e863e2c86d3) ) // matches pitboss2
ROM_END

/* Type 2 */

/*
    Pit Boss Megastar - Merit Industries Inc. 1994

    Games are: Great Solitaire, Run 21 and Trivia Whiz 2000.
    -------------------------------------------------------
    Some of the pinouts probably flash lighted buttons as
    most Merit games have this feature.
    -------------------------------------------------------

    EPROMS 1,2, and 3 are 27C2001's
    EPROMS 4 through 9 are 27C512's

    One 8 bank dip switch.

    Two YAMAHA V9938 Video Processors.

    21.47727 MHz Crystal tied into pin 63 on both the V9938's

    CPU Z80B

    Audio YM2149F
    Two Z80A-PIO

    One Goldstar GM76C88L-15 (6264) SRAM
    Eight V53C464AP80 (41464) RAMS

    One PALCE16V8H-25PC/4
    One GAL22V10B

    chaneman Sept.23 2004
*/

ROM_START( pitbossm )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "4",  0x00000, 0x10000, CRC(55e14fb1) SHA1(ec29764d1b63360f64b82452e0db8054b99fcca0) )
	ROM_LOAD( "5",  0x10000, 0x10000, CRC(853a1a99) SHA1(45e33442aa7e51c05c9ac8b8458937ee3ff4c21d) )
	ROM_LOAD( "6",  0x20000, 0x10000, CRC(47a9dfc7) SHA1(eca100003f5605bcf405f610a0458ccb67894d35) )
	ROM_LOAD( "7",  0x30000, 0x10000, CRC(b9fb4203) SHA1(84b514d9739d9c2ab1081cfc7cdedb41155ee038) )
	ROM_LOAD( "8",  0x40000, 0x10000, CRC(574fb3c7) SHA1(213741df3055b97ddd9889c2aa3d3e863e2c86d3) ) // == u16 on pitboss2/spitboss
	ROM_LOAD( "9",  0x50000, 0x10000, CRC(27034061) SHA1(cff6be592a4a3ab01c204b081470f224e6186c4d) )

	ROM_REGION( 0xc0000, REGION_USER1, 0 ) // extra data / extra banks?
	ROM_LOAD( "1",  0x00000, 0x40000, CRC(590a1565) SHA1(b80ea967b6153847b2594e9c59bfe87559022b6c) )
	ROM_LOAD( "2",  0x40000, 0x40000, CRC(606f1656) SHA1(7f1e3a698a34d3c3b8f9f2cd8d5224b6c096e941) )
	ROM_LOAD( "3",  0x80000, 0x40000, CRC(35f4ca46) SHA1(87917b3017f505fae65d6bfa2c7d6fb503c2da6a) )
ROM_END

/* Type 3 */

/*
    Mega Touch 3
    by Merit Industries

    Dumped by NAZ!
    on 9/20/1998


    System Info
    -----------
     This is a counter top Touch screen game.

    processor.. Z80
    sound processor- YM2149
    other chips- two Yamaha V9938
             one LM1203
             one PC165500N
             one PB255a or L5220574
             One Dallas DS1204 Data Key
             One dallas DS1225Y 16k Non-volitile RAM
             Two Z80APIO
*/

ROM_START( megat3 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD( "megat3.u38",  0x00000, 0x80000, CRC(85f48b91) SHA1(7a38644ac7ee55a254c037122af919fb268744a1) )
	ROM_LOAD( "megat3.u37",  0x00000, 0x80000, CRC(96680164) SHA1(dfb8e07ba0e87316a947238e7a00fbf4d6ed5fe4) )
	ROM_LOAD( "megat3.u36",  0x00000, 0x80000, CRC(96bb501e) SHA1(f48ef238e8543676c42e3b85464a25ac179dcdd1) )

	ROM_REGION( 0x80000, REGION_USER1, 0 ) // extra data / extra banks?
	ROM_LOAD( "megat3.u32",  0x00000, 0x80000, CRC(ac969296) SHA1(7e09e9141637339b83c21f2488560cdf8a460069) )
ROM_END

/*
    Mega Touch 5
    by Merit Industries

    Dumped by NAZ!
    on 9/20/1998


    System Info
    -----------
     This is a counter top Touch screen game.

    processor.. Z80
    sound processor- YM2149
    other chips- two Yamaha V9938
             one LM1203
             one PC165500N
             one PB255a or L5220574
             One Dallas DS1204 Data Key
             One dallas DS1230Y 32k Non-volitile RAM
             Two Z80APIO
*/

ROM_START( megat5 )
	ROM_REGION( 0x180000, REGION_CPU1, 0 )
	ROM_LOAD( "megat5.u38",  0x00000, 0x80000, CRC(018e36c7) SHA1(8e9b457238a40b10d59887d13bac9c0a05c73614) )
	ROM_LOAD( "megat5.u37",  0x00000, 0x80000, CRC(b713a1c5) SHA1(d6ccba2ea90fd0e2ecf15249514231eed54000c1) )
	ROM_LOAD( "megat5.u36",  0x00000, 0x80000, CRC(0bed9e27) SHA1(1414385ce562b127e1ddeccc20ea4ff2a7098b7e) )

	ROM_REGION( 0x80000, REGION_USER1, 0 ) // extra data / extra banks?
	ROM_LOAD( "megat5.u32",  0x00000, 0x80000, CRC(89932443) SHA1(68d2fbf2a5050fc5371595a105fe06f4276b0b67) )
ROM_END


/* Type 1 */
GAME( 1988, pitboss2,  0,    meritm, meritm, 0, ROT0, "Merit", "Pitboss II", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 198?, spitboss,  0,    meritm, meritm, 0, ROT0, "Merit", "Super Pitboss", GAME_NO_SOUND|GAME_NOT_WORKING )
/* Type 2 */
GAME( 1994, pitbossm,  0,    meritm, meritm, 0, ROT0, "Merit", "Pitboss Megastar", GAME_NO_SOUND|GAME_NOT_WORKING )
/* Type 3 */
GAME( 1995, megat3,    0,    meritm, meritm, 0, ROT0, "Merit", "Megatouch 3", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 1997, megat5,    0,    meritm, meritm, 0, ROT0, "Merit", "Megatouch 5", GAME_NO_SOUND|GAME_NOT_WORKING )
