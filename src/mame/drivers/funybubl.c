/*

Funny Bubble ...

It's a puzzloop rip-off .. but with two Z80 CPUs
the program roms say omega 1997
the gfx roms say 1999
title screen has no date

( a z80 as the main cpu in 1999 ??! )

todo :
Funny Bubble has three clocks, 8 mhz, 12 mhz and 30 mhz
8 mhz is near to roms 1/3/4, 30 mhz near to rom 2 (and the z80), 12 mhz is near to the gfx roms (7-10)

convert to tilemaps

*/



#include "driver.h"
#include "sound/okim6295.h"

/* video/funybubl.c */
extern UINT8* funybubl_banked_videoram;
extern UINT8 *funybubl_paletteram;
WRITE8_HANDLER ( funybubl_paldatawrite );
VIDEO_START(funybubl);
VIDEO_UPDATE(funybubl);



static WRITE8_HANDLER ( funybubl_vidram_bank_w )
{
	if ((data&1) == 0)
		memory_set_bankptr(1,&funybubl_banked_videoram[0x000000]);
	else
		memory_set_bankptr(1,&funybubl_banked_videoram[0x001000]);
}

static WRITE8_HANDLER ( funybubl_cpurombank_w )
{
	unsigned char *rom = memory_region(REGION_CPU1);

		memory_set_bankptr(2,&rom[0x10000+0x4000*(data&0x3f)]);
}



WRITE8_HANDLER( funybubl_soundcommand_w )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(1,0, HOLD_LINE);
}

WRITE8_HANDLER( funybubl_oki_bank_sw )
{
	OKIM6295_set_bank_base(0, ((data & 1) * 0x40000));
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK2)	// banked port 1?
	AM_RANGE(0xc400, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc800, 0xcfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd000, 0xdfff) AM_READ(MRA8_BANK1)	// banked port 0?
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc400, 0xcfff) AM_WRITE(funybubl_paldatawrite) AM_BASE(&funybubl_paletteram) // palette
	AM_RANGE(0xd000, 0xdfff) AM_WRITE(MWA8_BANK1)	// banked port 0?
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
	AM_RANGE(0x06, 0x06) AM_READ(MRA8_NOP)		/* Nothing is done with the data read */
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(funybubl_vidram_bank_w)	// vidram bank
	AM_RANGE(0x01, 0x01) AM_WRITE(funybubl_cpurombank_w)		// rom bank?
	AM_RANGE(0x03, 0x03) AM_WRITE(funybubl_soundcommand_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP)		/* Written directly after IO port 0 */
	AM_RANGE(0x07, 0x07) AM_WRITE(MWA8_NOP)		/* Reset something on startup - Sound CPU ?? */
ADDRESS_MAP_END


/* Sound CPU */

static ADDRESS_MAP_START( soundreadmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9800, 0x9800) AM_READ(OKIM6295_status_0_r)
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundwritemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(funybubl_oki_bank_sw)
	AM_RANGE(0x9800, 0x9800) AM_WRITE(OKIM6295_data_0_w)
ADDRESS_MAP_END



INPUT_PORTS_START( funybubl )
	PORT_START	/* System inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(    0x28, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x38, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, "Hard 1" )
	PORT_DIPSETTING(    0x18, "Hard 2" )
	PORT_DIPSETTING(    0x10, "Hard 3" )
	PORT_DIPSETTING(    0x08, "Hard 4" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x80, "Semi" )
	PORT_DIPSETTING(    0x00, "Full" )

	/* Looks like the PCB shows an empty DSW 2 location */
INPUT_PORTS_END



