/******************************************************************************
*
*  Fidelity Electronics Z80 based board driver
*  By Kevin 'kevtris' Horton, Jonathan Gevaryahu AKA Lord Nightmare and Sandro Ronco
*
*  All detailed RE work done by Kevin 'kevtris' Horton
*
*  TODO:
*  * Figure out why it says the first speech line twice; it shouldn't. (It sometimes does this on the sensory chess challenger real hardware)
*  * Get rom locations from pcb (done for UVC, VCC is probably similar)
*  * Add sensory chess challenger to driver, similar hardware.
*
***********************************************************************

Talking Chess Challenger (VCC)
Advanced Talking Chess Challenger (UVC)
(which both share the same hardware)
----------------------

The CPU is a Z80 running at 4MHz.  The TSI chip runs at around 25KHz, using a
470K / 100pf RC network.  This system is very very basic, and is composed of just
the Z80, 4 ROMs, the TSI chip, and an 8255.


The Z80's interrupt inputs are all pulled to VCC, so no interrupts are used.

Reset is connected to a power-on reset circuit and a button on the keypad (marked RE).

The TSI chip connects to a 4K ROM.  All of the 'Voiced' Chess Challengers
use this same ROM  (three or four).  The later chess boards use a slightly different part
number, but the contents are identical.

Memory map (VCC):
-----------
0000-0FFF: 4K 2332 ROM 101-32103
1000-1FFF: 4K 2332 ROM VCC2
2000-2FFF: 4K 2332 ROM VCC3
4000-5FFF: 1K RAM (2114 SRAM x2)
6000-FFFF: empty

Memory map (UVC):
-----------
0000-1FFF: 8K 2364 ROM 101-64017
2000-2FFF: 4K 2332 ROM 101-32010
4000-5FFF: 1K RAM (2114 SRAM x2)
6000-FFFF: empty

I/O map:
--------
00-FF: 8255 port chip [LN edit: 00-03, mirrored over the 00-FF range; program accesses F4-F7]


8255 connections:
-----------------

PA.0 - segment G, TSI A0
PA.1 - segment F, TSI A1
PA.2 - segment E, TSI A2
PA.3 - segment D, TSI A3
PA.4 - segment C, TSI A4
PA.5 - segment B, TSI A5
PA.6 - segment A, language latch Data
PA.7 - TSI START line, language latch clock (see below)

PB.0 - dot commons
PB.1 - NC
PB.2 - digit 0, bottom dot
PB.3 - digit 1, top dot
PB.4 - digit 2
PB.5 - digit 3
PB.6 - enable language switches (see below)
PB.7 - TSI DONE line

(button rows pulled up to 5V thru 2.2K resistors)
PC.0 - button row 0, german language jumper
PC.1 - button row 1, french language jumper
PC.2 - button row 2, spanish language jumper
PC.3 - button row 3, special language jumper
PC.4 - button column A
PC.5 - button column B
PC.6 - button column C
PC.7 - button column D


language switches:
------------------

When PB.6 is pulled low, the language switches can be read.  There are four.
They connect to the button rows.  When enabled, the row(s) will read low if
the jumper is present.  English only VCC's do not have the 367 or any pads stuffed.
The jumpers are labelled: french, german, spanish, and special.


language latch:
---------------

There's an unstuffed 7474 on the board that connects to PA.6 and PA.7.  It allows
one to latch the state of A12 to the speech ROM.  The english version has the chip
missing, and a jumper pulling "A12" to ground.  This line is really a negative
enable.

To make the VCC multi-language, one would install the 74367 (note: it must be a 74367
or possibly a 74LS367.  A 74HC367 would not work since they rely on the input current
to keep the inputs pulled up), solder a piggybacked ROM to the existing english
speech ROM, and finally install a 7474 dual flipflop.

This way, the game can then detect which secondary language is present, and then it can
automatically select the correct ROM(s).  I have to test wether it will do automatic
determination and give you a language option on power up or something.

***********************************************************************

Sensory Chess Challenger
add me

******************************************************************************/

