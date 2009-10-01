/***************************************************************************
Jupiter Ace memory map

    CPU: Z80
        0000-1fff ROM
        2000-23ff Mirror of 2400-27FF
        2400-27ff RAM (1K RAM used for screen and edit/cassette buffer)
        2800-2bff Mirror of 2C00-2FFF
        2c00-2fff RAM (1K RAM for char set, write only)
        3000-3bff Mirror of 3C00-3FFF
        3c00-3fff RAM (1K RAM standard)
        4000-7fff RAM (16K Expansion)
        8000-ffff RAM (48K Expansion)

Interrupts:

    IRQ:
        50Hz vertical sync

Ports:

    Out 0xfe:
        Tape and buzzer

    In 0xfe:
        Keyboard input, tape, and buzzer
***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/jupiter.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "sound/speaker.h"
#include "formats/jupi_tap.h"


/* This needs to be moved to drivers/xtal */
#define XTAL_6_5MHz	6500000

static emu_timer	*jupiter_set_irq_timer;
static emu_timer	*jupiter_clear_irq_timer;
static UINT8		*jupiter_charram;
static UINT8		*jupiter_expram;


static READ8_HANDLER( jupiter_io_r );
static WRITE8_HANDLER( jupiter_port_fe_w );
static WRITE8_HANDLER( jupiter_expram_w );
static WRITE8_HANDLER( jupiter_vh_charram_w );


/* memory w/r functions */
static ADDRESS_MAP_START( jupiter_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2400, 0x27ff) AM_MIRROR(0x0400) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x2c00, 0x2fff) AM_MIRROR(0x0400) AM_WRITE(jupiter_vh_charram_w) AM_BASE(&jupiter_charram)
	AM_RANGE(0x3c00, 0x3fff) AM_MIRROR(0x0c00) AM_RAM
	AM_RANGE(0x4800, 0xffff) AM_RAM AM_RAM_WRITE( jupiter_expram_w ) AM_BASE(&jupiter_expram)			/* Expansion RAM */
ADDRESS_MAP_END

/* port i/o functions */
static ADDRESS_MAP_START( jupiter_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x0000, 0xffff ) AM_READWRITE( jupiter_io_r, jupiter_port_fe_w )
ADDRESS_MAP_END


/* keyboard input */

static INPUT_PORTS_START (jupiter)
	PORT_START("KEY0")	/* 0: 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Symbol Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(':')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR('\xA3')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR('?')

	PORT_START("KEY1")	/* 1: 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) 			PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR('|')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR('\\')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')

	PORT_START("KEY2")	/* 2: 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('<')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR('>')

	PORT_START("KEY3")	/* 3: 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%')

	PORT_START("KEY4")	/* 4: 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) 			PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&')

	PORT_START("KEY5")	/* 5: 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR('"')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(';')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(0x00A9)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(']')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y') PORT_CHAR('[')

	PORT_START("KEY6")	/* 6: 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR('-')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h  H  \xE2\x86\x91") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')

	PORT_START("KEY7")	/* 7: 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR('.')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(',')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR('*')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR('/')

	PORT_START("CFG")	/* 8: machine config */
	PORT_DIPNAME( 0x03, 2, "RAM Size")
	PORT_DIPSETTING(	0, "No RAM Expansion")
	PORT_DIPSETTING(	1, "16KB RAM Expansion")
	PORT_DIPSETTING(	2, "48KB RAM Expansion")
INPUT_PORTS_END


