/***************************************************************************

        IQ-151

        12/05/2009 Skeleton driver.

        07/June/2011 Added screen & keyboard (by looking at the Z80 code)


This computer depends on RAM just happening to be certain values at powerup.
If the conditions are not met, it may crash.

Monitor Commands:
C Call (address)
D Dump memory, any key to dump more, Return to finish
F Fill memory (start, end, withwhat)
G Goto (address)
L Cassette load
M Move (source start, source end, destination)
R Run
S Edit memory
W Cassette save (start, end, goto (0 for null))
X Display/Edit registers


ToDo:
- Add whatever devices may exist.

- Line 32 does not scroll, should it show?
  (could be reserved for a status line in a terminal mode)

- Note that the system checks for 3E at C000, if exist, jump to C000;
  otherwise then checks for non-FF at C800, if so, jumps to C800. Could be
  extra roms or some sort of boot device.

- Key beep sounds better if clock speed changed to 1MHz, but it is still
  highly annoying. Press Ctrl-G to hear the 2-tone bell.

- Cassette support is tested only in the emulator (monitor, BASIC and AMOS),
  needs to be tested with a real cassette dump.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/pic8259.h"
#include "machine/i8255.h"
#include "sound/speaker.h"
#include "imagedev/cartslot.h"
#include "imagedev/cassette.h"
#include "machine/upd765.h"
#include "formats/basicdsk.h"


#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)


class iq151_state : public driver_device
{
public:
	iq151_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_pic(*this, "pic8259"),
		  m_speaker(*this, SPEAKER_TAG),
		  m_fdc(*this, "fdc"),
		  m_cassette(*this, CASSETTE_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_pic;
	required_device<device_t> m_speaker;
	required_device<device_t> m_fdc;
	required_device<cassette_image_device> m_cassette;

	DECLARE_READ8_MEMBER(keyboard_row_r);
	DECLARE_READ8_MEMBER(keyboard_column_r);
	DECLARE_READ8_MEMBER(ppi_portc_r);
	DECLARE_WRITE8_MEMBER(ppi_portc_w);
	DECLARE_WRITE8_MEMBER(boot_bank_w);
	DECLARE_WRITE_LINE_MEMBER(pic_set_int_line);
	UINT8 *m_p_videoram;
	UINT8 m_vblank_irq_state;
	const UINT8 *m_p_chargen;
	UINT8 m_width;
	UINT8 m_cassette_clk;
	UINT8 m_cassette_data;
	virtual void machine_reset();
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);

	// AMOS
	UINT8 *m_amos_banks[4];
	DECLARE_WRITE8_MEMBER(amos_bankswitch_w);
};

READ8_MEMBER(iq151_state::keyboard_row_r)
{
	char kbdrow[6];
	UINT8 data = 0xff;

	for (int i = 0; i < 8; i++)
	{
		sprintf(kbdrow,"X%X",i);
		data &= input_port_read(machine(), kbdrow);
	}

	return data;
}

READ8_MEMBER(iq151_state::keyboard_column_r)
{
	char kbdrow[6];
	UINT8 data = 0x00;

	for (int i = 0; i < 8; i++)
	{
		sprintf(kbdrow,"X%X",i);
		if (input_port_read(machine(), kbdrow) == 0xff)
			data |= (1 << i);
	}

	return data;
}

READ8_MEMBER(iq151_state::ppi_portc_r)
{
	UINT8 data = 0x00;

	if (m_cassette_data & 0x06)
	{
		// cassette read
		data |= ((m_cassette_clk & 1) << 5);
		data |= (m_cassette->input() > 0.00 ? 0x80 : 0x00);
	}
	else
	{
		// kb read
		data = input_port_read(machine(), "X8");
	}

	return (data & 0xf0) | (m_cassette_data & 0x0f);
}


WRITE8_MEMBER(iq151_state::ppi_portc_w)
{
	speaker_level_w(m_speaker, BIT(data, 3));

	m_cassette_data = data;
}

WRITE8_MEMBER(iq151_state::boot_bank_w)
{
	memory_set_bank(machine(), "boot", data & 1);
}

static ADDRESS_MAP_START(iq151_mem, AS_PROGRAM, 8, iq151_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_RAMBANK("boot")
	AM_RANGE( 0x0800, 0x7fff ) AM_RAM
	AM_RANGE( 0xe000, 0xe7ff ) AM_ROM AM_REGION("disc2", 0)
	AM_RANGE( 0xe800, 0xefff ) AM_RAM AM_BASE(m_p_videoram)		// on Video 32/64 cartridge
	AM_RANGE( 0xf000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(iq151_io, AS_IO, 8, iq151_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x80, 0x80 ) AM_WRITE(boot_bank_w)
	AM_RANGE( 0x84, 0x87 ) AM_DEVREADWRITE("ppi8255", i8255_device, read, write)
	AM_RANGE( 0x88, 0x89 ) AM_DEVREADWRITE_LEGACY("pic8259", pic8259_r, pic8259_w )
	AM_RANGE( 0xaa, 0xaa ) AM_DEVREAD_LEGACY("fdc", upd765_status_r)
	AM_RANGE( 0xab, 0xab ) AM_DEVREADWRITE_LEGACY("fdc", upd765_data_r, upd765_data_w)
	AM_RANGE( 0xfc, 0xff ) AM_READ_PORT("FE")
ADDRESS_MAP_END


static INPUT_CHANGED( iq151_break )
{
	iq151_state *state = field.machine().driver_data<iq151_state>();
	pic8259_ir5_w(state->m_pic, newval & 1);
}

/* Input ports */
static INPUT_PORTS_START( iq151 )
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)		PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)		PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)		PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)		PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)		PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7)		PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)		PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)		PORT_CHAR('Q') PORT_CHAR('q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)		PORT_CHAR('W') PORT_CHAR('w')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)		PORT_CHAR('E') PORT_CHAR('e')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)		PORT_CHAR('R') PORT_CHAR('r')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)		PORT_CHAR('T') PORT_CHAR('t')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)		PORT_CHAR('Y') PORT_CHAR('y')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)		PORT_CHAR('U') PORT_CHAR('u')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)		PORT_CHAR('I') PORT_CHAR('i')

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)		PORT_CHAR('A') PORT_CHAR('a')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)		PORT_CHAR('S') PORT_CHAR('s')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)		PORT_CHAR('D') PORT_CHAR('d')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)		PORT_CHAR('F') PORT_CHAR('f')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)		PORT_CHAR('G') PORT_CHAR('g')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)		PORT_CHAR('H') PORT_CHAR('h')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)		PORT_CHAR('J') PORT_CHAR('j')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)		PORT_CHAR('K') PORT_CHAR('k')

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)		PORT_CHAR('Z') PORT_CHAR('z')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)		PORT_CHAR('X') PORT_CHAR('x')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)		PORT_CHAR('C') PORT_CHAR('c')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)		PORT_CHAR('V') PORT_CHAR('v')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)		PORT_CHAR('B') PORT_CHAR('b')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)		PORT_CHAR('N') PORT_CHAR('n')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)		PORT_CHAR('M') PORT_CHAR('m')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)		PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)		PORT_CHAR('O') PORT_CHAR('o')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)		PORT_CHAR('L') PORT_CHAR('l')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL)		PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ins") PORT_CODE(KEYCODE_INSERT)		PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(F1))

	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)		PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)		PORT_CHAR('P') PORT_CHAR('p')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DL") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)			PORT_CHAR(UCHAR_MAMEKEY(F2))

	PORT_START("X6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)			PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("IL") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(F4))

	PORT_START("X7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_") PORT_CODE(KEYCODE_END) // its actually some sort of graphic character
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)		PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)			PORT_CHAR(UCHAR_MAMEKEY(F3))

	PORT_START("X8")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)  PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FA") PORT_CODE(KEYCODE_RSHIFT)		// Function A
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FB") PORT_CODE(KEYCODE_RCONTROL)		// Function B

	PORT_START("BREAK")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC)   PORT_CHANGED(iq151_break, 0)  PORT_CHAR(UCHAR_MAMEKEY(ESC))

	PORT_START("FE")
	PORT_DIPNAME( 0xff, 0xff, "Screen Width")
	PORT_DIPSETTING(    0xfe, "64")
	PORT_DIPSETTING(    0xff, "32")
