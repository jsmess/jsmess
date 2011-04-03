/***************************************************************************

        DEC ZRT-80

        12/05/2009 Skeleton driver.

        16/02/2011 Working.

        The beeper is external, frequency not known. I've made a reasonable
        assumption of frequency and lengths.

        Make sure 'mode' dipswitch is set to 'local' so you can see your
        typing.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6845.h"
#include "machine/ins8250.h"
#include "machine/terminal.h"
#include "sound/beep.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)

class zrt80_state : public driver_device
{
public:
	zrt80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG),
	m_crtc(*this, "crtc"),
	m_8250(*this, "ins8250"),
	m_beep(*this, "beep")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_crtc;
	required_device<device_t> m_8250;
	required_device<device_t> m_beep;
	DECLARE_READ8_MEMBER( zrt80_10_r );
	DECLARE_WRITE8_MEMBER( zrt80_30_w );
	DECLARE_WRITE8_MEMBER( zrt80_38_w );
	DECLARE_WRITE8_MEMBER( zrt80_kbd_put );
	UINT8 m_term_data;
	const UINT8 *m_p_videoram;
	const UINT8 *m_p_chargen;
	virtual void machine_reset();
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};

READ8_MEMBER( zrt80_state::zrt80_10_r )
{
	UINT8 ret = m_term_data;
	cputag_set_input_line(m_machine, "maincpu", INPUT_LINE_NMI, CLEAR_LINE);
	return ret;
}

static TIMER_CALLBACK( zrt80_beepoff )
{
	zrt80_state *state = machine.driver_data<zrt80_state>();
	beep_set_state(state->m_beep, 0);
}

WRITE8_MEMBER(zrt80_state::zrt80_30_w)
{
	m_machine.scheduler().timer_set(attotime::from_msec(100), FUNC(zrt80_beepoff));
	beep_set_state(m_beep, 1);
}

WRITE8_MEMBER(zrt80_state::zrt80_38_w)
{
	m_machine.scheduler().timer_set(attotime::from_msec(400), FUNC(zrt80_beepoff));
	beep_set_state(m_beep, 1);
}

static ADDRESS_MAP_START(zrt80_mem, AS_PROGRAM, 8, zrt80_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM // Z25 - Main firmware
	AM_RANGE(0x1000, 0x1fff) AM_ROM // Z24 - Expansion
	AM_RANGE(0x4000, 0x43ff) AM_RAM	// Board RAM
	// Normally video RAM is 0x800 but could be expanded up to 8K
	AM_RANGE(0xc000, 0xdfff) AM_RAM	AM_BASE(m_p_videoram) // Video RAM

ADDRESS_MAP_END

static ADDRESS_MAP_START( zrt80_io, AS_IO, 8, zrt80_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x07) AM_DEVREADWRITE_LEGACY("ins8250", ins8250_r, ins8250_w )
	AM_RANGE(0x08, 0x08) AM_DEVWRITE_LEGACY("crtc", mc6845_address_w)
	AM_RANGE(0x09, 0x09) AM_DEVREADWRITE_LEGACY("crtc", mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x10, 0x17) AM_READ(zrt80_10_r)
	AM_RANGE(0x18, 0x1F) AM_READ_PORT("DIPSW2")
	AM_RANGE(0x20, 0x27) AM_READ_PORT("DIPSW3")
	AM_RANGE(0x30, 0x37) AM_WRITE(zrt80_30_w)
	AM_RANGE(0x38, 0x3F) AM_WRITE(zrt80_38_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( zrt80 )
	PORT_START("DIPSW1")
		PORT_DIPNAME( 0x01, 0x01, "Composite Sync" )
		PORT_DIPSETTING(    0x01, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x02, 0x02, "Vertical Sync" )
		PORT_DIPSETTING(    0x02, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x04, 0x00, "Video" )
		PORT_DIPSETTING(    0x04, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x08, 0x08, "Keypad" )
		PORT_DIPSETTING(    0x08, "Numeric" )
		PORT_DIPSETTING(    0x00, "Alternate Keyboard" )
		PORT_DIPNAME( 0x10, 0x10, "Horizontal Sync" )
		PORT_DIPSETTING(    0x10, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x20, 0x20, "CPU" )
		PORT_DIPSETTING(    0x20, "Operating" )
		PORT_DIPSETTING(    0x00, "Reset" )
		PORT_DIPNAME( 0x40, 0x40, "Keyboard strobe" )
		PORT_DIPSETTING(    0x40, "Negative" )
		PORT_DIPSETTING(    0x00, "Positive" )
		PORT_DIPNAME( 0x80, 0x00, "Beeper" )
		PORT_DIPSETTING(    0x80, "Silent" )
		PORT_DIPSETTING(    0x00, "Enable" )
	PORT_START("DIPSW2")
		PORT_DIPNAME( 0x0f, 0x05, "Baud rate" )
		PORT_DIPSETTING(    0x00, "50" )
		PORT_DIPSETTING(    0x01, "75" )
		PORT_DIPSETTING(    0x02, "110" )
		PORT_DIPSETTING(    0x03, "134.5" )
		PORT_DIPSETTING(    0x04, "150" )
		PORT_DIPSETTING(    0x05, "300" )
		PORT_DIPSETTING(    0x06, "600" )
		PORT_DIPSETTING(    0x07, "1200" )
		PORT_DIPSETTING(    0x08, "1800" )
		PORT_DIPSETTING(    0x09, "2000" )
		PORT_DIPSETTING(    0x0a, "2400" )
		PORT_DIPSETTING(    0x0b, "3600" )
		PORT_DIPSETTING(    0x0c, "4800" )
		PORT_DIPSETTING(    0x0d, "7200" )
		PORT_DIPSETTING(    0x0e, "9600" )
		PORT_DIPSETTING(    0x0f, "19200" )
		PORT_DIPNAME( 0x30, 0x20, "Parity" )
		PORT_DIPSETTING(    0x00, "Odd" )
		PORT_DIPSETTING(    0x10, "Even" )
		PORT_DIPSETTING(    0x20, "Marking" )
		PORT_DIPSETTING(    0x30, "Spacing" )
		PORT_DIPNAME( 0x40, 0x40, "Handshake" )
		PORT_DIPSETTING(    0x40, "CTS" )
		PORT_DIPSETTING(    0x00, "XON/XOFF" )
		PORT_DIPNAME( 0x80, 0x80, "Line Feed" )
		PORT_DIPSETTING(    0x80, "No LF on CR" )
		PORT_DIPSETTING(    0x00, "Auto" )
	PORT_START("DIPSW3")
		PORT_DIPNAME( 0x07, 0x07, "Display" )
		PORT_DIPSETTING(    0x00, "96 x 24 15750Hz, 50Hz" )
		PORT_DIPSETTING(    0x01, "80 x 48 15750Hz, 50Hz" )
		PORT_DIPSETTING(    0x02, "80 x 24 15750Hz, 50Hz" )
		PORT_DIPSETTING(    0x03, "96 x 24 15750Hz, 60Hz" )
		PORT_DIPSETTING(    0x04, "80 x 48 18700Hz, 50Hz" )
		PORT_DIPSETTING(    0x05, "80 x 24 17540Hz, 60Hz" )
		PORT_DIPSETTING(    0x06, "80 x 48 15750Hz, 60Hz" )
		PORT_DIPSETTING(    0x07, "80 x 24 15750Hz, 60Hz" )
		PORT_DIPNAME( 0x18, 0x18, "Emulation" )
		PORT_DIPSETTING(    0x00, "Adds" )
		PORT_DIPSETTING(    0x08, "Beehive" )
		PORT_DIPSETTING(    0x10, "LSI ADM-3" )
		PORT_DIPSETTING(    0x18, "Heath H-19" )
		PORT_DIPNAME( 0x20, 0x00, "Mode" )
		PORT_DIPSETTING(    0x20, "Line" )
		PORT_DIPSETTING(    0x00, "Local" )
		PORT_DIPNAME( 0x40, 0x40, "Duplex" )
		PORT_DIPSETTING(    0x40, "Full" )
		PORT_DIPSETTING(    0x00, "Half" )
		PORT_DIPNAME( 0x80, 0x80, "Wraparound" )
		PORT_DIPSETTING(    0x80, "Disabled" )
		PORT_DIPSETTING(    0x00, "Enabled" )
INPUT_PORTS_END


MACHINE_RESET_MEMBER(zrt80_state)
{
	beep_set_frequency(m_beep, 800);
}

VIDEO_START_MEMBER( zrt80_state )
{
	m_p_chargen = m_machine.region("chargen")->base();
}

SCREEN_UPDATE_MEMBER( zrt80_state )
{
	mc6845_update(m_crtc, &bitmap, &cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( zrt80_update_row )
{
	zrt80_state *state = device->machine().driver_data<zrt80_state>();
	UINT8 chr,gfx,inv;
	UINT16 mem,x;
	UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);
	UINT8 polarity = input_port_read(device->machine(), "DIPSW1") & 4 ? 0xff : 0;

	for (x = 0; x < x_count; x++)
	{
		inv = polarity;
		if (x == cursor_x) inv ^= 0xff;
		mem = (ma + x) & 0x1fff;
		chr = state->m_p_videoram[mem];

		if BIT(chr, 7)
		{
			inv ^= 0xff;
			chr &= 0x7f;
		}

		gfx = state->m_p_chargen[(chr<<4) | ra] ^ inv;

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

static const mc6845_interface zrt80_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	zrt80_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

static INS8250_INTERRUPT( zrt80_com_interrupt )
{
	cputag_set_input_line(device->machine(), "maincpu", INPUT_LINE_IRQ0, state);
}

static const ins8250_interface zrt80_com_interface =
{
	2457600,
	zrt80_com_interrupt,
	NULL,
	NULL,
	NULL
};

WRITE8_MEMBER( zrt80_state::zrt80_kbd_put )
{
	m_term_data = data;
	cputag_set_input_line(m_machine, "maincpu", INPUT_LINE_NMI, ASSERT_LINE);
}

static GENERIC_TERMINAL_INTERFACE( zrt80_terminal_intf )
{
	DEVCB_DRIVER_MEMBER(zrt80_state, zrt80_kbd_put)
};


/* F4 Character Displayer */
static const gfx_layout zrt80_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( zrt80 )
	GFXDECODE_ENTRY( "chargen", 0x0000, zrt80_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( zrt80, zrt80_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_2_4576MHz)
	MCFG_CPU_PROGRAM_MAP(zrt80_mem)
	MCFG_CPU_IO_MAP(zrt80_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MCFG_GFXDECODE(zrt80)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("beep", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* Devices */
	MCFG_MC6845_ADD("crtc", MC6845, XTAL_20MHz / 8, zrt80_crtc6845_interface)
	MCFG_INS8250_ADD( "ins8250", zrt80_com_interface )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, zrt80_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( zrt80 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD("zrt80mon.z25", 0x0000, 0x1000, CRC(e6ea96dc) SHA1(e3075e30bb2b85f9288d0b8b8cdf1d2b4f7586fd) )
	//z24 is 2nd chip, used as expansion

	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD("zrt80chr.z30", 0x0000, 0x0800, CRC(4dbdc60f) SHA1(20e393f7207a8440029c8290cdf2f121d317a37e) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, zrt80,  0,       0, 	zrt80,	zrt80,	 0, "Digital Research Computers", "ZRT-80", 0)

