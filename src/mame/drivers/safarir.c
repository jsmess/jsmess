/****************************************************************************

Safari Rally by SNK/Taito

Driver by Zsolt Vasvari


This hardware is a precursor to Phoenix.

----------------------------------

CPU board

76477        18MHz

              8080

Video board


 RL07  2114
       2114
       2114
       2114
       2114           RL01 RL02
       2114           RL03 RL04
       2114           RL05 RL06
 RL08  2114

11MHz

----------------------------------

TODO:

- colors (8 colors originally, see game flyer screen shots)
- SN76477 sound

****************************************************************************/

#include "driver.h"
#include "sound/sn76477.h"


UINT8 *safarir_ram1, *safarir_ram2;
size_t safarir_ram_size;

static UINT8 *safarir_ram;

static tilemap *bg_tilemap, *fg_tilemap;


WRITE8_HANDLER( safarir_ram_w )
{
	if (safarir_ram[offset] != data)
	{
		safarir_ram[offset] = data;

		if (offset < 0x400)
		{
			tilemap_mark_tile_dirty(fg_tilemap, offset);
		}
		else
		{
			tilemap_mark_tile_dirty(bg_tilemap, offset - 0x400);
		}
	}
}

READ8_HANDLER( safarir_ram_r )
{
	return safarir_ram[offset];
}

WRITE8_HANDLER( safarir_scroll_w )
{
	tilemap_set_scrollx(bg_tilemap, 0, data);
}

WRITE8_HANDLER( safarir_ram_bank_w )
{
	safarir_ram = data ? safarir_ram1 : safarir_ram2;
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

static void get_bg_tile_info(int tile_index)
{
	int code = safarir_ram[tile_index + 0x400];

	SET_TILE_INFO(0, code & 0x7f, code >> 7, 0)
}

static void get_fg_tile_info(int tile_index)
{
	int code = safarir_ram[tile_index];
	int flags = ((tile_index & 0x1d) && (tile_index & 0x1e)) ? 0 : TILE_IGNORE_TRANSPARENCY;

	SET_TILE_INFO(1, code & 0x7f, code >> 7, flags)
}

VIDEO_START( safarir )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

VIDEO_UPDATE( safarir )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
	return 0;
}


static unsigned short colortable_source[] =
{
	0x00, 0x01,
	0x00, 0x02,
	0x00, 0x03,
	0x00, 0x04,
	0x00, 0x05,
	0x00, 0x06,
	0x00, 0x07,
};

static PALETTE_INIT( safarir )
{
	palette_set_color(machine, 0, 0x00, 0x00, 0x00);
	palette_set_color(machine, 1, 0x80, 0x80, 0x80);
	palette_set_color(machine, 2, 0xff, 0xff, 0xff);

	palette_set_color(machine, 3, 0x00, 0x00, 0x00);
	palette_set_color(machine, 4, 0x00, 0x00, 0x00);
	palette_set_color(machine, 5, 0x00, 0x00, 0x00);
	palette_set_color(machine, 6, 0x00, 0x00, 0x00);
	palette_set_color(machine, 7, 0x00, 0x00, 0x00);

	memcpy(colortable, colortable_source, sizeof(colortable_source));
}


static ADDRESS_MAP_START( safarir_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_ROM
	AM_RANGE(0x2000, 0x27ff) AM_READWRITE(safarir_ram_r, safarir_ram_w) AM_BASE(&safarir_ram1) AM_SIZE(&safarir_ram_size)
	AM_RANGE(0x2800, 0x28ff) AM_WRITE(safarir_ram_bank_w)
	AM_RANGE(0x2c00, 0x2cff) AM_WRITE(safarir_scroll_w)
	AM_RANGE(0x3000, 0x30ff) AM_WRITENOP	/* goes to SN76477 */
	AM_RANGE(0x3400, 0x3400) AM_WRITENOP // ???
	AM_RANGE(0x3800, 0x38ff) AM_READ(input_port_0_r)
	AM_RANGE(0x3c00, 0x3cff) AM_READ(input_port_1_r)
	AM_RANGE(0x8000, 0x87ff) AM_WRITENOP AM_BASE(&safarir_ram2)	/* only here to initialize pointer */
	AM_RANGE(0xffe0, 0xffe6) AM_WRITENOP // ???
ADDRESS_MAP_END


INPUT_PORTS_START( safarir )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x04, "Acceleration Rate" )
	PORT_DIPSETTING(    0x00, "Slowest" )
	PORT_DIPSETTING(    0x04, "Slow" )
	PORT_DIPSETTING(    0x08, "Fast" )
	PORT_DIPSETTING(    0x0c, "Fastest" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "5000" )
	PORT_DIPSETTING(    0x40, "7000" )
	PORT_DIPSETTING(    0x60, "9000" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,	/* 8*8 chars */
	128,	/* 128 characters */
	1,		/* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 2 },
	{ REGION_GFX2, 0, &charlayout, 0, 2 },
	{ -1 } /* end of array */
};

