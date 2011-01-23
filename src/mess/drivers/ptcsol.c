/***************************************************************************

    Processor Technology Corp. SOL-20

    07/2009 Skeleton driver.

    Info from: http://www.sol20.org/

    Note that the SOLOS dump comes from the Solace emu. Not being sure if
    it has been verified with a real SOL-20 (even if I think it has been), I
    marked it as a BAD_DUMP.

    The roms DPMON and CONSOL are widely available on the net as ENT files,
    which can be loaded into memory with the Paste option, then exported to
    binary files with the debugger. They are working, however the ENT files
    do not indicate the values of unused rom space. Therefore they are marked
    as BAD_DUMP.

    Note that the CONSOL rom is basically a dumb terminal program and doesn't
    do anything useful unless the MODE key (whatever that is) is pressed.

    Other OS ROMs to be dumped:
      - CONSOL
      - CUTER
      - BOOTLOAD
      - DPMON (3rd party)

    Other roms needed:
      - Dump of the 2 character generators (motorola 6574 and 6575)
      - Dump of U18 in the keyboard

    Hardware variants (ever shipped?):
      - SOL-10, i.e. a stripped down version without expansion backplane and
        numeric keypad
      - SOL-PC, i.e. the single board version, basically it consisted of the
        SOL-20 board only

    Similarity to Exidy Sorcerer:
      - Some hardware is the same design (parallel ports, serial ports, 2
        tape players, the cassette circuits)
      - The cassette format is totally identical, in fact tapes from one
        machine can be loaded on the other natively. They won't run of course.
      - Sorcerer monitor commands are almost identical to the SOLOS ones.
        To illustrate some differences:
        Run a program in memory  GO (sorcerer) EX (SOLOS)
        Load a file from tape    LO (Sorcerer) GE (SOLOS)
        Set tape operation to 300 baud  SE T=1 (sorcerer)  SE TA 1 (SOLOS)
        Most other commands are identical. The EN command (enter bytes into
        memory) is rather primitive in SOLOS, better on Sorcerer.
      - Sol20 has "personality modules" where you could plug a new OS into
        a special socket on the motherboard. Sorcerer improved upon this by
        being the first computer to use cartridges.
      - Some circuits are completely different... the video and keyboard are
        notable examples, while the address of ram on the basic machines is
        another major difference.
****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/terminal.h"
#include "sound/wave.h"
#include "imagedev/cassette.h"
#include "machine/ay31015.h"

typedef struct {
	struct {
		int length;		/* time cassette level is at input.level */
		int level;		/* cassette level */
		int bit;		/* bit being read */
	} input;
	struct {
		int length;		/* time cassette level is at output.level */
		int level;		/* cassette level */
		int bit;		/* bit to to output */
	} output;
} cass_data_t;

class sol20_state : public driver_device
{
public:
	sol20_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_cass1(*this, "cassette1"),
	m_cass2(*this, "cassette2"),
	m_uart(*this, "uart"),
	m_uart_s(*this, "uart_s"),
	m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_cass1;
	required_device<device_t> m_cass2;
	required_device<device_t> m_uart;
	required_device<device_t> m_uart_s;
	required_device<device_t> m_terminal;
	DECLARE_READ8_MEMBER( sol20_fa_r );
	DECLARE_READ8_MEMBER( sol20_fb_r );
	DECLARE_READ8_MEMBER( sol20_fc_r );
	DECLARE_WRITE8_MEMBER( sol20_fa_w );
	DECLARE_WRITE8_MEMBER( sol20_fb_w );
	DECLARE_WRITE8_MEMBER( sol20_fe_w );
	DECLARE_WRITE8_MEMBER( sol20_kbd_put );
	UINT8 m_sol20_fa;
	UINT8 m_sol20_fc;
	UINT8 m_sol20_fe;
	const UINT8 *FNT;
	const UINT8 *m_videoram;
	UINT8 m_framecnt;
	emu_timer *m_cassette_timer;
	cass_data_t m_cass_data;
};


/* timer to read cassette waveforms */


static device_t *cassette_device_image(running_machine *machine)
{
	sol20_state *state = machine->driver_data<sol20_state>();
	if (state->m_sol20_fa & 0x40)
		return machine->device("cassette2");
	else
		return machine->device("cassette1");
}


