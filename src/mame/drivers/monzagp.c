/***************************************************************************
RB BO ITALY - Driving game of some sort.
MONZA GP From Leante Games (Olympia) ??
========================================
www.andys-arcade.com

Dumped by Andy Welburn on a windy and rainy day 07/07/04

Possibly has clk/dir type steering.

Shows RB BO ITALY on the title screen and is a top-down driving game,
a lot like monaco GP, it even has stages where you have headlights.
Board colour, screening, track patterns, and most importantly
component type and colour of sockets indicates to me a pcb made in
the same factory as 'Sidam' and some 'Olympia' games. There is no
manufacturer name, no game name, all i see is : AA20/80 etched
on the underside of the pcb.

I have had this pcb for a number of years, i always thought it was
some sort of pinball logic pcb so didn't treat it too well. When it
came to clearing out my boxes of junk i took another look at it, and
it was the bank of 4116 rams that made me take a closer look.

I hooked it up and saw some video on my scope, then it died.
The +12v had shorted.. Suspecting the godamn tantalum capacitors
(often short out for no reason) i found a shorted one, removed
it and away we went. It had seperate H + V sync, so i loaded
a 74ls08 into a spare ic space and AND'ed the two signals to get
composite, voila, i got a stable picture. The colours aren't right,
and maybe the video isn't quite right either, but it worked enough
for me to realise i had never seen a game like it, so i dumped it.

I couldn't get any sound out of it, could be broken, or not
hooked up right, i would suspect the latter is the case.


Hardware :
==========
2x  z80's
1x  AY-3-8910
1x  8255
2   pairs of 2114 RAM (512 bytes each)
16x 4116 RAM (2k each)
4x  2716 (ROM)
12x 2708 (ROM)


ROMS layout:
------------
(add .bin to the end to get the filenames)
YC,YD,YA and YB are all 2716 eproms, everything else is 2708's.

(tabulation used here, see photo for clarity)

YC  YD      XX
YA  YB
            XC
        XI  XM
        X-* XD
        XP  XS
        XA  XE
        XR  XO

*- denotes a rom where the label had fallen off.. so its X-.bin


- enjoy!
Andy Welburn
www.andys-arcade.com

Known issues:
- the 8255 is not hooked up - it may play a key role in why the game isn't working propely
- some of the data ROMs are not mapped correctly.  Only ROMs containing code are mapped with reasonable
  certainty, because they contain absolute addreses.
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"

VIDEO_START( monzagp )
{
	return 0;
}

VIDEO_UPDATE( monzagp )
{ /* WRONG */
	unsigned char *source = videoram;
	int sy;
//  return;
	for( sy=0; sy<256; sy++ )
	{
		UINT16 *dest = BITMAP_ADDR16(bitmap, sy, 0);
		int sx;
		for( sx=0; sx<512; sx++ )
		{
			dest[sx] = *source++;
		}
	}
	return 0;
}

/* the master cpu transmits data to the slave CPU one word at a time using a rapid sequence of triggered NMIs
 * the slave cpu pauses as it enters its irq, awaiting this burst of data
 */
static UINT8 mDataBuffer[2];
static WRITE8_HANDLER ( write_data ) { mDataBuffer[offset] = data; }
static READ8_HANDLER ( send_data ) { cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE); return 0; }
static READ8_HANDLER( read_data ){ return mDataBuffer[offset]; }

static ADDRESS_MAP_START( readport_master, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(send_data)
ADDRESS_MAP_END

static ADDRESS_MAP_START( monzagp_master, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x23ff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x2800, 0x2800) AM_READ(input_port_2_r) /* buttons */
	AM_RANGE(0x2802, 0x2802) AM_READ(input_port_3_r) /* steering? - polled in nmi */
	AM_RANGE(0x3000, 0x3000) AM_WRITE(AY8910_control_port_0_w) /* ??? */
	AM_RANGE(0x3800, 0x3800) AM_WRITE(AY8910_write_port_0_w) /* ??? */
	AM_RANGE(0x4000, 0x4000) AM_READ( input_port_0_r )
	AM_RANGE(0x47ff, 0x4800) AM_WRITE(write_data) /* read at 9FFF by slave */
	AM_RANGE(0x6000, 0x6000) AM_READ( input_port_1_r )
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_slave, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x05, 0x06) AM_READ(MRA8_NOP) // ?
ADDRESS_MAP_END

