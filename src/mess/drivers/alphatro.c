/***************************************************************************

	Triumph-Adler's Alphatronic PC

	skeleton driver

	z80 + HD46505 as a CRTC

        Note the boot rom (E000-FFFF) has various areas of zero bytes,
        in these areas are connected the keyboard scan lines etc.

        Looks like videoram is shadowed behind rom at F000-F7CF (filled
        with 0x20), and F800-FDFF is filled with 0x30 (colour/attribute?).

        BASIC should get copied from ROM into lower RAM when it is needed.

        Appears to use an amber monitor.

***************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/ram.h"

#define MAIN_CLOCK XTAL_4MHz

#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)

class alphatro_state : public driver_device
{
public:
	alphatro_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_crtc(*this, "crtc")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<mc6845_device> m_crtc;
	DECLARE_READ8_MEMBER(keyboard_r);
	DECLARE_WRITE8_MEMBER(video_w);
	DECLARE_READ8_MEMBER(video_r);
	DECLARE_READ8_MEMBER(vblank_r);

	emu_timer* m_sys_timer;
	UINT8 *m_p_videoram;
	UINT8 *m_p_chargen;
	virtual void video_start();
	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
    virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
private:
    static const device_timer_id SYSTEM_TIMER = 0;

};

void alphatro_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
	case SYSTEM_TIMER:
		device_set_input_line(m_maincpu,INPUT_LINE_IRQ0,HOLD_LINE);
		break;
	}
}

READ8_MEMBER( alphatro_state::vblank_r)
{
	return (space.machine().device<screen_device>("screen")->vblank() ? 0x80 : 0x00);
}

READ8_MEMBER( alphatro_state::keyboard_r )
{
	switch(offset)
	{
	case 0:
		return input_port_read(space.machine(),"key_row_a");
	case 1:
		return input_port_read(space.machine(),"key_row_b");
	case 2:
		return input_port_read(space.machine(),"key_row_c");
	case 3:
		return input_port_read(space.machine(),"key_row_d");
	case 4:
		return input_port_read(space.machine(),"key_row_e");
	case 5:
		return input_port_read(space.machine(),"key_row_f");
	case 6:
		return input_port_read(space.machine(),"key_row_g");
	case 7:
		return input_port_read(space.machine(),"key_row_h");
	case 8:
		return input_port_read(space.machine(),"key_row_i");
	case 9:
		return input_port_read(space.machine(),"key_row_j");
	case 10:
		return input_port_read(space.machine(),"key_row_k");
	case 11:
		return input_port_read(space.machine(),"key_row_l");
	default:
		return 0x00;
	}
	return 0x00;
}

WRITE8_MEMBER( alphatro_state::video_w )
{
	m_p_videoram[offset] = data;
}

READ8_MEMBER( alphatro_state::video_r )
{
	return m_p_videoram[offset];
}

VIDEO_START_MEMBER( alphatro_state )
{
	m_p_chargen = machine().region("chargen")->base();
	m_p_videoram = machine().region("vram")->base();
}

SCREEN_UPDATE_MEMBER( alphatro_state )
{
	m_crtc->update( &bitmap, &cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( alphatro_update_row )
{
	alphatro_state *state = device->machine().driver_data<alphatro_state>();
	UINT8 chr,gfx,attr;
	UINT16 mem,x;
	UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

	for (x = 0; x < x_count; x++)
	{
		UINT8 inv=0;
		if (x == cursor_x) inv=0xff;
		mem = (ma + x) & 0x7ff;
		chr = state->m_p_videoram[mem];
		attr = state->m_p_videoram[mem | 0x800];

		if (BIT(attr, 7))
		{
			inv ^= 0xff;
			chr &= 0x7f;
		}

		/* get pattern of pixels for that character scanline */
		gfx = state->m_p_chargen[(chr<<4) | ra] ^ inv;

		/* Display a scanline of a character (8 pixels) */
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

static ADDRESS_MAP_START( alphatro_map, AS_PROGRAM, 8, alphatro_state )
	AM_RANGE(0x0000, 0xefff) AM_RAMBANK("bank1") // this is really ram, basic gets copied to here when it is needed
//	AM_RANGE(0x6000, 0xdfff) AM_RAM
//	AM_RANGE(0xe000, 0xe46f) AM_RAM
//	AM_RANGE(0xe470, 0xe4cf) AM_RAM // keyboard out?
//	AM_RANGE(0xe4d0, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xfdff) AM_READWRITE(video_r,video_w)
	AM_RANGE(0xfe00, 0xffff) AM_RAM // for the stack
ADDRESS_MAP_END

