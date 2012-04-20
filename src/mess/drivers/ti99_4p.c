/****************************************************************************

    SNUG SGCPU (a.k.a. 99/4p) system

    This system is a reimplementation of the old ti99/4a console.  It is known
    both as the 99/4p ("peripheral box", since the system is a card to be
    inserted in the peripheral box, instead of a self contained console), and
    as the SGCPU ("Second Generation CPU", which was originally the name used
    in TI documentation to refer to either (or both) TI99/5 and TI99/8
    projects).

    The SGCPU was designed and built by the SNUG (System 99 Users Group),
    namely by Michael Becker for the hardware part and Harald Glaab for the
    software part.  It has no relationship with TI.

    The card is architectured around a 16-bit bus (vs. an 8-bit bus in every
    other TI99 system).  It includes 64kb of ROM, including a GPL interpreter,
    an internal DSR ROM which contains system-specific code, part of the TI
    extended Basic interpreter, and up to 1Mbyte of RAM.  It still includes a
    16-bit to 8-bit multiplexer in order to support extension cards designed
    for TI99/4a, but it can support 16-bit cards, too.  It does not include
    GROMs, video or sound: instead, it relies on the HSGPL and EVPC cards to
    do the job.

    IMPORTANT: The SGCPU card relies on a properly set up HSGPL flash memory
    card; without, it will immediately lock up. It is impossible to set it up
    from here (a bootstrap problem; you cannot start without the HSGPL).
    The best chance is to start a ti99_4ev with a plugged-in HSGPL
    and go through the setup process there. Copy the nvram files of the hsgpl into this
    driver's nvram subdirectory. The contents will be directly usable for the SGCPU.

    Michael Zapf

    February 2012: Rewritten as class

*****************************************************************************/


#include "emu.h"
#include "cpu/tms9900/tms9900.h"
#include "sound/wave.h"
#include "sound/dac.h"
#include "sound/sn76496.h"

#include "machine/tms9901.h"
#include "machine/ti99/peribox.h"

#include "imagedev/cassette.h"
#include "machine/ti99/videowrp.h"
#include "machine/ti99/peribox.h"

#define TMS9901_TAG "tms9901"
#define SGCPU_TAG "sgcpu"

#define SAMSMEM_TAG "samsmem"
#define PADMEM_TAG "padmem"

#define VERBOSE 1
#define LOG logerror