// identical to sorcerer
static TIMER_CALLBACK(sol20_cassette_tc)
{
	sol20_state *state = machine->driver_data<sol20_state>();
	UINT8 cass_ws = 0;
	switch (state->m_sol20_fa & 0x20)
	{
		case 0x20:				/* Cassette 300 baud */

			/* loading a tape - this is basically the same as the super80.
                           We convert the 1200/2400 Hz signal to a 0 or 1, and send it to the uart. */

			state->m_cass_data.input.length++;

			cass_ws = (cassette_input(cassette_device_image(machine)) > +0.02) ? 1 : 0;

			if (cass_ws != state->m_cass_data.input.level)
			{
				state->m_cass_data.input.level = cass_ws;
				state->m_cass_data.input.bit = ((state->m_cass_data.input.length < 0x6) || (state->m_cass_data.input.length > 0x20)) ? 1 : 0;
				state->m_cass_data.input.length = 0;
				ay31015_set_input_pin( state->m_uart, AY31015_SI, state->m_cass_data.input.bit );
			}

			/* saving a tape - convert the serial stream from the uart, into 1200 and 2400 Hz frequencies.
                           Synchronisation of the frequency pulses to the uart is extremely important. */

			state->m_cass_data.output.length++;
			if (!(state->m_cass_data.output.length & 0x1f))
			{
				cass_ws = ay31015_get_output_pin( state->m_uart, AY31015_SO );
				if (cass_ws != state->m_cass_data.output.bit)
				{
					state->m_cass_data.output.bit = cass_ws;
					state->m_cass_data.output.length = 0;
				}
			}

			if (!(state->m_cass_data.output.length & 3))
			{
				if (!((state->m_cass_data.output.bit == 0) && (state->m_cass_data.output.length & 4)))
				{
					state->m_cass_data.output.level ^= 1;			// toggle output state, except on 2nd half of low bit
					cassette_output(cassette_device_image(machine), state->m_cass_data.output.level ? -1.0 : +1.0);
				}
			}
			return;

		case 0x00:			/* Cassette 1200 baud */
			/* loading a tape */
			state->m_cass_data.input.length++;

			cass_ws = (cassette_input(cassette_device_image(machine)) > +0.02) ? 1 : 0;

			if (cass_ws != state->m_cass_data.input.level || state->m_cass_data.input.length == 10)
			{
				state->m_cass_data.input.bit = ((state->m_cass_data.input.length < 10) || (state->m_cass_data.input.length > 0x20)) ? 1 : 0;
				if ( cass_ws != state->m_cass_data.input.level )
				{
					state->m_cass_data.input.length = 0;
					state->m_cass_data.input.level = cass_ws;
				}
				ay31015_set_input_pin( state->m_uart, AY31015_SI, state->m_cass_data.input.bit );
			}

			/* saving a tape - convert the serial stream from the uart, into 600 and 1200 Hz frequencies. */

			state->m_cass_data.output.length++;
			if (!(state->m_cass_data.output.length & 7))
			{
				cass_ws = ay31015_get_output_pin( state->m_uart, AY31015_SO );
				if (cass_ws != state->m_cass_data.output.bit)
				{
					state->m_cass_data.output.bit = cass_ws;
					state->m_cass_data.output.length = 0;
				}
			}

			if (!(state->m_cass_data.output.length & 7))
			{
				if (!((state->m_cass_data.output.bit == 0) && (state->m_cass_data.output.length & 8)))
				{
					state->m_cass_data.output.level ^= 1;			// toggle output state, except on 2nd half of low bit
					cassette_output(cassette_device_image(machine), state->m_cass_data.output.level ? -1.0 : +1.0);
				}
			}
			return;
	}
}


READ8_MEMBER( sol20_state::sol20_fa_r )
{
	/* set unused bits high */
	UINT8 data = 0x26;

	ay31015_set_input_pin( m_uart, AY31015_SWE, 0 );
	data |= ay31015_get_output_pin( m_uart, AY31015_TBMT ) ? 0x80 : 0;
	data |= ay31015_get_output_pin( m_uart, AY31015_DAV  ) ? 0x40 : 0;
	data |= ay31015_get_output_pin( m_uart, AY31015_OR   ) ? 0x10 : 0;
	data |= ay31015_get_output_pin( m_uart, AY31015_FE   ) ? 0x08 : 0;
	ay31015_set_input_pin( m_uart, AY31015_SWE, 1 );

	return data | (m_sol20_fa & 1);
}

READ8_MEMBER( sol20_state::sol20_fb_r)
{
	UINT8 data = ay31015_get_received_data( m_uart );
	ay31015_set_input_pin( m_uart, AY31015_RDAV, 0 );
	ay31015_set_input_pin( m_uart, AY31015_RDAV, 1 );
	return data;
}