static ADDRESS_MAP_START( alphatro_io, AS_IO, 8, alphatro_state )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x10, 0x10) AM_READ(vblank_r)
	AM_RANGE(0x20, 0x2f) AM_READ(keyboard_r) // keyboard in?
	AM_RANGE(0x50, 0x50) AM_DEVWRITE("crtc", mc6845_device, address_w)
	AM_RANGE(0x51, 0x51) AM_DEVREADWRITE("crtc", mc6845_device, register_r, register_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( alphatro )
	PORT_START("key_row_a")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)

    PORT_START("key_row_b")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad - /") PORT_CODE(KEYCODE_MINUS_PAD)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad =") PORT_CODE(KEYCODE_ASTERISK) // keypad equals
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad + x") PORT_CODE(KEYCODE_PLUS_PAD)

    PORT_START("key_row_c")
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)

	PORT_START("key_row_d")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)

	PORT_START("key_row_e")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)

	PORT_START("key_row_f")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)

	PORT_START("key_row_g")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)

	PORT_START("key_row_h")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_DEL)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE)

	PORT_START("key_row_i")
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)

	PORT_START("key_row_j")
    PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
    PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)

	PORT_START("key_row_k")
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("INS DEL") PORT_CODE(KEYCODE_BACKSPACE)
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Graph") PORT_CODE(KEYCODE_PGUP) // GRAPH
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Clear Home") PORT_CODE(KEYCODE_HOME)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Keypad Enter (?)") PORT_CODE(KEYCODE_ENTER_PAD)

    PORT_START("key_row_l")
    PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)
    PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
    PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
    PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
    PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
    PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)

INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,16,
	RGN_FRAC(1,1),
	1,
	{ RGN_FRAC(0,1) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static GFXDECODE_START( alphatro )
	GFXDECODE_ENTRY( "chargen", 0, charlayout,     0, 1 )
GFXDECODE_END

void alphatro_state::machine_start()
{
	m_sys_timer = timer_alloc(SYSTEM_TIMER);
}

void alphatro_state::machine_reset()
{
	UINT8* RAM = machine().device<ram_device>("ram")->pointer();
	UINT8* ROM = machine().region("maincpu")->base();
	cpu_set_reg(machine().device("maincpu"), STATE_GENPC, 0xe000);
	memcpy(RAM,ROM,0x10000); // copy BASIC to RAM, which the undumped IPL is supposed to do.
	memory_set_bankptr(machine(), "bank1",RAM);
	// m_sys_timer->adjust(attotime::zero,0,attotime::from_msec(10));  // interrupts aren't enabled yet, so not much point in setting this
}

//static PALETTE_INIT( alphatro )
//{
//}

static const mc6845_interface h19_crtc6845_interface =
{
	"screen",
	8,
	NULL,
	alphatro_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static MACHINE_CONFIG_START( alphatro, alphatro_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80,MAIN_CLOCK)
	MCFG_CPU_PROGRAM_MAP(alphatro_map)
	MCFG_CPU_IO_MAP(alphatro_io)

//	MCFG_MACHINE_START(alphatro)
//	MCFG_MACHINE_RESET(alphatro)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) // not correct
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MCFG_GFXDECODE(alphatro)
	//MCFG_PALETTE_INIT(alphatro)
	//MCFG_PALETTE_LENGTH(8)
	MCFG_PALETTE_INIT(monochrome_amber)
	MCFG_PALETTE_LENGTH(2)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
//  MCFG_SOUND_ADD("aysnd", AY8910, MAIN_CLOCK/4)
//  MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	/* Devices */
	MCFG_MC6845_ADD("crtc", MC6845, XTAL_12_288MHz / 8, h19_crtc6845_interface) // clk unknown

	MCFG_RAM_ADD("ram")
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
2258: DB BB         in   a,($BB)
225A: DB B5         in   a,($B5)
225C: DB C1         in   a,($C1)
225E: DB CA         in   a,($CA)
2260: DB F4         in   a,($F4)
2262: DB F7         in   a,($F7)
2264: DB DF         in   a,($DF)
2266: DB 06         in   a,($06)
2268: DC 09 DC      call c,$DC09

in various places of the ROM ... most likely a bad dump.
*/

ROM_START( alphatro )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASE00)
	ROM_LOAD( "rom1.bin",     0x0000, 0x2000, BAD_DUMP CRC(1509b15a) SHA1(225c36411de680eb8f4d6b58869460a58e60c0cf) )
	ROM_LOAD( "rom2.bin",     0x2000, 0x2000, BAD_DUMP CRC(998a865d) SHA1(294fe64e839ae6c4032d5db1f431c35e0d80d367) )
	ROM_LOAD( "rom3.bin",     0x4000, 0x2000, BAD_DUMP CRC(55cbafef) SHA1(e3376b92f80d5a698cdcb2afaa0f3ef4341dd624) )
	ROM_LOAD( "2764.ic-1038", 0xe000, 0x2000, BAD_DUMP CRC(e337db3b) SHA1(6010bade6a21975636383179903b58a4ca415e49) ) // ???

	ROM_REGION( 0x10000, "user1", ROMREGION_ERASE00 )
	ROM_LOAD( "613256.ic-1058"	,     0x0000, 0x6000, BAD_DUMP CRC(ceea4cb3) SHA1(b332dea0a2d3bb2978b8422eb0723960388bb467) )
	ROM_LOAD( "ipl.bin",     0x8000, 0x2000, NO_DUMP  )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "2732.ic-1067",      0x0000, 0x1000, BAD_DUMP CRC(61f38814) SHA1(35ba31c58a10d5bd1bdb202717792ca021dbe1a8) ) // should be 1:1

	ROM_REGION( 0x1000, "vram", ROMREGION_ERASE00 )
ROM_END

COMP( 1983, alphatro,   0,       0,    alphatro,   alphatro,  0,  "Triumph-Adler", "Alphatronic PC", GAME_NOT_WORKING | GAME_NO_SOUND)
