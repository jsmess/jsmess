/***************************************************************************

Bank Panic memory map (preliminary)
Similar to Appoooh

driver by Nicola Salmoria


0000-dfff ROM
e000-e7ff RAM
f000-f3ff Video RAM #1
f400-f7ff Color RAM #1
f800-fbff Video RAM #2
fc00-ffff Color RAM #2

I/O
read:
00  IN0
01  IN1
02  IN2
04  DSW

write:
00  SN76496 #1
01  SN76496 #2
02  SN76496 #3
05  horizontal scroll
07  bit 0-1 = at least one of these two controls the playfield priority
    bit 2-3 = ?
    bit 4 = NMI enable
    bit 5 = flip screen
    bit 6-7 = ?

***************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"


extern UINT8 *bankp_videoram2;
extern UINT8 *bankp_colorram2;

extern WRITE8_HANDLER( bankp_videoram_w );
extern WRITE8_HANDLER( bankp_colorram_w );
extern WRITE8_HANDLER( bankp_videoram2_w );
extern WRITE8_HANDLER( bankp_colorram2_w );
extern WRITE8_HANDLER( bankp_scroll_w );
extern WRITE8_HANDLER( bankp_out_w );

extern PALETTE_INIT( bankp );
extern VIDEO_START( bankp );
extern VIDEO_UPDATE( bankp );



static ADDRESS_MAP_START( bankp_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_ROM
	AM_RANGE(0xe000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xf3ff) AM_RAM AM_WRITE(bankp_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xf400, 0xf7ff) AM_RAM AM_WRITE(bankp_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xf800, 0xfbff) AM_RAM AM_WRITE(bankp_videoram2_w) AM_BASE(&bankp_videoram2)
	AM_RANGE(0xfc00, 0xffff) AM_RAM AM_WRITE(bankp_colorram2_w) AM_BASE(&bankp_colorram2)
ADDRESS_MAP_END


static ADDRESS_MAP_START( bankp_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(input_port_0_r, SN76496_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(input_port_1_r, SN76496_1_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(input_port_2_r, SN76496_2_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_3_r)
	AM_RANGE(0x05, 0x05) AM_WRITE(bankp_scroll_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(bankp_out_w)
ADDRESS_MAP_END



INPUT_PORTS_START( bankp )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Coin A/B" )	PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Coin C" )	PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )	PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )	PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, "70K 200K 500K" )
	PORT_DIPSETTING(    0x10, "100K 400K 800K" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )	PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )	PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )	PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the bitplanes are packed in one byte */
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 8 consecutive bytes */
};
static const gfx_layout charlayout2 =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,	/* 3 bits per pixel */
	{ 0, 2048*8*8, 2*2048*8*8 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 32 },
	{ REGION_GFX2, 0, &charlayout2,  32*4, 16 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( bankp )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 15468480/4)
	MDRV_CPU_PROGRAM_MAP(bankp_map, 0)
	MDRV_CPU_IO_MAP(bankp_io_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(3*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32*4+16*8)

	MDRV_PALETTE_INIT(bankp)
	MDRV_VIDEO_START(bankp)
	MDRV_VIDEO_UPDATE(bankp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76496, 15468480/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SN76496, 15468480/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(SN76496, 15468480/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bankp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr6175.bin",  0x0000, 0x4000, CRC(044552b8) SHA1(8d50ba062483d4789cfd3ed86cea53dff0ff6968) )
	ROM_LOAD( "epr6174.bin",  0x4000, 0x4000, CRC(d29b1598) SHA1(8c1ee4d23d8d6f93af3e22f2cba189b0055994fb) )
	ROM_LOAD( "epr6173.bin",  0x8000, 0x4000, CRC(b8405d38) SHA1(0f62a972f38b4ddcea77eb0e1d76c70ddbcb7b11) )
	ROM_LOAD( "epr6176.bin",  0xc000, 0x2000, CRC(c98ac200) SHA1(1bdb87868deebe03da18280e617530c24118da1c) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr6165.bin",  0x0000, 0x2000, CRC(aef34a93) SHA1(513895cd3144977b3d9b5ac7f2bf40384d69e157) )	/* playfield #1 chars */
	ROM_LOAD( "epr6166.bin",  0x2000, 0x2000, CRC(ca13cb11) SHA1(3aca0b0d3f052a742e1cd0b96bfad834e78fcd7d) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "epr6172.bin",  0x0000, 0x2000, CRC(c4c4878b) SHA1(423143d81408eda96f87bdc3a306517c473cbe00) )	/* playfield #2 chars */
	ROM_LOAD( "epr6171.bin",  0x2000, 0x2000, CRC(a18165a1) SHA1(9a7513ea84f9231edba4e637df28a1705c8cdeb0) )
	ROM_LOAD( "epr6170.bin",  0x4000, 0x2000, CRC(b58aa8fa) SHA1(432b43cd9af4e3dab579cfd191b731aa11ceb121) )
	ROM_LOAD( "epr6169.bin",  0x6000, 0x2000, CRC(1aa37fce) SHA1(6e2402683145de8972a53c9ec01da9a422392bed) )
	ROM_LOAD( "epr6168.bin",  0x8000, 0x2000, CRC(05f3a867) SHA1(9da11c3cea967c5f0d7397c0ff4f87b4b1446c4c) )
	ROM_LOAD( "epr6167.bin",  0xa000, 0x2000, CRC(3fa337e1) SHA1(5fdc45436be27cceb5157bd6201c30e3de28fd7b) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "pr6177.clr",   0x0000, 0x020, CRC(eb70c5ae) SHA1(13613dad6c14004278f777d6f3f62712a2a85773) ) 	/* palette */
	ROM_LOAD( "pr6178.clr",   0x0020, 0x100, CRC(0acca001) SHA1(54c354d825a24a9085867b114a2cd6835baebe55) ) 	/* charset #1 lookup table */
	ROM_LOAD( "pr6179.clr",   0x0120, 0x100, CRC(e53bafdb) SHA1(7a414f6db5476dd7d0217e5b846ed931381eda02) ) 	/* charset #2 lookup table */
ROM_END


GAME( 1984, bankp, 0, bankp, bankp, 0, ROT0, "[Sanritsu] Sega", "Bank Panic", 0 )
