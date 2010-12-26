/***************************************************************************

    Osborne Executive driver file

Screen is 80x24

On boot at least bios seems to be enabled.
Boot sequence:
- set stack to and initialize Fxxx
- write stuff to/initialize 2000-27FF
- set PA to 80
- initialize some more i/o
- set PA to C0
- test C000-CFFF
- Set PA to 80
- i/o things
- Set PA to C0
- Clear VRAM with 0x20
- Write "PERFORMING SELF-TEST" to VRAM
- Set PA to 80
- (bp 0307)
- Test 2000-27FF
- Set SP to 27FF
- Set PA to 80
- Test 4000-FFFF
- Set PA to 81
- Test 4000-EFFF
- (bp 2c6)
- Test i/o


***************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "sound/speaker.h"
#include "machine/wd17xx.h"
#include "machine/6821pia.h"
#include "machine/z80dart.h"
#include "machine/pit8253.h"
#include "devices/messram.h"


#define MAIN_CLOCK	23961600


class osbexec_state : public driver_device
{
public:
	osbexec_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	running_device	*maincpu;
	running_device	*mb8877;
	running_device	*messram;
	running_device	*pia_0;
	running_device	*pia_1;
	running_device	*sio;
	running_device	*speaker;

	region_info	*fontram_region;
	region_info *vram_region;
	UINT8	*fontram;
	UINT8	*vram;
	emu_timer *video_timer;

	/* PIA 0 (UD12) */
	UINT8	pia0_porta;
	UINT8	pia0_portb;
	int		pia0_irq_state;

	/* PIA 1 (UD8) */
	int		pia1_irq_state;

	/* Vblank counter ("RTC") */
	UINT8	rtc;

	void set_banks(running_machine *machine)
	{
		UINT8 *messram_ptr = messram_get_ptr( messram );

		if ( pia0_porta & 0x01 )
			messram_ptr += 0x10000;

		memory_set_bankptr( machine, "4000", messram_ptr + 0x4000 );
		memory_set_bankptr( machine, "c000", messram_ptr + 0xc000 );
		memory_set_bankptr( machine, "e000", messram_ptr + 0xe000 );

		if ( pia0_porta & 0x80 )
			memory_set_bankptr( machine, "0000", memory_region(machine, "maincpu") );

		if ( pia0_porta & 0x40 )
			memory_set_bankptr(machine, "c000", vram_region->base() );
	}

	void update_irq_state(running_machine *machine)
	{
		if ( pia0_irq_state || pia1_irq_state )
			cpu_set_input_line( maincpu, 0, ASSERT_LINE );
		else
			cpu_set_input_line( maincpu, 0, CLEAR_LINE );
	}
};


static WRITE8_HANDLER( osbexec_0000_w )
{
	osbexec_state *state = space->machine->driver_data<osbexec_state>();

	/* Font RAM writing is enabled when ROM bank is enabled */
	if ( state->pia0_porta & 0x80 )
	{
		if ( offset < 0x1000 )
			state->fontram[ offset ] = data;
	}
}


static READ8_HANDLER( osbexec_rtc_r )
{
	osbexec_state *state = space->machine->driver_data<osbexec_state>();

	return state->rtc;
}


static ADDRESS_MAP_START( osbexec_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x1FFF ) AM_READ_BANK("0000") AM_WRITE( osbexec_0000_w )	/* ROM and maybe also banked ram */
	AM_RANGE( 0x2000, 0x3FFF ) AM_RAM											/* Banked? RAM */
	AM_RANGE( 0x4000, 0xBFFF ) AM_RAMBANK("4000")								/* Banked ram */
	AM_RANGE( 0xC000, 0xDFFF ) AM_RAMBANK("c000")								/* Video ram / Banked RAM */
	AM_RANGE( 0xE000, 0xEFFF ) AM_RAMBANK("e000")								/* Banked ram */
	AM_RANGE( 0xF000, 0xFFFF ) AM_RAM											/* 4KB of non-banked RAM for system stack etc */
ADDRESS_MAP_END


