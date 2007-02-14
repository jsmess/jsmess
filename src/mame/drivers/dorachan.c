/*
Dorachan (Dora-Chan ?) (c) 1980 Craul Denshi
Driver by Tomasz Slanina

Similar to Beam Invader
Todo:
- discrete sound
- d14.rom - is it color map ?
- dips (if any) - bits 5,6,7 of input port 0 ?
*/

#include "driver.h"
extern int dorachan_ctrl;

WRITE8_HANDLER( dorachan_videoram_w );

static READ8_HANDLER( dorachan_protection_r )
{

	switch (activecpu_get_previouspc())
	{
		case 0x70ce : return 0xf2;
		case 0x72a2 : return 0xd5;
		case 0x72b5 : return 0xcb;
	}
	mame_printf_debug("unhandled $2400 read @ %x\n",activecpu_get_previouspc());
	return 0xff;
}

static READ8_HANDLER( dorachan_status_r )
{
/* to avoid resetting (when player 2 starts) bit 0 need to be reversed when screen is flipped */
	return ((cpu_getscanline()>100)?1:0)^(dorachan_ctrl>>6);
}

static WRITE8_HANDLER(dorachan_ctrl_w)
{
	dorachan_ctrl=data;
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1800, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2400, 0x2400) AM_READ(dorachan_protection_r)
	AM_RANGE(0x2800, 0x2800) AM_READ(input_port_0_r)
	AM_RANGE(0x2c00, 0x2c00) AM_READ(input_port_1_r)
	AM_RANGE(0x3800, 0x3800) AM_READ(dorachan_status_r)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6000, 0x77ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x17ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1800, 0x1fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2000, 0x23ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(dorachan_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x6000, 0x77ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x02, 0x02) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x03, 0x03) AM_WRITE(dorachan_ctrl_w)
ADDRESS_MAP_END

INPUT_PORTS_START( dorachan )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )


	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
INPUT_PORTS_END

static MACHINE_DRIVER_START( dorachan )
	MDRV_CPU_ADD(Z80, 2000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(1*8, 31*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END

ROM_START( dorachan )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "c1.e1",      0x0000, 0x400, CRC(29d66a96) SHA1(a0297d87574af65c6ded99aeb377ac407f6f163f) )
	ROM_LOAD( "d2.e2",      0x0400, 0x400, CRC(144b6cd1) SHA1(195ce86e912a4b395097008c6d812fd75a1a2482) )
	ROM_LOAD( "d3.e3",      0x0800, 0x400, CRC(a9a1bed7) SHA1(98af6f851c4477f770b6bd67e5465b5a271311ee) )
	ROM_LOAD( "d4.e5",      0x0c00, 0x400, CRC(099ddf4b) SHA1(e4dd2b17a4320615204c66c24f60e58db13a5319) )
	ROM_LOAD( "c5.e6",      0x1000, 0x400, CRC(49449dab) SHA1(3627c16cc17fae9de2294a37602b726e107d0a13) )
	ROM_LOAD( "d6.e7",      0x1400, 0x400, CRC(5e409680) SHA1(f5e4d820c0f0493d724cd0d3da1113bccc09c2c3) )
	ROM_LOAD( "c7.e8",      0x2000, 0x400, CRC(b331a5ff) SHA1(1053953c76dddff450b9c9037e7797d50f9c7046) )
	ROM_LOAD( "d8.rom",     0x6000, 0x400, CRC(5fe1e731) SHA1(8e5dcb5f8d1d6f8c06808dd808f8bce7b07014ee) )
	ROM_LOAD( "d9.rom",     0x6400, 0x400, CRC(338881a8) SHA1(cd725b42c3f96826e94345698738f6b5a532d3d5) )
	ROM_LOAD( "d10.rom",    0x6800, 0x400, CRC(f8c59517) SHA1(655a976b1221e5aff69e0c0cc58d02c0b7bb6197) )
	ROM_LOAD( "d11.rom",    0x6c00, 0x400, CRC(c2e0f066) SHA1(be6b780a8957d945e5634ac9689b440a41e9a2a4) )
	ROM_LOAD( "d12.rom",    0x7000, 0x400, CRC(275e5dc1) SHA1(ac07db4b428daa49a52c679de95ddedbea0076b9) )
	ROM_LOAD( "d13.rom",    0x7400, 0x400, CRC(24ccfcf9) SHA1(85e5052ee657f518b0509eb64e494bc3a74e651e) )
	ROM_REGION( 0x400, REGION_USER1, 0 )
	ROM_LOAD( "d14.rom",    0x0000, 0x400, CRC(c0d3ee84) SHA1(f2207c685ce8d5144a373c28f11d2cebf9518b65) ) /* color map ? */
ROM_END

GAME( 1980, dorachan, 0, dorachan, dorachan, 0, ROT90, "Craul Denshi", "Dorachan",GAME_NO_SOUND)

