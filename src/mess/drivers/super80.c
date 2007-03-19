/*
	super80.c

	Super-80 computer sold by Dick Smith Electronics in 1981 as a kit.

	Several variants of video hardware:
	* 32*16 monochrome display
	* 32*16 color display
	* 80*24 monochrome display
	* 80*24 color display

	We only emulate the first variant for now.

	architecture:
	* z80 @ 2MHz
	* 16k, 32k or 48k RAM (>0000->bfff), plus 4k of optional extra RAM in
	  >f000->ffff
	* 12k ROM (>c000->efff range)
*/

#include <math.h>
#include "driver.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "video/generic.h"
#include "cpu/z80/z80daisy.h"

static void pio_interrupt(int state);

static int vidpg;
static int charset;

static z80pio_interface z80pio_intf =
{
	pio_interrupt
	NULL,
	NULL
};

static void pio_interrupt(int state)
{
	cpunum_set_input_line(0, 0, state);
}

static void keyboard_scan(void)
{
	UINT8 sel;
	int i;
	UINT8 state = 0xff;

	sel = z80pio_p_r(0, 0);
	for (i=0; i<8; i++)
	{
		if (! (sel & (1 << i)))
			state &= readinputport(i);
	}
	z80pio_p_w(0, 1, state);
}

static void reset_timer_callback(int dummy)
{
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x0000);
}

static MACHINE_RESET( super8 )
{
	/* reset PIO */
	z80pio_init(0, &z80pio_intf);
	z80pio_reset(0);
	keyboard_scan();
	/* enable ROM in base >0000, and re-enable RAM shortly thereafter */
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0xC000);
	timer_set(TIME_IN_USEC(10), 0, reset_timer_callback);
}

static VIDEO_START( super80 )
{
	return 0;
}

static VIDEO_UPDATE( super80 )
{
	int x, y, chr, col;
	UINT8 *RAM = memory_region(REGION_CPU1) + vidpg;

	for (y=0; y<16; y++)
		for (x=0; x<32; x++)
		{
			chr = *RAM++;
			if (charset)
			{
				col = (chr & 0x80) ? 1 : 0;
				chr &= 0x7f;
			}
			else
				col = 0;
			drawgfx(bitmap, Machine->gfx[charset], chr, col, 0, 0, x*8, y*10,
						&Machine->screen[0].visarea, TRANSPARENCY_NONE, 0);
		}
	return 0;
}

/*
	port $F0: General Purpose output port
	Bit 0 - cassette output
	Bit 1 - cassette relay control; 0=relay on
	Bit 2 - turns screen on and off;0=screen off
	Bit 3 - Available for user projects [We will use it for sound]
	Bit 4 - Available for user projects [We will use it for video switching]
	Bit 5 - cassette LED; 0=LED on
	Bit 6/7 - not decoded
*/
static WRITE8_HANDLER (super80_gpo_w)
{
	speaker_level_w(0, (data & 0x08) ? 1 : 0);
	charset = (data & 0x10) ? 1 : 0;
}

/*
	port $F1: Video page output port
	Bit 0 - not decoded
	Bits 1 to 7 - choose video page to display
	Bit 1 controls A9, bit 2 does A10, etc
*/
static WRITE8_HANDLER (super80_vidpg_w)
{
	vidpg = (data & ~1) << 8;
}

/*
	port $F2: General purpose input port
	Bit 0 - cassette input
	Bit 1 - Available for user projects
	Bit 2 - Available for user projects
	Bit 3 - not decoded
	Bit 4 - Switch A [These switches are actual DIP switches on the motherboard]
	Bit 5 - Switch B
	Bit 6 - Switch C
	Bit 7 - Switch D
*/
static READ8_HANDLER (super80_gpi_r)
{
	return 0;
}

static WRITE8_HANDLER (super80_z80pio_w)
{
	z80pio_0_w(offset, data);
	if (offset == 0)
		/* port A write: scan keyboard */
		keyboard_scan();
}

