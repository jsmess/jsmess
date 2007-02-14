/***************************************************************************

  Ace starspinner driver, (under heavy construction !!!)

  22-07-2004: Re-Animator
  25-02-2005: Curt Coder

Any fixes for this driver should be forwarded to the AGEMAME forum at (http://www.mameworld.info)

Those with Z80 expertise are invited to assist with this driver.

   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-----------------------------------------
-----------+---+-----------------+-----------------------------------------


***************************************************************************/

/*

Starspinner
ACE, 1982?

PCB Layout
----------

|---------------------------------------------------------------------------|
|                                                                           |
|   XTAL    BAT                2114                                         |
|                                                                           |
|                                                   14-1-102          P1    |
|                                                                           |
|                                                                           |
|                                                                           |
|                                                   14-1-102                |
|                                                                           |
|   5       2114                                                            |
|                                                                           |
|   6       2114                                    14-1-102                |
|                                                                           |
|   7                                                                       |
|                                                                           |
|   8       5501    5501                            14-1-102                |
|                                                                           |
|   h9                                              16-1-101                |
|                                                                           |
|   h10                                             16-1-101                |
|                                                                           |
|   h11         Z80                                 16-1-101                |
|                                                                           |
|   h12                                                               P2    |
|                                                                           |
|                                           DSWA    DSWB                    |
|                                                                           |
|---------------------------------------------------------------------------|

Notes:
    Z80  - NEC D780C running at ? MHz (DIP40)
    5501 - Toshiba TC5501P 256 x4 SRAM (DIP22)
    2114 - NEC uPD2114LC 1k x8 DRAM (DIP18)
    XTAL - ? MHz
    BAT  - VARTA Ni-Cd 3.6V 100 mAh
    DSWA - 8-way DIP switch
    DSWB - 8-way DIP switch
    P1   - 4x10 pin connector to power supply
    P2   - 4x10 pin connector to control panel

    ROMs:

Filename        Device Type     Use
------------------------------------------------
h9.h9           2732 (4K)       \ Z80 Program
h10.h10         2732 (4K)       |
h11.h11         2732 (4K)       |
h12.h12         2732 (4K)       /

8.h8            2716 (2K)       Tiles

5.h5            2716 (2K)       \ Sprites
6.h6            2716 (2K)       |
7.h7            2716 (2K)       /

16-1-101.b9     ?               \ Palette?
16-1-101.b10    ?               |
16-1-101.b11    ?               /

*/

#include "driver.h"

static UINT8 *nvram;
static size_t nvram_size;

static NVRAM_HANDLER( nvram )
{
  if ( read_or_write )
  {	// writing
	mame_fwrite(file,nvram,nvram_size);
  }
  else
  { // reading
	if ( file ) mame_fread(file,nvram,nvram_size);
	else 		memset(nvram,0x00,nvram_size);
  }
}

/* video */

//static tilemap *bg_tilemap;

PALETTE_INIT( starspnr )
{
}

#if 0
WRITE8_HANDLER( starspnr_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( starspnr_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0xf0) << 4);
	int color = attr & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

#endif
VIDEO_START(starspnr)
{
#if 0
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);
#endif
	return 0;
}

VIDEO_UPDATE(starspnr)
{
//  tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

/* Memory Map */

static ADDRESS_MAP_START( starspnr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

/*
    Control Panel Buttons:

    1 Gamble
    2 Cancel/Collect
    3 Hold
    4 Hold
    5 Hold
    6 Hold
    7 Take
    8 Start

*/
INPUT_PORTS_START( starspnr )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_I)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_CODE(KEYCODE_K)

	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout spritelayout =
{
	32, 32,
	16,
	3,
	{ 0, 0x800*8, 0x1000*8 },
	{  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
	  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	},
	{  0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	   8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32,
	  16*32,17*32,18*32,19*32,20*32,21*32,22*32,23*32,
	  24*32,25*32,26*32,27*32,28*32,29*32,30*32,31*32
	},
	32*32
};

static const gfx_layout charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,   0, 16 },
	{ REGION_GFX2, 0x0000, &spritelayout, 0, 16 },
	{ -1 }
};

/* Interrupt Generator */

static INTERRUPT_GEN( timer_irq )
{
#if 0
  if ( timer_enabled )
  {
	irq_timer_stat = 0x01;

	cpunum_set_input_line(0, M6809_IRQ_LINE, HOLD_LINE );
  }
#endif
}

/* Machine Driver */

static MACHINE_DRIVER_START( starspnr )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, 4000000)	// ???
	MDRV_CPU_PROGRAM_MAP(starspnr_map, 0)
	MDRV_CPU_PERIODIC_INT(timer_irq, TIME_IN_HZ(1000))	// generate 1000 IQR's per second

	MDRV_NVRAM_HANDLER(nvram)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_VIDEO_START(starspnr)
	MDRV_VIDEO_UPDATE(starspnr)
	MDRV_PALETTE_INIT(starspnr)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( starspnr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "h9.h9",	  0x0000, 0x1000, CRC(083068aa) SHA1(160a5f3bf33d0a53354f98295cd67022762928b6) )
	ROM_LOAD( "h10.h10",  0x1000, 0x1000, CRC(a0a96e55) SHA1(de4dc0da5a1f358085817690cc6bdc8d94a849f8) )
	ROM_LOAD( "h11.h11",  0x2000, 0x1000, CRC(ab045396) SHA1(8b3aea0b0d55f62d5b6fbd39664beb93559d2213) )
	ROM_LOAD( "h12.h12",  0x3000, 0x1000, CRC(8571f3f5) SHA1(e8b60a604a4a0368b6063b15b328c68f351cb740) )

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.h8",	  0x0000, 0x0800, CRC(0dd38c3c) SHA1(4da0cd00c76d3be2164f141ccd8c72dd9578ee61) )

	ROM_REGION( 0x1800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5.h5",	  0x0000, 0x0800, CRC(df49876f) SHA1(68077304f096491baeddc1d6b4dc62f90de71903) )
	ROM_LOAD( "6.h6",	  0x0800, 0x0800, CRC(d992e2f6) SHA1(7841efec7d81689c82b8da501cce743436e7e8d4) )
	ROM_LOAD( "7.h7",	  0x1000, 0x0800, CRC(d5a40e88) SHA1(5cac8d85123720cdbb8b4630b14a27cf0ceef33f) )

	ROM_REGION( 0x300, REGION_PROMS, 0 )
	ROM_LOAD( "16-1-101.b9",  0x0000, 0x0100, NO_DUMP )
	ROM_LOAD( "16-1-101.b10", 0x0100, 0x0100, NO_DUMP )
	ROM_LOAD( "16-1-101.b11", 0x0200, 0x0100, NO_DUMP )
ROM_END

/* Game Driver */

GAME( 1982?, starspnr, 0, starspnr, starspnr, 0, ROT270, "ACE", "Starspinner (Dutch/Nederlands)", GAME_NOT_WORKING | GAME_NO_SOUND )