static ADDRESS_MAP_START( monzagp_slave, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READ(MRA8_ROM) /* xx */
	AM_RANGE(0x1000, 0x13ff) AM_READ(MRA8_ROM) /* xc */
	AM_RANGE(0x1800, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9fff, 0xa000) AM_READ(read_data)
	AM_RANGE(0xc000, 0xffff) AM_READ(MRA8_RAM) AM_WRITE(MWA8_RAM) AM_BASE( &videoram )
ADDRESS_MAP_END

/***************************************************************************/

static const gfx_layout tile_layout =
{
	// game doesn't appear to use character-based graphics, but this allows us to view contents of the data
	// ROMs that do contain bitmaps
	16,16,
	RGN_FRAC(1,1),
	1, /* 1 bit per pixel */
	{ 4 },
	{ 0x003,0x002,0x001,0x000,0x083,0x082,0x081,0x080,
	  0x103,0x102,0x101,0x100,0x183,0x182,0x181,0x180 },
	{
		0x0*8, 0x1*8, 0x2*8, 0x3*8, 0x4*8, 0x5*8, 0x6*8, 0x7*8,
		0x8*8, 0x9*8, 0xa*8, 0xb*8, 0xc*8, 0xd*8, 0xe*8, 0xf*8
	},
	0x200
};

static const gfx_layout tile_layout2 =
{
	16,16,
	RGN_FRAC(1,1),
	1, /* 1 bit per pixel */
	{ 0 },
	{ 0x003,0x002,0x001,0x000,0x083,0x082,0x081,0x080,
	  0x103,0x102,0x101,0x100,0x183,0x182,0x181,0x180 },
	{
		0x0*8, 0x1*8, 0x2*8, 0x3*8, 0x4*8, 0x5*8, 0x6*8, 0x7*8,
		0x8*8, 0x9*8, 0xa*8, 0xb*8, 0xc*8, 0xd*8, 0xe*8, 0xf*8
	},
	0x200
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tile_layout,  0, 4 },
	{ REGION_GFX1, 0x0000, &tile_layout2,  0, 4 },
	{ -1 }
};

static MACHINE_DRIVER_START( monzagp )
	MDRV_CPU_ADD(Z80,4000000)
	MDRV_CPU_PROGRAM_MAP(monzagp_master,0)
	MDRV_CPU_IO_MAP(readport_master,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,4000000)
	MDRV_CPU_PROGRAM_MAP(monzagp_slave,0)
	MDRV_CPU_IO_MAP(readport_slave,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	/* video hardware */
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(300)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512,256)
	MDRV_SCREEN_VISIBLE_AREA(0,511,0,255)
	MDRV_PALETTE_LENGTH(100)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(monzagp)
	MDRV_VIDEO_UPDATE(monzagp)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

INPUT_PORTS_START( monzagp )
	PORT_START_TAG("DSWA") /* 0x4000 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "DSWA-0" )
	PORT_DIPSETTING(    0x01, "DSWA-1" )
	PORT_DIPSETTING(    0x02, "DSWA-2" )
	PORT_DIPSETTING(    0x03, "DSWA-3" )
	PORT_DIPSETTING(    0x04, "DSWA-4" )
	PORT_DIPSETTING(    0x05, "DSWA-5" )
	PORT_DIPSETTING(    0x06, "DSWA-6" )
	PORT_DIPSETTING(    0x07, "DSWA-7" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "DSWA-0" )
	PORT_DIPSETTING(    0x08, "DSWA-1" )
	PORT_DIPSETTING(    0x10, "DSWA-2" )
	PORT_DIPSETTING(    0x18, "DSWA-3" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Unknown ) ) /* test? */
	PORT_DIPSETTING(    0x00, "DSWA-0" )
	PORT_DIPSETTING(    0x20, "DSWA-1" )
	PORT_DIPSETTING(    0x40, "DSWA-2" )
	PORT_DIPSETTING(    0x60, "DSWA-3" )
	PORT_DIPSETTING(    0x80, "DSWA-4" )
	PORT_DIPSETTING(    0xa0, "DSWA-5" )
	PORT_DIPSETTING(    0xc0, "DSWA-6" )
	PORT_DIPSETTING(    0xe0, "DSWA-7" )

	PORT_START_TAG("DSWB") /* 0x6000 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "DSWB-0" )
	PORT_DIPSETTING(    0x01, "DSWB-1" )
	PORT_DIPSETTING(    0x02, "DSWB-2" )
	PORT_DIPSETTING(    0x03, "DSWB-3" )
	PORT_DIPSETTING(    0x04, "DSWB-4" )
	PORT_DIPSETTING(    0x05, "DSWB-5" )
	PORT_DIPSETTING(    0x06, "DSWB-6" )
	PORT_DIPSETTING(    0x07, "DSWB-7" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "DSWB-0" )
	PORT_DIPSETTING(    0x08, "DSWB-1" )
	PORT_DIPSETTING(    0x10, "DSWB-2" )
	PORT_DIPSETTING(    0x18, "DSWB-3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "DSWB-0" )
	PORT_DIPSETTING(    0x20, "DSWB-1" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) /* unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) /* unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* 2800 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT (0xf8, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START /* 2802 */
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