static const gfx_layout tiles16x16x8_1_layout =
{
	8,8,
	RGN_FRAC(1,8),
	8,
	{ RGN_FRAC(3,8),RGN_FRAC(2,8),RGN_FRAC(1,8),RGN_FRAC(0,8),RGN_FRAC(7,8),RGN_FRAC(6,8),RGN_FRAC(5,8), RGN_FRAC(4,8) },
	{ 0, 1, 2,  3,  4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout tiles16x16x8_2_layout =
{
	16,16,
	RGN_FRAC(1,4),
	8,
	{ RGN_FRAC(3,4)+4,RGN_FRAC(3,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(1,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(0,4)+4, RGN_FRAC(0,4)+0  },
	{ 0, 1,2,3, 8,9,10,11, 256,257,258,259, 264,265,266,267},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16x8_1_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles16x16x8_2_layout, 0, 16 },
	{ -1 }
};



DRIVER_INIT( funybubl )
{
	funybubl_banked_videoram = auto_malloc (0x2000);

	memory_set_bankptr(1,&funybubl_banked_videoram[0x000000]);
}



static MACHINE_DRIVER_START( funybubl )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(soundreadmem,soundwritemem)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(12*8, 512-12*8-1, 16, 256-16-1)
//  MDRV_SCREEN_VISIBLE_AREA(0*8, 512-1, 0, 256-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(funybubl)
	MDRV_VIDEO_UPDATE(funybubl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1056000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1_pin7high) // clock frequency & pin 7 not verified
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



ROM_START( funybubl )
	ROM_REGION( 0x50000, REGION_CPU1, 0 ) /* main z80, lots of banked data */
	ROM_LOAD( "2.bin", 0x00000, 0x40000, CRC(d684c13f) SHA1(6a58b44dd775f374d6fd476a8fd175c28a83a495)  )
	ROM_RELOAD ( 0x10000, 0x40000 )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT  ) // bg gfx 8x8x8
	ROM_LOAD( "7.bin", 0x000000, 0x40000, CRC(87603d7b) SHA1(21aec4cd011691f8608c3ddab83697bd89634fc8) )
	ROM_LOAD( "8.bin", 0x040000, 0x40000, CRC(ab6031bd) SHA1(557793817f98c07c82caab4293aed7dffa4dbf7b) )
	ROM_LOAD( "9.bin", 0x080000, 0x40000, CRC(0e8352ff) SHA1(29679a7ece2585e1a66296439b68bd56c937e313) )
	ROM_LOAD( "10.bin",0x0c0000, 0x40000, CRC(df7dd356) SHA1(13b9f40714dfa7b8cebc0191dcdde88b51f5e78c) )
	ROM_LOAD( "13.bin",0x100000, 0x40000, CRC(9f57bdd5) SHA1(6fd60da5f5eee0251e3a08957952ed9f037eeaec) )
	ROM_LOAD( "14.bin",0x140000, 0x40000, CRC(2ac15ea3) SHA1(de5be6378b4b6eee6faf532c9ef14bd609041cb3) )
	ROM_LOAD( "15.bin",0x180000, 0x40000, CRC(9a5e66a6) SHA1(cbe727e4f1e9a7072520d2e30eb0047cc67bff1b) )
	ROM_LOAD( "16.bin",0x1c0000, 0x40000, CRC(218060b3) SHA1(35124afce7f0f998b5c4761bbc888235de4e56ef) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "5.bin", 0x000000, 0x80000, CRC(b7ebbc00) SHA1(92520fda2f8f242b8cd49aeaac21b279f48276bf) )
	ROM_LOAD( "6.bin", 0x080000, 0x80000, CRC(28afc396) SHA1(555d51948ffb237311112dcfd0516a43f603ff03) )
	ROM_LOAD( "11.bin",0x100000, 0x80000, CRC(9e8687cd) SHA1(42fcba2532ae5028fcfc1df50750d99ad2586820) )
	ROM_LOAD( "12.bin",0x180000, 0x80000, CRC(63f0e810) SHA1(5c7ed32ee8dc1d9aabc8d136ec370471096356c2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound z80 (not much code here ..) */
	ROM_LOAD( "1.bin", 0x00000,  0x10000, CRC(b8b5b675) SHA1(0a02ccd09bb2ae20efe49e3ca2006331aea0e2a7) )
	ROM_FILL(          0x08000,  0x08000, 0x00 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "3.bin", 0x00000,  0x20000, CRC(a2d780f4) SHA1(bebba3db21ab9ddde8c6f19db3b67c869df582eb) )
	ROM_RELOAD(        0x40000,  0x20000 )
	ROM_LOAD( "4.bin", 0x20000,  0x20000, CRC(1f7e9269) SHA1(5c16b49a4e94aec7606d088c2d45a77842ab565b) )
	ROM_CONTINUE(      0x60000,  0x20000 )
ROM_END


GAME( 1999, funybubl, 0, funybubl, funybubl, funybubl, ROT0, "Comad", "Funny Bubble", 0 )
