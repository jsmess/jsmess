/*

	Atari Portfolio

	http://portfolio.wz.cz/
	http://www.pofowiki.de/doku.php
	http://www.best-electronics-ca.com/portfoli.htm
	http://www.atari-portfolio.co.uk/pfnews/pf9.txt

	Undumped cartridges:

	Utility-Card				HPC-701
	Finance-Card				HPC-702
	Science-Card				HPC-703	
	File Manager / Tutorial		HPC-704
	PowerBasic					HPC-705
	Instant Spell				HPC-709
	Hyperlist					HPC-713
	Bridge Baron				HPC-724
	Wine Companion				HPC-725	
	Diet / Cholesterol Counter	HPC-726	
	Astrologer					HPC-728	
	Stock Tracker				HPC-729	
	Chess						HPC-750

*/

/*

	TODO:

	- clock is running too fast
	- access violation after OFF command
	- create chargen ROM from tech manual
	- memory error interrupt vector
	- i/o port 8051
	- screen contrast
	- system tick frequency selection (1 or 128 Hz)
	- speaker
	- credit card memory (A:/B:)
	- software list

*/

#include "emu.h"
#include "includes/portfoli.h"
#include "cpu/i86/i86.h"
#include "devices/cartslot.h"
#include "devices/messram.h"
#include "devices/printer.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/ins8250.h"
#include "machine/nvram.h"
#include "sound/speaker.h"
#include "video/hd61830.h"

//**************************************************************************
//	MACROS / CONSTANTS
//**************************************************************************

enum
{
	INT_TICK = 0,
	INT_KEYBOARD,
	INT_ERROR,
	INT_EXTERNAL
};

enum
{
	PID_COMMCARD = 0x00,
	PID_SERIAL,
	PID_PARALLEL,
	PID_PRINTER,
	PID_MODEM,
	PID_NONE = 0xff
};

static UINT8 INTERRUPT_VECTOR[] = { 0x08, 0x09, 0x00 };

//**************************************************************************
//	INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  check_interrupt - check interrupt status
//-------------------------------------------------

static void check_interrupt(running_machine *machine)
{
	portfolio_state *state = machine->driver_data<portfolio_state>();

	int level = (state->ip & state->ie) ? ASSERT_LINE : CLEAR_LINE;
	
	cputag_set_input_line(machine, M80C88A_TAG, INPUT_LINE_INT0, level);
}

//-------------------------------------------------
//  trigger_interrupt - trigger interrupt request
//-------------------------------------------------

static void trigger_interrupt(running_machine *machine, int level)
{
	portfolio_state *state = machine->driver_data<portfolio_state>();

	// set interrupt pending bit
	state->ip |= 1 << level;

	check_interrupt(machine);
}

//-------------------------------------------------
//  irq_status_r - interrupt status read
//-------------------------------------------------

static READ8_HANDLER( irq_status_r )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	return state->ip;
}

//-------------------------------------------------
//  irq_mask_w - interrupt enable mask
//-------------------------------------------------

static WRITE8_HANDLER( irq_mask_w )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	state->ie = data;
	//logerror("IE %02x\n", data);

	check_interrupt(space->machine);
}

//-------------------------------------------------
//  sivr_w - serial interrupt vector register
//-------------------------------------------------

static WRITE8_HANDLER( sivr_w )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	state->sivr = data;
	//logerror("SIVR %02x\n", data);
}

//-------------------------------------------------
//  IRQ_CALLBACK( portfolio_int_ack )
//-------------------------------------------------

static IRQ_CALLBACK( portfolio_int_ack )
{
	portfolio_state *state = device->machine->driver_data<portfolio_state>();

	UINT8 vector = state->sivr;
	
	for (int i = 0; i < 4; i++)
	{
		if (BIT(state->ip, i))
		{
			// clear interrupt pending bit
			state->ip &= ~(1 << i);

			if (i == 3)
				vector = state->sivr;
			else
				vector = INTERRUPT_VECTOR[i];

			break;
		}
	}

	check_interrupt(device->machine);

	return vector;
}

