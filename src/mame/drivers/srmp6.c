/*
    Super Real Mahjong P6 (JPN Ver.)
    (c)1996 Seta

WIP driver by Sebastien Volpe

according prg ROM (offset $0fff80):

    S12 SYSTEM
    SUPER REAL MAJAN P6
    SETA CO.,LTD
    19960410
    V1.00

TODO:
 - gfx decoding / emulation
 - sound emulation (appears to be really close to st0016)

Are there other games on this 'System S12' hardware ???

---------------- dump infos ----------------

[Jun/15/2000]

Super Real Mahjong P6 (JPN Ver.)
(c)1996 Seta

SX011
E47-REV01B

CPU:    68000-16
Sound:  NiLe
OSC:    16.0000MHz
        42.9545MHz
        56.0000MHz

Chips:  ST-0026 NiLe (video, sound)
        ST-0017


SX011-01.22  chr, samples (?)
SX011-02.21
SX011-03.20
SX011-04.19
SX011-05.18
SX011-06.17
SX011-07.16
SX011-08.15

SX011-09.10  68000 data

SX011-10.4   68000 prg.
SX011-11.5


Dumped 06/15/2000
*/


#include "driver.h"


/***************************************************************************
    video - VERY preliminary
***************************************************************************/

VIDEO_START(srmp6)
{
	return 0;
}

/* sprite RAM format: 8 words, 1st word = 0x8000 means end of list

Offset:         Format:                     Value:

0000.w
                    f--- ---- ---- ----     End of list
                    -??? ???? ???? ???-
                    ---- ---- ---- ---0     Sprite visible?

0002.w                                      Code
0004.w                                      X Position
0006.w                                      Y Position

0008.w                                      attributes/color?
000a.w                                      attributes/color?

000c.l                                      used ?

cpu #0 (PC=00011B2A): unmapped program memory word write to 00400490 = 0001 & FFFF - sprite visible ?
cpu #0 (PC=00011B2E): unmapped program memory word write to 00400492 = 0233 & FFFF - code
cpu #0 (PC=00011B32): unmapped program memory word write to 00400494 = 00D8 & FFFF - x
cpu #0 (PC=00011B36): unmapped program memory word write to 00400496 = 0080 & FFFF - y
cpu #0 (PC=00011B3A): unmapped program memory word write to 00400498 = E000 & FFFF - attributes/color?
cpu #0 (PC=00011B3E): unmapped program memory word write to 0040049A = 0000 & FFFF - attributes/color?
cpu #0 (PC=00011B40): unmapped program memory word write to 0040049C = 0000 & FFFF -\
cpu #0 (PC=00011B40): unmapped program memory word write to 0040049E = 0000 & FFFF -/ used ?

cpu #0 (PC=00011B4E): unmapped program memory word write to 004004B0 = 8000 & FFFF - end of list
*/

/* tile RAM format: where's color ?

cpu #0 (PC=000118E8): unmapped program memory word write to 00412002 = 0005 & FFFF - kind of init, interleaved with 'meaningful' words
cpu #0 (PC=000118EE): unmapped program memory word write to 00412008 = 0000 & FFFF
cpu #0 (PC=000118F4): unmapped program memory word write to 0041200A = 0000 & FFFF
...
cpu #0 (PC=000118E8): unmapped program memory word write to 00416EB2 = 0005 & FFFF
cpu #0 (PC=000118EE): unmapped program memory word write to 00416EB8 = 0000 & FFFF
cpu #0 (PC=000118F4): unmapped program memory word write to 00416EBA = 0000 & FFFF


cpu #0 (PC=0001170A): unmapped program memory word write to 00412000 = 4004 & FFFF
cpu #0 (PC=00011712): unmapped program memory word write to 00412004 = 0000 & FFFF
cpu #0 (PC=0001171A): unmapped program memory word write to 00412006 = 0000 & FFFF
...
cpu #0 (PC=0001170A): unmapped program memory word write to 004133A0 = 4004 & FFFF - tile code [*]
cpu #0 (PC=00011712): unmapped program memory word write to 004133A4 = 0140 & FFFF - x (00..140, step 16d) -> res = 320d
cpu #0 (PC=0001171A): unmapped program memory word write to 004133A6 = 00E0 & FFFF - y (00..F0, step 16d)  -> res = 240d

[*] tile code is 'incremented by 4', suggesting there are 4 8x8 tiles arranged to make a 16x16 one ???
*/