static ADDRESS_MAP_START( osbexec_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x03 ) AM_DEVREADWRITE( "pia_0", pia6821_r, pia6821_w )		/* 6821 PIA @ UD12 */
	/* 0x04 - 0x07 - 8253 @UD1 */
	/* 0x08 - 0x0B - MB8877 @UB17 */
	AM_RANGE( 0x0C, 0x0F ) AM_DEVREADWRITE( "sio", z80dart_ba_cd_r, z80dart_ba_cd_w )	/* SIO @ UD4 */
	/* 0x10 - 0x13 - 6821 PIA @UD8 */
	/* 0x14 - 0x17 - kbd */
	AM_RANGE( 0x18, 0x1b ) AM_READ( osbexec_rtc_r )								/* "RTC" @ UE13/UF13 */
	/* ?? - vid ? */
ADDRESS_MAP_END


static INPUT_PORTS_START( osbexec )
	PORT_START("ROW0")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH2)	PORT_CHAR('[') PORT_CHAR(']')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT)		PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL)		PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)			PORT_CHAR('\t')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)			PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("ROW1")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)			PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)			PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)			PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)			PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)			PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)			PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)			PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)			PORT_CHAR('1') PORT_CHAR('!')

	PORT_START("ROW2")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)			PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)			PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)			PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)			PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)			PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START("ROW3")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)			PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)			PORT_CHAR('q') PORT_CHAR('Q')

	PORT_START("ROW4")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)			PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('a') PORT_CHAR('A')

	PORT_START("ROW5")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)			PORT_CHAR('z') PORT_CHAR('Z')

	PORT_START("ROW6")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)			PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)		PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("ROW7")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_DIPNAME( 0x08, 0, "Alpha Lock" ) PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


static PALETTE_INIT( osbexec )
{
	palette_set_color_rgb( machine, 0, 0, 0, 0 );	/* Black */
	palette_set_color_rgb( machine, 1, 0, 255, 0 );	/* Full */
	palette_set_color_rgb( machine, 2, 0, 128, 0 );	/* Dimmed */
}


/*
  UD12 - 6821 PIA

  Port A:
  PA7 - ROM BANK ENA
  PA6 - VRAM BANK ENA
  PA5 - BANK6ENA
  PA4 - BANK5ENA
  PA3 - BANK4ENA
  PA2 - BANK3ENA
  PA1 - BANK2ENA
  PA0 - BANK1ENA
  CA1 - DMA IRQ
  CA2 - KBD STB (i/o)

  Port B:
  PB7 - MODEM RI (input)
  PB6 - MODEM DSR (input)
  PB5 - TXC SEL
  PB4 - RXC SEL
  PB3 - speaker
  PB2 - DSEL2
  PB1 - DSEL1
  PB0 - DDEN
  CB1 - VBlank (input)
  CB2 - 60/50 (?)
*/

static READ8_DEVICE_HANDLER( osbexec_pia0_a_r )
{
	osbexec_state *state = device->machine->driver_data<osbexec_state>();

	return state->pia0_porta;
}


static WRITE8_DEVICE_HANDLER( osbexec_pia0_a_w )
{
	osbexec_state *state = device->machine->driver_data<osbexec_state>();

	logerror("osbexec_pia0_a_w: %02x\n", data );

	state->pia0_porta = data;

	state->set_banks( device->machine );
}


static READ8_DEVICE_HANDLER( osbexec_pia0_b_r )
{
	osbexec_state *state = device->machine->driver_data<osbexec_state>();

	return state->pia0_portb;
}


static WRITE8_DEVICE_HANDLER( osbexec_pia0_b_w )
{
	osbexec_state *state = device->machine->driver_data<osbexec_state>();

	state->pia0_portb = data;

	speaker_level_w( state->speaker, ( data & 0x08 ) ? 0 : 1 );
	wd17xx_dden_w( state->mb8877, ( data & 0x01 ) ? 1 : 0 );
}


static WRITE_LINE_DEVICE_HANDLER( osbexec_pia0_irq )
{
	osbexec_state *st = device->machine->driver_data<osbexec_state>();

	st->pia0_irq_state = state;
	st->update_irq_state( device->machine );
}