//**************************************************************************
//	KEYBOARD
//**************************************************************************

//-------------------------------------------------
//  scan_keyboard - scan keyboard
//-------------------------------------------------

static void scan_keyboard(running_machine *machine)
{
	portfolio_state *state = machine->driver_data<portfolio_state>();

	UINT8 keycode = 0xff;

	static const char *const keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	for (int row = 0; row < 8; row++)
	{
		UINT8 data = input_port_read(machine, keynames[row]);

		if (data != 0xff)
		{
			for (int col = 0; col < 8; col++)
			{
				if (!BIT(data, col))
				{
					keycode = (row * 8) + col;
				}
			}
		}
	}

	if (keycode != 0xff)
	{
		// key pressed
		if (keycode != state->keylatch)
		{
			state->keylatch = keycode;

			trigger_interrupt(machine, INT_KEYBOARD);
		}
	}
	else
	{
		// key released
		if (state->keylatch != 0xff)
		{
			state->keylatch |= 0x80;

			trigger_interrupt(machine, INT_KEYBOARD);
		}
	}
}

//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( keyboard_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( keyboard_tick )
{
	scan_keyboard(timer.machine);
}

//-------------------------------------------------
//  keyboard_r - keyboard scan code register
//-------------------------------------------------

static READ8_HANDLER( keyboard_r )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	return state->keylatch;
}

//**************************************************************************
//	INTERNAL SPEAKER
//**************************************************************************

//-------------------------------------------------
//  speaker_w - internal speaker output
//-------------------------------------------------

static WRITE8_HANDLER( speaker_w )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		
		5		
		6		
		7		speaker level

	*/

	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	speaker_level_w(state->speaker, !BIT(data, 7));

	//logerror("SPEAKER %02x\n", data);
}

//**************************************************************************
//	POWER MANAGEMENT
//**************************************************************************

//-------------------------------------------------
//  power_w - power management
//-------------------------------------------------

static WRITE8_HANDLER( power_w )
{
	/*

		bit		description

		0		
		1		1=power off
		2		
		3		
		4		
		5		
		6		
		7		

	*/

	//logerror("POWER %02x\n", data);
}

//-------------------------------------------------
//  battery_r - battery status
//-------------------------------------------------

static READ8_HANDLER( battery_r )
{
	/*

		bit		signal		description

		0		
		1		
		2		
		3		
		4		
		5		PDET		1=peripheral connected
		6		BATD?		0=battery low
		7					1=cold boot

	*/

	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	UINT8 data = 0;

	/* peripheral detect */
	data |= (state->pid != PID_NONE) << 5;

	/* battery status */
	data |= BIT(input_port_read(space->machine, "BATTERY"), 0) << 6;

	return data;
}

//-------------------------------------------------
//  unknown_w - ?
//-------------------------------------------------

static WRITE8_HANDLER( unknown_w )
{
	//logerror("UNKNOWN %02x\n", data);
}

//**************************************************************************
//	SYSTEM TIMERS
//**************************************************************************

//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( system_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( system_tick )
{
	trigger_interrupt(timer.machine, INT_TICK);
}

//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( counter_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( counter_tick )
{
	portfolio_state *state = timer.machine->driver_data<portfolio_state>();

	state->counter++;
}

//-------------------------------------------------
//  counter_r - counter register read
//-------------------------------------------------

static READ8_HANDLER( counter_r )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	UINT8 data = 0;

	switch (offset)
	{
	case 0:
		data = state->counter & 0xff;
		break;

	case 1:
		data = state->counter >> 1;
		break;
	}

	return data;
}

//-------------------------------------------------
//  counter_w - counter register write
//-------------------------------------------------