class ti99_4p : public driver_device
{
public:
	ti99_4p(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_WRITE_LINE_MEMBER( console_ready );
	DECLARE_WRITE_LINE_MEMBER( extint );
	DECLARE_WRITE_LINE_MEMBER( notconnected );

	DECLARE_READ16_MEMBER( memread );
	DECLARE_WRITE16_MEMBER( memwrite );

	DECLARE_READ16_MEMBER( samsmem_read );
	DECLARE_WRITE16_MEMBER( samsmem_write );

	// CRU (Communication Register Unit) handling
	DECLARE_READ8_MEMBER( cruread );
	DECLARE_WRITE8_MEMBER( cruwrite );
	DECLARE_READ8_MEMBER( read_by_9901 );
	DECLARE_WRITE_LINE_MEMBER(keyC0);
	DECLARE_WRITE_LINE_MEMBER(keyC1);
	DECLARE_WRITE_LINE_MEMBER(keyC2);
	DECLARE_WRITE_LINE_MEMBER(cs_motor);
	DECLARE_WRITE_LINE_MEMBER(audio_gate);
	DECLARE_WRITE_LINE_MEMBER(cassette_output);
	DECLARE_WRITE8_MEMBER(tms9901_interrupt);
	DECLARE_WRITE_LINE_MEMBER(handset_ack);
	DECLARE_WRITE_LINE_MEMBER(alphaW);

	void set_tms9901_INT2_from_v9938(v99x8_device &vdp, int state);

	device_t*				m_cpu;
	ti_sound_system_device*	m_sound;
	ti_exp_video_device*	m_video;
	cassette_image_device*	m_cassette;
	peribox_device*			m_peribox;
	tms9901_device*			m_tms9901;

	// Pointer to ROM0
	UINT16 *m_rom0;

	// Pointer to DSR ROM
	UINT16 *m_dsr;

	// Pointer to ROM6, first bank
	UINT16 *m_rom6a;

	// Pointer to ROM6, second bank
	UINT16 *m_rom6b;

	// AMS RAM (1 Mib)
	UINT16 *m_ram;

	// Scratch pad ram (1 KiB)
	UINT16 *m_scratchpad;

private:
	DECLARE_READ16_MEMBER( datamux_read );
	DECLARE_WRITE16_MEMBER( datamux_write );
	void	set_key(int number, int data);
	int		m_keyboard_column;
	int		m_alphalock_line;

	// True if SGCPU DSR is enabled
	bool m_internal_dsr;

	// True if SGCPU rom6 is enabled
	bool m_internal_rom6;

	// Offset to the ROM6 bank.
	int m_rom6_bank;

	// TRUE when mapper is active
	bool m_map_mode;

	// TRUE when mapper registers are accessible
	bool m_access_mapper;

	UINT8	m_lowbyte;
	UINT8	m_highbyte;
	UINT8	m_latch;

	/* Mapper registers */
	UINT8 m_mapper[16];
};

static ADDRESS_MAP_START(memmap, AS_PROGRAM, 16, ti99_4p)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE( memread, memwrite )
ADDRESS_MAP_END

static ADDRESS_MAP_START(cru_map, AS_IO, 8, ti99_4p)
	AM_RANGE(0x0000, 0x003f) AM_DEVREAD(TMS9901_TAG, tms9901_device, read)
	AM_RANGE(0x0000, 0x01ff) AM_READ( cruread )

	AM_RANGE(0x0000, 0x01ff) AM_DEVWRITE(TMS9901_TAG, tms9901_device, write)
	AM_RANGE(0x0000, 0x0fff) AM_WRITE( cruwrite )
ADDRESS_MAP_END

/*
    Input ports, used by machine code for TI keyboard and joystick emulation.

    Since the keyboard microcontroller is not emulated, we use the TI99/4a 48-key keyboard,
    plus two optional joysticks.
*/

static INPUT_PORTS_START(ti99_4p)
	/* 3 ports for mouse */
	PORT_START("MOUSEX") /* Mouse - X AXIS */
		PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSEY") /* Mouse - Y AXIS */
		PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE0") /* Mouse - buttons */
		PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Button 1") PORT_PLAYER(1)
		PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Button 2") PORT_PLAYER(1)

