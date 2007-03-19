/*

 One + Two
 (c) Barko 1997

 Driver by David Haywood and Pierpaolo Prazzoli

*/

#include "driver.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"

static tilemap *fg_tilemap;
static UINT8 *fgram;

static void get_fg_tile_info(int tile_index)
{
	int code = (fgram[tile_index*2+1]<<8) | fgram[tile_index*2];
	int color = (fgram[tile_index*2+1] & 0x80) >> 7;

	code &= 0x7fff;

	SET_TILE_INFO(0, code, color, 0)
}

static WRITE8_HANDLER( onetwo_fgram_w )
{
	fgram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset/2);
}

static WRITE8_HANDLER( onetwo_cpubank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1) + 0x10000;

	memory_set_bankptr(1,&RAM[data * 0x4000]);
}

static WRITE8_HANDLER( onetwo_coin_counters_w )
{
	watchdog_reset_w(0, data & 0x01);
	coin_counter_w(0, data & 0x02);
	coin_counter_w(1, data & 0x04);
}

static WRITE8_HANDLER( onetwo_soundlatch_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

/* Main CPU */

static ADDRESS_MAP_START( main_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM AM_REGION(REGION_CPU1, 0x10000)
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc800, 0xc87f) AM_READWRITE(MRA8_RAM, paletteram_xBBBBBRRRRRGGGGG_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0xc900, 0xc97f) AM_READWRITE(MRA8_RAM, paletteram_xBBBBBRRRRRGGGGG_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(MRA8_RAM, onetwo_fgram_w) AM_BASE(&fgram)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_cpu_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(input_port_0_r, onetwo_coin_counters_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(input_port_1_r, onetwo_soundlatch_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(input_port_2_r, onetwo_cpubank_w)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_4_r)
ADDRESS_MAP_END

/* Sound CPU */

static ADDRESS_MAP_START( sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf800) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_cpu_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YM3812_status_port_0_r, YM3812_control_port_0_w)
	AM_RANGE(0x20, 0x20) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x40, 0x40) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xc0, 0xc0) AM_WRITE(soundlatch_clear_w)
ADDRESS_MAP_END

INPUT_PORTS_START( onetwo )
	PORT_START
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
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Stage Selection" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static const gfx_layout tiles8x8x6_layout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+4, RGN_FRAC(0,3)+0, RGN_FRAC(0,3)+4, RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x6_layout, 0, 2 },
	{ -1 }
};

VIDEO_START( onetwo )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);

	return 0;
}

VIDEO_UPDATE( onetwo )
{
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
	return 0;
}

static void irqhandler(int linestate)
{
	cpunum_set_input_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	irqhandler	/* IRQ Line */
};

static MACHINE_DRIVER_START( onetwo )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(main_cpu,0)
	MDRV_CPU_IO_MAP(main_cpu_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_cpu,0)
	MDRV_CPU_IO_MAP(sound_cpu_io,0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(16))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x80)

	MDRV_VIDEO_START(onetwo)
	MDRV_VIDEO_UPDATE(onetwo)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 8000000)
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1056000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( onetwo )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* main z80 */
	ROM_LOAD( "am27c010.0", 0x10000,  0x20000, CRC(83431e6e) SHA1(61ab386a1d0af050f091f5df28c55ad5ad1a0d4b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound z80 */
	ROM_LOAD( "s27c512.2",  0x00000,  0x10000, CRC(90aba4f3) SHA1(914b1c8684993ddc7200a3d61e07f4f6d59e9d02) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m27c4000.3", 0x000000, 0x80000, CRC(c72ff3a0) SHA1(17394d8a8b5ef4aee9522d87ba92ef1285f4d76a) )
	ROM_LOAD( "m27c4000.4", 0x080000, 0x80000, CRC(0ca40557) SHA1(ca2db57d64ece90f2066f15b276c8d5827dcb4fa) )
	ROM_LOAD( "zr040p.5",   0x100000, 0x80000, CRC(664b6679) SHA1(f9f78bd34fb58e24f890a540382392e1c9d01220) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "tm27c020.1", 0x000000, 0x40000, CRC(b10d3132) SHA1(42613e17b6a1300063b8355596a2dc7bcd903777) )
ROM_END

GAME( 1997, onetwo, 0, onetwo, onetwo, 0, ROT0, "Barko", "One + Two", 0 )