static WRITE8_HANDLER( counter_w )
{
	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	switch (offset)
	{
	case 0:
		state->counter = (state->counter & 0xff00) | data;
		break;

	case 1:
		state->counter = (data << 8) | (state->counter & 0xff);
		break;
	}
}

//**************************************************************************
//	EXPANSION
//**************************************************************************

//-------------------------------------------------
//  ncc1_w - credit card memory select
//-------------------------------------------------

static WRITE8_HANDLER( ncc1_w )
{
	address_space *program = cputag_get_address_space(space->machine, M80C88A_TAG, ADDRESS_SPACE_PROGRAM);

	if (BIT(data, 0))
	{
		// system ROM
		UINT8 *rom = memory_region(space->machine, M80C88A_TAG);
		memory_install_rom(program, 0xc0000, 0xdffff, 0, 0, rom);
	}
	else
	{
		// credit card memory
		memory_unmap_readwrite(program, 0xc0000, 0xdffff, 0, 0);
	}

	//logerror("NCC %02x\n", data);
}

//-------------------------------------------------
//  pid_r - peripheral identification
//-------------------------------------------------

static READ8_HANDLER( pid_r )
{
	/*

		PID		peripheral

		00		communication card
		01		serial port
		02		parallel port
		03		printer peripheral
		04		modem
		05-3f	reserved
		40-7f	user peripherals
		80		file-transfer interface
		81-ff	reserved

	*/

	portfolio_state *state = space->machine->driver_data<portfolio_state>();

	return state->pid;
}

//**************************************************************************
//	ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( portfolio_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( portfolio_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x1efff) AM_RAM AM_SHARE("nvram1")
	AM_RANGE(0x1f000, 0x9efff) AM_RAM // expansion
	AM_RANGE(0xb0000, 0xb0fff) AM_MIRROR(0xf000) AM_RAM AM_SHARE("nvram2") // video RAM
	AM_RANGE(0xc0000, 0xdffff) AM_ROM AM_REGION(M80C88A_TAG, 0) // or credit card memory
	AM_RANGE(0xe0000, 0xfffff) AM_ROM AM_REGION(M80C88A_TAG, 0x20000)
ADDRESS_MAP_END

//-------------------------------------------------
//  ADDRESS_MAP( portfolio_io )
//-------------------------------------------------

static ADDRESS_MAP_START( portfolio_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x8000, 0x8000) AM_READ(keyboard_r)
	AM_RANGE(0x8010, 0x8010) AM_DEVREADWRITE(HD61830_TAG, hd61830_data_r, hd61830_data_w)
	AM_RANGE(0x8011, 0x8011) AM_DEVREADWRITE(HD61830_TAG, hd61830_status_r, hd61830_control_w)
	AM_RANGE(0x8020, 0x8020) AM_WRITE(speaker_w)
	AM_RANGE(0x8030, 0x8030) AM_WRITE(power_w)
	AM_RANGE(0x8040, 0x8041) AM_READWRITE(counter_r, counter_w)
	AM_RANGE(0x8050, 0x8050) AM_READWRITE(irq_status_r, irq_mask_w)
	AM_RANGE(0x8051, 0x8051) AM_READWRITE(battery_r, unknown_w)
	AM_RANGE(0x8060, 0x8060) AM_RAM AM_BASE_MEMBER(portfolio_state, contrast)
//	AM_RANGE(0x8070, 0x8077) AM_DEVREADWRITE(M82C50A_TAG, ins8250_r, ins8250_w) Serial Interface
//	AM_RANGE(0x8078, 0x807b) AM_DEVREADWRITE(M82C55A_TAG, i8255a_r, i8255a_w) Parallel Interface
	AM_RANGE(0x807c, 0x807c) AM_WRITE(ncc1_w)
	AM_RANGE(0x807f, 0x807f) AM_READWRITE(pid_r, sivr_w)
ADDRESS_MAP_END

//**************************************************************************
//	INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( portfolio )
//-------------------------------------------------

