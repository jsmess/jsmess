/***************************************************************************

        PC/M by Miodrag Milanovic

        http://www.li-pro.net/pcm.phtml  (in German)

        12/05/2009 Preliminary driver.

        14/02/2011 Added keyboard (from terminal).

        Commands:
        1 select memory bank 1
        2 select memory bank 2
        B
        C start cp/m from the inbuilt CCP
        D Debugger
        Fx Format disk A or B
        G  Jump to address
        I List files on tape
        L filename.typ  Load file from tape
        R read from disk
        S filename aaaa / bbbb save a file to tape
        V verify
        W write to disk
        X
        Z set tape baud (1200, 2400, 3600 (default), 4800)
        filename   start running this .COM file

        Therefore if you enter random input, it will lock up while trying to
        load up a file of that name. Filenames on disk and tape are of the
        standard 8.3 format. You must specify an extension.

        Here is an example of starting the debugger, executing a command in
        it, then exiting back to the monitor.

        D
        U
        E

        In practice, the I and R commands produce an error, while all disk
        commands are directed to tape. The F command lists the files on a
        tape.

        ToDo:
        - Add bankswitching
        - Add NMI generator
        - Find out if there really is any floppy-disk feature - the schematic
          has no mention of it.
        - Check the screen - it only uses the first 16 lines.
        - Add the 6 LEDs.
        - Replace the terminal keyboard with an inbuilt one.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "imagedev/cassette.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/terminal.h"


class pcm_state : public driver_device
{
public:
	pcm_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG),
	m_pio_s(*this, "z80pio_s"),
	m_pio_u(*this, "z80pio_u"),
	m_sio(*this, "z80sio"),
	m_ctc_s(*this, "z80ctc_s"),
	m_ctc_u(*this, "z80ctc_u"),
	m_speaker(*this, "speaker"),
	m_cass(*this, "cassette")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	required_device<device_t> m_pio_s;
	required_device<device_t> m_pio_u;
	required_device<device_t> m_sio;
	required_device<device_t> m_ctc_s;
	required_device<device_t> m_ctc_u;
	required_device<device_t> m_speaker;
	required_device<device_t> m_cass;
	DECLARE_READ8_MEMBER( pcm_84_r );
	DECLARE_READ8_MEMBER( pcm_85_r );
	DECLARE_WRITE_LINE_MEMBER( pcm_82_w );
	DECLARE_WRITE8_MEMBER( pcm_85_w );
	DECLARE_WRITE8_MEMBER( pcm_kbd_put );
	UINT8 *m_videoram;
	UINT8 m_term_data;
	UINT8 m_step;
	UINT8 m_85;
};


WRITE_LINE_MEMBER( pcm_state::pcm_82_w )
{
	speaker_level_w(m_speaker, state);
}


/* PIO connections as far as i could decipher

PortA is input and connects to the keyboard
PortB is mostly output and connects to a series of LEDs,
      but also carries the cassette control & data lines.

A0-A6 ascii codes from the keyboard
A7 strobe, high while a key is pressed
B0 power indicator LED
B1 Run/Stop LED
B2 Sound on/off LED
B3 n/c
B4 High=Save, Low=Load LED
B5 Motor On LED
B6 Save data
B7 Load data
There is also a HALT LED, connected directly to the processor.
*/

READ8_MEMBER( pcm_state::pcm_84_r )
{
	UINT8 ret = m_term_data;
	if (m_step < 2)
	{
		ret |= 0x80;
		m_step++;
	}
	return ret;
}

READ8_MEMBER( pcm_state::pcm_85_r )
{
	UINT8 data = m_85 & 0x7f;

	if (cassette_input(m_cass) > 0.03)
		data |= 0x80;

	return data;
}

