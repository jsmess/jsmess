/***************************************************************************

	Sharp X1

	05/2009 Skeleton driver.

	=============  X1 series  =============

	X1 (CZ-800C) - November, 1982
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (4KB) + chargen (2KB)
	 * RAM: Main memory (64KB) + VRAM (4KB) + RAM for PCG (6KB) + GRAM (48KB, Option)
	 * Text Mode: 80x25 or 40x25
	 * Graphic Mode: 640x200 or 320x200, 8 colors
	 * Sound: PSG 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, Cassette port (2700 baud)

	X1C (CZ-801C) - October, 1983
	 * same but only 48KB GRAM

	X1D (CZ-802C) - October, 1983
	 * same as X1C but with a 3" floppy drive (notice: 3" not 3" 1/2!!)

	X1Cs (CZ-803C) - June, 1984
	 * two expansion I/O ports

	X1Ck (CZ-804C) - June, 1984
	 * same as X1Cs
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji 1st level

	X1F Model 10 (CZ-811C) - July, 1985
	 * Re-designed
	 * ROM: IPL (4KB) + chargen (2KB)

	X1F Model 20 (CZ-812C) - July, 1985
	 * Re-designed (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available

	X1G Model 10 (CZ-820C) - July, 1986
	 * Re-designed again
	 * ROM: IPL (4KB) + chargen (2KB)

	X1G Model 30 (CZ-822C) - July, 1986
	 * Re-designed again (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available

	X1twin (CZ-830C) - December, 1986
	 * Re-designed again (same as Model 10)
	 * ROM: IPL (4KB) + chargen (2KB) + Kanji
	 * Built Tape drive plus a 5" floppy drive was available
	 * It contains a PC-Engine

	=============  X1 Turbo series  =============

	X1turbo Model 30 (CZ-852C) - October, 1984
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (32KB) + chargen (8KB) + Kanji (128KB)
	 * RAM: Main memory (64KB) + VRAM (6KB) + RAM for PCG (6KB) + GRAM (96KB)
	 * Text Mode: 80xCh or 40xCh with Ch = 10, 12, 20, 25 (same for Japanese display)
	 * Graphic Mode: 640x200 or 320x200, 8 colors
	 * Sound: PSG 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, built-in Cassette interface,
		2 Floppy drive for 5" disks, two expansion I/O ports

	X1turbo Model 20 (CZ-851C) - October, 1984
	 * same as Model 30, but only 1 Floppy drive is included

	X1turbo Model 10 (CZ-850C) - October, 1984
	 * same as Model 30, but Floppy drive is optional and GRAM is 48KB (it can
		be expanded to 96KB however)

	X1turbo Model 40 (CZ-862C) - July, 1985
	 * same as Model 30, but uses tv screen (you could watch television with this)

	X1turboII (CZ-856C) - November, 1985
	 * same as Model 30, but restyled, cheaper and sold with utility software

	X1turboIII (CZ-870C) - November, 1986
	 * with two High Density Floppy driver

	X1turboZ (CZ-880C) - December, 1986
	 * CPU: z80A @ 4MHz, 80C49 x 2 (one for key scan, the other for TV & Cas Ctrl)
	 * ROM: IPL (32KB) + chargen (8KB) + Kanji 1st & 2nd level
	 * RAM: Main memory (64KB) + VRAM (6KB) + RAM for PCG (6KB) + GRAM (96KB)
	 * Text Mode: 80xCh or 40xCh with Ch = 10, 12, 20, 25 (same for Japanese display)
	 * Graphic Mode: 640x200 or 320x200, 8 colors [in compatibility mode],
		640x400, 8 colors (out of 4096); 320x400, 64 colors (out of 4096);
		320x200, 4096 colors [in multimode],
	 * Sound: PSG 8 octave + FM 8 octave
	 * I/O Ports: Centronic ports, 2 Joystick ports, built-in Cassette interface,
		2 Floppy drive for HD 5" disks, two expansion I/O ports

	X1turboZII (CZ-881C) - December, 1987
	 * same as turboZ, but added 64KB expansion RAM

	X1turboZIII (CZ-888C) - December, 1988
	 * same as turboZII, but no more built-in cassette drive

	Please refer to http://www2s.biglobe.ne.jp/~ITTO/x1/x1menu.html for
	more info

	BASIC has to be loaded from external media (tape or disk), the
	computer only has an Initial Program Loader (IPL)

	TODO: everything, in particular to sort out the BIOS dumps
	(especially CGROM dumps). Also, notice this is a color computer,
	despite the skeleton code says it's black ad white

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "video/mc6845.h"


static VIDEO_START( x1 )
{
}

static VIDEO_UPDATE( x1 )
{
	const gfx_element *gfx = screen->machine->gfx[0];
	int count = 0;

	int y,x;

	for (y=0;y<25;y++)
	{
		for (x=0;x<40;x++)
		{
			int tile = videoram[count];
			int color = colorram[count];

			//int colour = tile>>12;
			drawgfx_opaque(bitmap,cliprect,gfx,tile,color,0,0,x*8,y*8);

			count++;
		}
	}

	return 0;
}

static ADDRESS_MAP_START( x1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( x1_io , ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x0700, 0x0701) AM_WRITENOP //YM-2151 reg/data
//	AM_RANGE(0x0704, 0x0707) AM_NOP //ctc regs
//	AM_RANGE(0x0e80, 0x0e82) AM_WRITENOP //kanji registers?
//	AM_RANGE(0x0ff8, 0x0fff) AM_WRITENOP //fdc registers
//	AM_RANGE(0x1000, 0x12ff) AM_RAM //paletteram
//	AM_RANGE(0x1300, 0x13ff) AM_WRITENOP //ply port, mirrored
//	AM_RANGE(0x1400, 0x17ff) AM_RAM //pcg?
	AM_RANGE(0x1800, 0x1800) AM_DEVWRITE("crtc", mc6845_address_w)
	AM_RANGE(0x1801, 0x1801) AM_DEVWRITE("crtc", mc6845_register_w)
//	AM_RANGE(0x1900, 0x19ff) AM_NOP //sub port, mirrored
	AM_RANGE(0x1a00, 0x1a03) AM_MIRROR(0xfc) AM_DEVREADWRITE("ppi8255_0", ppi8255_r, ppi8255_w) //8255, mirrored
	AM_RANGE(0x1b00, 0x1bff) AM_DEVWRITE("ay", ay8910_data_w) //PSG / ay-3-8910 data port
	AM_RANGE(0x1c00, 0x1cff) AM_DEVWRITE("ay", ay8910_address_w) //PSG / ay-3-8910 reg port
//	AM_RANGE(0x1d00, 0x1dff) AM_WRITENOP //ROM bankswitch = 1
//	AM_RANGE(0x1e00, 0x1eff) AM_WRITENOP //ROM bankswitch = 0
//	AM_RANGE(0x1f80, 0x1f8f) AM_WRITENOP //dma
//	AM_RANGE(0x1f90, 0x1f93) AM_NOP //sio
//	AM_RANGE(0x1fa0, 0x1fa3) AM_NOP //ctc regs
//	AM_RANGE(0x1fa8, 0x1fab) AM_NOP //ctc regs
//	AM_RANGE(0x1fd0, 0x1fdf) AM_RAM //scrn?
	AM_RANGE(0x2000, 0x2fff) AM_RAM AM_BASE(&colorram)//txt mode RAM
	AM_RANGE(0x3000, 0x3fff) AM_RAM AM_BASE(&videoram)//txt mode RAM
	AM_RANGE(0x4000, 0x5fff) AM_RAM //gfx mode RAM
ADDRESS_MAP_END

static const gfx_layout x1_chars_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout x1_chars_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,8,9,10,11,12,13,14,15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

/* TODO: separe the different x1 decodings accordingly */
static GFXDECODE_START( x1 )
	GFXDECODE_ENTRY( "cgrom", 0x00000, x1_chars_8x8,    0, 0x40 )
	GFXDECODE_ENTRY( "cgrom", 0x00000, x1_chars_16x16,  0, 0x40 ) //only x1turboz uses this so far
	GFXDECODE_ENTRY( "kanji", 0x27000, x1_chars_16x16,  0, 0x40 ) //needs to be checked when the ROM will be redumped
