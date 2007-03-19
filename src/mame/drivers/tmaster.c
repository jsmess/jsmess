/* Touch Master */

/*

Main CPU: 68000
Sound Chip: OKI6295

Protection (some sets): DS1225a?

According to readme's TM3k might not be protected.

GFX might be some kind of blitter device, with memory mapped framebuffer?

*/

#include "driver.h"
#include "sound/okim6295.h"

VIDEO_START( tmaster )
{
	return 0;
}

VIDEO_UPDATE( tmaster )
{
	return 0;
}

READ16_HANDLER( tmaster_unk_0x300010_r )
{
	return mame_rand(Machine);
}

WRITE16_HANDLER( tmaster_unk_0x300020_w )
{

}

WRITE16_HANDLER( tmaster_unk_0x300040_w )
{

}

WRITE16_HANDLER( tmaster_unk_0x300070_w )
{

}

WRITE16_HANDLER( tmaster_unk_0x500010_w )
{

}

WRITE16_HANDLER( tmaster_unk_0x500020_w )
{

}

WRITE16_HANDLER( tmaster_unk_0x500000_range_w )
{

}

WRITE16_HANDLER( tmaster_unk_0x800010_w )
{

}

READ16_HANDLER( tmaster_unk_0x500010_r )
{
	return mame_rand(Machine);
}

static ADDRESS_MAP_START( tmaster_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x200000, 0x27ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x300010, 0x300011) AM_READ(tmaster_unk_0x300010_r)

	AM_RANGE(0x500010, 0x500011) AM_READ(tmaster_unk_0x500010_r)

	AM_RANGE(0x600000, 0x600fff) AM_READ(MRA16_RAM)

ADDRESS_MAP_END

static ADDRESS_MAP_START( tmaster_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x200000, 0x27ffff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x300020, 0x300021) AM_WRITE(tmaster_unk_0x300020_w)
	AM_RANGE(0x300040, 0x300041) AM_WRITE(tmaster_unk_0x300040_w)
	AM_RANGE(0x300070, 0x300071) AM_WRITE(tmaster_unk_0x300070_w)

	/* blitter writes? */
	AM_RANGE(0x500000, 0x50000f) AM_WRITE(tmaster_unk_0x500000_range_w)
	AM_RANGE(0x500010, 0x500011) AM_WRITE(tmaster_unk_0x500010_w)
	AM_RANGE(0x500020, 0x500021) AM_WRITE(tmaster_unk_0x500020_w)

	AM_RANGE(0x580000, 0x580001) AM_WRITE(MWA16_NOP) // often

	AM_RANGE(0x600000, 0x600fff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16) // looks like palettes, maybe

	AM_RANGE(0x800010, 0x800011) AM_WRITE(tmaster_unk_0x800010_w)
ADDRESS_MAP_END



INPUT_PORTS_START( tmaster )
    PORT_START
    PORT_DIPNAME( 0x0001, 0x0001, "DSW" )
    PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
    PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
    PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


/* Gfx are not tile based...*/
static const gfx_layout tmaster_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0,4,8,12,16,20,24,28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tmaster_layout,   0x300, 2  },
	{ REGION_GFX2, 0, &tmaster_layout,   0x300, 2  },
	{ REGION_GFX3, 0, &tmaster_layout,   0x300, 2  },
	{ -1 } /* end of array */
};

static INTERRUPT_GEN( tmaster_interrupt )
{
	cpunum_set_input_line(0, cpu_getiloops()+1, HOLD_LINE);	/* IRQs 1, 2, and 3 */
}

static MACHINE_DRIVER_START( tmaster )
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)	 // or 6mhz?
	MDRV_CPU_PROGRAM_MAP(tmaster_readmem,tmaster_writemem)
	MDRV_CPU_VBLANK_INT(tmaster_interrupt,3) // ??
	/* irqs 1,2,3 look valid */

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(tmaster)
	MDRV_VIDEO_UPDATE(tmaster)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 1122000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.47)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.47)
MACHINE_DRIVER_END

/*
Touch Master
1996, Midway

68000 @ 12MHz or 6MHz
u51 - u52 program code
u36 -> u39 gfx
u8 sound
OKI6295
NVSRAM DS1225a
Philips SCN68681
Xlinx XC3042a

Dumped by ANY
*/

