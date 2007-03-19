/*

  Cross Pang (c)1998 F2 System

  No Copyright Notice is displayed in the game however the following page
  lists it as being by F2 System, Released April 1998
  http://www.f2.co.kr/eng/f2system/intro5.asp

  The sample rom says 'Oksan' (Oksan made Pass, its unclear how they are
  related to this game)

  The game uses an U6612 sound chip which is YM3812 compatible.

  Audio Test isn't correct when a sound is tested, instead musics are right.

  The alpha effect is a guess, it might be something more simple like flicker

driver by Pierpaolo Prazzoli
some bits by David Haywood

*/

#include "driver.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"

extern UINT16 *crospang_bg_videoram,*crospang_fg_videoram;

extern VIDEO_START( crospang );
extern VIDEO_UPDATE( crospang );

extern WRITE16_HANDLER ( crospang_fg_scrolly_w );
extern WRITE16_HANDLER ( crospang_bg_scrolly_w );
extern WRITE16_HANDLER ( crospang_fg_scrollx_w );
extern WRITE16_HANDLER ( crospang_bg_scrollx_w );
extern WRITE16_HANDLER ( crospang_fg_videoram_w );
extern WRITE16_HANDLER ( crospang_bg_videoram_w );

static WRITE16_HANDLER ( crospang_soundlatch_w )
{
	if(ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
	}
}

/* main cpu */

static ADDRESS_MAP_START( crospang_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x120000, 0x1207ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x122000, 0x1227ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x2005ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x210000, 0x2107ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x280000, 0x280001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x280002, 0x280003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x280004, 0x280005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x320000, 0x32ffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( crospang_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x100002, 0x100003) AM_WRITE(crospang_fg_scrolly_w)
	AM_RANGE(0x100004, 0x100005) AM_WRITE(crospang_bg_scrollx_w)
	AM_RANGE(0x100006, 0x100007) AM_WRITE(crospang_bg_scrolly_w)
	AM_RANGE(0x100008, 0x100009) AM_WRITE(crospang_fg_scrollx_w)
	AM_RANGE(0x10000e, 0x10000f) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x120000, 0x1207ff) AM_WRITE(crospang_fg_videoram_w) AM_BASE(&crospang_fg_videoram)
	AM_RANGE(0x122000, 0x1227ff) AM_WRITE(crospang_bg_videoram_w) AM_BASE(&crospang_bg_videoram)
	AM_RANGE(0x200000, 0x2005ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x210000, 0x2107ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x270000, 0x270001) AM_WRITE(crospang_soundlatch_w)
	AM_RANGE(0x320000, 0x32ffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

/* sound cpu */

static ADDRESS_MAP_START( crospang_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( crospang_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( crospang_sound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(YM3812_status_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0x06, 0x06) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( crospang_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(OKIM6295_data_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START( crospang )
	PORT_START	/* Inputs */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	 ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	 ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	 ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	 ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Coins */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0003, 0x0002, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Number of Powers" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPNAME( 0x00c0, 0x0040, "Extra Balls" )
	PORT_DIPSETTING(      0x00c0, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x0040, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1000, "Minimum Balls per Row" )
	PORT_DIPSETTING(      0x1800, "3" )
	PORT_DIPSETTING(      0x1000, "4" )
	PORT_DIPSETTING(      0x0800, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout layout_16x16x4a =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
	{ 128,129,130,131,132,133,134,135, 0,1,2,3,4,5,6,7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7, 8*8, 8*9, 8*10, 8*11, 8*12, 8*13, 8*14, 8*15 },
	8*32
};

static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(2,4),RGN_FRAC(0,4),RGN_FRAC(3,4),RGN_FRAC(1,4) },
	{ 0,1,2,3,4,5,6,7, 128,129,130,131,132,133,134,135 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7, 8*8, 8*9, 8*10, 8*11, 8*12, 8*13, 8*14, 8*15 },
	8*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4a, 0, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4,  0, 0x30 }, // [1] Tiles
	{ -1 }
};

static void irqhandler(int linestate)
{
	cpunum_set_input_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	irqhandler	/* IRQ Line */
};

static MACHINE_DRIVER_START( crospang )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14318180/2)
	MDRV_CPU_PROGRAM_MAP(crospang_readmem,crospang_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80, 14318180/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(crospang_sound_readmem,crospang_sound_writemem)
	MDRV_CPU_IO_MAP(crospang_sound_readport,crospang_sound_writeport)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*8-1, 0, 30*8-1)

	MDRV_PALETTE_LENGTH(0x300)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(crospang)
	MDRV_VIDEO_UPDATE(crospang)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 14318180/4)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1056000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( crospang )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68k */
	ROM_LOAD16_BYTE( "p1.bin", 0x00001, 0x20000, CRC(0bcbbaad) SHA1(807f07be340d7af0aad8d49461b5a7f0221ea3b7) )
	ROM_LOAD16_BYTE( "p2.bin", 0x00000, 0x20000, CRC(0947d204) SHA1(35e7e277c51888a66d305994bf05c3f6bfc3c29e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* z80  */
	ROM_LOAD( "s1.bin", 0x00000, 0x10000, CRC(d61a224c) SHA1(5cd1b2d136ad58ab550c7ba135558d6c8a4cd8f6) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "s2.bin", 0x00000, 0x20000, CRC(9f9ecd22) SHA1(631ffe14018ba39658c435b8ecb23b19a14569ee) ) // sample rom contains oksan?

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* sprites */
	ROM_LOAD( "rom6.bin", 0x000000, 0x80000, CRC(9c633082) SHA1(18b8591b695ee429c9c9855d8cbba6249a4bd809) )
	ROM_LOAD( "rom5.bin", 0x080000, 0x80000, CRC(53a34dc5) SHA1(2e5cf8093bf507e81d7447736b7727c3fd20c471) )
	ROM_LOAD( "rom4.bin", 0x100000, 0x80000, CRC(9a91d494) SHA1(1c6280f662f1cf53f7f6defb7e215da75b573fdf) )
	ROM_LOAD( "rom3.bin", 0x180000, 0x80000, CRC(cc6e1fce) SHA1(eb5b3ca7222f48916dc6206f987b2669fe7e7c6b) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* bg tiles */
	ROM_LOAD( "rom1.bin", 0x00000, 0x40000, CRC(905042bb) SHA1(ed5b97e88d24e55f8fcfaaa34251582976cb2527) )
	ROM_LOAD( "rom2.bin", 0x40000, 0x40000, CRC(bc4381e9) SHA1(af0690c253bead3448db5ec8fb258d8284646e89) )
ROM_END

GAME( 1998, crospang, 0, crospang, crospang, 0, ROT0, "F2 System", "Cross Pang", 0 )
