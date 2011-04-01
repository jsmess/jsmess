/***************************************************************************

    SAM Coupe

    Miles Gordon Technology, 1989

    Driver by Lee Hammerton, Dirk Best


    Notes:
    ------

    Beware - the early ROMs are very buggy, and tend to go mad once BASIC
    starts paging.  ROM 10 (version 1.0) requires the CALL after F9 or BOOT
    because the ROM loads the bootstrap but fails to execute it. The address
    depends on the RAM size:

    On a 256K SAM: CALL 229385
    Or on a 512K (or larger) machine: CALL 491529


    Todo:
    -----

    - Attribute read
    - Better timing
    - Harddisk interfaces

***************************************************************************/

/* core includes */
#include "emu.h"
#include "includes/samcoupe.h"

/* components */
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "machine/msm6242.h"
#include "machine/ctronics.h"
#include "sound/saa1099.h"
#include "sound/speaker.h"

/* devices */
#include "imagedev/cassette.h"
#include "formats/tzx_cas.h"
#include "imagedev/flopdrv.h"
#include "formats/coupedsk.h"
#include "machine/ram.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define SAMCOUPE_XTAL_X1  XTAL_24MHz
#define SAMCOUPE_XTAL_X2  XTAL_4_433619MHz


/***************************************************************************
    I/O PORTS
***************************************************************************/

static READ8_HANDLER( samcoupe_disk_r )
{
	device_t *fdc = space->machine().device("wd1772");

	/* drive and side is encoded into bit 5 and 3 */
	wd17xx_set_drive(fdc, (offset >> 4) & 1);
	wd17xx_set_side(fdc, (offset >> 2) & 1);

	/* bit 1 and 2 select the controller register */
	switch (offset & 0x03)
	{
	case 0: return wd17xx_status_r(fdc, 0);
	case 1: return wd17xx_track_r(fdc, 0);
	case 2: return wd17xx_sector_r(fdc, 0);
	case 3: return wd17xx_data_r(fdc, 0);
	}

	return 0xff;
}

static WRITE8_HANDLER( samcoupe_disk_w )
{
	device_t *fdc = space->machine().device("wd1772");

	/* drive and side is encoded into bit 5 and 3 */
	wd17xx_set_drive(fdc, (offset >> 4) & 1);
	wd17xx_set_side(fdc, (offset >> 2) & 1);

	/* bit 1 and 2 select the controller register */
	switch (offset & 0x03)
	{
	case 0: wd17xx_command_w(fdc, 0, data); break;
	case 1: wd17xx_track_w(fdc, 0, data);   break;
	case 2: wd17xx_sector_w(fdc, 0, data);  break;
	case 3: wd17xx_data_w(fdc, 0, data);    break;
	}
}

static READ8_HANDLER( samcoupe_pen_r )
{
	screen_device *scr = space->machine().primary_screen;
	UINT8 data;

	if (offset & 0x100)
	{
		int vpos = scr->vpos();

		/* return the current screen line or 192 for the border area */
		if (vpos < SAM_BORDER_TOP || vpos >= SAM_BORDER_TOP + SAM_SCREEN_HEIGHT)
			data = 192;
		else
			data = vpos - SAM_BORDER_TOP;
	}
	else
	{
		/* horizontal position is encoded into bits 3 to 8 */
		data = scr->hpos() & 0xfc;
	}

	return data;
}

static WRITE8_HANDLER( samcoupe_clut_w )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	state->m_clut[(offset >> 8) & 0x0f] = data & 0x7f;
}

static READ8_HANDLER( samcoupe_status_r )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	UINT8 data = 0xe0;

	/* bit 5-7, keyboard input */
	if (!BIT(offset,  8)) data &= input_port_read(space->machine(), "keyboard_row_fe") & 0xe0;
	if (!BIT(offset,  9)) data &= input_port_read(space->machine(), "keyboard_row_fd") & 0xe0;
	if (!BIT(offset, 10)) data &= input_port_read(space->machine(), "keyboard_row_fb") & 0xe0;
	if (!BIT(offset, 11)) data &= input_port_read(space->machine(), "keyboard_row_f7") & 0xe0;
	if (!BIT(offset, 12)) data &= input_port_read(space->machine(), "keyboard_row_ef") & 0xe0;
	if (!BIT(offset, 13)) data &= input_port_read(space->machine(), "keyboard_row_df") & 0xe0;
	if (!BIT(offset, 14)) data &= input_port_read(space->machine(), "keyboard_row_bf") & 0xe0;
	if (!BIT(offset, 15)) data &= input_port_read(space->machine(), "keyboard_row_7f") & 0xe0;

	/* bit 0-4, interrupt source */
	data |= state->m_status;

	return data;
}