static READ8_HANDLER( jupiter_io_r )
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	UINT8 data = 0xff;

	if ( ( offset & 0xff ) != 0xfe )
		return data;

	if ( ! ( offset & 0x0100 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY0" ) );
	}
	if ( ! ( offset & 0x0200 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY1" ) );
	}
	if ( ! ( offset & 0x0400 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY2" ) );
	}
	if ( ! ( offset & 0x0800 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY3" ) );
	}
	if ( ! ( offset & 0x1000 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY4" ) );
	}
	if ( ! ( offset & 0x2000 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY5" ) );
	}
	if ( ! ( offset & 0x4000 ) )
	{
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY6" ) );
	}
	if ( ! ( offset & 0x8000 ) )
	{
//      cassette_output( devtag_get_device(space->machine, "cassette"), -1 );
		speaker_level_w(speaker,0);
		data = ( data & 0xe0 ) | ( data & input_port_read( space->machine, "KEY7" ) );;
	}
	if ( cassette_input(devtag_get_device(space->machine, "cassette")) > 0 )
	{
		data &= ~0x20;
	}
	return data;
}


static WRITE8_HANDLER( jupiter_port_fe_w )
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
//  cassette_output( devtag_get_device(machine, "cassette"), 1 );
	speaker_level_w(speaker,1);
}


static WRITE8_HANDLER( jupiter_expram_w )
{
	if ( offset > 0x4000 )
	{
		if ( input_port_read(space->machine, "CFG") >= 1 )
			jupiter_expram[offset] = data;
	}
	else
	{
		if ( input_port_read(space->machine, "CFG") == 2 )
			jupiter_expram[offset] = data;
	}
}


/* graphics output */

static PALETTE_INIT( jupiter )
{
	palette_set_color(machine,0,RGB_BLACK); /* black */
	palette_set_color(machine,1,RGB_WHITE); /* white */
	palette_set_color(machine,2,RGB_WHITE); /* white */
	palette_set_color(machine,3,RGB_BLACK); /* black */
}


static const gfx_layout jupiter_charlayout =
{
	8, 8,	/* 8x8 characters */
	128,	/* 128 characters */
	1,		/* 1 bits per pixel */
	{0},	/* no bitplanes; 1 bit per pixel */
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8 	/* each character takes 8 consecutive bytes */
};


static GFXDECODE_START( jupiter )
	GFXDECODE_ENTRY( "maincpu", 0x2c00, jupiter_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0x2c00, jupiter_charlayout, 2, 1 )
GFXDECODE_END


static WRITE8_HANDLER( jupiter_vh_charram_w )
{
	if(data == jupiter_charram[offset])
		return; /* no change */

	jupiter_charram[offset] = data;

	/* decode character graphics again */
	offset >>= 3;
	gfx_element_mark_dirty(space->machine->gfx[0], offset);
	gfx_element_mark_dirty(space->machine->gfx[1], offset);
}


static TIMER_CALLBACK( jupiter_set_irq_callback )
{
	cputag_set_input_line(machine, "maincpu", 0, ASSERT_LINE);
}


static TIMER_CALLBACK( jupiter_clear_irq_callback )
{
	cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);
}


static VIDEO_START( jupiter )
{
	jupiter_set_irq_timer = timer_alloc(machine,  jupiter_set_irq_callback, NULL );
	jupiter_clear_irq_timer = timer_alloc(machine,  jupiter_clear_irq_callback, NULL );

	timer_adjust_periodic( jupiter_set_irq_timer,
		video_screen_get_time_until_pos( machine->primary_screen, 31*8, 0 ),
		0, video_screen_get_frame_period( machine->primary_screen ) );

	timer_adjust_periodic( jupiter_clear_irq_timer,
		video_screen_get_time_until_pos( machine->primary_screen, 32*8, 0 ),
		0, video_screen_get_frame_period( machine->primary_screen ) );
}


static VIDEO_UPDATE( jupiter )
{
	int offs;

	for(offs = 0; offs < 768; offs++)
	{
		int code = videoram[offs];
		int sx, sy;

		sy = (offs / 32) << 3;
		sx = (offs % 32) << 3;

		drawgfx_opaque(bitmap, NULL, ( code & 0x80 ) ? screen->machine->gfx[1] : screen->machine->gfx[0],
			code & 0x7f, 0, 0,0, sx,sy);
	}

	return 0;
}

static DRIVER_INIT( jupiter )
{
	jupiter_charram = memory_region(machine, "maincpu")+0x2c00;
}

static const cassette_config jupiter_cassette_config =
{
	jupiter_cassette_formats,
	NULL,
	CASSETTE_STOPPED
};

/* machine definition */
static MACHINE_DRIVER_START( jupiter )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_6_5MHz/2)        /* 3.25 MHz */
	MDRV_CPU_PROGRAM_MAP(jupiter_mem)
	MDRV_CPU_IO_MAP(jupiter_io)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( jupiter )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS( XTAL_6_5MHz, 416, 0, 256, 264, 0, 192 )

	MDRV_GFXDECODE( jupiter )
	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(jupiter)

	MDRV_VIDEO_START( jupiter )
	MDRV_VIDEO_UPDATE( jupiter )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", jupiter_cassette_config )

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("ace")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(jupiter_ace)
MACHINE_DRIVER_END


ROM_START (jupiter)
	ROM_REGION (0x10000, "maincpu",0)
	ROM_LOAD ("jupiter.lo", 0x0000, 0x1000, CRC(dc8438a5) SHA1(8fa97eb71e5dd17c7d190c6587ee3840f839347c))
	ROM_LOAD ("jupiter.hi", 0x1000, 0x1000, CRC(4009f636) SHA1(98c5d4bcd74bcf014268cf4c00b2007ea5cc21f3))
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1981, jupiter,  0,	0,	jupiter,  jupiter,  jupiter,  0,	"Cantab",  "Jupiter Ace" , 0)