ROM_START( tm )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "tmaster.u52", 0x000001, 0x080000, CRC(e9fd30fc) SHA1(d91ea05d5f574603883336729fb9df705688945d) )
	ROM_LOAD16_BYTE( "tmaster.u51", 0x000000, 0x080000, CRC(edaa5874) SHA1(48b99bc7f5a6453def265967ca7d8eefdf9dc97b) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "tmaster.u8", 0x00000, 0x080000, CRC(f39ad4cf) SHA1(9bcb9a5dd3636d6541eeb3e737c7253ab0ed4e8d) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "tmaster.u36", 0x000000, 0x080000, CRC(204096ec) SHA1(9239923b7eedb6003c63ef2e8ff224edee657bbc) )
	ROM_LOAD16_BYTE( "tmaster.u38", 0x000001, 0x080000, CRC(68885ef6) SHA1(010602b59c33c3e490491a296ddaf8952e315b83) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "tmaster.u37", 0x000000, 0x080000, CRC(e0b6a9f7) SHA1(7e057ca87833c682e5be03668469259bbdefbf20) )
	ROM_LOAD16_BYTE( "tmaster.u39", 0x000001, 0x080000, CRC(cbb716cb) SHA1(4e8d8f6cbfb25a8161ff8fe7505d6b209650dd2b) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_ERASEFF )
ROM_END

/*
Touchmaster 3000
by Midway
touchscreen game
Dumped BY: N?Z!

All chips are SGS 27C801
---------------------------

Name_Board Location        Version               Use                      Checksum
-----------------------------------------------------------------------------------
TM3K_u8.bin                5.0  Audio Program & sounds          64d5
TM3K_u51.bin               5.01 Game Program & Cpu instructions 0c6c
TM3K_u52.bin               5.01 Game Program & Cpu instructions b2d8
TM3K_u36.bin               5.0  Video Images & Graphics         54f1
TM3K_u37.bin               5.0  Video Images & Graphics         4856
TM3K_u38.bin               5.0  Video Images & Graphics         5493
TM3K_u39.bin               5.0  Video Images & Graphics         6029
TM3K_u40.bin               5.0  Video Images & Graphics         ccb4
TM3K_u41.bin               5.0  Video Images & Graphics         54a7
u62 (NOT INCLUDED)         N/A  Battery Memory Module           N/A
J12 (NOT INCLUDED)         N/A  Security Key(not required for this Version)
-----------------------------------------------------------------------------------


The TOUCHMASTER 3000 is a bar top game, with many different kinds of games in it
including strip poker and Adult content (cool!). It does use a touchscreen but
I'm sure a EMU can make this Transition to a mouse pointer.

CPU- SCN68681c1n40
sound processor-xc3042A      www.xilinx.com
*/

ROM_START( tm3k )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "tm3k_u52.bin", 0x000001, 0x100000, CRC(8c6a0db7) SHA1(6b0eae60ea471cd8c4001749ac2677d8d4532567) )
	ROM_LOAD16_BYTE( "tm3k_u51.bin", 0x000000, 0x100000, CRC(c9522279) SHA1(e613b791f831271722f05b7e96c35519fa9fc174) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "tm3k_u8.bin", 0x00000, 0x100000, CRC(d0ae33c1) SHA1(a079def9a086a091fcc4493a44fec756d2470415) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "tm3k_u36.bin", 0x000000, 0x100000, CRC(7bde520d) SHA1(77750b689e2f0d47804042456e54bbd9c28deeac) )
	ROM_LOAD16_BYTE( "tm3k_u38.bin", 0x000001, 0x100000, CRC(a6683899) SHA1(d05024390917cdb1871d030996da8e1eb6460918) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "tm3k_u37.bin", 0x000000, 0x100000, CRC(18f50eb3) SHA1(a7c9d3b24b5fd110380ec87d9200d55cad473efc) )
	ROM_LOAD16_BYTE( "tm3k_u39.bin", 0x000001, 0x100000, CRC(206b56a6) SHA1(09e5e05bffd0a09abd24d668e2c59b56f2c79134) )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "tm3k_u40.bin", 0x000000, 0x100000, CRC(353df7ca) SHA1(d6c5d5449af6b6a3acee219778583904c5b554b4) )
	ROM_LOAD16_BYTE( "tm3k_u41.bin", 0x000001, 0x100000, CRC(74a36bca) SHA1(7ad594daa156dea40a25b390f26c2fd0550e66ff) )