	/* 4 ports for keyboard and joystick */
	PORT_START("KEY0")	/* col 0 */
		PORT_BIT(0x0088, IP_ACTIVE_LOW, IPT_UNUSED)
		/* The original control key is located on the left, but we accept the right control key as well */
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		/* TI99/4a has a second shift key which maps the same */
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		/* The original control key is located on the right, but we accept the left function key as well */
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FCTN") PORT_CODE(KEYCODE_RALT) PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_SHIFT_2)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(SPACE)") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= + QUIT") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
				/* col 1 */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("x X (DOWN)") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("w W ~") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR('~')
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("s S (LEFT)") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 @ INS") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 ( BACK") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("o O '") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR('\'')
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("l L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')

	PORT_START("KEY1")	/* col 2 */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("c C `") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR('`')
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("e E (UP)") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("d D (RIGHT)") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 # ERASE") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 * REDO") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("i I ?") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR('?')
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("k K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
				/* col 3 */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("v V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("r R [") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR('[')
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("f F {") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR('{')
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $ CLEAR") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 & AID") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("u U _") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR('_')
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("j J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("m M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')

	PORT_START("KEY2")	/* col 4 */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("b B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("t T ]") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(']')
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("g G }") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR('}')
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 % BEGIN") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 ^ PROC'D") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("h H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("n N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
				/* col 5 */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("z Z \\") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR('\\')
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("a A |") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR('|')
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 ! DEL") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("p P \"") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR('\"')
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ -") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('-')

	PORT_START("KEY3")	/* col 6: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x00E0, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(1UP)", CODE_NONE, OSD_JOY_UP*/) PORT_PLAYER(1)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(1DOWN)", CODE_NONE, OSD_JOY_DOWN, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(1RIGHT)", CODE_NONE, OSD_JOY_RIGHT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(1LEFT)", CODE_NONE, OSD_JOY_LEFT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(1FIRE)", CODE_NONE, OSD_JOY_FIRE, 0*/) PORT_PLAYER(1)
			/* col 7: "wired handset 2" (= joystick 2) */
		PORT_BIT(0xE000, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(2UP)", CODE_NONE, OSD_JOY2_UP, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(2DOWN)", CODE_NONE, OSD_JOY2_DOWN, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(2RIGHT)", CODE_NONE, OSD_JOY2_RIGHT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(2LEFT)", CODE_NONE, OSD_JOY2_LEFT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(2FIRE)", CODE_NONE, OSD_JOY2_FIRE, 0*/) PORT_PLAYER(2)

	PORT_START("ALPHA")	/* one more port for Alpha line */
		PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Alpha Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE

INPUT_PORTS_END

/*
    Memory access
*/
READ16_MEMBER( ti99_4p::memread )
{
	int addroff = offset << 1;
	if (m_rom0 == NULL) return 0;	// premature access

	UINT16 zone = addroff & 0xe000;
	UINT16 value = 0;

	if (zone==0x0000)
	{
		// ROM0
		value = m_rom0[(addroff & 0x1fff)>>1];
		return value;
	}
	if (zone==0x2000 || zone==0xa000 || zone==0xc000 || zone==0xe000)
	{
		value = samsmem_read(space, offset, mem_mask);
		return value;
	}

	if (zone==0x4000)
	{
		if (m_internal_dsr)
		{
			value = m_dsr[(addroff & 0x1fff)>>1];
			return value;
		}
		else
		{
			if (m_access_mapper && ((addroff & 0xffe0)==0x4000))
			{
				value = m_mapper[offset & 0x000f]<<8;
				return value;
			}
		}
	}

	if (zone==0x6000 && m_internal_rom6)
	{
		if (m_rom6_bank==0)
			value = m_rom6a[(addroff & 0x1fff)>>1];
		else
			value = m_rom6b[(addroff & 0x1fff)>>1];

		return value;
	}

	// Scratch pad RAM and sound
	// speech is in peribox
	// groms are in hsgpl in peribox
	if (zone==0x8000)
	{
		if ((addroff & 0xfff0)==0x8400)	// cannot read from sound
		{
			value = 0;
			return value;
		}
		if ((addroff & 0xfc00)==0x8000)
		{
			value = m_scratchpad[(addroff & 0x03ff)>>1];
			return value;
		}
		// Video: 8800, 8802
		if ((addroff & 0xfffd)==0x8800)
		{
			value = m_video->read16(space, offset, mem_mask);
			return value;
		}
	}

	// If we are here, check the peribox via the datamux
	// catch-all for unmapped zones
	value = datamux_read(space, offset, mem_mask);
	return value;
}

WRITE16_MEMBER( ti99_4p::memwrite )
{
//  device_adjust_icount(m_cpu, -4);

	int addroff = offset << 1;
	UINT16 zone = addroff & 0xe000;

	if (zone==0x0000)
	{
		// ROM0
		if (VERBOSE>4) LOG("sgcpu: ignoring ROM write access at %04x\n", addroff);
		return;
	}

	if (zone==0x2000 || zone==0xa000 || zone==0xc000 || zone==0xe000)
	{
		samsmem_write(space, offset, data, mem_mask);
		return;
	}

	if (zone==0x4000)
	{
		if (m_internal_dsr)
		{
			if (VERBOSE>4) LOG("sgcpu: ignoring DSR write access at %04x\n", addroff);
			return;
		}
		else
		{
			if (m_access_mapper && ((addroff & 0xffe0)==0x4000))
			{
				COMBINE_DATA(&m_mapper[offset & 0x000f]);
				return;
			}
		}
	}

	if (zone==0x6000 && m_internal_rom6)
	{
		m_rom6_bank = offset & 0x0001;
		return;
	}

	// Scratch pad RAM and sound
	// speech is in peribox
	// groms are in hsgpl in peribox
	if (zone==0x8000)
	{
		if ((addroff & 0xfff0)==0x8400)		//sound write
		{
			m_sound->write(space, 0, (data >> 8) & 0xff);
			return;
		}
		if ((addroff & 0xfc00)==0x8000)
		{
			COMBINE_DATA(&m_scratchpad[(addroff & 0x03ff)>>1]);
			return;
		}
		// Video: 8C00, 8C02
		if ((addroff & 0xfffd)==0x8c00)
		{
			m_video->write16(space, offset, data, mem_mask);
			return;
		}
	}

	// If we are here, check the peribox via the datamux
	// catch-all for unmapped zones
	datamux_write(space, offset, data, mem_mask);
}

/***************************************************************************
    Internal datamux; similar to TI-99/4A. However, here we have just
    one device, the peripheral box, so it is much simpler.
***************************************************************************/

READ16_MEMBER( ti99_4p::datamux_read )
{
	UINT8 hbyte = 0;
	UINT16 addroff = (offset << 1);

	m_peribox->readz(space, addroff+1, &m_latch, mem_mask);
	m_lowbyte = m_latch;

	// Takes three cycles
	device_adjust_icount(m_cpu, -3);

	m_peribox->readz(space, addroff, &hbyte, mem_mask);
	m_highbyte = hbyte;

	// Takes three cycles
	device_adjust_icount(m_cpu, -3);

	// use the latch and the currently read byte and put it on the 16bit bus
//  printf("read  address = %04x, value = %04x, memmask = %4x\n", addroff,  (hbyte<<8) | sgcpu->latch, mem_mask);
	return (hbyte<<8) | m_latch ;
}

/*
    Write access.
    TODO: use the 16-bit expansion in the box for suitable cards
*/
WRITE16_MEMBER( ti99_4p::datamux_write )
{
	UINT16 addroff = (offset << 1);
//  printf("write address = %04x, value = %04x, memmask = %4x\n", addroff, data, mem_mask);

	// read more about the datamux in datamux.c

	// byte-only transfer, high byte
	// we use the previously read low byte to complete
	if (mem_mask == 0xff00)
		data = data | m_lowbyte;

	// byte-only transfer, low byte
	// we use the previously read high byte to complete
	if (mem_mask == 0x00ff)
		data = data | (m_highbyte << 8);

	// Write to the PEB
	m_peribox->write(space, addroff+1, data & 0xff);

	// Takes three cycles
	device_adjust_icount(m_cpu,-3);

	// Write to the PEB
	m_peribox->write(space, addroff, (data>>8) & 0xff);

	// Takes three cycles
	device_adjust_icount(m_cpu,-3);
}

/***************************************************************************
   CRU interface
***************************************************************************/

#define MAP_CRU_BASE 0x0f00
#define SAMS_CRU_BASE 0x1e00

/*
    CRU write
*/
WRITE8_MEMBER( ti99_4p::cruwrite )
{
	int addroff = offset<<1;

	if ((addroff & 0xff00)==MAP_CRU_BASE)
	{
		if ((addroff & 0x000e)==0)	m_internal_dsr = data;
		if ((addroff & 0x000e)==2)	m_internal_rom6 = data;
		if ((addroff & 0x000e)==4)	m_peribox->senila((data!=0)? ASSERT_LINE : CLEAR_LINE);
		if ((addroff & 0x000e)==6)	m_peribox->senilb((data!=0)? ASSERT_LINE : CLEAR_LINE);
		// TODO: more CRU bits? 8=Fast timing / a=KBENA
		return;
	}
	if ((addroff & 0xff00)==SAMS_CRU_BASE)
	{
		if ((addroff & 0x000e)==0) m_access_mapper = data;
		if ((addroff & 0x000e)==2) m_map_mode = data;
		return;
	}

	// No match - pass to peribox
	m_peribox->cruwrite(addroff, data);
}

READ8_MEMBER( ti99_4p::cruread )
{
	UINT8 value = 0;
	m_peribox->crureadz(offset<<4, &value);
	return value;
}

/***************************************************************************
   AMS Memory implementation
***************************************************************************/

/*
    Memory read. The SAMS card has two address areas: The memory is at locations
    0x2000-0x3fff and 0xa000-0xffff, and the mapper area is at 0x4000-0x401e
    (only even addresses).
*/
READ16_MEMBER( ti99_4p::samsmem_read )
{
	UINT32 address = 0;
	int addroff = offset << 1;

	// select memory expansion
	if (m_map_mode)
		address = (m_mapper[(addroff>>12) & 0x000f] << 12) + (addroff & 0x0fff);
	else // transparent mode
		address = addroff;

	return m_ram[address>>1];
}

/*
    Memory write
*/
WRITE16_MEMBER( ti99_4p::samsmem_write )
{
	UINT32 address = 0;
	int addroff = offset << 1;

	// select memory expansion
	if (m_map_mode)
		address = (m_mapper[(addroff>>12) & 0x000f] << 12) + (addroff & 0x0fff);
	else // transparent mode
		address = addroff;

	COMBINE_DATA(&m_ram[address>>1]);
}

/***************************************************************************
    Keyboard/tape control
****************************************************************************/
static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };

READ8_MEMBER( ti99_4p::read_by_9901 )
{
	int answer=0;

	switch (offset & 0x03)
	{
	case TMS9901_CB_INT7:
		// Read pins INT3*-INT7* of TI99's 9901.
		//
		// signification:
		// (bit 1: INT1 status)
		// (bit 2: INT2 status)
		// bit 3-7: keyboard status bits 0 to 4
		answer = ((input_port_read(machine(), keynames[m_keyboard_column >> 1]) >> ((m_keyboard_column & 1) * 8)) << 3) & 0xF8;
		if (m_alphalock_line==false)
			answer &= ~(input_port_read(*this, "ALPHA") << 3);
		break;

	case TMS9901_INT8_INT15:
		// Read pins INT8*-INT15* of TI99's 9901.
		// bit 0-2: keyboard status bits 5 to 7
		// bit 3: tape input mirror
		// bit 5-7: weird, not emulated
		if (m_keyboard_column == 7)	answer = 0x07;
		else				answer = ((input_port_read(machine(), keynames[m_keyboard_column >> 1]) >> ((m_keyboard_column & 1) * 8)) >> 5) & 0x07;
		break;

	case TMS9901_P0_P7:
		break;

	case TMS9901_P8_P15:
		// Read pins P8-P15 of TI99's 9901.
		// bit 26: high
		// bit 27: tape input
		answer = 4;
		if (m_cassette->input() > 0) answer |= 8;
		break;
	}
	return answer;
}

/*
    WRITE key column select (P2-P4)
*/
void ti99_4p::set_key(int number, int data)
{
	if (data!=0)	m_keyboard_column |= 1 << number;
	else			m_keyboard_column &= ~(1 << number);
}

WRITE_LINE_MEMBER( ti99_4p::keyC0 )
{
	set_key(0, state);
}

WRITE_LINE_MEMBER( ti99_4p::keyC1 )
{
	set_key(1, state);
}

WRITE_LINE_MEMBER( ti99_4p::keyC2 )
{
	set_key(2, state);
}

/*
    WRITE alpha lock line (P5)
*/
WRITE_LINE_MEMBER( ti99_4p::alphaW )
{
	m_alphalock_line = state;
}

/*
    command CS1 (only) tape unit motor (P6)
*/
WRITE_LINE_MEMBER( ti99_4p::cs_motor )
{
	m_cassette->change_state((state!=0)? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,CASSETTE_MASK_MOTOR);
}

/*
    audio gate (P8)

    Set to 1 before using tape: this enables the mixing of tape input sound
    with computer sound.

    We do not really need to emulate this as the tape recorder generates sound
    on its own.
*/
WRITE_LINE_MEMBER( ti99_4p::audio_gate )
{
}

/*
    tape output (P9)
*/
WRITE_LINE_MEMBER( ti99_4p::cassette_output )
{
	m_cassette->output((state!=0)? +1 : -1);
}

WRITE8_MEMBER( ti99_4p::tms9901_interrupt )
{
	// offset contains the interrupt level (0-15)
	if (data==ASSERT_LINE)
	{
		// The TMS9901 should normally be connected with the CPU by 5 wires:
		// INTREQ* and IC0-IC3. The last four lines deliver the interrupt level.
		// On the TI-99 systems these IC lines are not used; the input lines
		// at the CPU are hardwired to level 1.
		device_execute(m_cpu)->set_input_line_and_vector(0, ASSERT_LINE, 1);
	}
	else
	{
		device_execute(m_cpu)->set_input_line(0, CLEAR_LINE);
	}
}

/* TMS9901 setup. The callback functions pass a reference to the TMS9901 as device. */
const tms9901_interface tms9901_wiring_ti99_4p =
{
	TMS9901_INT1 | TMS9901_INT2 | TMS9901_INTC,	/* only input pins whose state is always known */

	// read handler
	DEVCB_DRIVER_MEMBER(ti99_4p, read_by_9901),

	{	// write handlers
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, keyC0),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, keyC1),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, keyC2),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, alphaW),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, cs_motor),
		DEVCB_NULL,
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, audio_gate),
		DEVCB_DRIVER_LINE_MEMBER(ti99_4p, cassette_output),
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL,
		DEVCB_NULL
	},

	/* interrupt handler */
	DEVCB_DRIVER_MEMBER(ti99_4p, tms9901_interrupt)
};