INPUT_PORTS_END


WRITE_LINE_MEMBER( iq151_state::pic_set_int_line )
{
	device_set_input_line(m_maincpu, 0, state ?  HOLD_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN( iq151_vblank_interrupt )
{
	iq151_state *state = device->machine().driver_data<iq151_state>();

	pic8259_ir6_w(state->m_pic, state->m_vblank_irq_state & 1);
	state->m_vblank_irq_state ^= 1;
}

static IRQ_CALLBACK(iq151_irq_callback)
{
	iq151_state *state = device->machine().driver_data<iq151_state>();

	return pic8259_acknowledge(state->m_pic);
}

static TIMER_DEVICE_CALLBACK( cassette_timer )
{
	iq151_state *state = timer.machine().driver_data<iq151_state>();

	state->m_cassette_clk ^= 1;

	state->m_cassette->output((state->m_cassette_data & 1) ^ (state->m_cassette_clk & 1) ? +1 : -1);
}

DRIVER_INIT( iq151 )
{
	iq151_state *state = machine.driver_data<iq151_state>();

	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 1, RAM + 0xf800, 0);
	memory_configure_bank(machine, "boot", 1, 1, RAM + 0x0000, 0);

	memset(state->m_amos_banks, 0, sizeof(state->m_amos_banks));

	device_set_irq_callback(state->m_maincpu, iq151_irq_callback);
}

MACHINE_RESET_MEMBER( iq151_state )
{
	m_width = input_port_read(machine(), "FE") == 0xff ? 32 : 64;
	machine().primary_screen->set_visible_area(0, (m_width == 32 ? 32*8 : 64*6)-1, 0, 32*8-1);
	memory_set_bank(machine(), "boot", 0);

	m_vblank_irq_state = 0;
}

VIDEO_START_MEMBER( iq151_state )
{
	m_p_chargen = machine().region("chargen")->base();
	m_width = 32;
}

// Note that the screen width is controlled by the port FE dipswitch, however there
// is a software setting at memory [001F]. Poking this can cause strange things to
// happen.
SCREEN_UPDATE_MEMBER( iq151_state )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,x;
	UINT16 chrstart = (m_width == 32) ? 0x800 : 0; // choose which charrom to use
	UINT16 ma = (m_width == 32) ? 0x400 : 0; // start of videoram

	for (y = 0; y < 32; y++)
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(&bitmap, sy++, 0);

			for (x = ma; x < ma + m_width; x++)
			{
				chr = m_p_videoram[x];
				if (chrstart > 0) chr &= 0x7f; // 32chr rom only has 128 characters

				gfx = m_p_chargen[chrstart | (chr<<3) | ra ];

				// in Video 32, chars above 0x7f have colors inverted
				if (m_width == 32 && m_p_videoram[x] > 0x7f)
					gfx = ~gfx;

				/* Display a scanline of a character */
				if (m_width == 32)
				{
					// bit 7 and 6 are ignored in Video 64 mode
					*p++ = BIT(gfx, 7);
					*p++ = BIT(gfx, 6);
				}
				*p++ = BIT(gfx, 5);
				*p++ = BIT(gfx, 4);
				*p++ = BIT(gfx, 3);
				*p++ = BIT(gfx, 2);
				*p++ = BIT(gfx, 1);
				*p++ = BIT(gfx, 0);
			}
		}
		ma+=m_width;
	}
	return 0;
}