GFXDECODE_END

static PALETTE_INIT(x1)
{
	int i;

	for(i=0;i<0x300;i++)
		palette_set_color(machine, i,MAKE_RGB(0x00,0x00,0x00));

	for (i = 0; i < 64; i++) //text mode colors
	{
		palette_set_color_rgb(machine, 2*i+1, pal1bit(i >> 2), pal1bit(i >> 1), pal1bit(i >> 0));
		palette_set_color_rgb(machine, 2*i+0, pal1bit(i >> 5), pal1bit(i >> 4), pal1bit(i >> 3));
	}

	//bitmap mode,TODO
	for(i=0;i<8;i++)
		palette_set_color_rgb(machine, 0x200+i, pal1bit(i >> 2), pal1bit(i >> 1), pal1bit(i >> 0));
}

static MACHINE_RESET( x1 )
{
}


static UINT8 hres_320, io_switch;

static READ8_DEVICE_HANDLER( x1_porta_r )
{
	//printf("PPI Port A read\n");
	return 0xff;
}

/* this port is system related */
static READ8_DEVICE_HANDLER( x1_portb_r )
{
	//printf("PPI Port B read\n");
	/*
	x--- ---- "v disp"
	-x-- ---- "sub cpu ibf"
	--x- ---- "sub cpu obf"
	---x ---- ROM/RAM flag (0=ROM, 1=RAM)
	---- x--- "busy"
	---- -x-- "v sync"
	---- --x- "cmt read"
	---- ---x "cmt test" (active low)
	*/
	return input_port_read(device->machine, "SYSTEM");
}

