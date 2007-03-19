/*

    Jack Potten's Poker (?)
    Omega 1981 (?)

    I'm not sure of the name because the board is not working, i guessed the name
    by looking inside the roms.

    Dumped by ShinobiZ (Thierry) and Coy (Gerald) on 26/12/1998.

    CPU: SY6502
    SND: SY6545 + AY-3-8912
    OSC: 10.000

    ROM:

    Program roms are roms 23*.*, on the board, there is a number near each roms
    looks to be the address of the rom :

            23-91   1800
            23-92   2000
            23-93   2800
            23-94   3000
            23-9    3800

    Graphics are in roms CG*.*, there is no type indication on these rams, i hope
    i read them correctly.

    There is also 3 sets of switches on the board :

            SW1     1       300     SW2     1       OPT1
                    2       600             2       OPT2
                    3       1200            3       OPT3
                    4       2400            4       OPT4
                    5       4800            5       OPT5
                    6       9600            6       DIS
                    7       -               7       +VPOL
                    8       -               8       +HPOL

            SW3     no indications on the board

    The color prom is included (82s129).

    The sound rom is missing on the board :(

    There is also two big chips (same size as the 6502) they are "SY6520/SY6820"

    more infos -> coydumps@hotmail.com

    ---

    http://www.acedistribuslots.com/casino_electronics_inc.htm

    ---

    AGEMAME Driver by Curt Coder

Any fixes for this driver should be forwarded to the AGEMAME forum at (http://www.mameworld.info)

*/

#include "driver.h"
#include "sound/ay8910.h"

static tilemap *bg_tilemap;

PALETTE_INIT( vp906iii )
{
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	int i;

	for (i = 0; i < 0x100; i++)
	{
		COLOR(0, i) = (*color_prom) & 0x0f;
		color_prom++;
	}

#if 0
	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		// red component

		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		bit3 = (*color_prom >> 3) & 0x01;

		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;

		// green component

		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		bit3 = (*color_prom >> 3) & 0x01;

		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;

		// blue component

		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		bit3 = (*color_prom >> 3) & 0x01;

		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;

		palette_set_color(machine, i, r, g, b);
	}
#endif
}

WRITE8_HANDLER( vp906iii_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( vp906iii_colorram_w )
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
	int code = videoram[tile_index];
	int color = attr & 0xff;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START(vp906iii)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	return 0;
}

VIDEO_UPDATE(vp906iii)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

static ADDRESS_MAP_START( vp906iii_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0880, 0x0880) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0881, 0x0881) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x08c4, 0x08c4) AM_READ(input_port_0_r) AM_WRITENOP // ???
	AM_RANGE(0x08c5, 0x08c5) AM_WRITENOP // ???
	AM_RANGE(0x08c6, 0x08c6) AM_READ(input_port_1_r) AM_WRITENOP // ???
	AM_RANGE(0x08c7, 0x08c7) AM_WRITENOP // ???
	AM_RANGE(0x08c8, 0x08c8) AM_READ(input_port_2_r) AM_WRITENOP // ???
	AM_RANGE(0x08c9, 0x08c9) AM_WRITENOP // ???
	AM_RANGE(0x08ca, 0x08ca) AM_READ(input_port_3_r) AM_WRITENOP // ???
	AM_RANGE(0x08cb, 0x08cb) AM_WRITENOP // ???
	AM_RANGE(0x1000, 0x13ff) AM_RAM AM_WRITE(vp906iii_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x1400, 0x17ff) AM_RAM AM_WRITE(vp906iii_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xd800, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( vp906iii )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_I)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 0") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 1") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 2") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 3") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 4") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 5") PORT_CODE(KEYCODE_H)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 6") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Bit 7") PORT_CODE(KEYCODE_K)

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Set max bet to 40")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Set max bet to 10")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8, 8,
	1024,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,	   0, 64 },
	{ -1 }
};


static MACHINE_DRIVER_START( vp906iii )
	// basic machine hardware
	MDRV_CPU_ADD(M6502, 10000000/12)	// ???
	MDRV_CPU_PROGRAM_MAP(vp906iii_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_PALETTE_INIT(vp906iii)
	MDRV_VIDEO_START(vp906iii)
	MDRV_VIDEO_UPDATE(vp906iii)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 10000000/12)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

MACHINE_DRIVER_END

ROM_START( vp906iii )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "23-91.u5",	 0xd800, 0x0800, CRC(b49035e2) SHA1(b94a0245ca64d15b1496d1b272ffc0ce80f85526) )
	ROM_LOAD( "23-92.u6",	 0xe000, 0x0800, CRC(d9ffaa73) SHA1(e39d10121e16f89cd8d30a5391a14dc3d4b13a46) )
	ROM_LOAD( "23-93.u7",	 0xe800, 0x0800, CRC(f4e44280) SHA1(a03e5f03ed86c8ad7900fab0ef6a71c76eba3232) )
	ROM_LOAD( "23-94.u8",	 0xf000, 0x0800, CRC(8372f4d0) SHA1(de289b65cbe30c92b46fa87b9262ff7f9cfa0431) )
	ROM_LOAD( "23-9.u9",	 0xf800, 0x0800, CRC(bfcb934d) SHA1(b7cfa049bdd773368cb8326bcdfabbf474d15bb4) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cg-0.u67",	 0x0000, 0x0800, CRC(b626ad89) SHA1(551b75f4559d11a4f8f56e38982114a21c77d4e7) )
	ROM_LOAD( "cg-2a.u68",	 0x0800, 0x0800, CRC(6e3e9b1d) SHA1(14eb8d14ce16719a6ad7d13db01e47c8f05955f0) )
	ROM_LOAD( "cg-2b.u69",	 0x1000, 0x0800, CRC(6bbb1e2d) SHA1(51ee282219bf84218886ad11a24bc6a8e7337527) )
	ROM_LOAD( "cg-2c.u70",	 0x1800, 0x0800, CRC(f2f94661) SHA1(f37f7c0dff680fd02897dae64e13e297d0fdb3e7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound.rom",	 0x0000, 0x0800, NO_DUMP ) // sound rom is missing according to readme info

	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "82s129n.u28", 0x0000, 0x0100, CRC(6db5a344) SHA1(5f1a81ac02a2a74252decd3bb95a5436cc943930) )
ROM_END

GAME( 1985, vp906iii, 0, vp906iii, vp906iii, 0, ROT0, "Casino Electronics Inc.", "906III Video Poker", GAME_NOT_WORKING )