/***************************************************************************
    Control lines
****************************************************************************/

WRITE_LINE_MEMBER( ti99_4p::console_ready )
{
	if (VERBOSE>6) LOG("READY line set ... not yet connected, level=%02x\n", state);
}

WRITE_LINE_MEMBER( ti99_4p::extint )
{
	if (VERBOSE>6) LOG("EXTINT level = %02x\n", state);
	if (m_tms9901 != NULL)
		m_tms9901->set_single_int(1, state);
}

WRITE_LINE_MEMBER( ti99_4p::notconnected )
{
	if (VERBOSE>6) LOG("Setting a not connected line ... ignored\n");
}

/*****************************************************************************/

static PERIBOX_CONFIG( peribox_conf )
{
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, extint),			// INTA
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, notconnected),	// INTB
	DEVCB_DRIVER_LINE_MEMBER(ti99_4p, console_ready),	// READY
	0x70000												// Address bus prefix (AMA/AMB/AMC)
};

DRIVER_INIT( ti99_4p )
{
}

MACHINE_START( ti99_4p )
{
	ti99_4p *driver = machine.driver_data<ti99_4p>();

	driver->m_cpu = machine.device("maincpu");
	driver->m_peribox = static_cast<peribox_device*>(machine.device(PERIBOX_TAG));
	driver->m_sound = static_cast<ti_sound_system_device*>(machine.device(TISOUND_TAG));
	driver->m_video = static_cast<ti_exp_video_device*>(machine.device(VIDEO_SYSTEM_TAG));
	driver->m_cassette = static_cast<cassette_image_device*>(machine.device(CASSETTE_TAG));
	driver->m_tms9901 = static_cast<tms9901_device*>(machine.device(TMS9901_TAG));

	driver->m_ram = (UINT16*)(*machine.root_device().memregion(SAMSMEM_TAG));
	driver->m_scratchpad = (UINT16*)(*machine.root_device().memregion(PADMEM_TAG));

	driver->m_peribox->senila(CLEAR_LINE);
	driver->m_peribox->senilb(CLEAR_LINE);

	UINT16 *rom = (UINT16*)(*machine.root_device().memregion("maincpu"));
	driver->m_rom0  = rom + 0x2000;
	driver->m_dsr   = rom + 0x6000;
	driver->m_rom6a = rom + 0x3000;
	driver->m_rom6b = rom + 0x7000;
}

