/***************************************************************************

        Central Data cd2650

        08/04/2010 Skeleton driver.

        No info available on this computer apart from a few newsletters.
        The system only uses 1000-14FF for videoram and 17F0-17FF for
        scratch ram. All other ram is optional.

        All commands must be in upper case. They are A,B,C,D,E,I,L,R,V.
        L,D,V appear to be commands to load, dump and verify tapes.
        The in and output lines for tapes are SENSE and FLAG, which is
        the usual with S2650 systems. The remaining commands have
        unknown functions.

        TODO
        - Lots, probably. The computer is a complete mystery. No pictures,
                manuals or schematics exist.
        - Find out how the scrolling works - if it scrolls at all.
        - Using the terminal keyboard, as it is unknown if it has its own.
        - Cassette interface.

        We used Winarcadia emulator to get an idea of what the computer
        might possibly look like, but we DID NOT read the source code.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"
#include "machine/terminal.h"


class cd2650_state : public driver_device
{
public:
	cd2650_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	const UINT8 *m_charrom;
	const UINT8 *m_videoram;
	UINT8 m_term_data;
};


static READ8_HANDLER( cd2650_keyin_r )
{
	cd2650_state *state = space->machine().driver_data<cd2650_state>();
	UINT8 ret = state->m_term_data;
	state->m_term_data = 0x80;
	if ((ret > 0x60) && (ret < 0x7b))
		ret -= 0x20; // upper case only
	return ret;
}

static ADDRESS_MAP_START(cd2650_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff) AM_ROM
	AM_RANGE( 0x0400, 0x0fff) AM_RAM
	AM_RANGE( 0x1000, 0x17ff) AM_RAM AM_BASE_MEMBER(cd2650_state, m_videoram)
	AM_RANGE( 0x1800, 0x7fff) AM_RAM // expansion ram? the system doesn't use it
ADDRESS_MAP_END

static ADDRESS_MAP_START( cd2650_io , AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(S2650_DATA_PORT,S2650_DATA_PORT) AM_READ(cd2650_keyin_r)
	//AM_RANGE(S2650_SENSE_PORT, S2650_FO_PORT) AM_READWRITE(cd2650_cass_in,cd2650_cass_out)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( cd2650 )
INPUT_PORTS_END


static MACHINE_RESET(cd2650)
{
	cd2650_state *state = machine.driver_data<cd2650_state>();
	state->m_term_data = 0x80;
}

static VIDEO_START( cd2650 )
{
	cd2650_state *state = machine.driver_data<cd2650_state>();
	state->m_charrom = machine.region("chargen")->base();
}

static SCREEN_UPDATE( cd2650 )
{
/* The video is unusual in that the characters in each line are spaced at 16 bytes in memory,
    thus line 1 starts at 1000, line 2 at 1001, etc. There are 16 lines of 80 characters.
    Further, the letters have bit 6 set low, thus the range is 01 to 1A.
    When the bottom of the screen is reached, it does not scroll, it just wraps around.
    There is no scroll register or any indication of how scrolling might be accomplished. */

	cd2650_state *state = screen->machine().driver_data<cd2650_state>();
	UINT16 offset = 0; // if we figure out how the scrolling works, that calculation goes here
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,x,mem;

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = 0; x < 80; x++)
			{
				gfx = 0;
				if (ra < 9)
				{
					mem = offset + y + (x<<4);

					if (mem > 0x4ff)
						mem -= 0x500;

					chr = state->m_videoram[mem];

					if (chr < 0x20)
						chr |= 0x40;

					gfx = state->m_charrom[(chr<<4) | ra ];
				}

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
	}
	return 0;
}

static WRITE8_DEVICE_HANDLER( cd2650_kbd_put )
{
	cd2650_state *state = device->machine().driver_data<cd2650_state>();
	state->m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( cd2650_terminal_intf )
{
	DEVCB_HANDLER(cd2650_kbd_put)
};

static MACHINE_CONFIG_START( cd2650, cd2650_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(cd2650_mem)
	MCFG_CPU_IO_MAP(cd2650_io)

	MCFG_MACHINE_RESET(cd2650)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 160)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 159)
	MCFG_SCREEN_UPDATE(cd2650)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(cd2650)

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, cd2650_terminal_intf) // keyboard only
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( cd2650 )
	ROM_REGION( 0x8000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "cd2650.rom", 0x0000, 0x0400, CRC(5397328e) SHA1(7106fdb60e1ad2bc5e8e45527f348c23296e8d6a))

	/* character generator not dumped, using the one from 'c10' for now */
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, BAD_DUMP CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1977, cd2650,  0,       0,	cd2650, 	cd2650, 	 0,   "Central Data",   "CD 2650",		GAME_NOT_WORKING | GAME_NO_SOUND )

