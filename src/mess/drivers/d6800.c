/*

    The Dream 6800 is a CHIP-8 computer roughly modelled on the Cosmac VIP.
    It was decribed in Electronics Australia magazine in 4 articles starting
    in May 1979. It has 1k of ROM and 1k of RAM. The video consists of 64x32
    pixels. The keyboard is a hexcode 4x4 matrix, plus a Function key.

    Function keys:
    FN 0 - Modify memory - firstly enter a 4-digit address, then 2-digit data
                    the address will increment by itself, enter the next byte.
                    FN by itself will step to the next address.

    FN 1 - Tape load

    FN 2 - Tape save

    FN 3 - Run. Enter the 4-digit go address, then it starts executing.


    Information and programs can be found at http://chip8.com/?page=78


    To change the large numbers at the bottom, start in debug mode, enter
    some data at memory 6 and 7, then G to run.


    TODO:
    - Cassette
    - CPU should freeze while screen is being drawn
    - Keyboard (should work but it doesn't, due to inadequate PIA emulation)

    Current situation:
    - It starts, displays initial screen
    - It checks the status bits looking for a keypress
    - When a key is pressed, it gets into a loop at C2E8-C2EB, waiting for
      a memory location to change. This means it needs an interrupt to kick
      in, from a keypress.
    - The pia only looks for an interrupt when it gets polled, which is
      not the case here, so the emulation hangs.
*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/beep.h"
#include "imagedev/cassette.h"
#include "sound/wave.h"
#include "sound/speaker.h"
#include "machine/6821pia.h"


class d6800_state : public driver_device
{
public:
	d6800_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
		  m_maincpu(*this, "maincpu"),
		  m_cass(*this, "cassette"),
		  m_pia(*this, "pia"),
		  m_speaker(*this, "speaker")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cass;
	required_device<device_t> m_pia;
	required_device<device_t> m_speaker;
	DECLARE_READ8_MEMBER( d6800_cassette_r );
	DECLARE_WRITE8_MEMBER( d6800_cassette_w );
	DECLARE_READ8_MEMBER( d6800_keyboard_r );
	DECLARE_WRITE8_MEMBER( d6800_keyboard_w );
	DECLARE_READ_LINE_MEMBER( d6800_fn_key_r );
	DECLARE_READ_LINE_MEMBER( d6800_keydown_r );
	DECLARE_READ_LINE_MEMBER( d6800_rtc_pulse );
	DECLARE_WRITE_LINE_MEMBER( d6800_screen_w );
	UINT8 m_keylatch;
	UINT8 m_screen_on;
	UINT8 m_rtc;
	UINT8 *m_videoram;
};


/* Memory Maps */