ROM_START( monzagp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 code */
	ROM_LOAD( "yd.bin", 0x0000, 0x800, CRC(5eb61bb7) SHA1(b897ecc7fa9aa1ae4e095d22d16a901b9d439a8e) )
	ROM_LOAD( "yc.bin", 0x0800, 0x800, CRC(f7468a3b) SHA1(af1664e30b732b3d5321e76659961af3ebeb1237) )
	ROM_LOAD( "yb.bin", 0x1000, 0x800, CRC(9f21506e) SHA1(6b46ff4815b8a02b190ec13e067f9a6687980774) )
	ROM_LOAD( "ya.bin", 0x1800, 0x800, CRC(23fbcf14) SHA1(e8b5a9b01f715356c14aa41dbc9ca26732d3a4e4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code */
	ROM_LOAD( "xx.bin", 0x0000, 0x400, CRC(059d6294) SHA1(38f075753e7a9fcabb857e5587e8a5966052cbcd) )
	ROM_LOAD( "xc.bin", 0x1000, 0x400, CRC(397fd1f3) SHA1(e6b927933847ddcdbbcbeb5e5f37fea063356b24) )
//  ROM_LOAD( "xp.bin", 0x1800, 0x400, CRC(4b6d63ef) SHA1(16f9e31e588b989f5259ab59c0a3a2c7787f3a16) ) // ? where mapped?
	ROM_LOAD( "xi.bin", 0x1c00, 0x400, CRC(ef54efa2) SHA1(c8464f11ccfd9eaf9aefb2cd3ac2b9e8bc2d11b6) ) // contains bitmap for "R.B."
	ROM_LOAD( "x-.bin", 0x2000, 0x400, CRC(fea8e31e) SHA1(f85eac74d32ebd28170b466b136faf21a8ab220f) )
	ROM_LOAD( "xd.bin", 0x2400, 0x400, CRC(0c601fc9) SHA1(e655f292b502a14068f5c35428001f8ceedf3637) )
	ROM_LOAD( "xs.bin", 0x2800, 0x400, CRC(5d15ac52) SHA1(b4f97854018f72e4086c7d830d1b312aea1420a7) )
//  ROM_LOAD( "xa.bin", 0x2c00, 0x400, CRC(a95f5461) SHA1(2645fb93bc4ad5354eef5a385fa94021fb7291dc) ) // ? where mapped?
//  ROM_LOAD( "xr.bin", 0x3000, 0x400, CRC(8a8667aa) SHA1(53f34b6c5327d4398de644d7f318d460da56c2de) ) // ? where mapped?
//  ROM_LOAD( "xm.bin", 0x3400, 0x400, CRC(64ebb7de) SHA1(fc5477bbedf44e93a578a71d2ff376f6f0b51a71) ) // ? where mapped?
//  ROM_LOAD( "xo.bin", 0x3800, 0x400, CRC(c1d7f67c) SHA1(2ddfe9e59e323cd041fd760531b9e15ccd050058) ) // ? where mapped?
//  ROM_LOAD( "xe.bin", 0x3c00, 0x400, CRC(e0e81120) SHA1(14a77dfd069be342df4dbb1b747443c6d121d3fe) ) // ? where mapped?

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE ) /* fake - for viewing purposes; these are actually mapped in CPU address space */
	ROM_LOAD( "xp.bin", 0x0000, 0x400, CRC(4b6d63ef) SHA1(16f9e31e588b989f5259ab59c0a3a2c7787f3a16) )/* contains characters: "AEIOSXTDNMVGYRPL" */
	ROM_LOAD( "xa.bin", 0x0400, 0x400, CRC(a95f5461) SHA1(2645fb93bc4ad5354eef5a385fa94021fb7291dc) )
	ROM_LOAD( "xr.bin", 0x0800, 0x400, CRC(8a8667aa) SHA1(53f34b6c5327d4398de644d7f318d460da56c2de) )
	ROM_LOAD( "xm.bin", 0x0c00, 0x400, CRC(64ebb7de) SHA1(fc5477bbedf44e93a578a71d2ff376f6f0b51a71) )
	ROM_LOAD( "xo.bin", 0x1000, 0x400, CRC(c1d7f67c) SHA1(2ddfe9e59e323cd041fd760531b9e15ccd050058) )
	ROM_LOAD( "xe.bin", 0x1400, 0x400, CRC(e0e81120) SHA1(14a77dfd069be342df4dbb1b747443c6d121d3fe) )
ROM_END

static ppi8255_interface ppi8255_intf =
{
	1, 			/* 1 chips */
	{0},		/* Port A read */
	{0},		/* Port B read */
	{0},		/* Port C read */
	{0},		/* Port A write */
	{0},		/* Port B write */
	{0}, 		/* Port C write */
};

DRIVER_INIT( monzagp )
{
	ppi8255_init(&ppi8255_intf);
}

/*    YEAR, NAME,    PARENT,   MACHINE, INPUT,   INIT,    MONITOR, COMPANY,                   FULLNAME */
GAME( 1981,monzagp, 0,        monzagp, monzagp, monzagp, ROT0,   "Leante Games (Olympia?)", "Monza GP", GAME_NOT_WORKING|GAME_NO_SOUND )