#define ADDRESS_MAP_MODERN

/* Core includes */
#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/i8255a.h"
#include "machine/i8243.h"
#include "sound/beep.h"
#include "sound/s14001a.h"
#include "includes/fidelz80.h"
#include "fidelz80.lh"
#include "abc.lh"

#include "debugger.h"

/* Devices */

/******************************************************************************
    I8255 Device
******************************************************************************/

void fidelz80_state::update_display(running_machine *machine)
{
	// data for the 4x 7seg leds, bits are 0bxABCDEFG
	UINT8 out_digit = BITSWAP8( m_digit_data,7,0,1,2,3,4,5,6 ) & 0x7f;

	if (m_led_selected&0x04)
	{
		output_set_digit_value(0, out_digit);

		output_set_led_value(1, m_led_selected & 0x01);
	}
	if (m_led_selected&0x08)
	{
		output_set_digit_value(1, out_digit);

		output_set_led_value(0, m_led_selected & 0x01);
	}
	if (m_led_selected&0x10)
	{
		output_set_digit_value(2, out_digit);
	}
	if (m_led_selected&0x20)
	{
		output_set_digit_value(3, out_digit);
	}
}

READ8_MEMBER( fidelz80_state::fidelz80_portc_r )
{
	UINT8 data = 0xff;

	switch (m_kp_matrix & 0xf0)
	{
		case 0xe0:
			data = input_port_read(space.machine, "LINE1");
			break;
		case 0xd0:
			data = input_port_read(space.machine, "LINE2");
			break;
		case 0xb0:
			data = input_port_read(space.machine, "LINE3");
			break;
		case 0x70:
			data = input_port_read(space.machine, "LINE4");
			break;
	}

	return data;
}

WRITE8_MEMBER( fidelz80_state::fidelz80_portb_w )
{
	if (!(data & 0x80))
	{
		if (data&0x1) // common for two leds
			m_led_data = (data&0xc);

		m_led_selected = data;

		update_display(space.machine);
	}

	// ignoring the language switch enable for now, is bit 0x40
};

WRITE8_MEMBER( fidelz80_state::fidelz80_portc_w )
{
	m_kp_matrix = data;
};

READ8_MEMBER( fidelz80_state::cc10_portb_r )
{
	/*
    x--- ---- level select
    -xxx xxxx ??
    */

	return input_port_read(space.machine, "LEVEL");
}

WRITE8_MEMBER( fidelz80_state::cc10_porta_w )
{
	beep_set_state(m_beep, (data & 0x80) ? 0 : 1);

	m_digit_data = data;

	update_display(space.machine);
}

READ8_MEMBER( fidelz80_state::vcc_portb_r )
{
	if (s14001a_bsy_r(m_speech) != 0)
		return 0x80;
	else
		return 0x00;
}

WRITE8_MEMBER( fidelz80_state::vcc_porta_w )
{
	s14001a_set_volume(m_speech, 15); // hack, s14001a core should assume a volume of 15 unless otherwise stated...
	s14001a_reg_w(m_speech, data & 0x3f);
	s14001a_rst_w(m_speech, (data & 0x80)>>7);

	if (!(data & 0x80))
	{
		m_digit_data = data;

		update_display(space.machine);
	}
}

static I8255A_INTERFACE( cc10_ppi8255_intf )
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(fidelz80_state, cc10_portb_r),
	DEVCB_DRIVER_MEMBER(fidelz80_state, fidelz80_portc_r),
	DEVCB_DRIVER_MEMBER(fidelz80_state, cc10_porta_w),
	DEVCB_DRIVER_MEMBER(fidelz80_state, fidelz80_portb_w),
	DEVCB_DRIVER_MEMBER(fidelz80_state, fidelz80_portc_w)
};