VIDEO_UPDATE(srmp6)
{
	const UINT16 *source = spriteram16;
	const UINT16 *finish = source+spriteram_size/2;
	const gfx_element *gfx = Machine->gfx[0];

	fillbitmap(bitmap, get_black_pen(machine), cliprect);

	// parse sprite list
	while( source<finish )
	{
		int num  = source[1];
		int xpos = source[2];
		int ypos = source[3];
		int color = 0;
		int flipx = 0;
		int flipy = 0;

		if (source[0]&0x8000) break;	// end of list

		drawgfx(
				bitmap,
				gfx,
				num,
				color,
				flipx,flipy,
				xpos,ypos,
				cliprect,
				TRANSPARENCY_PEN,0
				);

		source += 0x8;
	}
	return 0;
}

/***************************************************************************
    audio - VERY preliminary

    TODO: watch similarities with st0016
***************************************************************************/

/*
cpu #0 (PC=00011F7C): unmapped program memory word write to 004E0002 = 0000 & FFFF  0?
cpu #0 (PC=00011F84): unmapped program memory word write to 004E0004 = 0D50 & FFFF  lo word \ sample start
cpu #0 (PC=00011F8A): unmapped program memory word write to 004E0006 = 0070 & FFFF  hi word / address?

cpu #0 (PC=00011FBA): unmapped program memory word write to 004E000A = 0000 & FFFF  0?
cpu #0 (PC=00011FC2): unmapped program memory word write to 004E0018 = 866C & FFFF  lo word \ sample stop
cpu #0 (PC=00011FC8): unmapped program memory word write to 004E001A = 00AA & FFFF  hi word / address?

cpu #0 (PC=00011FD0): unmapped program memory word write to 004E000C = 0FFF & FFFF
cpu #0 (PC=00011FD8): unmapped program memory word write to 004E001C = FFFF & FFFF
cpu #0 (PC=00011FDC): unmapped program memory word write to 004E001E = FFFF & FFFF

cpu #0 (PC=00026EFA): unmapped program memory word write to 004E0100 = 0001 & FFFF  ctrl word r/w: bit 7-0 = voice 8-1 (1=play,0=stop)

voice #1: $4E0000-$4E001F
voice #2: $4E0020-$4E003F
voice #3: $4E0040-$4E005F
...
voice #8: $4E00E0-$4E00FF

voice regs:
offset  description
+00
+02     always 0?
+04     lo-word \ sample
+06     hi-word / counter ?
+0A
+18 \
+1A /
+1C \
+1E /

*/

/***************************************************************************
    Main CPU memory handlers
***************************************************************************/

static UINT16 srmp6_input_select = 0;

WRITE16_HANDLER( srmp6_input_select_w )
{
	srmp6_input_select = data & 0x0f;
}

READ16_HANDLER( srmp6_inputs_r )
{
	if (offset == 0)			// DSW
		return readinputport(4);

	switch(srmp6_input_select)	// inputs
	{
		case 1<<0: return readinputport(0);
		case 1<<1: return readinputport(1);
		case 1<<2: return readinputport(2);
		case 1<<3: return readinputport(3);
	}

	return 0;
}


static UINT16 *video_regs;

WRITE16_HANDLER( video_regs_w )
{
	switch(offset)
	{

		case 0x5e/2: // bank switch, used by ROM check
			memory_set_bankptr(1,(UINT16 *)(memory_region(REGION_USER2) + (data & 0x0f)*0x200000));
			break;


		/* unknown registers - there are others */

		// set by IT4 (jsr $b3c), according flip screen dsw
		case 0x48/2: //     0 /  0xb0 if flipscreen
		case 0x52/2: //     0 / 0x2ef if flipscreen
		case 0x54/2: // 0x152 / 0x15e if flipscreen

		// set by IT4 ($82e-$846)
		case 0x56/2: // written 8,9,8,9 successively

		// set by IT4
		case 0x5c/2: // either 0x40 explicitely in many places, or according $2083b0 (IT4)

		default:
			logerror("video_regs_w (PC=%06X): %04x = %04x & %04x\n", activecpu_get_previouspc(), offset*2, data, mem_mask);
			break;
	}
	COMBINE_DATA(&video_regs[offset]);
}

