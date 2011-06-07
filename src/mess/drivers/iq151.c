/***************************************************************************

        IQ-151

        12/05/2009 Skeleton driver.

        07/June/2011 Added screen & keyboard (by looking at the Z80 code)


This computer depends on RAM just happening to be certain values at powerup.
If the conditions are not met, it may crash.

Monitor Commands:
C
D Dump memory, any key to dump more, Return to finish
F Fill memory (start end withwhat)
G Go
L
M
R
S Edit memory
W
X Display/Edit registers


ToDo:
- Add whatever devices may exist.

- Line 32 does not scroll, should it show?
  (could be reserved for a status line in a terminal mode)

- Sort out the issue with memory location 6. If it isn't zero the computer hangs.
  But if it is always forced to zero, the keys repeat way too fast. Timer
  currently set to 3Hz for reasonable repeat, but it is far from perfect.

- Note that the system checks for 3E at C000, if exist, jump to C000;
  otherwise then checks for non-FF at C800, if so, jumps to C800. Could be
  extra roms or some sort of boot device.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)


class iq151_state : public driver_device
{
public:
	iq151_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	DECLARE_READ8_MEMBER(keyboard_r);
	UINT8 *m_p_ram;
	UINT8 *m_p_videoram;
	const UINT8 *m_p_chargen;
	UINT8 m_framecnt;
	virtual void machine_reset();
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

READ8_MEMBER(iq151_state::keyboard_r)
{
	char kbdrow[6];
	UINT8 i,j;

	if (offset) // port 85 - get row
	{
		j = 0xfe;
		for (i = 0; i < 8; i++)
		{
			sprintf(kbdrow,"X%X",i);
			if (input_port_read(machine(), kbdrow))
				return j;
			else
				j = (j << 1) | 1; // rotate
		}
	}
	else // port 84 check all rows for a key and gets column
	{
		for (i = 0; i < 8; i++)
		{
			sprintf(kbdrow,"X%X",i);
			j = input_port_read(machine(), kbdrow);
			if (j) return ~j;
		}
	}
	return 0xff;
}


static ADDRESS_MAP_START(iq151_mem, AS_PROGRAM, 8, iq151_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff ) AM_RAMBANK("boot")
	AM_RANGE( 0x0800, 0xdfff ) AM_RAM AM_REGION("maincpu", 0x0800) // puts FF into C800 so we can boot
	AM_RANGE( 0xe000, 0xe7ff ) AM_ROM
	AM_RANGE( 0xe800, 0xebff ) AM_RAM
	AM_RANGE( 0xec00, 0xefff ) AM_RAM AM_BASE(m_p_videoram)
	AM_RANGE( 0xf000, 0xffff ) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(iq151_io, AS_IO, 8, iq151_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x84, 0x85 ) AM_READ(keyboard_r)
	AM_RANGE( 0x86, 0x86 ) AM_READ_PORT("X8")
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( iq151 )
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)		PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)		PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)		PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4)		PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)		PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)		PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7)		PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)		PORT_CHAR('8') PORT_CHAR('(')

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)		PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)		PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)		PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)		PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)		PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)		PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)		PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)		PORT_CHAR('i') PORT_CHAR('I')

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)		PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)		PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)		PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)		PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)		PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)		PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)		PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)		PORT_CHAR('k') PORT_CHAR('K')

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)		PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)		PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)		PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)		PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)		PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)		PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)		PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',') PORT_CHAR('<')

	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)		PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)		PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)		PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x1d
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x1c
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x01

	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)		PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)		PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x1e
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x02

	PORT_START("X6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x1a
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x05
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x0b
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x0c
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x04

	PORT_START("X7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("_") PORT_CODE(KEYCODE_END) // its actually some sort of graphic character
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x19
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x18
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED) // 0x03

	PORT_START("X8")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
INPUT_PORTS_END


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( iq151_boot )
{
	memory_set_bank(machine, "boot", 0);
}

static TIMER_DEVICE_CALLBACK( iq151a )
{
	iq151_state *state = timer.machine().driver_data<iq151_state>();
	state->m_p_ram[6]=0; // fixme
}

DRIVER_INIT( iq151 )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xf800);
}

MACHINE_RESET_MEMBER( iq151_state )
{
	memory_set_bank(machine(), "boot", 1);
	machine().scheduler().timer_set(attotime::from_usec(5), FUNC(iq151_boot));
}

VIDEO_START_MEMBER( iq151_state )
{
	m_p_chargen = machine().region("chargen")->base();
	m_p_ram = machine().region("maincpu")->base();
}

SCREEN_UPDATE_MEMBER( iq151_state )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;
	UINT16 cursor = m_p_ram[12] | ((m_p_ram[13] & 3) << 8); // cursor address masked

	m_framecnt++;

	for (y = 0; y < 32; y++)
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(&bitmap, sy++, 0);

			for (x = ma; x < ma+32; x++)
			{
				chr = m_p_videoram[x];

				gfx = m_p_chargen[(chr<<3) | ra ];

				//display cursor if at cursor position and flash on
				if ( (x==cursor) && (m_framecnt & 0x08) )
					gfx = 0xff;

				/* Display a scanline of a character */
				*p++ = BIT(gfx, 7);
				*p++ = BIT(gfx, 6);
				*p++ = BIT(gfx, 5);
				*p++ = BIT(gfx, 4);
				*p++ = BIT(gfx, 3);
				*p++ = BIT(gfx, 2);
				*p++ = BIT(gfx, 1);
				*p++ = BIT(gfx, 0);
			}
		}
		ma+=32;
	}
	return 0;
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
	GFXDECODE_ENTRY( "chargen", 0x0000, iq151_32_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "chargen", 0x0400, iq151_64_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( iq151, iq151_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(iq151_mem)
	MCFG_CPU_IO_MAP(iq151_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 32*8-1, 0, 32*8-1)
	MCFG_GFXDECODE(iq151)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(monochrome_green)

	MCFG_TIMER_ADD_PERIODIC("iq151a", iq151a, attotime::from_hz(3) )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( iq151 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* A number of bios versions here. The load address is shown for each */
	ROM_LOAD( "iq151_disc2_12_5_1987_v4_0.rom", 0xe000, 0x0800, CRC(b189b170) SHA1(3e2ca80934177e7a32d0905f5a0ad14072f9dabf))

	ROM_SYSTEM_BIOS( 0, "orig", "Original" )
	ROMX_LOAD( "iq151_monitor_orig.rom", 0xf000, 0x1000, CRC(acd10268) SHA1(4d75c73f155ed4dc2ac51a9c22232f869cca95e2),ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "disasm", "Disassembler" )
	ROMX_LOAD( "iq151_monitor_disasm.rom", 0xf000, 0x1000, CRC(45c2174e) SHA1(703e3271a124c3ef9330ae399308afd903316ab9),ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "cpm", "CPM" )
	ROMX_LOAD( "iq151_monitor_cpm.rom", 0xf000, 0x1000, CRC(26f57013) SHA1(4df396edc375dd2dd3c82c4d2affb4f5451066f1),ROM_BIOS(3))
	ROM_SYSTEM_BIOS( 3, "cpmold", "CPM (old)" )
	ROMX_LOAD( "iq151_monitor_cpm_old.rom", 0xf000, 0x1000, CRC(6743e1b7) SHA1(ae4f3b1ba2511a1f91c4e8afdfc0e5aeb0fb3a42),ROM_BIOS(4))

	ROM_REGION( 0x0c00, "chargen", ROMREGION_INVERT )
	ROM_LOAD( "iq151_video32font.rom", 0x0000, 0x0400, CRC(395567a7) SHA1(18800543daf4daed3f048193c6ae923b4b0e87db))
	ROM_LOAD( "iq151_video64font.rom", 0x0400, 0x0800, CRC(cb6f43c0) SHA1(4b2c1d41838d569228f61568c1a16a8d68b3dadf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 198?, iq151,  0,       0,      iq151,     iq151,   iq151, "ZPA Novy Bor", "IQ-151", GAME_NOT_WORKING | GAME_NO_SOUND)