READ8_MEMBER( sol20_state::sol20_fc_r )
{
	m_sol20_fa |= 1;
	return m_sol20_fc;
}

WRITE8_MEMBER( sol20_state::sol20_fa_w )
{
	m_sol20_fa &= 1;
	m_sol20_fa |= (data & 0xf0);

	/* cassette 1 motor */
	cassette_change_state(m_cass1,
		(BIT(data, 7)) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	/* cassette 2 motor */
	cassette_change_state(m_cass2,
		(BIT(data, 6)) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	if (data & 0xc0)
		timer_adjust_periodic(m_cassette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(19200));
	else
		timer_adjust_oneshot(m_cassette_timer, attotime_zero, 0);

	// bit 5 baud rate */
	ay31015_set_receiver_clock( m_uart, (BIT(data, 5)) ? 4800.0 : 19200.0);
	ay31015_set_transmitter_clock( m_uart, (BIT(data, 5)) ? 4800.0 : 19200.0);
}

WRITE8_MEMBER( sol20_state::sol20_fb_w )
{
	ay31015_set_transmit_data( m_uart, data );
}

WRITE8_MEMBER( sol20_state::sol20_fe_w )
{
	m_sol20_fe = data;
}

static ADDRESS_MAP_START( sol20_mem, ADDRESS_SPACE_PROGRAM, 8, sol20_state)
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK("boot")
	AM_RANGE(0X0800, 0Xbfff) AM_RAM	// optional s100 ram
	AM_RANGE(0xc000, 0xc7ff) AM_ROM
	AM_RANGE(0Xc800, 0Xcbff) AM_RAM	// system ram
	AM_RANGE(0Xcc00, 0Xcfff) AM_RAM	AM_BASE(m_videoram)
	AM_RANGE(0Xd000, 0Xffff) AM_RAM	// optional s100 ram
ADDRESS_MAP_END

static ADDRESS_MAP_START( sol20_io , ADDRESS_SPACE_IO, 8, sol20_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xfa, 0xfa) AM_READWRITE(sol20_fa_r,sol20_fa_w)
	AM_RANGE(0xfb, 0xfb) AM_READWRITE(sol20_fb_r,sol20_fb_w)
	AM_RANGE(0xfc, 0xfc) AM_READ(sol20_fc_r)
	AM_RANGE(0xfe, 0xfe) AM_WRITE(sol20_fe_w)
/*  AM_RANGE(0xf8, 0xf8) serial status in (bit 6=data av, bit 7=tmbe)
    AM_RANGE(0xf9, 0xf9) serial data in, out
    AM_RANGE(0xfa, 0xfa) general status in (bit 0=keyb data av, bit 1=parin data av, bit 2=parout ready)
    AM_RANGE(0xfb, 0xfb) tape
    AM_RANGE(0xfc, 0xfc) keyboard data in
    AM_RANGE(0xfd, 0xfd) parallel data in, out
    AM_RANGE(0xfe, 0xfe) scroll register
    AM_RANGE(0xff, 0xff) sense switches */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sol20 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END

static const ay31015_config sol20_ay31015_config =
{
	AY_3_1015,
	4800.0,
	4800.0,
	NULL,
	NULL,
	NULL
};


static const cassette_config sol20_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( sol20_boot )
{
	memory_set_bank(machine, "boot", 0);
}

static MACHINE_START( sol20 )
{
	sol20_state *state = machine->driver_data<sol20_state>();
	state->m_cassette_timer = timer_alloc(machine, sol20_cassette_tc, NULL);
}

static MACHINE_RESET( sol20 )
{
	sol20_state *state = machine->driver_data<sol20_state>();
	state->m_sol20_fe=0;
	state->m_sol20_fa=1;
	// set hard-wired uart pins
	ay31015_set_input_pin( state->m_uart, AY31015_CS, 0 );
	ay31015_set_input_pin( state->m_uart, AY31015_NB1, 1);
	ay31015_set_input_pin( state->m_uart, AY31015_NB2, 1);
	ay31015_set_input_pin( state->m_uart, AY31015_TSB, 1);
	ay31015_set_input_pin( state->m_uart, AY31015_EPS, 1);
	ay31015_set_input_pin( state->m_uart, AY31015_NP,  1);
	ay31015_set_input_pin( state->m_uart, AY31015_CS, 1 );
	// boot-bank
	memory_set_bank(machine, "boot", 1);
	timer_set(machine, ATTOTIME_IN_USEC(9), NULL, 0, sol20_boot);
}

static DRIVER_INIT( sol20 )
{
	UINT8 *RAM = machine->region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0xc000);
}

static VIDEO_START( sol20 )
{
	sol20_state *state = machine->driver_data<sol20_state>();
	state->FNT = machine->region("chargen")->base();
}

static VIDEO_UPDATE( sol20 )
{
	sol20_state *state = screen->machine->driver_data<sol20_state>();
/* visible screen is 64 x 16, with start position controlled by scroll register */
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma,x,inv;

	state->m_framecnt++;

	ma = state->m_sol20_fe << 6; // scroll register

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 13; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 64; x++)
			{
				chr = state->m_videoram[x & 0x3ff];
				inv = 0;

				/* Take care of flashing characters */
				//if ((chr & 0x80) && (state->m_framecnt & 0x08))
				if (chr & 0x80)
					inv ^= 0xff;

				chr &= 0x7f;

				if (ra == 0)
					gfx = 0;
				else
				if (ra < 10)
					gfx = state->FNT[(chr<<4) | (ra-1) ] ^ inv;
				else
					gfx = inv;

				/* Display a scanline of a character */
				*p++ = ( gfx & 0x80 ) ? 1 : 0;
				*p++ = ( gfx & 0x40 ) ? 1 : 0;
				*p++ = ( gfx & 0x20 ) ? 1 : 0;
				*p++ = ( gfx & 0x10 ) ? 1 : 0;
				*p++ = ( gfx & 0x08 ) ? 1 : 0;
				*p++ = ( gfx & 0x04 ) ? 1 : 0;
				*p++ = ( gfx & 0x02 ) ? 1 : 0;
				*p++ = ( gfx & 0x01 ) ? 1 : 0;
			}
		}
		ma+=64;
	}
	return 0;
}