static WRITE8_HANDLER( samcoupe_line_int_w )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	state->m_line_int = data;
}

static READ8_HANDLER( samcoupe_lmpr_r )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	return state->m_lmpr;
}

static WRITE8_HANDLER( samcoupe_lmpr_w )
{
	address_space *space_program = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();

	state->m_lmpr = data;
	samcoupe_update_memory(space_program);
}

static READ8_HANDLER( samcoupe_hmpr_r )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	return state->m_hmpr;
}

static WRITE8_HANDLER( samcoupe_hmpr_w )
{
	address_space *space_program = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();

	state->m_hmpr = data;
	samcoupe_update_memory(space_program);
}

static READ8_HANDLER( samcoupe_vmpr_r )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	return state->m_vmpr;
}

static WRITE8_HANDLER( samcoupe_vmpr_w )
{
	address_space *space_program = space->machine().device("maincpu")->memory().space(AS_PROGRAM);
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();

	state->m_vmpr = data;
	samcoupe_update_memory(space_program);
}

static READ8_HANDLER( samcoupe_midi_r )
{
	logerror("%s: read from midi port\n", space->machine().describe_context());
	return 0xff;
}

static WRITE8_HANDLER( samcoupe_midi_w )
{
	logerror("%s: write to midi port: 0x%02x\n", space->machine().describe_context(), data);
}

static READ8_HANDLER( samcoupe_keyboard_r )
{
	device_t *cassette = space->machine().device("cassette");
	UINT8 data = 0x1f;

	/* bit 0-4, keyboard input */
	if (!BIT(offset,  8)) data &= input_port_read(space->machine(), "keyboard_row_fe") & 0x1f;
	if (!BIT(offset,  9)) data &= input_port_read(space->machine(), "keyboard_row_fd") & 0x1f;
	if (!BIT(offset, 10)) data &= input_port_read(space->machine(), "keyboard_row_fb") & 0x1f;
	if (!BIT(offset, 11)) data &= input_port_read(space->machine(), "keyboard_row_f7") & 0x1f;
	if (!BIT(offset, 12)) data &= input_port_read(space->machine(), "keyboard_row_ef") & 0x1f;
	if (!BIT(offset, 13)) data &= input_port_read(space->machine(), "keyboard_row_df") & 0x1f;
	if (!BIT(offset, 14)) data &= input_port_read(space->machine(), "keyboard_row_bf") & 0x1f;
	if (!BIT(offset, 15)) data &= input_port_read(space->machine(), "keyboard_row_7f") & 0x1f;

	if (offset == 0xff00)
	{
		data &= input_port_read(space->machine(), "keyboard_row_ff") & 0x1f;

		/* if no key has been pressed, return the mouse state */
		if (data == 0x1f)
			data = samcoupe_mouse_r(space->machine());
	}

	/* bit 5, lightpen strobe */
	data |= 1 << 5;

	/* bit 6, cassette input */
	data |= (cassette_input(cassette) > 0 ? 1 : 0) << 6;

	/* bit 7, external memory */
	data |= 1 << 7;

	return data;
}

static WRITE8_HANDLER( samcoupe_border_w )
{
	device_t *cassette = space->machine().device("cassette");
	device_t *speaker = space->machine().device("speaker");
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();

	state->m_border = data;

	/* bit 3, cassette output */
	cassette_output(cassette, BIT(data, 3) ? -1.0 : +1.0);

	/* bit 4, beep */
	speaker_level_w(speaker, BIT(data, 4));
}

static READ8_HANDLER( samcoupe_attributes_r )
{
	samcoupe_state *state = space->machine().driver_data<samcoupe_state>();
	return state->m_attribute;
}

static READ8_DEVICE_HANDLER( samcoupe_lpt1_busy_r )
{
	return centronics_busy_r(device);
}

static WRITE8_DEVICE_HANDLER( samcoupe_lpt1_strobe_w )
{
	centronics_strobe_w(device, data);
}

static READ8_DEVICE_HANDLER( samcoupe_lpt2_busy_r )
{
	return centronics_busy_r(device);
}