static INPUT_PORTS_START( portfolio )
	PORT_START("ROW0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Atari") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')

	PORT_START("ROW1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del Ins") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Alt") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')

	PORT_START("ROW2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) PORT_CHAR(UCHAR_MAMEKEY(TAB))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')

	PORT_START("ROW3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('"') PORT_CHAR('`')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')

	PORT_START("ROW4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')

	PORT_START("ROW5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('8')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("ROW6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Fn") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("ROW7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("BATTERY")
	PORT_CONFNAME( 0x01, 0x01, "Battery Status" )
	PORT_CONFSETTING( 0x01, DEF_STR( Normal ) )
	PORT_CONFSETTING( 0x00, "Low Battery" )

	PORT_START("PERIPHERAL")
	PORT_CONFNAME( 0xff, PID_NONE, "Peripheral" )
	PORT_CONFSETTING( PID_NONE, DEF_STR( None ) )
	PORT_CONFSETTING( PID_PARALLEL, "Intelligent Parallel Interface (HPC-101)" )
	PORT_CONFSETTING( PID_SERIAL, "Serial Interface (HPC-102)" )
INPUT_PORTS_END

//**************************************************************************
//	VIDEO
//**************************************************************************

//-------------------------------------------------
//  PALETTE_INIT( portfolio )
//-------------------------------------------------

static PALETTE_INIT( portfolio )
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}

//-------------------------------------------------
//  VIDEO_UPDATE( portfolio )
//-------------------------------------------------

static VIDEO_UPDATE( portfolio )
{
	portfolio_state *state = screen->machine->driver_data<portfolio_state>();

	hd61830_update(state->hd61830, bitmap, cliprect);

	return 0;
}

//-------------------------------------------------
//  HD61830_INTERFACE( lcdc_intf )
//-------------------------------------------------

static HD61830_RD_READ( hd61830_rd_r )
{
	UINT16 address = (md << 3) | ((ma >> 12) & 0x07);
	UINT8 data = memory_region(device->machine, HD61830_TAG)[address];

	return data;
}

static HD61830_INTERFACE( lcdc_intf )
{
	SCREEN_TAG,
	hd61830_rd_r
};

//-------------------------------------------------
//  gfx_layout charlayout
//-------------------------------------------------

static const gfx_layout charlayout =
{
	6, 8,
	256,
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

//-------------------------------------------------
//  GFXDECODE( portfolio )
//-------------------------------------------------

static GFXDECODE_START( portfolio )
	GFXDECODE_ENTRY( HD61830_TAG, 0, charlayout, 0, 2 )
GFXDECODE_END

//**************************************************************************
//	DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  I8255A_INTERFACE( ppi_intf )
//-------------------------------------------------

static WRITE8_DEVICE_HANDLER( ppi_pb_w )
{
	/*

        bit     signal

        PB0		strobe
        PB1		autofeed
        PB2		init/reset
        PB3		select in
        PB4		
        PB5		
        PB6		
        PB7		

    */

	portfolio_state *state = device->machine->driver_data<portfolio_state>();

	/* strobe */
	centronics_strobe_w(state->centronics, BIT(data, 0));
	
	/* autofeed */
	centronics_autofeed_w(state->centronics, BIT(data, 1));
	
	/* init/reset */
	centronics_init_w(state->centronics, BIT(data, 2));
	
	/* select in */
	//centronics_select_in_w(state->centronics, BIT(data, 3));
}

static READ8_DEVICE_HANDLER( ppi_pc_r )
{
	/*

        bit     signal

        PC0		paper
        PC1		select
        PC2		
        PC3		error
        PC4		busy
        PC5		ack
        PC6		
        PC7		

    */

	portfolio_state *state = device->machine->driver_data<portfolio_state>();

	UINT8 data = 0;

	/* paper end */
	data |= centronics_pe_r(state->centronics);

	/* select */
	data |= centronics_vcc_r(state->centronics) << 1;

	/* error */
	data |= centronics_fault_r(state->centronics) << 3;

	/* busy */
	data |= centronics_busy_r(state->centronics) << 4;

	/* acknowledge */
	data |= centronics_ack_r(state->centronics) << 5;

	return data;
}

static I8255A_INTERFACE( ppi_intf )
{
	DEVCB_NULL,													// Port A read
	DEVCB_NULL,													// Port B read
	DEVCB_HANDLER(ppi_pc_r),									// Port C read
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	// Port A write
	DEVCB_HANDLER(ppi_pb_w),									// Port B write
	DEVCB_NULL													// Port C write
};

//-------------------------------------------------
//  ins8250_interface i8250_intf
//-------------------------------------------------

static INS8250_INTERRUPT( i8250_intrpt_w )
{
//	trigger_interrupt(device->machine, INT_EXTERNAL);
}

const ins8250_interface i8250_intf =
{
	XTAL_1_8432MHz,
	i8250_intrpt_w,
	NULL,
	NULL,
	NULL
};

//-------------------------------------------------
//  centronics_interface centronics_intf
//-------------------------------------------------

static centronics_interface centronics_intf =
{
	TRUE,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
};

//**************************************************************************
//	IMAGE LOADING
//**************************************************************************

//-------------------------------------------------
//  DEVICE_IMAGE_LOAD( portfolio_cart )
//-------------------------------------------------

static DEVICE_IMAGE_LOAD( portfolio_cart )
{
	return IMAGE_INIT_FAIL;
}

//**************************************************************************
//	MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( portfolio )
//-------------------------------------------------

static MACHINE_START( portfolio )
{
	portfolio_state *state = machine->driver_data<portfolio_state>();
	address_space *program = cputag_get_address_space(machine, M80C88A_TAG, ADDRESS_SPACE_PROGRAM);

	/* set CPU interrupt vector callback */
	cpu_set_irq_callback(machine->device(M80C88A_TAG), portfolio_int_ack);

	/* find devices */
	state->hd61830 = machine->device(HD61830_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);
	state->speaker = machine->device(SPEAKER_TAG);
	state->timer_tick = machine->device<timer_device>(TIMER_TICK_TAG);

	/* memory expansions */
	switch (messram_get_size(machine->device("messram")))
	{
	case 128 * 1024:
		memory_unmap_readwrite(program, 0x1f000, 0x9efff, 0, 0);
		break;

	case 384 * 1024:
		memory_unmap_readwrite(program, 0x5f000, 0x9efff, 0, 0);
		break;
	}

	/* set initial values */
	state->keylatch = 0xff;
	state->sivr = 0x2a;
	state->pid = 0xff;

	/* register for state saving */
	state_save_register_global(machine, state->ip);
	state_save_register_global(machine, state->ie);
	state_save_register_global(machine, state->sivr);
	state_save_register_global(machine, state->counter);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->contrast);
	state_save_register_global(machine, state->pid);
}

//-------------------------------------------------
//  MACHINE_RESET( portfolio )
//-------------------------------------------------

static MACHINE_RESET( portfolio )
{
	portfolio_state *state = machine->driver_data<portfolio_state>();
	address_space *io = cputag_get_address_space(machine, M80C88A_TAG, ADDRESS_SPACE_IO);

	/* peripheral */
	state->pid = input_port_read(machine, "PERIPHERAL");

	switch (state->pid)
	{
	case PID_SERIAL:
		memory_install_readwrite8_device_handler(io, machine->device(M82C50A_TAG), 0x8070, 0x8077, 0, 0, ins8250_r, ins8250_w);
		break;

	case PID_PARALLEL:
		memory_install_readwrite8_device_handler(io, machine->device(M82C55A_TAG), 0x8078, 0x807b, 0, 0, i8255a_r, i8255a_w);
		break;
	}
}

//**************************************************************************
//	MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( portfolio )
//-------------------------------------------------

static MACHINE_CONFIG_START( portfolio, portfolio_state )
    /* basic machine hardware */
    MDRV_CPU_ADD(M80C88A_TAG, I8088, XTAL_4_9152MHz)
    MDRV_CPU_PROGRAM_MAP(portfolio_mem)
    MDRV_CPU_IO_MAP(portfolio_io)
	
	MDRV_MACHINE_START(portfolio)
	MDRV_MACHINE_RESET(portfolio)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, LCD)
	MDRV_SCREEN_REFRESH_RATE(72)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(240, 64)
	MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 64-1)

	MDRV_DEFAULT_LAYOUT(layout_lcd)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(portfolio)

	MDRV_VIDEO_UPDATE(portfolio)
	MDRV_GFXDECODE(portfolio)

	MDRV_HD61830_ADD(HD61830_TAG, XTAL_4_9152MHz/2/2, lcdc_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_I8255A_ADD(M82C55A_TAG, ppi_intf)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, centronics_intf)
	MDRV_INS8250_ADD(M82C50A_TAG, i8250_intf) // should be MDRV_INS8250A_ADD
	MDRV_TIMER_ADD_PERIODIC("counter", counter_tick, HZ(XTAL_32_768kHz/16384))
	MDRV_TIMER_ADD_PERIODIC(TIMER_TICK_TAG, system_tick, HZ(XTAL_32_768kHz/32768))

	/* fake keyboard */
	MDRV_TIMER_ADD_PERIODIC("keyboard", keyboard_tick, USEC(2500))

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_INTERFACE("portfolio_cart")
	MDRV_CARTSLOT_LOAD(portfolio_cart)

	/* memory card */
/*	MDRV_MEMCARD_ADD("memcard_a")
	MDRV_MEMCARD_EXTENSION_LIST("bin")
	MDRV_MEMCARD_LOAD(portfolio_memcard)
	MDRV_MEMCARD_SIZE_OPTIONS("32K,64K,128K")

	MDRV_MEMCARD_ADD("memcard_b")
	MDRV_MEMCARD_EXTENSION_LIST("bin")
	MDRV_MEMCARD_LOAD(portfolio_memcard)
	MDRV_MEMCARD_SIZE_OPTIONS("32K,64K,128K")*/

	/* software lists */
//	MDRV_SOFTWARE_LIST_ADD("cart_list", "pofo")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("128K")
	MDRV_RAM_EXTRA_OPTIONS("384K,640K")

	MDRV_NVRAM_ADD_RANDOM_FILL("nvram1")
	MDRV_NVRAM_ADD_RANDOM_FILL("nvram2")
MACHINE_CONFIG_END

//**************************************************************************
//	ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( pofo )
//-------------------------------------------------

ROM_START( pofo )
    ROM_REGION( 0x40000, M80C88A_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "dip1072", "DIP DOS 1.072" )
	ROMX_LOAD( "rom b.u4", 0x00000, 0x20000, BAD_DUMP CRC(c9852766) SHA1(c74430281bc717bd36fd9b5baec1cc0f4489fe82), ROM_BIOS(1) ) // dumped with debug.com
	ROMX_LOAD( "rom a.u3", 0x20000, 0x20000, BAD_DUMP CRC(b8fb730d) SHA1(1b9d82b824cab830256d34912a643a7d048cd401), ROM_BIOS(1) ) // dumped with debug.com

	ROM_REGION( 0x800, HD61830_TAG, 0 )
	ROM_LOAD( "hd61830 external character generator", 0x000, 0x800, BAD_DUMP CRC(747a1db3) SHA1(a4b29678fdb43791a8ce4c1ec778f3231bb422c5) ) // typed in from manual
ROM_END

//**************************************************************************
//	SYSTEM DRIVERS
//**************************************************************************

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY     FULLNAME        FLAGS */
COMP( 1989, pofo,	0,		0,		portfolio,	portfolio,	0,		"Atari",	"Portfolio",	GAME_NOT_WORKING | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