//**************************************************************************
//  IQ151 cartridges management
//**************************************************************************

WRITE8_MEMBER(iq151_state::amos_bankswitch_w)
{
	address_space *prog_space = m_maincpu->memory().space(AS_PROGRAM);

	if (m_amos_banks[data & 3] != NULL)
		prog_space->install_rom(0x8000, 0xbfff, m_amos_banks[data & 3]);
	else
		prog_space->unmap_readwrite(0x8000, 0xbfff);
}


static DEVICE_IMAGE_LOAD( iq151_cart )
{
	iq151_state *state = image.device().machine().driver_data<iq151_state>();

	if (image.software_entry() != NULL)
	{
		// get the cartridge type
		const char *cart_type = image.get_feature("cart_type");
		address_space *space = state->m_maincpu->memory().space(AS_PROGRAM);

		if (!strcmp(cart_type, "BASIC6"))
		{
			// BASIC6 cartridge
			space->install_rom(0xc800, 0xe7ff, image.get_software_region("rom"));

			return IMAGE_INIT_PASS;
		}
		else if (!strcmp(cart_type, "BASICG"))
		{
			// BASIC-G cartridge
			space->install_rom(0xb000, 0xbfff, image.get_software_region("rom_a"));
			space->install_rom(0xc800, 0xe7ff, image.get_software_region("rom_b"));
			return IMAGE_INIT_PASS;
		}
		else if (!strncmp(cart_type, "AMOS", 4))
		{
			// AMOS cartridges

			switch ((const char)cart_type[4])
			{
			case '0':	// Pascal cartridge
				state->m_amos_banks[0] = (UINT8*)image.get_software_region("rom");
				space->install_rom(0x8000, 0xbfff, state->m_amos_banks[0]);
				break;
			case '1':	// Pascal 1 cartridge
				state->m_amos_banks[1] = (UINT8*)image.get_software_region("rom");
				break;
			case '2':	// Assembler cartridge
				state->m_amos_banks[2] = (UINT8*)image.get_software_region("rom");
				break;
			}

			// install AMOS bankswitch handler
			address_space *io = state->m_maincpu->memory().space(AS_IO);
			io->install_write_handler(0xec, 0xef, write8_delegate(FUNC(iq151_state::amos_bankswitch_w), state));

			return IMAGE_INIT_PASS;
		}
		else
		{
			image.seterror(IMAGE_ERROR_UNSPECIFIED, "Unknown cartridge type");
			return IMAGE_INIT_FAIL;
		}
	}

	return IMAGE_INIT_FAIL;
}

