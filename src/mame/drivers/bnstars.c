/*
Vs. Janshi Brandnew Stars
(c)1997 Jaleco

Single board version with Dual Screen output
(MS32 version also exists)

Main PCB
--------

PCB ID  : MB-93141 EB91022-20081
CPU     : NEC D70632GD-20 (V70)
OSC     : 48.000MHz, 40.000MHz
RAM     : Cypress CY7C199-25 (x20)
          OKI M511664-80 (x8)
DIPs    : 8 position (x3)
OTHER   : Some PALs

          Custom chips:
                       JALECO SS91022-01 (208 PIN PQFP)
                       JALECO SS91022-02 (100 PIN PQFP) (x2)
                       JALECO SS91022-03 (176 PIN PQFP) (x2)
                       JALECO SS91022-05 (120 PIN PQFP) (x2)
                       JALECO SS91022-07 (208 PIN PQFP) (x2)
                       JALECO GS91022-01 (120 PIN PQFP)
                       JALECO GS91022-02 (160 PIN PQFP)
                       JALECO GS91022-03 (100 PIN PQFP)
                       JALECO GS91022-04 (100 PIN PQFP)
                       JALECO GS90015-03 ( 80 PIN PQFP) (x3) (not present on MS32)


ROMs:     None


ROM PCB (equivalent to MS32 cartridge)
--------------------------------------
PCB ID  : MB-93142 EB93007-20082
DIPs    : determine the size of ROMs?
          8 position (x1, 6 and 8 is on, others are off)
          4 position (x2, both are off on off off)
OTHER   : Some PALs
          Custom chip: JALECO SS92046-01 (144 pin PQFP) (x2)
          (located on small plug-in board with) (ID: SE93139 EB91022-30056)
ROMs    : MB-93142.36   [2eb6a503] (IC42, also printed "?u?????j???[Ver1.2")
          MB-93142.37   [49f60882] (IC57, also printed "?u?????j???[Ver1.2")
          MB-93142.38   [6e1312cd] (IC58, also printed "?u?????j???[Ver1.2")
          MB-93142.39   [56b98539] (IC59, also printed "?u?????j???[Ver1.2")

          VSJANSHI5.6   [fdbbac21] (IC9, actual label is "VS?W?????V 5 Ver1.0")
          VSJANSHI6.5   [fdbbac21] (IC8, actual label is "VS?W?????V 6 Ver1.0")

?@?@?@?@?@MR96004-01.20 [3366d104] (IC29)
?@?@?@?@?@MR96004-02.28 [ad556664] (IC49)
?@?@?@?@?@MR96004-03.21 [b399e2b1] (IC30)
?@?@?@?@?@MR96004-04.29 [f4f4cf4a] (IC50)
?@?@?@?@?@MR96004-05.22 [cd6c357e] (IC31)
?@?@?@?@?@MR96004-06.30 [fc6daad7] (IC51)
?@?@?@?@?@MR96004-07.23 [177e32fa] (IC32)
?@?@?@?@?@MR96004-08.31 [f6df27b2] (IC52)

?@?@?@?@?@MR96004-09.1  [603143e8] (IC4)
?@?@?@?@?@MR96004-09.7  [603143e8] (IC10)

?@?@?@?@?@MR96004-11.11 [e6da552c] (IC17)
?@?@?@?@?@MR96004-11.13 [e6da552c] (IC19)


Sound PCB
---------
PCB ID  : SB-93143 EB91022-20083
SOUND   : Z80, YMF271-F, YAC513(x2)
OSC     : 8.000MHz, 16.9344MHz
DIPs    : 4 position (all off)
ROMs    : SB93145.5     [0424e899] (IC34, also printed "Ver1.0") - Sound program
          MR96004-10.1  [125661cd] (IC19 - Samples)

Sound sub PCB
-------------
PCB ID  : SB-93145 EB93007-20086
SOUND   : YMF271-F
ROMs    : MR96004-10.1  [125661cd] (IC5 - Samples)

*/

#include "driver.h"
#include "sound/ymf271.h"
#include "rendlay.h"

/* drivers/ms32/c */
extern void ms32_rearrange_sprites(int region);
extern void decrypt_ms32_tx(int addr_xor,int data_xor, int region);
extern void decrypt_ms32_bg(int addr_xor,int data_xor, int region);

VIDEO_START(bnstars)
{

}

VIDEO_UPDATE(bnstars)
{
	return 0;
}

INPUT_PORTS_START( bnstars )
INPUT_PORTS_END



/* sprites are contained in 256x256 "tiles" */
static const gfx_layout spritelayout =
{
	256,256,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 256*8 },	/* line modulo */
	256*256*8	/* char modulo */
};