/* the following is copied from spaceinv */
struct SN76477interface sn76477_interface =
{
	0	/* N/C */,		/*  4  noise_res         */
	0	/* N/C */,		/*  5  filter_res        */
	0	/* N/C */,		/*  6  filter_cap        */
	0	/* N/C */,		/*  7  decay_res         */
	0	/* N/C */,		/*  8  attack_decay_cap  */
	RES_K(100) ,		/* 10  attack_res        */
	RES_K(56)  ,		/* 11  amplitude_res     */
	RES_K(10)  ,		/* 12  feedback_res      */
	0	/* N/C */,		/* 16  vco_voltage       */
	CAP_U(0.1) ,		/* 17  vco_cap           */
	RES_K(8.2) ,		/* 18  vco_res           */
	5.0		 ,			/* 19  pitch_voltage     */
	RES_K(120) ,		/* 20  slf_res           */
	CAP_U(1.0) ,		/* 21  slf_cap           */
	0	/* N/C */,		/* 23  oneshot_cap       */
	0	/* N/C */,		/* 24  oneshot_res       */
	0,			    	/* 22    vco                    */
	1,					/* 26 mixer A           */
	1,					/* 25 mixer B           */
	1,					/* 27 mixer C           */
	1,					/* 1  envelope 1        */
	1,					/* 28 envelope 2        */
	1				    /* 9     enable */
};

static MACHINE_DRIVER_START( safarir )

	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 3072000)	/* 3 MHz ? */
	MDRV_CPU_PROGRAM_MAP(safarir_map, 0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 26*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*7)

	MDRV_PALETTE_INIT(safarir)
	MDRV_VIDEO_START(safarir)
	MDRV_VIDEO_UPDATE(safarir)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( safarir )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "rl01",		0x0000, 0x0400, CRC(cf7703c9) SHA1(b4182df9332b355edaa518462217a6e31e1c07b2) )
	ROM_LOAD( "rl02",		0x0400, 0x0400, CRC(1013ecd3) SHA1(2fe367db8ca367b36c5378cb7d5ff918db243c78) )
	ROM_LOAD( "rl03",		0x0800, 0x0400, CRC(84545894) SHA1(377494ceeac5ad58b70f77b2b27b609491cb7ffd) )
	ROM_LOAD( "rl04",		0x0c00, 0x0400, CRC(5dd12f96) SHA1(a80ac0705648f0807ea33e444fdbea450bf23f85) )
	ROM_LOAD( "rl05",		0x1000, 0x0400, CRC(935ed469) SHA1(052a59df831dcc2c618e9e5e5fdfa47548550596) )
	ROM_LOAD( "rl06",		0x1400, 0x0400, CRC(24c1cd42) SHA1(fe32ecea77a3777f8137ca248b8f371db37b8b85) )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rl08",		0x0000, 0x0400, CRC(d6a50aac) SHA1(0a0c2cefc556e4e15085318fcac485b82bac2416) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rl07",		0x0000, 0x0400, CRC(ba525203) SHA1(1c261cc1259787a7a248766264fefe140226e465) )
ROM_END


DRIVER_INIT( safarir )
{
	safarir_ram = safarir_ram1;
}


GAME( 1979, safarir, 0, safarir, safarir, safarir, ROT90, "SNK", "Safari Rally (Japan)", GAME_NO_SOUND | GAME_WRONG_COLORS )
