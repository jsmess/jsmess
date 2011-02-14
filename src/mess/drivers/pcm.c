/***************************************************************************

        PC/M by Miodrag Milanovic

        12/05/2009 Preliminary driver.

        14/02/2011 Added keyboard (from terminal).

        Commands haven't been checked out yet, but an
        example is the following sequence: (press return at end of each line)

        D
        U
        E

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"


class pcm_state : public driver_device
{
public:
	pcm_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	DECLARE_READ8_MEMBER( pcm_84_r );
	DECLARE_WRITE8_MEMBER( pcm_kbd_put );
	UINT8 *videoram;
	UINT8 term_data;
	UINT8 step;
};

READ8_MEMBER( pcm_state::pcm_84_r )
{
	UINT8 ret = term_data;
	if (step < 2)
	{
		ret |= 0x80;
		step++;
	}
	return ret;
}


static ADDRESS_MAP_START(pcm_mem, ADDRESS_SPACE_PROGRAM, 8, pcm_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x1fff ) AM_ROM  // ROM
	AM_RANGE( 0x2000, 0xf7ff ) AM_RAM  // RAM
	AM_RANGE( 0xf800, 0xffff ) AM_RAM AM_BASE(videoram) // Video RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pcm_io, ADDRESS_SPACE_IO, 8, pcm_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x84, 0x84) AM_READ(pcm_84_r)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pcm )
INPUT_PORTS_END

WRITE8_MEMBER( pcm_state::pcm_kbd_put )
{
	term_data = data;
	step = 0;
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

static VIDEO_UPDATE( pcm )
{
	pcm_state *state = screen->machine->driver_data<pcm_state>();
	UINT8 code;
	UINT8 line;
	int y, x, j, b;

	UINT8 *gfx = screen->machine->region("chargen")->base();

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 64; x++)
		{
			code = state->videoram[(0x400 + y*64 + x) & 0x7ff];
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

	MCFG_MACHINE_RESET(pcm)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1)
	MCFG_GFXDECODE(pcm)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(pcm)
	MCFG_VIDEO_UPDATE(pcm)

	MCFG_GENERIC_TERMINAL_ADD("terminal", pcm_terminal_intf)
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
	ROM_REGION(0x0800, "k7659",0)
	ROM_LOAD ("k7659n.d8", 0x0000, 0x0800, CRC(7454bf0a) SHA1(b97e7df93778fa371b96b6f4fb1a5b1c8b89d7ba) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1988, pcm,	 0,       0,	 pcm,		pcm,	 0, 	 "Mugler/Mathes",   "PC/M", GAME_NOT_WORKING | GAME_NO_SOUND)