static const gfx_layout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 16*8 },	/* line modulo */
	16*16*8		/* char modulo */
};


static const gfx_layout txlayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	8*8*8		/* char modulo */
};

static GFXDECODE_START( bnstars )
	GFXDECODE_ENTRY( REGION_GFX1, 0, spritelayout, 0x0000, 0x10 )

	GFXDECODE_ENTRY( REGION_GFX2, 0, bglayout,     0x0000, 0x10 ) /* Roz scr1 */
	GFXDECODE_ENTRY( REGION_GFX3, 0, bglayout,     0x0000, 0x10 ) /* Roz scr2 */

	GFXDECODE_ENTRY( REGION_GFX4, 0, bglayout,     0x0000, 0x10 ) /* Bg scr1 */
	GFXDECODE_ENTRY( REGION_GFX5, 0, txlayout,     0x0000, 0x10 ) /* Tx scr1 */

	GFXDECODE_ENTRY( REGION_GFX6, 0, bglayout,     0x0000, 0x10 ) /* Bg scr2 */
	GFXDECODE_ENTRY( REGION_GFX7, 0, txlayout,     0x0000, 0x10 ) /* Tx scr2 */

GFXDECODE_END



static ADDRESS_MAP_START( bnstars_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM
	AM_RANGE(0xffe00000, 0xffffffff) AM_READWRITE(MRA32_BANK1, MWA32_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bnstars_z80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static struct YMF271interface ymf271_interface1 =
{
	REGION_SOUND1
};

static struct YMF271interface ymf271_interface2 =
{
	REGION_SOUND2
};

static MACHINE_DRIVER_START( bnstars )

	/* basic machine hardware */
	MDRV_CPU_ADD(V70, 20000000) // 20MHz
	MDRV_CPU_PROGRAM_MAP(bnstars_map,0)
//  MDRV_CPU_VBLANK_INT(ms32_interrupt,32)

	MDRV_CPU_ADD(Z80, 4000000) /* audio CPU */
	MDRV_CPU_PROGRAM_MAP(bnstars_z80_map, 0)

	MDRV_INTERLEAVE(1000)

//  MDRV_MACHINE_RESET(ms32)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_GFXDECODE(bnstars)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_DEFAULT_LAYOUT(layout_dualhsxs)

	MDRV_SCREEN_ADD("left", 0x000)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)

	MDRV_SCREEN_ADD("right", 0x000)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)

	MDRV_VIDEO_START(bnstars)
	MDRV_VIDEO_UPDATE(bnstars)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMF271, 16934400)
	MDRV_SOUND_CONFIG(ymf271_interface1)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(YMF271, 16934400)
	MDRV_SOUND_CONFIG(ymf271_interface2)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

MACHINE_DRIVER_END


