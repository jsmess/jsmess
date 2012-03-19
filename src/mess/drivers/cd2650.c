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
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/s2650/s2650.h"
#include "machine/terminal.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE16_MEMBER(name) UINT32 name::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)

class cd2650_state : public driver_device
{
public:
	cd2650_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_terminal(*this, TERMINAL_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_terminal;
	DECLARE_READ8_MEMBER( cd2650_keyin_r );
	DECLARE_WRITE8_MEMBER( kbd_put );
	const UINT8 *m_p_chargen;
	const UINT8 *m_p_videoram;
	UINT8 m_term_data;
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};


READ8_MEMBER( cd2650_state::cd2650_keyin_r )
{
	UINT8 ret = m_term_data;
	m_term_data = 0x80;
	if ((ret > 0x60) && (ret < 0x7b))
		ret -= 0x20; // upper case only
	return ret;
}

static ADDRESS_MAP_START(cd2650_mem, AS_PROGRAM, 8, cd2650_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff) AM_ROM
	AM_RANGE( 0x0400, 0x0fff) AM_RAM
	AM_RANGE( 0x1000, 0x17ff) AM_RAM AM_BASE(m_p_videoram)
	AM_RANGE( 0x1800, 0x7fff) AM_RAM // expansion ram? the system doesn't use it
ADDRESS_MAP_END

static ADDRESS_MAP_START( cd2650_io, AS_IO, 8, cd2650_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(S2650_DATA_PORT,S2650_DATA_PORT) AM_READ(cd2650_keyin_r)
	//AM_RANGE(S2650_SENSE_PORT, S2650_FO_PORT) AM_READWRITE(cd2650_cass_in,cd2650_cass_out)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( cd2650 )
INPUT_PORTS_END


MACHINE_RESET_MEMBER(cd2650_state)
{
	m_term_data = 0x80;
}

VIDEO_START_MEMBER(cd2650_state)
{
	m_p_chargen = machine().region("chargen")->base();
}

SCREEN_UPDATE16_MEMBER(cd2650_state)
{
/* The video is unusual in that the characters in each line are spaced at 16 bytes in memory,
    thus line 1 starts at 1000, line 2 at 1001, etc. There are 16 lines of 80 characters.
    Further, the letters have bit 6 set low, thus the range is 01 to 1A.
    When the bottom of the screen is reached, it does not scroll, it just wraps around.
    There is no scroll register or any indication of how scrolling might be accomplished. */

	UINT16 offset = 0; // if we figure out how the scrolling works, that calculation goes here
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,x,mem;

	for (y = 0; y < 16; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16 *p = &bitmap.pix16(sy++);

			for (x = 0; x < 80; x++)
			{
				gfx = 0;
				if (ra < 9)
				{
					mem = offset + y + (x<<4);

					if (mem > 0x4ff)
						mem -= 0x500;

					chr = m_p_videoram[mem];

					if (chr < 0x20)
						chr |= 0x40;

					gfx = m_p_chargen[(chr<<4) | ra ];
				}

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
	}
	return 0;
}

WRITE8_MEMBER( cd2650_state::kbd_put )
{
	m_term_data = data;
}

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(cd2650_state, kbd_put)
};

static MACHINE_CONFIG_START( cd2650, cd2650_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(cd2650_mem)
	MCFG_CPU_IO_MAP(cd2650_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(cd2650_state, screen_update)
	MCFG_SCREEN_SIZE(640, 160)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 159)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf) // keyboard only
	MCFG_DEVICE_REMOVE(":terminal:terminal_screen")
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

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY        FULLNAME       FLAGS */
COMP( 1977, cd2650, 0,      0,       cd2650,    cd2650,  0,   "Central Data",   "CD 2650", GAME_NOT_WORKING | GAME_NO_SOUND )