/*
    set the state of int2 (called by the v9938)
*/
void ti99_4p::set_tms9901_INT2_from_v9938(v99x8_device &vdp, int state)
{
	m_tms9901->set_single_int(2, state);
}

/*
    Reset the machine.
*/
MACHINE_RESET( ti99_4p )
{
	ti99_4p *driver = machine.driver_data<ti99_4p>();
	driver->m_tms9901->set_single_int(12, 0);
}

TIMER_DEVICE_CALLBACK( ti99_4p_hblank_interrupt )
{
	timer.machine().device<v9938_device>(V9938_TAG)->interrupt();
}

/*
    Machine description.
*/
static MACHINE_CONFIG_START( ti99_4p_60hz, ti99_4p )
	/* basic machine hardware */
	/* TMS9900 CPU @ 3.0 MHz */
	MCFG_CPU_ADD("maincpu", TMS9900, 3000000)
	MCFG_CPU_PROGRAM_MAP(memmap)
	MCFG_CPU_IO_MAP(cru_map)
	MCFG_MACHINE_START( ti99_4p )

	/* video hardware */
	/* FIXME: (MZ) Lowered the screen rate to 30 Hz. This is a quick hack to
    restore normal video speed for V9938-based systems until the V9938 implementation
    is properly fixed. */
	MCFG_TI_V9938_ADD(VIDEO_SYSTEM_TAG, 30, SCREEN_TAG, 2500, 512+32, (212+28)*2, DEVICE_SELF, ti99_4p, set_tms9901_INT2_from_v9938)
	MCFG_TIMER_ADD_SCANLINE("scantimer", ti99_4p_hblank_interrupt, SCREEN_TAG, 0, 1)

	/* tms9901 */
	MCFG_TMS9901_ADD(TMS9901_TAG, tms9901_wiring_ti99_4p, 3000000)

	/* Peripheral expansion box (SGCPU composition) */
	MCFG_PERIBOX_SG_ADD( PERIBOX_TAG, peribox_conf )

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(TISOUNDCHIP_TAG, SN94624, 3579545/8)	/* 3.579545 MHz */
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MCFG_TI_SOUND_ADD( TISOUND_TAG )

	/* Cassette drives */
	MCFG_CASSETTE_ADD( CASSETTE_TAG, default_cassette_interface )

	MCFG_SOUND_WAVE_ADD(WAVE_TAG, CASSETTE_TAG)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END


ROM_START(ti99_4p)
	/*CPU memory space*/
	ROM_REGION16_BE(0x10000, "maincpu", 0)
	ROM_LOAD16_BYTE("sgcpu_hb.bin", 0x0000, 0x8000, CRC(aa100730) SHA1(35e585b2dcd3f2a0005bebb15ede6c5b8c787366) ) /* system ROMs */
	ROM_LOAD16_BYTE("sgcpu_lb.bin", 0x0001, 0x8000, CRC(2a5dc818) SHA1(dec141fe2eea0b930859cbe1ebd715ac29fa8ecb) ) /* system ROMs */
	ROM_REGION16_BE(0x080000, SAMSMEM_TAG, 0)
	ROM_FILL(0x000000, 0x080000, 0x0000)
	ROM_REGION16_BE(0x0400, PADMEM_TAG, 0)
	ROM_FILL(0x000000, 0x0400, 0x0000)
ROM_END

/*    YEAR  NAME      PARENT   COMPAT   MACHINE      INPUT    INIT      COMPANY     FULLNAME */
COMP( 1996, ti99_4p,  0,	   0,		ti99_4p_60hz, ti99_4p, ti99_4p, "System 99 Users Group",		"SGCPU (a.k.a. 99/4P)" , 0 )