ROM_START( bnstars1 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* V70 code */
	ROM_LOAD32_BYTE( "mb-93142.36", 0x000003, 0x80000, CRC(2eb6a503) SHA1(27c02ab1b4321924fd4499844467ea4dc97de25d) )
	ROM_LOAD32_BYTE( "mb-93142.37", 0x000002, 0x80000, CRC(49f60882) SHA1(2ff5b0989aaf970103304a453773e0b9517ebb8d) )
	ROM_LOAD32_BYTE( "mb-93142.38", 0x000001, 0x80000, CRC(6e1312cd) SHA1(4c22f8f9f1574eefd96147453cf240f50c17f5dc) )
	ROM_LOAD32_BYTE( "mb-93142.39", 0x000000, 0x80000, CRC(56b98539) SHA1(5eb0e77729b31e6a100c1b43813a39fea57bedee) )

	/* Sprites - shared by both screens? */
	ROM_REGION( 0x1000000, REGION_GFX1, 0 ) /* sprites, don't dispose since we use GFX_RAW */
	ROM_LOAD32_WORD( "mr96004-01.20",   0x0000000, 0x200000, CRC(3366d104) SHA1(2de0cabe2ead777b5b02cade7f2003ef7f90b75b) )
	ROM_LOAD32_WORD( "mr96004-02.28",   0x0000002, 0x200000, CRC(ad556664) SHA1(4b36f8d8d9efa37cf515af41d14433e7eafa27a2) )
	ROM_LOAD32_WORD( "mr96004-03.21",   0x0400000, 0x200000, CRC(b399e2b1) SHA1(9b6a00a219db8d66dcf592160b7b5f7a86b8f0c9) )
	ROM_LOAD32_WORD( "mr96004-04.29",   0x0400002, 0x200000, CRC(f4f4cf4a) SHA1(fe497989cf96c68602f68f14920aed44fd934573) )
	ROM_LOAD32_WORD( "mr96004-05.22",   0x0800000, 0x200000, CRC(cd6c357e) SHA1(44cd2d0607c7ccd80f701cf1675fd283acb07252) )
	ROM_LOAD32_WORD( "mr96004-06.30",   0x0800002, 0x200000, CRC(fc6daad7) SHA1(99f14ac6b06ad9a8a3d2e9f69b693c7ce420a47d) )
	ROM_LOAD32_WORD( "mr96004-07.23",   0x0c00000, 0x200000, CRC(177e32fa) SHA1(3ca1f397dc28f1fa3a4136705b92c63e4e438f05) )
	ROM_LOAD32_WORD( "mr96004-08.31",   0x0c00002, 0x200000, CRC(f6df27b2) SHA1(60590976020d86bdccd4eaf57b349ea31bec6830) )

	/* Roz Tiles #1 (Screen 1) */
	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr96004-09.1", 0x000000, 0x200000, BAD_DUMP CRC(603143e8) SHA1(197ff72d4a6bf9cc5e89505c1a25edad104a9a65) )

	/* Roz Tiles #2 (Screen 2) */
	ROM_REGION( 0x200000, REGION_GFX3, 0 ) /* roz tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr96004-09.7", 0x000000, 0x200000, BAD_DUMP CRC(603143e8) SHA1(197ff72d4a6bf9cc5e89505c1a25edad104a9a65) )

	/* BG Tiles #1 (Screen 1?) */
	ROM_REGION( 0x200000, REGION_GFX4, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr96004-11.11", 0x000000, 0x200000,  CRC(e6da552c) SHA1(69a5af3015883793c7d1343243ccae23db9ef77c) )

	/* TX Tiles #1 (Screen 1?) */
	ROM_REGION( 0x080000, REGION_GFX5, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "vsjanshi6.5", 0x000000, 0x080000, CRC(fdbbac21) SHA1(c77d852e53126cc8ebfe1e79d1134e42b54d1aab) )

	/* BG Tiles #2 (Screen 1?) */
	ROM_REGION( 0x200000, REGION_GFX6, 0 ) /* bg tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "mr96004-11.13", 0x000000, 0x200000, CRC(e6da552c) SHA1(69a5af3015883793c7d1343243ccae23db9ef77c) )

	/* TX Tiles #2 (Screen 2?) */
	ROM_REGION( 0x080000, REGION_GFX7, 0 ) /* tx tiles, don't dispose since we use GFX_RAW */
	ROM_LOAD( "vsjanshi5.6", 0x000000, 0x080000, CRC(fdbbac21) SHA1(c77d852e53126cc8ebfe1e79d1134e42b54d1aab) )

	/* Sound Program (one, driving both screen sound) */
	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* z80 program */
	ROM_LOAD( "sb93145.5",  0x000000, 0x040000, CRC(0424e899) SHA1(fbcdebfa3d5f52b10cf30f7e416f5f53994e4d55) )
	ROM_RELOAD(              0x010000, 0x40000 )

	/* Samples #1 (Screen 1?) */
	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* samples - 8-bit signed PCM */
	ROM_LOAD( "mr96004-10.1",  0x000000, 0x200000, BAD_DUMP CRC(125661cd) SHA1(7c5fd952fb756364cc0ab6a53072f5cf8a04486c) )

	/* Samples #2 (Screen 2?) */
	ROM_REGION( 0x400000, REGION_SOUND2, 0 ) /* samples - 8-bit signed PCM */
	ROM_LOAD( "mr96004-10.1",  0x000000, 0x200000, BAD_DUMP CRC(125661cd) SHA1(7c5fd952fb756364cc0ab6a53072f5cf8a04486c) )
ROM_END


/* SS92046_01: bbbxing, f1superb, tetrisp, hayaosi1 */
static DRIVER_INIT (bnstars)
{
	ms32_rearrange_sprites(REGION_GFX1);

	decrypt_ms32_tx(0x00020,0x7e, REGION_GFX5);
	decrypt_ms32_bg(0x00001,0x9b, REGION_GFX4);
	decrypt_ms32_tx(0x00020,0x7e, REGION_GFX7);
	decrypt_ms32_bg(0x00001,0x9b, REGION_GFX6);

	memory_set_bankptr(1, memory_region(REGION_CPU1));
}

GAME( 1997, bnstars1, 0,        bnstars, bnstars, bnstars, ROT0,   "Jaleco", "Vs. Janshi Brandnew Stars", GAME_NOT_WORKING|GAME_NO_SOUND )