ROM_END

/*
Touchmaster 4000
by Midway
touchscreen game
Dumped BY: N?Z!

All chips are SGS 27C801
---------------------------

Name_Board Location        Version               Use                      Checksum
-----------------------------------------------------------------------------------
TM4K_u8.bin                6.0  Audio Program & sounds          DE0B
TM4K_u51.bin               6.02 Game Program & Cpu instructions FEA0
TM4K_u52.bin               6.02 Game Program & Cpu instructions 9A71
TM4K_u36.bin               6.0  Video Images & Graphics         54f1
TM4K_u37.bin               6.0  Video Images & Graphics         609E
TM4K_u38.bin               6.0  Video Images & Graphics         5493
TM4K_u39.bin               6.0  Video Images & Graphics         CB90
TM4K_u40.bin               6.0  Video Images & Graphics         208A
TM4K_u41.bin               6.0  Video Images & Graphics         385D
u62 (NOT INCLUDED)         N/A  Battery Memory Module           N/A
J12 (NOT INCLUDED)         N/A  Security Key(required for this Version)
-----------------------------------------------------------------------------------


The TOUCHMASTER 4000 is a bar top game, with many different kinds of games in it
including strip poker and Adult content (cool!). It does use a touchscreen but
I'm sure a EMU can make this Transition to a mouse pointer.

CPU- SCN68681c1n40
sound processor-xc3042A      www.xilinx.com
*/

ROM_START( tm4k )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "tm4k_u52.bin", 0x000001, 0x100000, CRC(6d412871) SHA1(ae27c7723b292daf6682c53bafac22e4a3cd1ece) )
	ROM_LOAD16_BYTE( "tm4k_u51.bin", 0x000000, 0x100000, CRC(3d8d7848) SHA1(31638f23cdd5e6cfbb2270e953f84fe1bd437950) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "tm4k_u8.bin", 0x00000, 0x100000, CRC(48c3782b) SHA1(bfe105ddbde8bbbd84665dfdd565d6d41926834a) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* these match tm3k */
	ROM_LOAD16_BYTE( "tm4k_u36.bin", 0x000000, 0x100000, CRC(7bde520d) SHA1(77750b689e2f0d47804042456e54bbd9c28deeac) )
	ROM_LOAD16_BYTE( "tm4k_u38.bin", 0x000001, 0x100000, CRC(a6683899) SHA1(d05024390917cdb1871d030996da8e1eb6460918) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "tm4k_u37.bin", 0x000000, 0x100000, CRC(bf49fafa) SHA1(b400667bf654dc9cd01a85c8b99670459400fd60) )
	ROM_LOAD16_BYTE( "tm4k_u39.bin", 0x000001, 0x100000, CRC(bac88cfb) SHA1(26ed169296b890c5f5b50c418c15299355a6592f) )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "tm4k_u40.bin", 0x000000, 0x100000, CRC(f6771a09) SHA1(74f71d5e910006c83a38170f24aa811c38a3e020) )
	ROM_LOAD16_BYTE( "tm4k_u41.bin", 0x000001, 0x100000, CRC(e97edb1e) SHA1(75510676cf1692ad03efd4ccd57d25af1cc8ef2a) )
ROM_END

GAME( 1996, tm,      0,        tmaster,    tmaster,    0, ROT0,  "Midway", "Touchmaster", GAME_NOT_WORKING )
GAME( 1997, tm3k,    0,        tmaster,    tmaster,    0, ROT0,  "Midway", "Touchmaster 3000 (v5.01)", GAME_NOT_WORKING )
GAME( 1998, tm4k,    0,        tmaster,    tmaster,    0, ROT0,  "Midway", "Touchmaster 4000 (v6.02)", GAME_NOT_WORKING )
