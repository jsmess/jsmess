/***************************************************************************

        Cromemco C-10 Personal Computer

        30/08/2010 Skeleton driver

        Driver currently gets to a loop where it waits for an interrupt.
        The interrupt routine presumably writes to FE69 which the loop is
        constantly looking at.

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"

#define MACHINE_RESET_MEMBER(name) void name::machine_reset()
#define VIDEO_START_MEMBER(name) void name::video_start()
#define SCREEN_UPDATE_MEMBER(name) bool name::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)

class c10_state : public driver_device
{
public:
	c10_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config),
	m_maincpu(*this, "maincpu")
	{ }

	required_device<cpu_device> m_maincpu;
	const UINT8 *m_p_chargen;
	const UINT8 *m_p_videoram;
	virtual void machine_reset();
	virtual void video_start();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
};



static ADDRESS_MAP_START(c10_mem, AS_PROGRAM, 8, c10_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("boot")
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xf0a1) AM_RAM
	AM_RANGE(0xf0a2, 0xffff) AM_RAM AM_BASE(m_p_videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( c10_io, AS_IO, 8, c10_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( c10 )
INPUT_PORTS_END

/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( c10_reset )
{
	memory_set_bank(machine, "boot", 0);
}

MACHINE_RESET_MEMBER(c10_state)
{
	memory_set_bank(m_machine, "boot", 1);
	m_machine.scheduler().timer_set(attotime::from_usec(4), FUNC(c10_reset));
}

VIDEO_START_MEMBER( c10_state )
{
	m_p_chargen = m_machine.region("chargen")->base();
}

/* This system appears to have inline attribute bytes of unknown meaning.
    Currently they are ignored. The word at FAB5 looks like it might be cursor location. */
SCREEN_UPDATE_MEMBER( c10_state )
{
	//static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x,xx;

	//framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16 *p = BITMAP_ADDR16(&bitmap, sy++, 0);

			xx = ma;
			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;
				if (ra < 9)
				{
					chr = m_p_videoram[xx++];

				//  /* Take care of flashing characters */
				//  if ((chr < 0x80) && (framecnt & 0x08))
				//      chr |= 0x80;

					if BIT(chr, 7)  // ignore attribute bytes
						x--;
					else
					{
						gfx = m_p_chargen[(chr<<4) | ra ];

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
		}
		ma+=96;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout c10_charlayout =
{
	8, 9,					/* 8 x 9 characters */
	512,					/* 512 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( c10 )
	GFXDECODE_ENTRY( "chargen", 0x0000, c10_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_CONFIG_START( c10, c10_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
	MCFG_CPU_PROGRAM_MAP(c10_mem)
	MCFG_CPU_IO_MAP(c10_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 250)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
	MCFG_GFXDECODE(c10)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
MACHINE_CONFIG_END

DRIVER_INIT( c10 )
{
	UINT8 *RAM = machine.region("maincpu")->base();
	memory_configure_bank(machine, "boot", 0, 2, &RAM[0x0000], 0x8000);
}

/* ROM definition */
ROM_START( c10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "c10_cros.bin", 0x8000, 0x4000, CRC(2ccf5983) SHA1(52f7c497f5284bf5df9eb0d6e9142bb1869d8c24))

	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 1982, c10,  0,       0,	c10,	c10,	 c10,	  "Cromemco",   "C-10",	GAME_NOT_WORKING | GAME_NO_SOUND)