WRITE8_MEMBER( sol20_state::sol20_kbd_put )
{
	m_sol20_fa &= 0xfe;
	m_sol20_fc = data | 0x80; // CONSOL requires bit 7 to be high, others don't care
}

static GENERIC_TERMINAL_INTERFACE( sol20_terminal_intf )
{
	DEVCB_DRIVER_MEMBER(sol20_state, sol20_kbd_put)
};

static MACHINE_CONFIG_START( sol20, sol20_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8080, XTAL_14_31818MHz/7)
	MCFG_CPU_PROGRAM_MAP(sol20_mem)
	MCFG_CPU_IO_MAP(sol20_io)

	MCFG_MACHINE_START(sol20)
	MCFG_MACHINE_RESET(sol20)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(512, 208)
	MCFG_SCREEN_VISIBLE_AREA(0, 511, 0, 207)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(sol20)
	MCFG_VIDEO_UPDATE(sol20)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave.1", "cassette1")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	// cass1 speaker
	MCFG_SOUND_WAVE_ADD("wave.2", "cassette2")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	// cass2 speaker

	// devices
	MCFG_CASSETTE_ADD( "cassette1", sol20_cassette_config )
	MCFG_CASSETTE_ADD( "cassette2", sol20_cassette_config )
	MCFG_AY31015_ADD( "uart", sol20_ay31015_config )
	MCFG_AY31015_ADD( "uart_s", sol20_ay31015_config )
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, sol20_terminal_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( sol20 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_SYSTEM_BIOS(0, "SOLOS", "SOLOS")
	ROMX_LOAD( "solos.bin", 0xc000, 0x0800, BAD_DUMP CRC(4d0af383) SHA1(ac4510c3380ed4a31ccf4f538af3cb66b76701ef), ROM_BIOS(1) )	// from solace emu
	ROM_SYSTEM_BIOS(1, "DPMON", "DPMON")
	ROMX_LOAD( "dpmon.bin", 0xc000, 0x0800, BAD_DUMP CRC(2a84f099) SHA1(60ff6e38082c50afcf0f40707ef65668a411008b), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS(2, "CONSOL", "CONSOL")
	ROMX_LOAD( "consol.bin", 0xc000, 0x0400, BAD_DUMP CRC(80bf6d85) SHA1(84b81c60bb08a3a5435ec1be56a67aa695bce099), ROM_BIOS(3) )
	/* Character generator rom is missing */

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
	/* There are actually 2 character generators available - the 6574 and the 6575 */
ROM_END

/* Driver */
/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT   COMPANY     FULLNAME   FLAGS */
COMP( 1976, sol20,  0,      0,      sol20,   sol20, sol20, "Processor Technology Corporation",  "SOL-20", GAME_NOT_WORKING )
