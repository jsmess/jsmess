/***************************************************************************

        Cromemco C-10 Personal Computer

        30/08/2010 Skeleton driver

        Driver currently gets to a loop where it waits for an interrupt.
        The interrupt routine presumably writes to FE69 which the loop is
        constantly looking at.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"


class c10_state : public driver_device
{
public:
	c10_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	const UINT8 *FNT;
	const UINT8 *videoram;
};



static ADDRESS_MAP_START(c10_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK("boot")
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_RAM AM_REGION("maincpu", 0xc000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( c10_io, ADDRESS_SPACE_IO, 8 )
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

static MACHINE_RESET(c10)
{
	memory_set_bank(machine, "boot", 1);
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, c10_reset);
}

static VIDEO_START( c10 )
{
	c10_state *state = machine->driver_data<c10_state>();
	state->FNT = memory_region(machine, "chargen");
	state->videoram = memory_region(machine, "maincpu")+0xf0a2;
}

/* This system appears to have inline attribute bytes of unknown meaning.
    Currently they are ignored. The word at FAB5 looks like it might be cursor location. */
static VIDEO_UPDATE( c10 )
{
	c10_state *state = screen->machine->driver_data<c10_state>();
	//static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x,xx;

	//framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			xx = ma;
			for (x = ma; x < ma + 80; x++)
			{
				gfx = 0;
				if (ra < 9)
				{
					chr = state->videoram[xx++];

				//	/* Take care of flashing characters */
				//	if ((chr < 0x80) && (framecnt & 0x08))
				//		chr |= 0x80;

					if (chr & 0x80)  // ignore attribute bytes
						x--;
					else
					{
						gfx = state->FNT[(chr<<4) | ra ];

						/* Display a scanline of a character */
						*p = ( gfx & 0x80 ) ? 1 : 0; p++;
						*p = ( gfx & 0x40 ) ? 1 : 0; p++;
						*p = ( gfx & 0x20 ) ? 1 : 0; p++;
						*p = ( gfx & 0x10 ) ? 1 : 0; p++;
						*p = ( gfx & 0x08 ) ? 1 : 0; p++;
						*p = ( gfx & 0x04 ) ? 1 : 0; p++;
						*p = ( gfx & 0x02 ) ? 1 : 0; p++;
						*p = ( gfx & 0x01 ) ? 1 : 0; p++;
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
	MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
	MDRV_CPU_PROGRAM_MAP(c10_mem)
	MDRV_CPU_IO_MAP(c10_io)

	MDRV_MACHINE_RESET(c10)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 250)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
	MDRV_GFXDECODE(c10)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(c10)
	MDRV_VIDEO_UPDATE(c10)
MACHINE_CONFIG_END

DRIVER_INIT( c10 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
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
COMP( 1982, c10,  0,       0,	c10,	c10,	 c10, 	  "Cromemco",   "C-10",		GAME_NOT_WORKING | GAME_NO_SOUND)