WRITE8_MEMBER( pcm_state::pcm_85_w )
{
	if (BIT(data, 5))
		cassette_change_state(m_cass, CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
	else
		cassette_change_state(m_cass, CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	cassette_output(m_cass, BIT(data, 6) ? -1.0 : +1.0);
	m_85 = data;
}



static ADDRESS_MAP_START(pcm_mem, AS_PROGRAM, 8, pcm_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_ROM  // ROM
	AM_RANGE( 0x2000, 0xf7ff ) AM_RAM  // RAM
	AM_RANGE( 0xf800, 0xffff ) AM_RAM AM_BASE(m_videoram) // Video RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pcm_io, AS_IO, 8, pcm_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE_LEGACY("z80ctc_s", z80ctc_r, z80ctc_w) // system CTC
	AM_RANGE(0x84, 0x87) AM_DEVREADWRITE_LEGACY("z80pio_s", z80pio_cd_ba_r, z80pio_cd_ba_w) // system PIO
	AM_RANGE(0x88, 0x8B) AM_DEVREADWRITE_LEGACY("z80sio", z80sio_cd_ba_r, z80sio_cd_ba_w) // SIO
	AM_RANGE(0x8C, 0x8F) AM_DEVREADWRITE_LEGACY("z80ctc_u", z80ctc_r, z80ctc_w) // user CTC
	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE_LEGACY("z80pio_u", z80pio_cd_ba_r, z80pio_cd_ba_w) // user PIO
	//AM_RANGE(0x94, 0x97) // bank select
	//AM_RANGE(0x98, 0x9B) // NMI generator
	//AM_RANGE(0x9C, 0x9F) // io ports available to the user
	// disk controller?
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pcm )
INPUT_PORTS_END

WRITE8_MEMBER( pcm_state::pcm_kbd_put )
{
	m_term_data = data;
	m_step = 0;
}

static GENERIC_TERMINAL_INTERFACE( pcm_terminal_intf )
{
	DEVCB_DRIVER_MEMBER(pcm_state, pcm_kbd_put)
};

static MACHINE_RESET(pcm)
{
}

static VIDEO_START( pcm )
{
}

static SCREEN_UPDATE( pcm )
{
	pcm_state *state = screen->machine().driver_data<pcm_state>();
	UINT8 code;
	UINT8 line;
	int y, x, j, b;

	UINT8 *gfx = screen->machine().region("chargen")->base();

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 64; x++)
		{
			code = state->m_videoram[(0x400 + y*64 + x) & 0x7ff];
			for(j = 0; j < 8; j++ )
			{
				line = gfx[code*8 + j];
				for (b = 0; b < 8; b++)
				{
					*BITMAP_ADDR16(bitmap, y*8+j, x * 8 + (7 - b)) =  ((line >> b) & 0x01);
				}
			}
		}
	}
	return 0;
}


// someone please check this
static const z80_daisy_config pcm_daisy_chain[] =
{
	{ "z80ctc_s" },		/* System ctc */
	{ "z80pio_s" },		/* System pio */
	{ "z80sio" },		/* sio */
	{ "z80pio_u" },		/* User pio */
	{ "z80ctc_u" },		/* User ctc */
	{ NULL }
};

static Z80CTC_INTERFACE( ctc_u_intf )
{
	0,				/* timer disablers */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), // interrupt callback
	DEVCB_NULL,			/* ZC/TO0 callback */
	DEVCB_NULL,			/* ZC/TO1 callback */
	DEVCB_NULL			/* ZC/TO2 callback */
};

static Z80CTC_INTERFACE( ctc_s_intf )
{
	0,				/* timer disablers */
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), // interrupt callback
	DEVCB_NULL,			/* ZC/TO0 callback - SIO channel A clock */
	DEVCB_NULL,			/* ZC/TO1 callback - SIO channel B clock */
	DEVCB_DRIVER_LINE_MEMBER(pcm_state, pcm_82_w) /* ZC/TO2 callback - speaker */
};

static Z80PIO_INTERFACE( pio_u_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), // interrupt callback
	DEVCB_NULL,			/* read port A */
	DEVCB_NULL,			/* write port A */
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_NULL,			/* read port B */
	DEVCB_NULL,			/* write port B */
	DEVCB_NULL			/* portB ready active callback */
};

static Z80PIO_INTERFACE( pio_s_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), // interrupt callback
	DEVCB_DRIVER_MEMBER(pcm_state, pcm_84_r),			/* read port A */
	DEVCB_NULL,			/* write port A */
	DEVCB_NULL,			/* portA ready active callback */
	DEVCB_DRIVER_MEMBER(pcm_state, pcm_85_r),			/* read port B */
	DEVCB_DRIVER_MEMBER(pcm_state, pcm_85_w),			/* write port B */
	DEVCB_NULL			/* portB ready active callback */
};

static const z80sio_interface sio_intf =
{
	0, //DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0), // interrupt callback
	0,			/* DTR changed handler */
	0,			/* RTS changed handler */
	0,			/* BREAK changed handler */
	0,			/* transmit handler - which channel is this for? */
	0			/* receive handler - which channel is this for? */
};