static WRITE8_DEVICE_HANDLER( samcoupe_lpt2_strobe_w )
{
	centronics_strobe_w(device, data);
}


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( samcoupe_mem, AS_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("bank2")
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( samcoupe_io, AS_IO, 8 )
	AM_RANGE(0x0080, 0x0081) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_WRITE(samcoupe_ext_mem_w)
	AM_RANGE(0x00e0, 0x00e7) AM_MIRROR(0xff10) AM_MASK(0xffff) AM_READWRITE(samcoupe_disk_r, samcoupe_disk_w)
	AM_RANGE(0x00e8, 0x00e8) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_DEVWRITE("lpt1", centronics_data_w)
	AM_RANGE(0x00e9, 0x00e9) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_DEVREADWRITE("lpt1", samcoupe_lpt1_busy_r, samcoupe_lpt1_strobe_w)
	AM_RANGE(0x00ea, 0x00ea) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_DEVWRITE("lpt2", centronics_data_w)
	AM_RANGE(0x00eb, 0x00eb) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_DEVREADWRITE("lpt2", samcoupe_lpt2_busy_r, samcoupe_lpt2_strobe_w)
	AM_RANGE(0x00f8, 0x00f8) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_pen_r, samcoupe_clut_w)
	AM_RANGE(0x00f9, 0x00f9) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_status_r, samcoupe_line_int_w)
	AM_RANGE(0x00fa, 0x00fa) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_lmpr_r, samcoupe_lmpr_w)
	AM_RANGE(0x00fb, 0x00fb) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_hmpr_r, samcoupe_hmpr_w)
	AM_RANGE(0x00fc, 0x00fc) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_vmpr_r, samcoupe_vmpr_w)
	AM_RANGE(0x00fd, 0x00fd) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_midi_r, samcoupe_midi_w)
	AM_RANGE(0x00fe, 0x00fe) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READWRITE(samcoupe_keyboard_r, samcoupe_border_w)
	AM_RANGE(0x00ff, 0x00ff) AM_MIRROR(0xff00) AM_MASK(0xffff) AM_READ(samcoupe_attributes_r)
	AM_RANGE(0x00ff, 0x00ff) AM_MIRROR(0xfe00) AM_MASK(0xffff) AM_DEVWRITE("saa1099", saa1099_data_w)
	AM_RANGE(0x01ff, 0x01ff) AM_MIRROR(0xfe00) AM_MASK(0xffff) AM_DEVWRITE("saa1099", saa1099_control_w)
ADDRESS_MAP_END


/***************************************************************************
    INTERRUPTS
***************************************************************************/

static TIMER_CALLBACK( irq_off )
{
	samcoupe_state *state = machine.driver_data<samcoupe_state>();
	/* adjust STATUS register */
	state->m_status |= param;

	/* clear interrupt */
	if ((state->m_status & 0x1f) == 0x1f)
		cputag_set_input_line(machine, "maincpu", 0, CLEAR_LINE);

}

void samcoupe_irq(device_t *device, UINT8 src)
{
	samcoupe_state *state = device->machine().driver_data<samcoupe_state>();

	/* assert irq and a timer to set it off again */
	device_set_input_line(device, 0, ASSERT_LINE);
	device->machine().scheduler().timer_set(attotime::from_usec(20), FUNC(irq_off), src);

	/* adjust STATUS register */
	state->m_status &= ~src;
}