static ADDRESS_MAP_START(super80_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE(MRA8_BANK1, MWA8_RAM)
	AM_RANGE(0x1000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static READ8_HANDLER( pio_0_r )
{
	return (offset & 2) ? z80pio_c_r(0, offset & 1) : z80pio_d_r(0, offset & 1);
}

static ADDRESS_MAP_START(super80_ports, ADDRESS_SPACE_IO, 8)
	//AM_RANGE(0xdc, 0xdc) AM_READWRITE(super80_parallel_r, super80_parallel_w)
	//AM_RANGE(0xdd, 0xdd) AM_READWRITE(super80_serial0_r, super80_serial0_w)
	//AM_RANGE(0xde, 0xde) AM_READWRITE(super80_serial1_r, super80_serial1_w)
	AM_RANGE(0xf0, 0xf0) AM_WRITE(super80_gpo_w)
	AM_RANGE(0xf1, 0xf1) AM_WRITE(super80_vidpg_w)
	AM_RANGE(0xf2, 0xf2) AM_READ(super80_gpi_r)
	AM_RANGE(0xf8, 0xfb) AM_READWRITE(pio_0_r, super80_z80pio_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( super80 )
	PORT_START	/* line 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_TILDE) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REPT") PORT_CODE(KEYCODE_LALT) 
	PORT_START	/* line 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) 
	PORT_START	/* line 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START	/* line 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINEFEED") PORT_CODE(KEYCODE_ENTER_PAD) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) 
	PORT_START	/* line 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\")PORT_CODE(KEYCODE_BACKSLASH) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BRK") PORT_CODE(KEYCODE_NUMLOCK) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START	/* line 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START	/* line 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START	/* line 7 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) 
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) 
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) 
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) 
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) 
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) 
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LOCK") PORT_CODE(KEYCODE_CAPSLOCK) 
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

gfx_layout super80_elg4_charlayout =
{
	8,10,					/* 8 x 10 characters */
	256,					/* 256 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8,  2*8,  4*8,  6*8,  8*8, 10*8, 12*8, 14*8,
	   1*8,  3*8,  5*8,  7*8,  9*8, 11*8, 13*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

gfx_layout super80_dslc_charlayout =
{
	8,10,					/* 8 x 10 characters */
	128,					/* 128 characters */
	1,						/* 1 bits per pixel */
	{ 0 },					/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8,  2*8,  4*8,  6*8,  8*8, 10*8, 12*8, 14*8,
	   1*8,  3*8,  5*8,  7*8,  9*8, 11*8, 13*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static gfx_decode super80_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &super80_elg4_charlayout, 0, 2},
	{ REGION_GFX1, 0x1000, &super80_dslc_charlayout, 0, 2},
	{ -1 }	/* end of array */
};

static UINT8 bw_palette[] =
{
	0x00,0x00,0x00,	/* black */
	0xff,0xff,0xff	/* white */
};

static short bw_colortable[] =
{
	0, 1,
	1, 0
};

static PALETTE_INIT( super80_bw )
{
	palette_set_colors(machine, 0, bw_palette, sizeof(bw_palette) / 3);
	memcpy(colortable, & bw_colortable, sizeof(bw_colortable));
}

static struct z80_irq_daisy_chain super80_daisy_chain[] =
{
	{z80pio_reset, z80pio_interrupt, z80pio_reti, 0},
	{0,0,0,-1}
};



static MACHINE_DRIVER_START( super80 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 2000000)		/* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(super80_mem, 0)
	MDRV_CPU_IO_MAP(super80_ports, 0)
	MDRV_CPU_CONFIG(super80_daisy_chain)
	//MDRV_CPU_VBLANK_INT(super80_interrupt,1)
	MDRV_SCREEN_REFRESH_RATE(50)
	//MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(2500))
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( super80 )

	MDRV_GFXDECODE(super80_gfxdecodeinfo)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 16*10)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0, 16*10-1)
	MDRV_PALETTE_LENGTH(sizeof(bw_palette)/sizeof(bw_palette[0])/3)
	MDRV_COLORTABLE_LENGTH(sizeof(bw_colortable)/sizeof(bw_colortable[0]))
	MDRV_PALETTE_INIT(super80_bw)

	MDRV_VIDEO_START(super80)
	MDRV_VIDEO_UPDATE(super80)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END


ROM_START( super80 )
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("s80c8r5.007",  0xc000, 0x1000, CRC(294f217c))
	ROM_LOAD("s80d8r5.007",  0xd000, 0x1000, CRC(9765793e))
	ROM_LOAD("s80e8r5.007",  0xe000, 0x1000, CRC(5f65d94b))

	ROM_REGION(0x1800,REGION_GFX1,0)
	ROM_LOAD("s80helg4",  0x0000, 0x1000, CRC(ebe763a7))
	ROM_LOAD("s80hdslc",  0x1000, 0x0800, CRC(cb4c81e2))
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

SYSTEM_CONFIG_START(super80)
	//CONFIG_DEVICE_CASSETTE			(1, NULL)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY   FULLNAME */
COMP( 1981, super80,  0,		0,		super80,  super80,  0,        super80,	"Dick Smith",  "Super-80" , 0)