static I8255A_INTERFACE( vcc_ppi8255_intf )
{
	DEVCB_NULL, // only bit 6 is readable (and only sometimes) and I'm not emulating the language latch unless needed
	DEVCB_DRIVER_MEMBER(fidelz80_state, vcc_portb_r), // bit 7 is readable and is the done line from the s14001a
	DEVCB_DRIVER_MEMBER(fidelz80_state, fidelz80_portc_r), // bits 0,1,2,3 are readable, have to do with input
	DEVCB_DRIVER_MEMBER(fidelz80_state, vcc_porta_w), // display segments and s14001a lines
	DEVCB_DRIVER_MEMBER(fidelz80_state, fidelz80_portb_w), // display digits and led dots
	DEVCB_DRIVER_MEMBER(fidelz80_state, fidelz80_portc_w), // bits 4,5,6,7 are writable, have to do with input
};

/******************************************************************************
    I8041 MCU
******************************************************************************/

WRITE8_MEMBER(fidelz80_state::kp_matrix_w)
{
	if (m_kp_matrix)
	{
		UINT16 out_data = BITSWAP16(m_digit_data,12,13,1,6,5,2,0,7,15,11,10,14,4,3,9,8);
		UINT16 out_digit = out_data & 0x3fff;
		//UINT8 out_led = BIT(out_data, 15);

		beep_set_state(m_beep, BIT(out_data, 14));

		// output the digit before update the matrix
		switch (m_kp_matrix)
		{
			case 0x01:
				output_set_digit_value(1, out_digit);
				break;
			case 0x02:
				output_set_digit_value(2, out_digit);
				break;
			case 0x04:
				output_set_digit_value(3, out_digit);
				break;
			case 0x08:
				output_set_digit_value(4, out_digit);
				break;
			case 0x10:
				output_set_digit_value(5, out_digit);
				break;
			case 0x20:
				output_set_digit_value(6, out_digit);
				break;
			case 0x40:
				output_set_digit_value(7, out_digit);
				break;
			case 0x80:
				output_set_digit_value(8, out_digit);
				break;
		}
	}

	m_digit_data = 0;
	memset(m_digit_line_status, 0, ARRAY_LENGTH(m_digit_line_status));

	m_kp_matrix = data;
}

READ8_MEMBER(fidelz80_state::exp_i8243_p2_r)
{
	UINT8 data = 0;

	switch (m_kp_matrix)
	{
		case 0x01:
			data = input_port_read(space.machine, "LINE1");
			break;
		case 0x02:
			data = input_port_read(space.machine, "LINE2");
			break;
		case 0x04:
			data = input_port_read(space.machine, "LINE3");
			break;
		case 0x08:
			data = input_port_read(space.machine, "LINE4");
			break;
		case 0x10:
			data = input_port_read(space.machine, "LINE5");
			break;
		case 0x20:
			data = input_port_read(space.machine, "LINE6");
			break;
		case 0x40:
			data = input_port_read(space.machine, "LINE7");
			break;
		case 0x80:
			data = input_port_read(space.machine, "LINE8");
			break;
		default:
			data = 0xf0;
	}
	return (m_i8243->i8243_p2_r(offset)&0x0f) | (data&0xf0);
}

WRITE8_MEMBER(fidelz80_state::exp_i8243_p2_w)
{
	m_i8243->i8243_p2_w(offset, data&0x0f);
}

// probably related to the card scanner
READ8_MEMBER(fidelz80_state::unknown_r)
{
	return 0;
}

READ8_MEMBER(fidelz80_state::rand_r)
{
	return space.machine->rand();
}

/******************************************************************************
    I8243 expander
******************************************************************************/