static INTERRUPT_GEN( samcoupe_frame_interrupt )
{
	/* signal frame interrupt */
	samcoupe_irq(device, SAM_FRAME_INT);
}


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( samcoupe )
	PORT_START("keyboard_row_fe")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)     PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)     PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR('?')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)     PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)     PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("keyboard_row_fd")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)     PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)     PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)     PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)     PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)     PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(F6))

	PORT_START("keyboard_row_fb")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)     PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)     PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)     PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)     PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('[')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)     PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(']')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(F9))

	PORT_START("keyboard_row_f7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)        PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)        PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)        PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)        PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)        PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)    PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)      PORT_CHAR('\t')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("keyboard_row_ef")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)         PORT_CHAR('0') PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)         PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)         PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)         PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)         PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)     PORT_CHAR('-') PORT_CHAR('/')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)    PORT_CHAR('+') PORT_CHAR('*')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DELETE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)

	PORT_START("keyboard_row_df")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)          PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)          PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)          PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('=') PORT_CHAR('_')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('"') PORT_CHAR('\xa9')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(F10))

	PORT_START("keyboard_row_bf")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)     PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR('\xa3')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)     PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)     PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)     PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR('^')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EDIT") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))

	PORT_START("keyboard_row_7f")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)     PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)     PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)     PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INV") PORT_CODE(KEYCODE_SLASH) PORT_CHAR(UCHAR_MAMEKEY(HOME)) PORT_CHAR('\\')

	PORT_START("keyboard_row_ff")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)     PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN)   PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT)   PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT)  PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("mouse_buttons")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("Mouse Button 1")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CODE(MOUSECODE_BUTTON3) PORT_NAME("Mouse Button 3")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("Mouse Button 2")
	PORT_BIT(0xf8, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("mouse_x")
	PORT_BIT(0xfff, 0x000, IPT_MOUSE_X) PORT_SENSITIVITY(50) PORT_KEYDELTA(0) PORT_REVERSE

	PORT_START("mouse_y")
	PORT_BIT(0xfff, 0x000, IPT_MOUSE_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(0)

	PORT_START("config")
	PORT_CONFNAME(0x01, 0x00, "Real Time Clock")
	PORT_CONFSETTING(   0x00, DEF_STR(None))
	PORT_CONFSETTING(   0x01, "SAMBUS")
INPUT_PORTS_END


/***************************************************************************
    PALETTE
***************************************************************************/

/*
    Decode colours for the palette as follows:

    bit     7       6       5       4       3       2       1       0
         nothing   G+4     R+4     B+4    ALL+1    G+2     R+2     B+2

*/
static PALETTE_INIT( samcoupe )
{
	for (int i = 0; i < 128; i++)
	{
		UINT8 b = BIT(i, 0) * 2 + BIT(i, 4) * 4 + BIT(i, 3);
		UINT8 r = BIT(i, 1) * 2 + BIT(i, 5) * 4 + BIT(i, 3);
		UINT8 g = BIT(i, 2) * 2 + BIT(i, 6) * 4 + BIT(i, 3);

		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}

	palette_normalize_range(machine.palette, 0, 127, 0, 255);
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static const cassette_config samcoupe_cassette_config =
{
	tzx_cassette_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED),
	NULL
};


static FLOPPY_OPTIONS_START( samcoupe )
	FLOPPY_OPTION
	(
		coupe_mgt, "mgt,dsk,sad", "SAM Coupe MGT disk image", coupe_mgt_identify, coupe_mgt_construct,
		HEADS([2])
		TRACKS([80])
		SECTORS(9-[10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1])
	)
	FLOPPY_OPTION
	(
		coupe_sad, "sad,dsk", "SAM Coupe SAD disk image", coupe_sad_identify, coupe_sad_construct,
		HEADS(1-[2]-255)
		TRACKS(1-[80]-255)
		SECTORS(1-[10]-255)
		SECTOR_LENGTH(64/128/256/[512]/1024/2048/4096)
		FIRST_SECTOR_ID([1])
	)
	FLOPPY_OPTION
	(
		coupe_sdf, "sdf,dsk,sad", "SAM Coupe SDF disk image", coupe_sdf_identify, coupe_sdf_construct,
		HEADS(1-[2])
		TRACKS(1-[80]-83)
		SECTORS(1-[10]-12)
		SECTOR_LENGTH(128/256/[512]/1024)
		FIRST_SECTOR_ID([1])
	)
FLOPPY_OPTIONS_END

static const floppy_config samcoupe_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(samcoupe),
	NULL
};

static const wd17xx_interface samcoupe_wd17xx_intf =
{
	DEVCB_LINE_GND,
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};

static MACHINE_CONFIG_START( samcoupe, samcoupe_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, SAMCOUPE_XTAL_X1 / 4) /* 6 MHz */
	MCFG_CPU_PROGRAM_MAP(samcoupe_mem)
	MCFG_CPU_IO_MAP(samcoupe_io)
	MCFG_CPU_VBLANK_INT("screen", samcoupe_frame_interrupt)

	MCFG_MACHINE_START(samcoupe)
	MCFG_MACHINE_RESET(samcoupe)

    /* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(SAMCOUPE_XTAL_X1/2, SAM_TOTAL_WIDTH, 0, SAM_BORDER_LEFT + SAM_SCREEN_WIDTH + SAM_BORDER_RIGHT, SAM_TOTAL_HEIGHT, 0, SAM_BORDER_TOP + SAM_SCREEN_HEIGHT + SAM_BORDER_BOTTOM)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_UPDATE(generic_bitmapped)

	MCFG_PALETTE_LENGTH(128)
	MCFG_PALETTE_INIT(samcoupe)

	MCFG_VIDEO_START(generic_bitmapped)

	/* devices */
	MCFG_CENTRONICS_ADD("lpt1", standard_centronics)
	MCFG_CENTRONICS_ADD("lpt2", standard_centronics)
	MCFG_MSM6242_ADD("sambus_clock")
	MCFG_WD1772_ADD("wd1772", samcoupe_wd17xx_intf)
	MCFG_CASSETTE_ADD("cassette", samcoupe_cassette_config)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MCFG_SOUND_ADD("saa1099", SAA1099, SAMCOUPE_XTAL_X1/3) /* 8 MHz */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_FLOPPY_2_DRIVES_ADD(samcoupe_floppy_config)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("512K")
	MCFG_RAM_EXTRA_OPTIONS("256K,1280K,1536K,2304K,2560K,3328K,3584K,4352K,4608K")
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

/*
    The bios is actually 32K. This is the combined version of the two old 16K MESS roms.
    It does match the 3.0 one the most, but the first half differs in one byte
    and in the second half, the case of the "plc" in the company string differs.
*/
ROM_START( samcoupe )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_SYSTEM_BIOS( 0,  "31",  "v3.1" )
	ROMX_LOAD( "rom31.z5",  0x0000, 0x8000, CRC(0b7e3585) SHA1(c86601633fb61a8c517f7657aad9af4e6870f2ee), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1,  "30",  "v3.0" )
	ROMX_LOAD( "rom30.z5",  0x0000, 0x8000, CRC(e535c25d) SHA1(d390f0be420dfb12b1e54a4f528b5055d7d97e2a), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2,  "25",  "v2.5" )
	ROMX_LOAD( "rom25.z5",  0x0000, 0x8000, CRC(ddadd358) SHA1(a25ed85a0f1134ac3a481a3225f24a8bd3a790cf), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3,  "24",  "v2.4" )
	ROMX_LOAD( "rom24.z5",  0x0000, 0x8000, CRC(bb23fee4) SHA1(10cd911ba237dd2cf0c2637be1ad6745b7cc89b9), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4,  "21",  "v2.1" )
	ROMX_LOAD( "rom21.z5",  0x0000, 0x8000, CRC(f6804b46) SHA1(11dcac5fdea782cdac03b4d0d7ac25d88547eefe), ROM_BIOS(5) )
	ROM_SYSTEM_BIOS( 5,  "20",  "v2.0" )
	ROMX_LOAD( "rom20.z5",  0x0000, 0x8000, CRC(eaf32054) SHA1(41736323f0236649f2d5fe111f900def8db93a13), ROM_BIOS(6) )
	ROM_SYSTEM_BIOS( 6,  "181", "v1.81" )
	ROMX_LOAD( "rom181.z5", 0x0000, 0x8000, CRC(d25e1de1) SHA1(cb0fa79e4d5f7df0b57ede08ea7ecc9ae152f534), ROM_BIOS(7) )
	ROM_SYSTEM_BIOS( 7,  "18",  "v1.8" )
	ROMX_LOAD( "rom18.z5",  0x0000, 0x8000, CRC(f626063f) SHA1(485e7d9e9a4f8a70c0f93cd6e69ff12269438829), ROM_BIOS(8) )
	ROM_SYSTEM_BIOS( 8,  "14",  "v1.4" )
	ROMX_LOAD( "rom14.z5",  0x0000, 0x8000, CRC(08799596) SHA1(b4e596051f2748dee9481ea4af7d15ccddc1e1b5), ROM_BIOS(9) )
	ROM_SYSTEM_BIOS( 9,  "13",  "v1.3" )
	ROMX_LOAD( "rom13.z5",  0x0000, 0x8000, CRC(2093768c) SHA1(af8d348fd080b18a4cbe9ed69d254be7be330146), ROM_BIOS(10) )
	ROM_SYSTEM_BIOS( 10, "12",  "v1.2" )
	ROMX_LOAD( "rom12.z5",  0x0000, 0x8000, CRC(7fe37dd8) SHA1(9339a0c1f72e8512c6f32dec15ab7d6c3bb04151), ROM_BIOS(11) )
	ROM_SYSTEM_BIOS( 11, "10",  "v1.0" )
	ROMX_LOAD( "rom10.z5",  0x0000, 0x8000, CRC(3659d31f) SHA1(d3de7bb74e04d5b4dc7477f70de54d540b1bcc07), ROM_BIOS(12) )
	ROM_SYSTEM_BIOS( 12, "04",  "v0.4" )
	ROMX_LOAD( "rom04.z5",  0x0000, 0x8000, CRC(f439e84e) SHA1(8bc457a5c764b0bb0aa7008c57f28c30248fc6a4), ROM_BIOS(13) )
	ROM_SYSTEM_BIOS( 13, "01",  "v0.1" )
	ROMX_LOAD( "rom01.z5",  0x0000, 0x8000, CRC(c04acfdf) SHA1(8976ed005c14905eec1215f0a5c28aa686a7dda2), ROM_BIOS(14) )
ROM_END

/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     INIT  COMPANY                        FULLNAME     FLAGS */
COMP( 1989, samcoupe, 0,      0,      samcoupe, samcoupe, 0,    "Miles Gordon Technology plc", "SAM Coupe", 0 )