static const pia6821_interface osbexec_pia0_config =
{
	DEVCB_HANDLER( osbexec_pia0_a_r ),	/* in_a_func */			/* port A - banking */
	DEVCB_HANDLER( osbexec_pia0_b_r),	/* in_b_func */			/* modem / speaker */
	DEVCB_NULL,							/* in_ca1_func */		/* DMA IRQ */
	DEVCB_NULL,							/* in_cb1_func */		/* Vblank (rtc irq) */
	DEVCB_NULL,							/* in_ca2_func */
	DEVCB_NULL,							/* in_cb2_func */
	DEVCB_HANDLER( osbexec_pia0_a_w ),	/* out_a_func */		/* port A - banking */
	DEVCB_HANDLER( osbexec_pia0_b_w ),	/* out_b_func */		/* modem / speaker */
	DEVCB_NULL,							/* out_ca2_func */		/* Keyboard strobe */
	DEVCB_NULL,							/* out_cb2_func */		/* 60/50 */
	DEVCB_LINE( osbexec_pia0_irq ),		/* irq_a_func */		/* IRQ */
	DEVCB_LINE( osbexec_pia0_irq )		/* irq_b_func */		/* IRQ */
};


static WRITE_LINE_DEVICE_HANDLER( osbexec_pia1_irq )
{
	osbexec_state *st = device->machine->driver_data<osbexec_state>();

	st->pia1_irq_state = state;
	st->update_irq_state( device->machine );
}


static const pia6821_interface osbexec_pia1_config =
{
	DEVCB_NULL,							/* in_a_func */
	DEVCB_NULL,							/* in_b_func */
	DEVCB_NULL,							/* in_ca1_func */
	DEVCB_NULL,							/* in_cb1_func */
	DEVCB_NULL,							/* in_ca2_func */
	DEVCB_NULL,							/* in_cb2_func */
	DEVCB_NULL,							/* out_a_func */
	DEVCB_NULL,							/* out_b_func */
	DEVCB_NULL,							/* out_ca2_func */
	DEVCB_NULL,							/* out_cb2_func */
	DEVCB_LINE( osbexec_pia1_irq ),		/* irq_a_func */
	DEVCB_LINE( osbexec_pia1_irq )		/* irq_b_func */
};


static Z80DART_INTERFACE( osbexec_sio_config )
{
	0, 0, 0, 0,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_NULL	//DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0)
};


/*
 * The Osborne Executive supports the following disc formats: (TODO: Verify)
 * - Osborne single density: 40 tracks, 10 sectors per track, 256-byte sectors (100 KByte)
 * - Osborne double density: 40 tracks, 5 sectors per track, 1024-byte sectors (200 KByte)
 * - IBM Personal Computer: 40 tracks, 8 sectors per track, 512-byte sectors (160 KByte)
 * - Xerox 820 Computer: 40 tracks, 18 sectors per track, 128-byte sectors (90 KByte)
 * - DEC 1820 double density: 40 tracks, 9 sectors per track, 512-byte sectors (180 KByte)
 *
 */