static WRITE8_DEVICE_HANDLER( digit_w )
{
	fidelz80_state *state = device->machine->driver_data<fidelz80_state>();

	if (state->m_kp_matrix)
		switch (offset)
		{
			case 0:
				if (state->m_digit_line_status[0] == 0)
				{
					state->m_digit_line_status[0] = 1;
					state->m_digit_data = (state->m_digit_data&(~0x000f)) | ((data<<0)&0x000f);
				}
				break;
			case 1:
				if (state->m_digit_line_status[1] == 0)
				{
					state->m_digit_line_status[1] = 1;
					state->m_digit_data = (state->m_digit_data&(~0x00f0)) | ((data<<4)&0x00f0);
				}
				break;
			case 2:
				if (state->m_digit_line_status[2] == 0)
				{
					state->m_digit_line_status[2] = 1;
					state->m_digit_data = (state->m_digit_data&(~0x0f00)) | ((data<<8)&0x0f00);
				}
				break;
			case 3:
				if (state->m_digit_line_status[3] == 0)
				{
					state->m_digit_line_status[3] = 1;
					state->m_digit_data = (state->m_digit_data&(~0xf000)) | ((data<<12)&0xf000);
				}
				break;
		}
}

/******************************************************************************
    basic machine
******************************************************************************/

WRITE8_MEMBER(fidelz80_state::mcu_data_w)
{
	upi41_master_w(m_i8041, 0, data);
}

WRITE8_MEMBER(fidelz80_state::mcu_command_w)
{
	upi41_master_w(m_i8041, 1, data);
}

READ8_MEMBER(fidelz80_state::mcu_data_r)
{
	return upi41_master_r(m_i8041, 0);
}

READ8_MEMBER(fidelz80_state::mcu_status_r)
{
	return upi41_master_r(m_i8041, 1);
}

void fidelz80_state::machine_reset()
{
	m_led_selected = 0;
	m_kp_matrix = 0;
	m_digit_data = 0;
	memset(m_digit_line_status, 0, ARRAY_LENGTH(m_digit_line_status));
}

/******************************************************************************
    Address Maps
******************************************************************************/

static ADDRESS_MAP_START(cc10_z80_mem, ADDRESS_SPACE_PROGRAM, 8, fidelz80_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x3000, 0x31ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(vcc_z80_mem, ADDRESS_SPACE_PROGRAM, 8, fidelz80_state)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x0fff) AM_ROM // 4k rom
	AM_RANGE(0x1000, 0x1fff) AM_ROM // 4k rom
	AM_RANGE(0x2000, 0x2fff) AM_ROM // 4k rom
	AM_RANGE(0x4000, 0x43ff) AM_RAM AM_MIRROR(0x1c00) // 1k ram (2114*2) mirrored 8 times
ADDRESS_MAP_END

static ADDRESS_MAP_START(abc_z80_mem, ADDRESS_SPACE_PROGRAM, 8, fidelz80_state)
    ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x1fff) AM_ROM // 8k rom
	AM_RANGE(0x2000, 0x3fff) AM_ROM // 8k rom
	AM_RANGE(0x4000, 0x5fff) AM_ROM // 8k rom
	AM_RANGE(0x6000, 0x67ff) AM_RAM // probably mirrored
ADDRESS_MAP_END

static ADDRESS_MAP_START(fidel_z80_io, ADDRESS_SPACE_IO, 8, fidelz80_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x03) AM_MIRROR(0xFC) AM_DEVREADWRITE_LEGACY("ppi8255", i8255a_r, i8255a_w) // 8255 i/o chip
ADDRESS_MAP_END