READ16_HANDLER( video_regs_r )
{
	logerror("video_regs_r (PC=%06X): %04x\n", activecpu_get_previouspc(), offset*2);
	return video_regs[offset];
}


static ADDRESS_MAP_START( srmp6, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x200000, 0x23ffff) AM_RAM					// work RAM
	AM_RANGE(0x600000, 0x7fffff) AM_READ(MRA16_BANK1)	// banked ROM (used by ROM check)
	AM_RANGE(0x800000, 0x9fffff) AM_ROM AM_REGION(REGION_USER1, 0)

	AM_RANGE(0x300000, 0x300005) AM_READWRITE(srmp6_inputs_r, srmp6_input_select_w)		// inputs
	AM_RANGE(0x480000, 0x480fff) AM_READWRITE(MRA16_RAM, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x4d0000, 0x4d0001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// watchdog

	// OBJ RAM: checked [$400000-$47dfff]
	AM_RANGE(0x400000, 0x401fff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)	// 512 sprites
	AM_RANGE(0x402000, 0x47dfff) AM_RAM // tiles? layout is different from sprites
//  AM_RANGE(0x402000, 0x40200f) AM_RAM // set by routine $11c48
//  AM_RANGE(0x402010, 0x40806f) AM_RAM // set by routine $11bf6 - 2nd sprite RAM maybe ?
//  AM_RANGE(0x412000, 0x416ebf) AM_RAM // probably larger

	// CHR RAM: checked [$500000-$5fffff]
	AM_RANGE(0x500000, 0x5fffff) AM_RAM
//  AM_RANGE(0x500000, 0x505cff) AM_RAM
//  AM_RANGE(0x505d00, 0x5fffff) AM_RAM

	AM_RANGE(0x4c0000, 0x4c006f) AM_READWRITE(video_regs_r, video_regs_w) AM_BASE(&video_regs)	// ? gfx regs ST-0026 NiLe
//  AM_RANGE(0x4e0000, 0x4e00ff) AM_READWRITE(sound_regs_r, sound_regs_w) AM_BASE(&sound_regs)  // ? sound regs (data) ST-0026 NiLe
//  AM_RANGE(0x4e0100, 0x4e0101) AM_READWRITE(sndctrl_reg_r, sndctrl_reg_w)                     // ? sound reg  (ctrl) ST-0026 NiLe
//  AM_RANGE(0x4e0110, 0x4e0111) AM_NOP // ? accessed once ($268dc, written $b.w)
//  AM_RANGE(0x5fff00, 0x5fff1f) AM_RAM // ? see routine $5ca8, video_regs related ???


//  AM_RANGE(0xf00004, 0xf00005) AM_RAM // ?
//  AM_RANGE(0xf00006, 0xf00007) AM_RAM // ?

ADDRESS_MAP_END


/***************************************************************************
    Port definitions
***************************************************************************/

INPUT_PORTS_START( srmp6 )

	PORT_START
	PORT_BIT ( 0xfe01, IP_ACTIVE_LOW, IPT_UNUSED ) // explicitely discarded
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_A )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_E )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_I )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_M )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_KAN )
	PORT_BIT ( 0x0040, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT ( 0x0100, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME(DEF_STR( Test )) PORT_CODE(KEYCODE_F2)

	PORT_START
	PORT_BIT ( 0xfe41, IP_ACTIVE_LOW, IPT_UNUSED ) // explicitely discarded
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_B )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_F )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_J )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_N )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_REACH )
	PORT_BIT ( 0x0180, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT ( 0xfe41, IP_ACTIVE_LOW, IPT_UNUSED ) // explicitely discarded
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_C )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_G )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_K )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_CHI )
	PORT_BIT ( 0x0020, IP_ACTIVE_LOW, IPT_MAHJONG_RON )
	PORT_BIT ( 0x0180, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT ( 0xfe61, IP_ACTIVE_LOW, IPT_UNUSED ) // explicitely discarded
	PORT_BIT ( 0x0002, IP_ACTIVE_LOW, IPT_MAHJONG_D )
	PORT_BIT ( 0x0004, IP_ACTIVE_LOW, IPT_MAHJONG_H )
	PORT_BIT ( 0x0008, IP_ACTIVE_LOW, IPT_MAHJONG_L )
	PORT_BIT ( 0x0010, IP_ACTIVE_LOW, IPT_MAHJONG_PON )
	PORT_BIT ( 0x0180, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 16-bit DSW1+DSW2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coinage ) )		// DSW1
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, "Re-Clothe" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Nudity" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Difficulty ) )	// DSW2
	PORT_DIPSETTING(      0x0000, "8" )
	PORT_DIPSETTING(      0x0100, "7" )
	PORT_DIPSETTING(      0x0200, "6" )
	PORT_DIPSETTING(      0x0300, "5" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPSETTING(      0x0500, "2" )
	PORT_DIPSETTING(      0x0600, "1" )
	PORT_DIPSETTING(      0x0700, "4" )
	PORT_DIPNAME( 0x0800, 0x0000, "Kuitan" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Continues ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

INPUT_PORTS_END

/***************************************************************************
    Graphics definitions
***************************************************************************/

/* all this is plain wrong, failed attempt at decoding the gfx */
/*
static const gfx_layout wrong_layout =
{
    8,8,
    RGN_FRAC(1,1),
    8,
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8 },
    { 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64},
    8*8*8
};

static const gfx_layout wrong_layout2 =
{
    8,8,
    RGN_FRAC(1,1),
    1,
    { 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8 },
    8*8*1
};
*/
static const gfx_layout wrong_layout3 = /* 16x16x9? seems to be some 18 bytes granularity in roms */
{
	16,16,
	RGN_FRAC(1,1),
	8/*9*/,
	{ /*0,*/ 1, 2, 3, 4, 5, 6, 7, 8 },
	{ STEP16(0,1)/*STEP16(0,9)*/ },
	{ STEP16(0,16*9) },
	16*16*9
};


static const gfx_decode gfxdecodeinfo[] =
{
//  { REGION_GFX1, 0, &wrong_layout,  0, 0x100 },   // sprites (8x8x?)
//  { REGION_GFX1, 0, &wrong_layout2, 0, 0x100 },   // tiles (16x16x?)
	{ REGION_GFX1, 0, &wrong_layout3, 0, 0x100 },	// tiles (16x16x9)
	{ -1 }
};

/***************************************************************************
    Machine driver
***************************************************************************/

static MACHINE_DRIVER_START( srmp6 )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(srmp6,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1) // don't trigger IT 3 for now...

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(srmp6)
	MDRV_VIDEO_UPDATE(srmp6)

	/* sound hardware */
MACHINE_DRIVER_END


/***************************************************************************
    ROM definition(s)
***************************************************************************/
/*
CHR?
    sx011-05.18 '00':[23a640,300000[
    sx011-04.19
    sx011-03.20
    sx011-02.21
    sx011-01.22 '00':[266670,400000[ (end)

most are 8 bits unsigned PCM 16/32KHz if stereo/mono
some voices in 2nd rom have lower sample rate
    sx011-08.15 <- samples: instruments, musics, sound FXs, voices
    sx011-07.16 <- samples: voices (cont'd), sound FXs, music, theme music
    sx011-06.17 <- samples: theme music (cont'd)
*/
ROM_START( srmp6 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "sx011-10.4", 0x000001, 0x080000, CRC(8f4318a5) SHA1(44160968cca027b3d42805f2dd42662d11257ef6) )
	ROM_LOAD16_BYTE( "sx011-11.5", 0x000000, 0x080000, CRC(7503d9cf) SHA1(03ab35f13b6166cb362aceeda18e6eda8d3abf50) )

	ROM_REGION( 0x200000, REGION_USER1, 0 ) /* 68000 Data */
	ROM_LOAD( "sx011-09.10", 0x000000, 0x200000, CRC(58f74438) SHA1(a256e39ca0406e513ab4dbd812fb0b559b4f61f2) )

	ROM_REGION( 0x1400000, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD( "sx011-05.18", 0x0000000, 0x400000, CRC(87e5fea9) SHA1(abd751b5744d6ac7e697774ea9a7f7455bf3ac7c) ) // CHR03
	ROM_LOAD( "sx011-04.19", 0x0400000, 0x400000, CRC(e90d331e) SHA1(d8afb1497cec8fe6de10d23d49427e11c4c57910) ) // CHR04
	ROM_LOAD( "sx011-03.20", 0x0800000, 0x400000, CRC(f1f24b35) SHA1(70d6848f77940331e1be8591a33d62ac22a3aee9) ) // CHR05
	ROM_LOAD( "sx011-02.21", 0x0c00000, 0x400000, CRC(c56d7e50) SHA1(355c64b38e7b266f386b9c0b906c8581fc15374b) ) // CHR06
	ROM_LOAD( "sx011-01.22", 0x1000000, 0x400000, CRC(785409d1) SHA1(3e31254452a30d929161a1ea3a3daa69de058364) ) // CHR07

	ROM_REGION( 0xc00000, REGION_SOUND1, 0) /* Samples */
	ROM_LOAD( "sx011-08.15", 0x0000000, 0x400000, CRC(01b3b1f0) SHA1(bbd60509c9ba78358edbcbb5953eafafd6e2eaf5) ) // CHR00
	ROM_LOAD( "sx011-07.16", 0x0400000, 0x400000, CRC(26e57dac) SHA1(91272268977c5fbff7e8fbe1147bf108bd2ed321) ) // CHR01
	ROM_LOAD( "sx011-06.17", 0x0800000, 0x400000, CRC(220ee32c) SHA1(77f39b54891c2381b967534b0f6d380962eadcae) ) // CHR02

 	/* CHRxx loaded here undecoded for ROM Check */
	ROM_REGION( 0x2000000, REGION_USER2, 0)	/* Banked ROM */
	ROM_LOAD( "sx011-08.15", 0x0000000, 0x400000, CRC(01b3b1f0) SHA1(bbd60509c9ba78358edbcbb5953eafafd6e2eaf5) ) // CHR00
	ROM_LOAD( "sx011-07.16", 0x0400000, 0x400000, CRC(26e57dac) SHA1(91272268977c5fbff7e8fbe1147bf108bd2ed321) ) // CHR01
	ROM_LOAD( "sx011-06.17", 0x0800000, 0x400000, CRC(220ee32c) SHA1(77f39b54891c2381b967534b0f6d380962eadcae) ) // CHR02
	ROM_LOAD( "sx011-05.18", 0x0c00000, 0x400000, CRC(87e5fea9) SHA1(abd751b5744d6ac7e697774ea9a7f7455bf3ac7c) ) // CHR03
	ROM_LOAD( "sx011-04.19", 0x1000000, 0x400000, CRC(e90d331e) SHA1(d8afb1497cec8fe6de10d23d49427e11c4c57910) ) // CHR04
	ROM_LOAD( "sx011-03.20", 0x1400000, 0x400000, CRC(f1f24b35) SHA1(70d6848f77940331e1be8591a33d62ac22a3aee9) ) // CHR05
	ROM_LOAD( "sx011-02.21", 0x1800000, 0x400000, CRC(c56d7e50) SHA1(355c64b38e7b266f386b9c0b906c8581fc15374b) ) // CHR06
	ROM_LOAD( "sx011-01.22", 0x1c00000, 0x400000, CRC(785409d1) SHA1(3e31254452a30d929161a1ea3a3daa69de058364) ) // CHR07
ROM_END


/***************************************************************************
    Driver initialization
***************************************************************************/

static DRIVER_INIT( srmp6 )
{
}


/***************************************************************************
    Game driver(s)
***************************************************************************/

/*GAME( YEAR,NAME,PARENT,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME,FLAGS)*/
GAME( 1995, srmp6, 0, srmp6, srmp6, srmp6, ROT0, "Seta", "Super Real Mahjong P6 (Japan)", GAME_NOT_WORKING | GAME_NO_SOUND)