/* F4 Character Displayer */
static const gfx_layout pcm_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( pcm )
	GFXDECODE_ENTRY( "chargen", 0x0000, pcm_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( pcm, pcm_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_10MHz /4)
	MCFG_CPU_PROGRAM_MAP(pcm_mem)
	MCFG_CPU_IO_MAP(pcm_io)
	MCFG_CPU_CONFIG(pcm_daisy_chain)

	MCFG_MACHINE_RESET(pcm)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)
	MCFG_SCREEN_UPDATE(pcm)
	MCFG_GFXDECODE(pcm)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(pcm)

	/* Sound */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD("wave", "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* Devices */
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, pcm_terminal_intf)
	MCFG_CASSETTE_ADD( "cassette", default_cassette_config )
	MCFG_Z80PIO_ADD( "z80pio_u", XTAL_10MHz /4, pio_u_intf )
	MCFG_Z80PIO_ADD( "z80pio_s", XTAL_10MHz /4, pio_s_intf )
	MCFG_Z80SIO_ADD( "z80sio", 4800, sio_intf ) // clocks come from the system ctc
	MCFG_Z80CTC_ADD( "z80ctc_u", 1379310.344828, ctc_u_intf ) // numbers need to be corrected
	MCFG_Z80CTC_ADD( "z80ctc_s", 1379310.344828, ctc_s_intf ) // numbers need to be corrected
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pcm )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v202", "Version 2.02" )
	ROMX_LOAD( "bios_v202.d14", 0x0000, 0x0800, CRC(27c24892) SHA1(a97bf9ef075de91330dc0c7cfd3bb6c7a88bb585), ROM_BIOS(1))
	ROMX_LOAD( "bios_v202.d15", 0x0800, 0x0800, CRC(e9cedc70) SHA1(913c526283d9289d0cb2157985bb48193df7aa16), ROM_BIOS(1))
	ROMX_LOAD( "bios_v202.d16", 0x1000, 0x0800, CRC(abe12001) SHA1(d8f0db6b141736d7715d588384fa49ab386bcc55), ROM_BIOS(1))
	ROMX_LOAD( "bios_v202.d17", 0x1800, 0x0800, CRC(2d48d1cc) SHA1(36a825140124dbe10d267fdf28b3eacec6f6d556), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v210", "Version 2.10" )
	ROMX_LOAD( "bios_v210.d14", 0x0000, 0x0800, CRC(45923112) SHA1(dde922533ebd0f6ac06d25b9786830ee3c7178b9), ROM_BIOS(2))
	ROMX_LOAD( "bios_v210.d15", 0x0800, 0x0800, CRC(e9cedc70) SHA1(913c526283d9289d0cb2157985bb48193df7aa16), ROM_BIOS(2))
	ROMX_LOAD( "bios_v210.d16", 0x1000, 0x0800, CRC(ee9ed77b) SHA1(12ea18e3e280f2a0657ff11c7bcdd658d325235c), ROM_BIOS(2))
	ROMX_LOAD( "bios_v210.d17", 0x1800, 0x0800, CRC(2d48d1cc) SHA1(36a825140124dbe10d267fdf28b3eacec6f6d556), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v330", "Version 3.30" )
	ROMX_LOAD( "bios_v330.d14", 0x0000, 0x0800, CRC(9bbfee10) SHA1(895002f2f4c711278f1e2d0e2a987e2d31472e4f), ROM_BIOS(3))
	ROMX_LOAD( "bios_v330.d15", 0x0800, 0x0800, CRC(4f8d5b40) SHA1(440b0be4cf45a5d450f9eb7684ceb809450585dc), ROM_BIOS(3))
	ROMX_LOAD( "bios_v330.d16", 0x1000, 0x0800, CRC(93fd0d91) SHA1(c8f1bbb63eca3c93560622581ecbb588716aeb91), ROM_BIOS(3))
	ROMX_LOAD( "bios_v330.d17", 0x1800, 0x0800, CRC(d8c7ce33) SHA1(9030d9a73ef1c12a31ac2cb9a593fb2a5097f24d), ROM_BIOS(3))

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD( "charrom.d113", 0x0000, 0x0800, CRC(5684b3c3) SHA1(418054aa70a0fd120611e32059eb2051d3b82b5a))

	ROM_REGION(0x0800, "k7659",0) // keyboard encoder
	ROM_LOAD ("k7659n.d8", 0x0000, 0x0800, CRC(7454bf0a) SHA1(b97e7df93778fa371b96b6f4fb1a5b1c8b89d7ba) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1988, pcm,	 0,       0,	 pcm,		pcm,	 0, 	 "Mugler/Mathes",   "PC/M", 0)