/* I/O system port */
static READ8_DEVICE_HANDLER( x1_portc_r )
{
	//printf("PPI Port C read\n");
	/*
	x--x xxxx <unknown>
	-x-- ---- 320 mode (r/w)
	--x- ---- i/o mode (r/w)
	*/
	return (input_port_read(device->machine, "IOSYS") & 0x9f) | hres_320 | io_switch;
}

static WRITE8_DEVICE_HANDLER( x1_porta_w )
{
	//printf("PPI Port A write %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( x1_portb_w )
{
	//printf("PPI Port B write %02x\n",data);
}

static WRITE8_DEVICE_HANDLER( x1_portc_w )
{
	//printf("PPI Port C write %02x\n",data);
	hres_320 = data & 0x40;
	io_switch = ~data & 0x20;
}

static const ppi8255_interface ppi8255_intf =
{
	DEVCB_HANDLER(x1_porta_r),						/* Port A read */
	DEVCB_HANDLER(x1_portb_r),						/* Port B read */
	DEVCB_HANDLER(x1_portc_r),						/* Port C read */
	DEVCB_HANDLER(x1_porta_w),						/* Port A write */
	DEVCB_HANDLER(x1_portb_w),						/* Port B write */
	DEVCB_HANDLER(x1_portc_w)						/* Port C write */
};

static const mc6845_interface mc6845_intf =
{
	"screen",	/* screen we are acting on */
	8,			/* number of pixels per video memory address */
	NULL,		/* before pixel update callback */
	NULL,		/* row update callback */
	NULL,		/* after pixel update callback */
	DEVCB_NULL,	/* callback for display state changes */
	DEVCB_NULL,	/* callback for cursor state changes */
	DEVCB_NULL,	/* HSYNC callback */
	DEVCB_NULL,	/* VSYNC callback */
	NULL		/* update address callback */
};

/* Input ports */
static INPUT_PORTS_START( x1 )
	PORT_START("SYSTEM")
	PORT_DIPNAME( 0x01, 0x01, "SYSTEM" )
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

	PORT_START("IOSYS")
	PORT_DIPNAME( 0x01, 0x01, "IOSYS" )
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

static MACHINE_DRIVER_START( x1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(x1_mem)
	MDRV_CPU_IO_MAP(x1_io)

	MDRV_MACHINE_RESET(x1)

	MDRV_PPI8255_ADD( "ppi8255_0", ppi8255_intf )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_PALETTE_LENGTH(0x300)
	MDRV_PALETTE_INIT(x1)

	MDRV_MC6845_ADD("crtc", MC6845, XTAL_4MHz/5, mc6845_intf)	/* unknown type and clock / divider, hand tuned to get ~60 fps */

	MDRV_GFXDECODE(x1)

	MDRV_VIDEO_START(x1)
	MDRV_VIDEO_UPDATE(x1)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ay", AY8910, XTAL_4MHz/8) //unknown clock / divider
	//MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(x1)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( x1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "default", "X1 IPL" )
	ROMX_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "X1 IPL (alt)" )
	ROMX_LOAD( "ipl_alt.x1", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a), ROM_BIOS(2) )

	ROM_REGION(0x0800, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )

	ROM_REGION(0x4ac00, "kanji", ROMREGION_ERASEFF)