/* F4 Character Displayer */
static const gfx_layout iq151_32_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout iq151_64_charlayout =
{
	6, 8,					/* 6 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( iq151 )
	GFXDECODE_ENTRY( "chargen", 0x0800, iq151_32_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "chargen", 0x0000, iq151_64_charlayout, 0, 1 )
GFXDECODE_END

const struct pic8259_interface iq151_pic8259_config =
{
	DEVCB_DRIVER_LINE_MEMBER(iq151_state, pic_set_int_line),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};

static I8255_INTERFACE( iq151_ppi8255_intf )
{
	DEVCB_DRIVER_MEMBER(iq151_state, keyboard_row_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(iq151_state, keyboard_column_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(iq151_state, ppi_portc_r),
	DEVCB_DRIVER_MEMBER(iq151_state, ppi_portc_w)
};

static LEGACY_FLOPPY_OPTIONS_START( iq151 )
	LEGACY_FLOPPY_OPTION( iq151_disk, "iqd", "IQ-151 disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([1])
		TRACKS([77])
		SECTORS([26])
		SECTOR_LENGTH([128])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface iq151_floppy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_8_SSSD,
	LEGACY_FLOPPY_OPTIONS_NAME(iq151),
	"floppy_8",
	NULL
};

static const upd765_interface iq151_fdc_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	NULL,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{ NULL, FLOPPY_0, FLOPPY_1, NULL }
};

static const cassette_interface iq151_cassette_interface =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED),
	"iq151_cass",
	NULL
};

static MACHINE_CONFIG_START( iq151, iq151_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8080, XTAL_2MHz)
	MCFG_CPU_PROGRAM_MAP(iq151_mem)
	MCFG_CPU_IO_MAP(iq151_io)
	MCFG_CPU_VBLANK_INT("screen", iq151_vblank_interrupt)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64*6, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*6-1, 0, 32*8-1)
	MCFG_GFXDECODE(iq151)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_PIC8259_ADD("pic8259", iq151_pic8259_config)

	MCFG_I8255_ADD("ppi8255", iq151_ppi8255_intf)

	MCFG_UPD72065_ADD("fdc", iq151_fdc_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(iq151_floppy_intf)

	MCFG_CASSETTE_ADD( CASSETTE_TAG, iq151_cassette_interface )
	MCFG_TIMER_ADD_PERIODIC("cassette_timer", cassette_timer, attotime::from_hz(2000))

	/* cartridge */
	// On real hardware only 4 slots are available, because one
	// slot is always used for the Video 32/64 cartridge.
	MCFG_CARTSLOT_ADD("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(iq151_cart)
	MCFG_CARTSLOT_INTERFACE("iq151_cart")
	MCFG_CARTSLOT_ADD("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(iq151_cart)
	MCFG_CARTSLOT_INTERFACE("iq151_cart")
	MCFG_CARTSLOT_ADD("cart3")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(iq151_cart)
	MCFG_CARTSLOT_INTERFACE("iq151_cart")
	MCFG_CARTSLOT_ADD("cart4")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(iq151_cart)
	MCFG_CARTSLOT_INTERFACE("iq151_cart")
	MCFG_CARTSLOT_ADD("cart5")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,rom")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(iq151_cart)
	MCFG_CARTSLOT_INTERFACE("iq151_cart")

	/* Software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list", "iq151_cart")
	MCFG_SOFTWARE_LIST_ADD("flop_list", "iq151_flop")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( iq151 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE )
	/* A number of bios versions here. The load address is shown for each */
	ROM_SYSTEM_BIOS( 0, "orig", "Original" )
	ROMX_LOAD( "iq151_monitor_orig.rom", 0xf000, 0x1000, CRC(acd10268) SHA1(4d75c73f155ed4dc2ac51a9c22232f869cca95e2),ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "disasm", "Disassembler" )
	ROMX_LOAD( "iq151_monitor_disasm.rom", 0xf000, 0x1000, CRC(45c2174e) SHA1(703e3271a124c3ef9330ae399308afd903316ab9),ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "cpm", "CPM" )
	ROMX_LOAD( "iq151_monitor_cpm.rom", 0xf000, 0x1000, CRC(26f57013) SHA1(4df396edc375dd2dd3c82c4d2affb4f5451066f1),ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "cpmold", "CPM (old)" )
	ROMX_LOAD( "iq151_monitor_cpm_old.rom", 0xf000, 0x1000, CRC(6743e1b7) SHA1(ae4f3b1ba2511a1f91c4e8afdfc0e5aeb0fb3a42),ROM_BIOS(4))

	ROM_REGION( 0x0c00, "chargen", ROMREGION_INVERT )
	ROM_LOAD( "iq151_video64font.rom", 0x0000, 0x0800, CRC(cb6f43c0) SHA1(4b2c1d41838d569228f61568c1a16a8d68b3dadf))
	ROM_LOAD( "iq151_video32font.rom", 0x0800, 0x0400, CRC(395567a7) SHA1(18800543daf4daed3f048193c6ae923b4b0e87db))

	ROM_REGION( 0x0800, "disc2", 0 )
	ROM_LOAD( "iq151_disc2_12_5_1987_v4_0.rom", 0x0000, 0x0800, CRC(b189b170) SHA1(3e2ca80934177e7a32d0905f5a0ad14072f9dabf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 198?, iq151,  0,       0,      iq151,     iq151,   iq151, "ZPA Novy Bor", "IQ-151", 0 )