static ADDRESS_MAP_START(abc_z80_io, ADDRESS_SPACE_IO, 8, fidelz80_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(mcu_data_r, mcu_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(mcu_status_r, mcu_command_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(abc_mcu_io, ADDRESS_SPACE_IO, 8, fidelz80_state)
	ADDRESS_MAP_UNMAP_LOW
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_WRITE(kp_matrix_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_READWRITE(exp_i8243_p2_r, exp_i8243_p2_w)
	AM_RANGE(MCS48_PORT_PROG, MCS48_PORT_PROG) AM_DEVWRITE_LEGACY("i8243", i8243_prog_w)
	
	// probably related to the card scanner
	AM_RANGE(MCS48_PORT_T0, MCS48_PORT_T0) AM_READ(unknown_r)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(rand_r)
ADDRESS_MAP_END

/******************************************************************************
 Input Ports
******************************************************************************/

static INPUT_CHANGED( fidelz80_trigger_reset )
{
	fidelz80_state *state = field->port->machine->driver_data<fidelz80_state>();

	cpu_set_input_line(state->m_maincpu, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_CHANGED( abc_trigger_reset )
{
	fidelz80_state *state = field->port->machine->driver_data<fidelz80_state>();

	cpu_set_input_line(state->m_maincpu, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
	cpu_set_input_line(state->m_i8041, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_PORTS_START( fidelz80 )
	PORT_START("LEVEL")		// cc10 only
		PORT_CONFNAME( 0x80, 0x00, "Number of levels" )
		PORT_CONFSETTING( 0x00, "10" )
		PORT_CONFSETTING( 0x80, "3" )

	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("RE") PORT_CODE(KEYCODE_R) PORT_CHANGED(fidelz80_trigger_reset, 0)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("LV") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("A1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("E5") PORT_CODE(KEYCODE_5)

	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CB") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("DM") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("B2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F6") PORT_CODE(KEYCODE_6)

	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CL") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("PB") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("C3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("G7") PORT_CODE(KEYCODE_7)

	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("EN") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("PV") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("D4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("H8") PORT_CODE(KEYCODE_8)
INPUT_PORTS_END

static INPUT_PORTS_START( abc )
	PORT_START("LINE1")
		PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("10") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("2") PORT_CODE(KEYCODE_2)

	PORT_START("LINE2")
		PORT_BIT(0x0f,  IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("1") PORT_CODE(KEYCODE_1)

	PORT_START("LINE3")
		PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("P") PORT_CODE(KEYCODE_P)

	PORT_START("LINE4")
		PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("NT") PORT_CODE(KEYCODE_N)

	PORT_START("LINE5")
		PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("EN") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("SC") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("PL") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Spades") PORT_CODE(KEYCODE_1_PAD)

	PORT_START("LINE6")
		PORT_BIT(0x0f,  IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CL") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("DB") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("VL") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Hearts") PORT_CODE(KEYCODE_2_PAD)

	PORT_START("LINE7")
		PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Beep on/off") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("PB") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CV") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Diamonds") PORT_CODE(KEYCODE_3_PAD)

	PORT_START("LINE8")
		PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED) PORT_UNUSED
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("RE") PORT_CODE(KEYCODE_R) PORT_CHANGED(abc_trigger_reset, 0)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("BR") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("DL") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Clubs") PORT_CODE(KEYCODE_4_PAD)
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/

static MACHINE_CONFIG_START( cc10, fidelz80_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(cc10_z80_mem)
    MCFG_CPU_IO_MAP(fidel_z80_io)
    MCFG_QUANTUM_TIME(HZ(60))

    /* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_fidelz80)

	/* other hardware */
	MCFG_I8255A_ADD("ppi8255", cc10_ppi8255_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( "beep", BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( vcc, fidelz80_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(vcc_z80_mem)
    MCFG_CPU_IO_MAP(fidel_z80_io)
    MCFG_QUANTUM_TIME(HZ(60))

    /* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_fidelz80)

	/* other hardware */
	MCFG_I8255A_ADD("ppi8255", vcc_ppi8255_intf)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speech", S14001A, 25000) // around 25khz
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( abc, fidelz80_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", Z80, XTAL_4MHz) // unknown clock/divider
    MCFG_CPU_PROGRAM_MAP(abc_z80_mem)
    MCFG_CPU_IO_MAP(abc_z80_io)
    MCFG_QUANTUM_TIME(HZ(60))

    /* video hardware */
	MCFG_DEFAULT_LAYOUT(layout_abc)

	/* other hardware */
	MCFG_CPU_ADD("mcu", I8041, XTAL_4MHz) // unknown clock/divider
	MCFG_CPU_IO_MAP(abc_mcu_io)

	MCFG_I8243_ADD("i8243", NULL, digit_w)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO( "mono" )
	MCFG_SOUND_ADD( "beep", BEEP, 0 )
	MCFG_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_CONFIG_END


/******************************************************************************
 ROM Definitions
******************************************************************************/

ROM_START( cc10 )
    ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "cc10.bin",   0x0000, 0x1000, CRC(bb9e6055) SHA1(18276e57cf56465a6352239781a828c5f3d5ba63))
ROM_END

ROM_START(vcc)
    ROM_REGION(0x10000, "maincpu", 0)
    ROM_LOAD("101-32103.bin", 0x0000, 0x1000, CRC(257BB5AB) SHA1(F7589225BB8E5F3EAC55F23E2BD526BE780B38B5)) // 32014.VCC??? at location b3?
    ROM_LOAD("vcc2.bin", 0x1000, 0x1000, CRC(F33095E7) SHA1(692FCAB1B88C910B74D04FE4D0660367AEE3F4F0)) // at location a2?
    ROM_LOAD("vcc3.bin", 0x2000, 0x1000, CRC(624F0CD5) SHA1(7C1A4F4497FE5882904DE1D6FECF510C07EE6FC6)) // at location a1?

    ROM_REGION(0x2000, "speech", 0)
    ROM_LOAD("vcc-engl.bin", 0x0000, 0x1000, CRC(F35784F9) SHA1(348E54A7FA1E8091F89AC656B4DA22F28CA2E44D)) // at location c4?
ROM_END

ROM_START(uvc)
    ROM_REGION(0x10000, "maincpu", 0)
    ROM_LOAD("101-64017.b3", 0x0000, 0x2000, CRC(F1133ABF) SHA1(09DD85051C4E7D364D43507C1CFEA5C2D08D37F4)) // "MOS // 101-64017 // 3880"
    ROM_LOAD("101-32010.a1", 0x2000, 0x1000, CRC(624F0CD5) SHA1(7C1A4F4497FE5882904DE1D6FECF510C07EE6FC6)) // "NEC P9Z021 // D2332C 228 // 101-32010", == vcc3.bin on vcc

    ROM_REGION(0x2000, "speech", 0)
    ROM_LOAD("101-32107.c4", 0x0000, 0x1000, CRC(F35784F9) SHA1(348E54A7FA1E8091F89AC656B4DA22F28CA2E44D)) // "NEC P9Y019 // D2332C 229 // 101-32107", == vcc-engl.bin on vcc
ROM_END

ROM_START(abc)
    ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD("bridge_w.bin", 0x0000, 0x2000, CRC(eb1620ef) SHA1(987a9abc8c685f1a68678ea4ee65ec4a99419179))
	ROM_LOAD("bridge_r.bin", 0x2000, 0x2000, CRC(74af0019) SHA1(8dc05950c254ca050b95b93e5d0cf48f913a6d49))
	ROM_LOAD("bridge_b.bin", 0x4000, 0x2000, CRC(341d9ca6) SHA1(370876573bb9408e75f4fc797304b6c64af0590a))

    ROM_REGION(0x1000, "mcu", 0)
    ROM_LOAD("100-1009.bin", 0x0000, 0x0400, CRC(60eb343f) SHA1(8a63e95ebd62e123bdecc330c0484a47c354bd1a))
ROM_END

/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT      COMPANY                     FULLNAME                                                    FLAGS */
COMP( 1978, cc10,       0,          0,      cc10,  fidelz80, 0,      "Fidelity Electronics",   "Chess Challenger 10",						GAME_NOT_WORKING )
COMP( 1979, vcc,        0,          0,      vcc,   fidelz80, 0,      "Fidelity Electronics",   "Talking Chess Challenger (model VCC)", GAME_NOT_WORKING )
COMP( 1980, uvc,        vcc,          0,      vcc,   fidelz80, 0,      "Fidelity Electronics",   "Advanced Talking Chess Challenger (model UVC)", GAME_NOT_WORKING )
COMP( 1980, abc,        0,          0,      abc,   abc, 	 0,      "Fidelity Electronics",   "Advanced Bridge Challenger (model ABC)",	GAME_NOT_WORKING )