static ADDRESS_MAP_START( d6800_map, AS_PROGRAM, 8, d6800_state )
	AM_RANGE(0x0000, 0x00ff) AM_RAM
	AM_RANGE(0x0100, 0x01ff) AM_RAM AM_BASE( m_videoram )
	AM_RANGE(0x0200, 0x07ff) AM_RAM
	AM_RANGE(0x8010, 0x8013) AM_DEVREADWRITE_LEGACY("pia", pia6821_r, pia6821_w)
	AM_RANGE(0xc000, 0xc3ff) AM_MIRROR(0x3c00) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( d6800 )
	PORT_START("LINE0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0xf0, 0xf0, IPT_UNUSED )

	PORT_START("LINE1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0xf0, 0xf0, IPT_UNUSED )

	PORT_START("LINE2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0xf0, 0xf0, IPT_UNUSED )

	PORT_START("LINE3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0xf0, 0xf0, IPT_UNUSED )

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("FN") PORT_CODE(KEYCODE_LSHIFT)
INPUT_PORTS_END

/* Video */

static SCREEN_UPDATE( d6800 )
{
	d6800_state *state = screen->machine().driver_data<d6800_state>();
	UINT8 x,y,gfx=0,i;

	for (y = 0; y < 32; y++)
	{
		UINT16 *p = BITMAP_ADDR16(bitmap, y, 0);

		for (x = 0; x < 8; x++)
		{
			if (state->m_screen_on)
				gfx = state->m_videoram[ x | (y<<3)];

			for (i = 0; i < 8; i++)
				*p++ = BIT(gfx, 7-i);
		}
	}
	return 0;
}

/* PIA6821 Interface */

static INTERRUPT_GEN( d6800_interrupt )
{
	d6800_state *state = device->machine().driver_data<d6800_state>();
	state->m_rtc = 1;
}

READ_LINE_MEMBER( d6800_state::d6800_rtc_pulse )
{
	UINT8 res = m_rtc;
	m_rtc = 0;
	return res;
}

READ_LINE_MEMBER( d6800_state::d6800_keydown_r )
{
	UINT8 data = input_port_read(m_machine, "LINE0")
	           & input_port_read(m_machine, "LINE1")
	           & input_port_read(m_machine, "LINE2")
	           & input_port_read(m_machine, "LINE3");

	return (data==0xff) ? 0 : 1;
}

READ_LINE_MEMBER( d6800_state::d6800_fn_key_r )
{
	return input_port_read(m_machine, "SPECIAL");
}

WRITE_LINE_MEMBER( d6800_state::d6800_screen_w )
{
	m_screen_on = state;
}

READ8_MEMBER( d6800_state::d6800_cassette_r )
{
	/*
    Cassette circuit consists of a 741 op-amp, a 74121 oneshot, and a 74LS74.
    When a pulse arrives, the oneshot is set. After a preset time, it triggers
    and the 74LS74 compares this pulse to the output of the 741. Therefore it
    knows if the tone is 1200 or 2400 Hz. Input to PIA is bit 7.
    */

	return 0xff;
}

WRITE8_MEMBER( d6800_state::d6800_cassette_w )
{
	/*
    Cassette circuit consists of a 566 and a transistor. The 556 runs at 2400
    or 1200 Hz depending on the state of the transistor. This is controlled by
    bit 0 of the PIA. Bit 6 drives the speaker.
    */

	speaker_level_w(m_speaker, BIT(data, 6));
}

READ8_MEMBER( d6800_state::d6800_keyboard_r )
{
	UINT8 data = 15;

	if (!BIT(m_keylatch, 4)) data &= input_port_read(m_machine, "LINE0");
	if (!BIT(m_keylatch, 5)) data &= input_port_read(m_machine, "LINE1");
	if (!BIT(m_keylatch, 6)) data &= input_port_read(m_machine, "LINE2");
	if (!BIT(m_keylatch, 7)) data &= input_port_read(m_machine, "LINE3");

	return data | m_keylatch;
}

WRITE8_MEMBER( d6800_state::d6800_keyboard_w )
{
	/*

        bit     description

        PA0     keyboard column 0
        PA1     keyboard column 1
        PA2     keyboard column 2
        PA3     keyboard column 3
        PA4     keyboard row 0
        PA5     keyboard row 1
        PA6     keyboard row 2
        PA7     keyboard row 3

    */


	m_keylatch = data & 0xf0;
}

static const pia6821_interface d6800_mc6821_intf =
{
	DEVCB_DRIVER_MEMBER(d6800_state, d6800_keyboard_r),	/* port A input */
	DEVCB_DRIVER_MEMBER(d6800_state, d6800_cassette_r),	/* port B input */
	DEVCB_DRIVER_LINE_MEMBER(d6800_state, d6800_keydown_r),	/* CA1 input */
	DEVCB_DRIVER_LINE_MEMBER(d6800_state, d6800_rtc_pulse),	/* CB1 input */
	DEVCB_DRIVER_LINE_MEMBER(d6800_state, d6800_fn_key_r),	/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_DRIVER_MEMBER(d6800_state, d6800_keyboard_w),	/* port A output */
	DEVCB_DRIVER_MEMBER(d6800_state, d6800_cassette_w),	/* port B output */
	DEVCB_NULL,						/* CA2 output */
	DEVCB_DRIVER_LINE_MEMBER(d6800_state, d6800_screen_w),	/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6800_IRQ_LINE)		/* IRQB output */
};

/* Machine Initialization */

static MACHINE_START( d6800 )
{
}

static MACHINE_RESET( d6800 )
{
}

/* Machine Drivers */

static const cassette_config d6800_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_CONFIG_START( d6800, d6800_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M6800, XTAL_4MHz/4)
	MCFG_CPU_PROGRAM_MAP(d6800_map)
	MCFG_CPU_VBLANK_INT("screen", d6800_interrupt)

	MCFG_MACHINE_START(d6800)
	MCFG_MACHINE_RESET(d6800)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64, 32)
	MCFG_SCREEN_VISIBLE_AREA(0, 63, 0, 31)
	MCFG_SCREEN_UPDATE(d6800)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MCFG_PIA6821_ADD("pia", d6800_mc6821_intf)
	MCFG_CASSETTE_ADD("cassette", d6800_cassette_config)
MACHINE_CONFIG_END

/* ROMs */

ROM_START( d6800 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "d6800.bin", 0xc000, 0x0400, CRC(3f97ca2e) SHA1(60f26e57a058262b30befceceab4363a5d65d877) )
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        COMPANY                             FULLNAME                FLAGS */
COMP( 1979, d6800,	0,	0,	d6800,	    d6800,	0,	"Electronics Australia",	"Dream 6800",	GAME_NOT_WORKING | GAME_NO_SOUND )