static FLOPPY_OPTIONS_START(osbexec )
	FLOPPY_OPTION( osd, "img", "Osborne single density", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( odd, "img", "Osborne double density", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([5])
		SECTOR_LENGTH([1024])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( ibm, "img", "IBM Personal Computer", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([8])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( xerox, "img", "Xerox 820 Computer", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([18])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
	FLOPPY_OPTION( dec, "img", "DEC 1820 double density", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([9])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END


static const floppy_config osbexec_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_SSDD_40,
	FLOPPY_OPTIONS_NAME(osbexec),
	NULL
};


static TIMER_CALLBACK( osbexec_video_callback )
{
	osbexec_state *state = machine->driver_data<osbexec_state>();
	int y = machine->primary_screen->vpos();

	/* Start of frame */
	if ( y == 0 )
	{
		/* Clear CB1 on PIA @ UD12 */
		pia6821_cb1_w( state->pia_0, 0 );
	}
	else if ( y == 240 )
	{
		/* Set CB1 on PIA @ UD12 */
		pia6821_cb1_w( state->pia_0, 1 );
		state->rtc++;
	}
	if ( y < 240 )
	{
		UINT16 row_addr = ( y / 10 ) * 128;
		UINT16 *p = BITMAP_ADDR16( machine->generic.tmpbitmap, y, 0 );
		UINT8 char_line = y % 10;

		for ( int x = 0; x < 80; x++ )
		{
			UINT8 ch = state->vram[ row_addr + x ];
			UINT8 attr = state->vram[ 0x1000 + row_addr + x ];
			UINT8 fg_col = ( attr & 0x80 ) ? 1 : 2;
			UINT8 font_bits = state->fontram[ ( ( attr & 0x10 ) ? 0x800 : 0 ) + ch * 16 + char_line ];

			/* Check for underline */
			if ( ( attr & 0x40 ) && char_line == 9 )
				font_bits = 0xFF;

			/* Check for blink */
			if ( ( attr & 0x20 ) && ( state->rtc & 0x10 ) )
				font_bits = 0;

			/* Check for inverse video */
			if ( attr & 0x10 )
				font_bits ^= 0xFF;

			for ( int b = 0; b < 8; b++ )
			{
				p[ x * 8 + b ] = ( font_bits & 0x80 ) ? fg_col : 0;
				font_bits <<= 1;
			}
		}
	}

	timer_adjust_oneshot( state->video_timer, machine->primary_screen->time_until_pos( y + 1, 0 ), 0 );
}


static DRIVER_INIT( osbexec )
{
	osbexec_state *state = machine->driver_data<osbexec_state>();

	state->fontram_region = machine->region_alloc( "fontram", 0x1000, 0 );
	state->vram_region = machine->region_alloc( "vram", 0x2000, 0 );
	state->vram = state->vram_region->base();
	state->fontram = state->fontram_region->base();


	memset( state->fontram, 0x00, 0x1000 );
	memset( state->vram, 0x00, 0x2000 );

	state->video_timer = timer_alloc( machine, osbexec_video_callback, NULL );
}


static MACHINE_START( osbexec )
{
	osbexec_state *state = machine->driver_data<osbexec_state>();

	state->maincpu = machine->device( "maincpu" );
	state->mb8877  = machine->device( "mb8877" );
	state->messram = machine->device( "messram" );
	state->pia_0   = machine->device( "pia_0" );
	state->pia_1   = machine->device( "pia_1" );
	state->sio     = machine->device( "sio" );
	state->speaker = machine->device( "speaker" );
}


static MACHINE_RESET( osbexec )
{
	osbexec_state *state = machine->driver_data<osbexec_state>();

	state->pia0_porta = 0xC0;		/* Enable ROM and VRAM on reset */

	state->set_banks( machine );

	timer_adjust_oneshot( state->video_timer, machine->primary_screen->time_until_pos( 0, 0 ), 0 );

	state->rtc = 0;
}


static const z80_daisy_config osbexec_daisy_config[] =
{
    { "sio" },
	{ NULL }
};


static MACHINE_CONFIG_START( osbexec, osbexec_state )
	MDRV_CPU_ADD( "maincpu", Z80, MAIN_CLOCK/6 )
	MDRV_CPU_PROGRAM_MAP( osbexec_mem)
	MDRV_CPU_IO_MAP( osbexec_io)
	MDRV_CPU_CONFIG( osbexec_daisy_config )

	MDRV_MACHINE_START( osbexec )
	MDRV_MACHINE_RESET( osbexec )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS( MAIN_CLOCK/2, 768, 0, 640, 260, 0, 240 )	/* May not be correct */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )
	MDRV_PALETTE_LENGTH( 3 )
	MDRV_PALETTE_INIT( osbexec )

	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )

//	MDRV_PIT8253_ADD( "pit", osbexec_pit_config )

	MDRV_PIA6821_ADD( "pia_0", osbexec_pia0_config )
	MDRV_PIA6821_ADD( "pia_1", osbexec_pia1_config )

	MDRV_Z80SIO2_ADD( "sio", MAIN_CLOCK/6, osbexec_sio_config )

	MDRV_MB8877_ADD("mb8877", default_wd17xx_interface_2_drives )

	MDRV_FLOPPY_2_DRIVES_ADD(osbexec_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")	/* 128KB Main RAM */
MACHINE_CONFIG_END


ROM_START( osbexec )
	ROM_REGION(0x2000, "maincpu", 0)
	ROM_LOAD( "execv12.ud18", 0x0000, 0x2000, CRC(70798c2f) SHA1(2145a72da563bed1d6d455c77e48cc011a5f1153) )	/* Checksum C6B2 */
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY     FULLNAME        FLAGS */
COMP( 1982, osbexec,    0,      0,      osbexec,    osbexec,    osbexec,    "Osborne",  "Executive",    GAME_NOT_WORKING )