ROM_END

ROM_START( x1ck )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "default", "X1 IPL" )
	ROMX_LOAD( "ipl.x1", 0x0000, 0x1000, CRC(7b28d9de) SHA1(c4db9a6e99873808c8022afd1c50fef556a8b44d), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "alt", "X1 IPL (alt)" )
	ROMX_LOAD( "ipl_alt.x1", 0x0000, 0x1000, CRC(e70011d3) SHA1(d3395e9aeb5b8bbba7654dd471bcd8af228ee69a), ROM_BIOS(2) )

	ROM_REGION(0x0800, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(e3995a57) SHA1(1c1a0d8c9f4c446ccd7470516b215ddca5052fb2) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END

ROM_START( x1turbo )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x0800, "cgrom", 0)
	/* This should be larger... hence, we are missing something (maybe part of the other fnt roms?) */
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END

/* X1 Turbo Z: IPL is supposed to be the same as X1 Turbo, but which dumps should be in "cgrom"?
Many emulators come with fnt0816.x1 & fnt1616.x1 but I am not sure about what was present on the real
X1 Turbo / Turbo Z */
ROM_START( x1turboz )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.x1t", 0x0000, 0x8000, CRC(2e8b767c) SHA1(44620f57a25f0bcac2b57ca2b0f1ebad3bf305d3) )

	ROM_REGION(0x4c600, "cgrom", 0)
	ROM_LOAD("fnt0808.x1", 0x0000, 0x0800, CRC(84a47530) SHA1(06c0995adc7a6609d4272417fe3570ca43bd0454) )
	ROM_SYSTEM_BIOS( 0, "font1", "Font set 1" )
	ROMX_LOAD("fnt0816.x1", 0x0800, 0x1000, BAD_DUMP CRC(34818d54) SHA1(2c5fdd73249421af5509e48a94c52c4e423402bf), ROM_BIOS(1) )
	/* I strongly suspect this is not genuine */
	ROMX_LOAD("fnt1616.x1", 0x01800, 0x4ac00, BAD_DUMP CRC(46826745) SHA1(b9e6c320611f0842df6f45673c47c3e23bc14272), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "font2", "Font set 2" )
	ROMX_LOAD("fnt0816_a.x1", 0x0800, 0x1000, BAD_DUMP CRC(98db5a6b) SHA1(adf1492ef326b0f8820a3caa1915ad5ab8138f49), ROM_BIOS(2) )
	/* I strongly suspect this is not genuine */
	ROMX_LOAD("fnt1616_a.x1", 0x01800, 0x4ac00, BAD_DUMP CRC(bc5689ae) SHA1(a414116e261eb92bbdd407f63c8513257cd5a86f), ROM_BIOS(2) )

	ROM_REGION(0x4ac00, "kanji", 0)
	/* This is clearly a bad dump: size does not make sense and from 0x28000 on there are only 0xff */
	ROM_LOAD("kanji2.rom", 0x0000, 0x4ac00, BAD_DUMP CRC(33800ef2) SHA1(fc07a31ee30db312c7995e887519a9173cb38c0d) )
ROM_END


/* Driver */

/*    YEAR  NAME       PARENT  COMPAT   MACHINE  INPUT  INIT  CONFIG COMPANY   FULLNAME      FLAGS */
COMP( 1982, x1,        0,      0,       x1,      x1,    0,    x1,    "Sharp",  "X1 (CZ-800C)",         GAME_NOT_WORKING)
COMP( 1984, x1ck,      x1,     0,       x1,      x1,    0,    x1,    "Sharp",  "X1Ck (CZ-804C)",       GAME_NOT_WORKING)
COMP( 1984, x1turbo,   x1,     0,       x1,      x1,    0,    x1,    "Sharp",  "X1 Turbo (CZ-850C)",   GAME_NOT_WORKING)
COMP( 1986, x1turboz,  x1,     0,       x1,      x1,    0,    x1,    "Sharp",  "X1 Turbo Z (CZ-880C)", GAME_NOT_WORKING)
